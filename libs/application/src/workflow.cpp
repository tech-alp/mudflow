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
#include "transcript.h"

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

// OBSERVE: run git, read the plan, read the ledger, measure what exists on
// disk. No evaluation happens here.
StatusFacts observe(const ProjectConfig& config, const Paths& paths)
{
    StatusFacts facts;
    facts.now = QDateTime::currentDateTimeUtc();
    facts.ledger = readLedger(paths);
    facts.plan = observePlan(config, paths.root);

    for (const RepositoryConfig& repository : config.repositories) {
        facts.repos.append(observeRepo(repository, expandPath(repository.path, paths.root)));
    }

    facts.hookError = readHookObservation(paths, facts.lastHookObserved);

    for (const ExecutionStarted& started : facts.ledger.started) {
        ExecutionFacts execution;
        execution.exec = started.exec;
        execution.hasHandoff = QFileInfo::exists(QDir(paths.handoffs).filePath(execution.exec + QStringLiteral(".md")));
        execution.worktreeExists = !started.worktree.isEmpty() && QFileInfo::exists(started.worktree);
        execution.agent = started.agent;
        // The newest timestamp across this execution's events. Scanning is
        // cheap and honest: no separate heartbeat to fall out of sync with the
        // record it claims to describe.
        const auto touch = [&execution](const QString& exec, const QDateTime& at) {
            if (exec == execution.exec && at.isValid() && (!execution.lastActivity.isValid() || at > execution.lastActivity)) {
                execution.lastActivity = at;
            }
        };
        for (const ExecutionStarted& e : facts.ledger.started) touch(e.exec, e.at);
        for (const ExecutionFinished& e : facts.ledger.finished) touch(e.exec, e.at);
        for (const EvidenceRecorded& e : facts.ledger.evidence) touch(e.exec, e.at);
        for (const NoteRecorded& e : facts.ledger.notes) touch(e.exec, e.at);
        facts.executions.append(execution);
    }
    return facts;
}

