#include "runmark/workflow.h"

#include "runmark/project_config.h"
#include "runmark/rules.h"
#include "runmark/error.h"
#include "config_io.h"
#include "git.h"
#include "handoff.h"
#include "ledger.h"
#include "paths.h"
#include "plan.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QRegularExpression>

namespace runmark {
namespace {

// Orchestration joins two separate duties: creating directories belongs to
// paths, writing excludes to git. Calling both together is the command's job.
void prepareState(const Paths& paths)
{
    ensureDirectories(paths);
    ensureGitExcludes(paths);
}

const RepositoryConfig& repositoryFor(const ProjectConfig& config, const QString& name)
{
    if (name.isEmpty() && config.repositories.size() == 1) {
        return config.repositories.constFirst();
    }
    for (const RepositoryConfig& repository : config.repositories) {
        if (repository.name == name) {
            return repository;
        }
    }
    fail(QStringLiteral("Unknown repository: %1").arg(name.isEmpty() ? QStringLiteral("(select --repo)") : name));
}

QJsonValue orNull(const QString& value)
{
    return value.isEmpty() ? QJsonValue::Null : QJsonValue(value);
}

// OBSERVE: run git, read the plan, read the ledger, measure what exists on
// disk. No evaluation happens here.
StatusFacts observe(const ProjectConfig& config, const Paths& paths)
{
    StatusFacts facts;
    facts.now = QDateTime::currentDateTimeUtc();
    facts.events = readEvents(paths);
    facts.plan = observePlan(config, paths.root);

    for (const RepositoryConfig& repository : config.repositories) {
        facts.repos.append(observeRepo(repository, expandPath(repository.path, paths.root)));
    }

    facts.hookError = readHookObservation(paths, facts.lastHookObserved);

    for (const QJsonObject& event : facts.events) {
        if (event.value(QStringLiteral("type")).toString() != QLatin1String("execution.started")) {
            continue;
        }
        ExecutionFacts execution;
        execution.exec = event.value(QStringLiteral("exec")).toString();
        execution.hasHandoff = QFileInfo::exists(QDir(paths.handoffs).filePath(execution.exec + QStringLiteral(".md")));
        const QString worktree = event.value(QStringLiteral("worktree")).toString();
        execution.worktreeExists = !worktree.isEmpty() && QFileInfo::exists(worktree);
        facts.executions.append(execution);
    }
    return facts;
}

} // namespace

ProjectConfig inspectProject(const QString& configPath)
{
    return loadProjectConfig(configPath);
}

StatusResult projectStatus(const QString& configPath)
{
    const ProjectConfig config = loadProjectConfig(configPath);
    const Paths paths = pathsFor(configPath);
    prepareState(paths);

    const StatusFacts facts = observe(config, paths);

    return {config.name, facts.repos, evaluate(config, facts)};
}

ResumeResult resumeExecution(const QString& configPath, const QString& taskOrExecution, bool observedByHook)
{
    const ProjectConfig config = loadProjectConfig(configPath);
    const Paths paths = pathsFor(configPath);
    // Nothing else proves the hook ran: a session opens, the hook fails
    // silently, and status reads that as a clean project. If the write fails,
    // ignore it -- the finding then says "never observed", the safe direction.
    if (observedByHook) writeHookObservation(paths);
    ResumeFacts facts = observeResumeLedger(paths, taskOrExecution);
    if (!facts.started.isEmpty()) {
        facts.planSha1 = observePlan(config, paths.root).sha1;
        readHandoff(paths, facts);
        observeResumeGit(config, paths, facts);
    }
    return {facts, evaluateResume(facts)};
}

StartResult startExecution(const QString& configPath, const QString& task, const QString& agent, const QString& repositoryName, const QStringList& instructions)
{
    const ProjectConfig config = loadProjectConfig(configPath);
    const Paths paths = pathsFor(configPath);
    prepareState(paths);

    const QRegularExpression taskPattern(config.taskIdPattern);
    const QRegularExpressionMatch taskMatch = taskPattern.match(task);
    if (!taskMatch.hasMatch() || taskMatch.capturedLength() != task.size() || task.contains(QLatin1Char('/')) || task.contains(QStringLiteral(".."))) {
        fail(QStringLiteral("Task does not match project.task_id_pattern: %1").arg(task));
    }
    if (agent != QLatin1String("codex") && agent != QLatin1String("claude")) {
        fail(QStringLiteral("Agent must be codex or claude"));
    }

    const RepositoryConfig& repository = repositoryFor(config, repositoryName);
    const QString repositoryPath = expandPath(repository.path, paths.root);
    const QString remoteBase = baseRef(repository);
    gitRequired(repositoryPath, {QStringLiteral("fetch"), QStringLiteral("--quiet"), repository.remote});
    // ADR-014: a dirty main repo does not block start. The base resolves from
    // a remote ref, so there is no technical obstacle; the state is recorded
    // and warned about. The worktree's own dirt is captured into a preserved
    // ref -- otherwise it would be evidence invisible to the merge-base..HEAD
    // diff accounting.
    const bool repositoryDirty = !gitRequired(repositoryPath, {QStringLiteral("status"), QStringLiteral("--porcelain")}).isEmpty();
    QVector<Finding> warnings;
    if (repositoryDirty) {
        warnings.append(finding(QStringLiteral("git.dirty_workspace"), QStringLiteral("warning"), QStringLiteral("git"),
            QStringLiteral("Repository has uncommitted changes"),
            repository.name + QStringLiteral(" was dirty when this execution started"),
            QStringLiteral("Commit or stash before the next start if this was unintended.")));
    }
    const QString remoteBaseSha = gitRequired(repositoryPath, {QStringLiteral("rev-parse"), remoteBase});
    const QString worktreeRoot = expandPath(config.worktreeRoot, paths.root);
    const QString worktree = QDir(worktreeRoot).filePath(task);
    const QString executionId = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd'T'HHmmss'Z'")) + QLatin1Char('-') + task;
    if (QFileInfo::exists(QDir(paths.ledger).filePath(executionId + QStringLiteral(".jsonl")))) {
        fail(QStringLiteral("Refusing start: execution already exists"));
    }
    QString branch;
    QString baseSha;
    QString headSha;
    QString workspaceSource;
    QString preservedRef;
    if (QFileInfo::exists(worktree)) {
        if (gitCommonDir(worktree) != gitCommonDir(repositoryPath)) {
            fail(QStringLiteral("Refusing start: existing worktree belongs to another repository"));
        }
        for (const QJsonObject& event : readEvents(paths)) {
            if (event.value(QStringLiteral("type")).toString() == QLatin1String("execution.started")
                    && event.value(QStringLiteral("worktree")).toString() == worktree) {
                fail(QStringLiteral("Refusing start: worktree already belongs to execution %1").arg(event.value(QStringLiteral("exec")).toString()));
            }
        }
        preservedRef = preserveWorktree(worktree, executionId);
        branch = gitRequired(worktree, {QStringLiteral("branch"), QStringLiteral("--show-current")});
        headSha = gitRequired(worktree, {QStringLiteral("rev-parse"), QStringLiteral("HEAD")});
        baseSha = gitRequired(worktree, {QStringLiteral("merge-base"), QStringLiteral("HEAD"), remoteBase});
        workspaceSource = QStringLiteral("adopted");
    } else {
        if (!QDir().mkpath(worktreeRoot)) {
            fail(QStringLiteral("Cannot create worktree root: %1").arg(worktreeRoot));
        }
        branch = QStringLiteral("task/") + task;
        gitRequired(repositoryPath, {QStringLiteral("worktree"), QStringLiteral("add"), QStringLiteral("-b"), branch, worktree, remoteBase});
        baseSha = remoteBaseSha;
        headSha = baseSha;
        workspaceSource = QStringLiteral("created");
    }

    const QJsonArray recordedInstructions = observeInstructions(config.instructions + instructions, paths.root);
    for (const QJsonValue& instruction : recordedInstructions) {
        if (instruction.toObject().value(QStringLiteral("sha1")).isNull()) {
            warnings.append(finding(QStringLiteral("context.instruction_unreadable"), QStringLiteral("warning"), QStringLiteral("context"),
                QStringLiteral("Instruction file cannot be read"), instruction.toObject().value(QStringLiteral("path")).toString()));
        }
    }

    appendEvent(paths, executionId, {
        {QStringLiteral("ts"), nowUtc()}, {QStringLiteral("type"), QStringLiteral("execution.started")}, {QStringLiteral("exec"), executionId},
        {QStringLiteral("task"), task}, {QStringLiteral("agent"), agent}, {QStringLiteral("repo"), repository.name},
        {QStringLiteral("worktree"), worktree}, {QStringLiteral("branch"), branch}, {QStringLiteral("base"), remoteBase},
        {QStringLiteral("workspace_source"), workspaceSource}, {QStringLiteral("repo_dirty"), repositoryDirty},
        {QStringLiteral("preserved_ref"), orNull(preservedRef)},
        {QStringLiteral("base_sha"), baseSha}, {QStringLiteral("head_sha"), headSha},
        {QStringLiteral("remote_base_sha"), remoteBaseSha},
        {QStringLiteral("instructions"), recordedInstructions},
        {QStringLiteral("plan_ref"), planReference(config, paths.root, task)},
        {QStringLiteral("plan_sha1"), sha1File(expandPath(config.planPath, paths.root))},
    });
    return {executionId, worktree, branch, workspaceSource, baseSha, preservedRef, warnings};
}

FinishResult finishExecution(const QString& configPath, const QString& executionId, const QString& outcome)
{
    if (outcome != QLatin1String("finished") && outcome != QLatin1String("interrupted") && outcome != QLatin1String("abandoned")) {
        fail(QStringLiteral("Outcome must be finished, interrupted, or abandoned"));
    }
    const Paths paths = pathsFor(configPath);
    loadProjectConfig(configPath);
    prepareState(paths);
    const QVector<QJsonObject> events = readEvents(paths);
    const QJsonObject started = startedEvent(events, executionId);
    for (const QJsonObject& event : events) {
        if (event.value(QStringLiteral("type")).toString() == QLatin1String("execution.finished")
                && event.value(QStringLiteral("exec")).toString() == executionId) {
            fail(QStringLiteral("Execution already finished: %1").arg(executionId));
        }
    }

    HandoffInput input;
    input.executionId = executionId;
    input.outcome = outcome;
    input.started = started;
    input.worktree = started.value(QStringLiteral("worktree")).toString();
    input.baseSha = started.value(QStringLiteral("base_sha")).toString();
    input.preservedRef = started.value(QStringLiteral("preserved_ref")).toString();

    const QString finishPreservedRef = preserveWorktree(input.worktree, executionId, input.preservedRef);
    if (!finishPreservedRef.isEmpty()) input.preservedRef = finishPreservedRef;

    input.headSha = gitRequired(input.worktree, {QStringLiteral("rev-parse"), QStringLiteral("HEAD")});
    const QString range = input.baseSha + QStringLiteral("..HEAD");
    input.commitLines = gitRequired(input.worktree, {QStringLiteral("log"), QStringLiteral("--format=%h%x09%s"), range}).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    input.files = gitRequired(input.worktree, {QStringLiteral("diff"), QStringLiteral("--name-only"), range}).split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    const QString shortstat = gitRequired(input.worktree, {QStringLiteral("diff"), QStringLiteral("--shortstat"), range});
    const QRegularExpression shortstatPattern(QStringLiteral("^(\\d+) files? changed(?:, (\\d+) insertions?\\(\\+\\))?(?:, (\\d+) deletions?\\(-\\))?$"));
    const QRegularExpressionMatch shortstatMatch = shortstatPattern.match(shortstat);
    if (!shortstat.isEmpty() && !shortstatMatch.hasMatch()) {
        fail(QStringLiteral("Unexpected git diff --shortstat output: %1").arg(shortstat));
    }
    if (shortstatMatch.hasMatch()) {
        input.filesChanged = shortstatMatch.captured(1).toInt();
        input.insertions = shortstatMatch.captured(2).toInt();
        input.deletions = shortstatMatch.captured(3).toInt();
    }

    QJsonArray commitJson;
    QJsonArray fileJson;
    for (const QString& commit : input.commitLines) commitJson.append(commit.section(QLatin1Char('\t'), 0, 0));
    for (const QString& file : input.files) fileJson.append(file);
    input.filesRef = writeEvidence(paths, {{QStringLiteral("files"), fileJson}});

    writeHandoff(paths, input, events);
    ResumeFacts handoffFacts;
    handoffFacts.exec = executionId;
    readHandoff(paths, handoffFacts);
    if (handoffFacts.handoff.sha1.isEmpty()) fail(QStringLiteral("Cannot hash generated handoff: ") + handoffFacts.handoff.error);

    appendEvent(paths, executionId, {{QStringLiteral("ts"), nowUtc()}, {QStringLiteral("type"), QStringLiteral("execution.finished")},
        {QStringLiteral("exec"), executionId}, {QStringLiteral("outcome"), outcome}, {QStringLiteral("head_sha"), input.headSha},
        {QStringLiteral("commits"), commitJson}, {QStringLiteral("files_changed"), input.filesChanged},
        {QStringLiteral("insertions"), input.insertions}, {QStringLiteral("deletions"), input.deletions},
        {QStringLiteral("files_ref"), input.filesRef}, {QStringLiteral("preserved_ref"), orNull(input.preservedRef)},
        {QStringLiteral("handoff_sha1"), handoffFacts.handoff.sha1}});
    return {executionId, outcome, input.headSha, input.preservedRef,
        QStringLiteral("handoffs/") + executionId + QStringLiteral(".md")};
}

void recordEvidence(const QString& configPath, const QString& executionId, const QString& kind, const QString& summary, const QString& reference)
{
    const QStringList kinds{QStringLiteral("commit"), QStringLiteral("diff"), QStringLiteral("test"), QStringLiteral("files"), QStringLiteral("command"), QStringLiteral("agent_summary"), QStringLiteral("manual_note")};
    if (!kinds.contains(kind) || summary.trimmed().isEmpty()) fail(QStringLiteral("Invalid evidence kind or empty summary"));
    const Paths paths = pathsFor(configPath);
    prepareState(paths);
    const QJsonObject started = startedEvent(readEvents(paths), executionId);
    appendEvent(paths, executionId, {{QStringLiteral("ts"), nowUtc()}, {QStringLiteral("type"), QStringLiteral("evidence.recorded")},
        {QStringLiteral("exec"), executionId}, {QStringLiteral("task"), started.value(QStringLiteral("task")).toString()},
        {QStringLiteral("kind"), kind}, {QStringLiteral("ref"), orNull(reference)}, {QStringLiteral("summary"), summary}});
}

void recordNote(const QString& configPath, const QString& executionId, const QString& kind, const QString& text, const QString& reference)
{
    const QStringList kinds{QStringLiteral("decision"), QStringLiteral("unresolved"), QStringLiteral("blocker")};
    if (!kinds.contains(kind) || text.trimmed().isEmpty()) fail(QStringLiteral("Invalid note kind or empty text"));
    const Paths paths = pathsFor(configPath);
    prepareState(paths);
    startedEvent(readEvents(paths), executionId);
    appendEvent(paths, executionId, {{QStringLiteral("ts"), nowUtc()}, {QStringLiteral("type"), QStringLiteral("note")},
        {QStringLiteral("exec"), executionId}, {QStringLiteral("kind"), kind}, {QStringLiteral("text"), text},
        {QStringLiteral("source"), QStringLiteral("human")}, {QStringLiteral("ref"), orNull(reference)}});
}

} // namespace runmark