// Test runs the agent runtime recorded between start and now become one
// runtime evidence event. Returns what happened to the transcript, so an
// unreadable one surfaces as a finding instead of "no tests ran".
// ponytail: attribution is by session and time window; two executions driven
// from one session at the same time share their test runs.
TranscriptRecord recordRuntimeTests(const ProjectConfig& config, const Paths& paths, const ExecutionStarted& started, Ledger& ledger)
{
    if (started.sessionId.isEmpty()) return {QStringLiteral("no_session")};
    const TranscriptFacts transcript = readTranscript(started.agent, started.sessionId, started.at);
    if (!transcript.error.isEmpty()) return {QStringLiteral("unavailable"), {}, transcript.error};

    const QRegularExpression testPattern(config.testCommandPattern);
    QJsonArray runs;
    const TranscriptCommand* last = nullptr;
    for (const TranscriptCommand& command : transcript.commands) {
        if (!testPattern.match(command.command).hasMatch()) continue;
        runs.append(QJsonObject{{QStringLiteral("at"), command.at.toString(Qt::ISODateWithMs)},
            {QStringLiteral("command"), command.command}, {QStringLiteral("exit_code"), command.exitCode}});
        last = &command;
    }
    const TranscriptRecord record{QStringLiteral("read"), transcript.path, {}, int(transcript.commands.size()), int(runs.size())};
    if (!last) return record;

    EvidenceRecorded evidence;
    evidence.exec = started.exec;
    evidence.at = QDateTime::currentDateTimeUtc();
    evidence.task = started.task;
    evidence.kind = QStringLiteral("test");
    evidence.fromRuntime = true;
    evidence.runtime = started.agent;
    evidence.exitCode = last->exitCode;
    evidence.ref = writeEvidence(paths, {{QStringLiteral("transcript"), transcript.path}, {QStringLiteral("runs"), runs}});
    evidence.summary = QStringLiteral("%1 test run(s); last: %2 (exit %3)")
        .arg(QString::number(runs.size()), last->command.left(200), QString::number(last->exitCode));
    appendEvent(paths, evidence);
    ledger.evidence.append(evidence);
    return record;
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
    if (facts.started) {
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
        for (const ExecutionStarted& other : readLedger(paths).started) {
            if (other.worktree == worktree) {
                fail(QStringLiteral("Refusing start: worktree already belongs to execution %1").arg(other.exec));
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

    const QVector<Instruction> recordedInstructions = observeInstructions(config.instructions + instructions, paths.root);
    for (const Instruction& instruction : recordedInstructions) {
        if (instruction.sha1.isEmpty()) {
            warnings.append(finding(QStringLiteral("context.instruction_unreadable"), QStringLiteral("warning"), QStringLiteral("context"),
                QStringLiteral("Instruction file cannot be read"), instruction.path));
        }
    }

    ExecutionStarted started;
    started.exec = executionId;
    started.at = QDateTime::currentDateTimeUtc();
    started.task = task;
    started.agent = agent;
    started.repo = repository.name;
    started.worktree = worktree;
    started.branch = branch;
    started.base = remoteBase;
    started.workspaceSource = workspaceSource;
    started.repoDirty = repositoryDirty;
    started.preservedRef = preservedRef;
    started.baseSha = baseSha;
    started.headSha = headSha;
    started.remoteBaseSha = remoteBaseSha;
    // Which runtime transcript finish reads for test runs (DATA_MODEL.md §3.3).
    started.sessionId = sessionIdFromEnvironment(agent);
    started.instructions = recordedInstructions;
    started.planRef = planReference(config, paths.root, task);
    started.planSha1 = sha1File(expandPath(config.planPath, paths.root));
    appendEvent(paths, started);
    return {executionId, worktree, branch, workspaceSource, baseSha, preservedRef, warnings};
}

FinishResult finishExecution(const QString& configPath, const QString& executionId, const QString& outcome)
{
    if (outcome != QLatin1String("finished") && outcome != QLatin1String("interrupted") && outcome != QLatin1String("abandoned")) {
        fail(QStringLiteral("Outcome must be finished, interrupted, or abandoned"));
    }
    const Paths paths = pathsFor(configPath);
    const ProjectConfig config = loadProjectConfig(configPath);
    prepareState(paths);
    Ledger ledger = readLedger(paths);
    const ExecutionStarted started = startedEvent(ledger, executionId);
    for (const ExecutionFinished& finished : ledger.finished) {
        if (finished.exec == executionId) fail(QStringLiteral("Execution already finished: %1").arg(executionId));
    }

    HandoffInput input;
    input.executionId = executionId;
    input.outcome = outcome;
    input.started = started;
    input.worktree = started.worktree;
    input.baseSha = started.baseSha;
    input.preservedRef = started.preservedRef;

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

    QStringList commits;
    for (const QString& commit : input.commitLines) commits.append(commit.section(QLatin1Char('\t'), 0, 0));
    input.filesRef = writeEvidence(paths, {{QStringLiteral("files"), QJsonArray::fromStringList(input.files)}});

    const TranscriptRecord transcript = recordRuntimeTests(config, paths, started, ledger);

    writeHandoff(paths, input, ledger);
    ResumeFacts handoffFacts;
    handoffFacts.exec = executionId;
    readHandoff(paths, handoffFacts);
    if (handoffFacts.handoff.sha1.isEmpty()) fail(QStringLiteral("Cannot hash generated handoff: ") + handoffFacts.handoff.error);

    ExecutionFinished finished;
    finished.exec = executionId;
    finished.at = QDateTime::currentDateTimeUtc();
    finished.outcome = outcome;
    finished.headSha = input.headSha;
    finished.commits = commits;
    finished.filesChanged = input.filesChanged;
    finished.insertions = input.insertions;
    finished.deletions = input.deletions;
    finished.filesRef = input.filesRef;
    finished.preservedRef = input.preservedRef;
    finished.handoffSha1 = handoffFacts.handoff.sha1;
    finished.transcript = transcript;
    appendEvent(paths, finished);
    return {executionId, outcome, input.headSha, input.preservedRef,
        QStringLiteral("handoffs/") + executionId + QStringLiteral(".md"),
        observeWorktreeCleanup(input.worktree, started.branch, started.base)};
}

void recordEvidence(const QString& configPath, const QString& executionId, const QString& kind, const QString& summary, const QString& reference)
{
    const QStringList kinds{QStringLiteral("commit"), QStringLiteral("diff"), QStringLiteral("test"), QStringLiteral("files"), QStringLiteral("command"), QStringLiteral("agent_summary"), QStringLiteral("manual_note")};
    if (!kinds.contains(kind) || summary.trimmed().isEmpty()) fail(QStringLiteral("Invalid evidence kind or empty summary"));
    const Paths paths = pathsFor(configPath);
    prepareState(paths);
    EvidenceRecorded evidence;
    evidence.exec = executionId;
    evidence.at = QDateTime::currentDateTimeUtc();
    evidence.task = startedEvent(readLedger(paths), executionId).task;
    evidence.kind = kind;
    evidence.ref = reference;
    evidence.summary = summary;
    appendEvent(paths, evidence);
}

void recordNote(const QString& configPath, const QString& executionId, const QString& kind, const QString& text, const QString& reference)
{
    const QStringList kinds{QStringLiteral("decision"), QStringLiteral("unresolved"), QStringLiteral("blocker")};
    if (!kinds.contains(kind) || text.trimmed().isEmpty()) fail(QStringLiteral("Invalid note kind or empty text"));
    const Paths paths = pathsFor(configPath);
    prepareState(paths);
    startedEvent(readLedger(paths), executionId);
    appendEvent(paths, NoteRecorded{executionId, QDateTime::currentDateTimeUtc(), kind, text, QStringLiteral("human"), reference});
}

} // namespace runmark
