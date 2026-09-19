#include "mudflow/workflow.h"

#include "mudflow/project_config.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>

#include <stdexcept>

namespace mudflow {
namespace {

struct Paths {
    QString root;
    QString state;
    QString ledger;
    QString evidence;
    QString handoffs;
};

struct ProcessResult {
    int exitCode;
    QString output;
    QString error;
};

[[noreturn]] void fail(const QString& message)
{
    throw std::runtime_error(message.toStdString());
}

QString expandPath(const QString& value, const QString& root)
{
    if (value == QLatin1String("~")) {
        return QDir::homePath();
    }
    if (value.startsWith(QLatin1String("~/"))) {
        return QDir::home().filePath(value.mid(2));
    }
    return QFileInfo(value).isAbsolute() ? QDir::cleanPath(value) : QDir(root).absoluteFilePath(value);
}

Paths pathsFor(const QString& configPath)
{
    QDir configDirectory = QFileInfo(configPath).absoluteDir();
    if (!configDirectory.cdUp()) {
        fail(QStringLiteral("Project config must be inside .mudflow"));
    }
    const QString root = configDirectory.absolutePath();
    const QString state = QDir(root).filePath(QStringLiteral(".mudflow"));
    return {root, state, QDir(state).filePath(QStringLiteral("ledger")), QDir(state).filePath(QStringLiteral("evidence")), QDir(state).filePath(QStringLiteral("handoffs"))};
}

void ensureDirectories(const Paths& paths)
{
    if (!QDir().mkpath(paths.ledger) || !QDir().mkpath(paths.evidence) || !QDir().mkpath(paths.handoffs)) {
        fail(QStringLiteral("Cannot create .mudflow state directories"));
    }
}

ProcessResult run(const QString& program, const QStringList& arguments)
{
    QProcess process;
    process.setProgram(program);
    process.setArguments(arguments);
    process.start();
    if (!process.waitForStarted(5000)) {
        fail(QStringLiteral("Cannot start %1: %2").arg(program, process.errorString()));
    }
    if (!process.waitForFinished(60000)) {
        process.kill();
        fail(QStringLiteral("Timed out: %1 %2").arg(program, arguments.join(QLatin1Char(' '))));
    }
    return {process.exitCode(), QString::fromUtf8(process.readAllStandardOutput()).trimmed(), QString::fromUtf8(process.readAllStandardError()).trimmed()};
}

ProcessResult git(const QString& repository, const QStringList& arguments)
{
    QStringList gitArguments{QStringLiteral("-C"), repository};
    gitArguments.append(arguments);
    return run(QStringLiteral("git"), gitArguments);
}

QString gitRequired(const QString& repository, const QStringList& arguments)
{
    const ProcessResult result = git(repository, arguments);
    if (result.exitCode != 0) {
        fail(QStringLiteral("git -C %1 %2: %3").arg(repository, arguments.join(QLatin1Char(' ')), result.error));
    }
    return result.output;
}

QString nowUtc()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

QString sha1File(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(&file);
    return QString::fromLatin1(hash.result().toHex());
}

void appendEvent(const Paths& paths, const QString& executionId, const QJsonObject& event)
{
    QFile file(QDir(paths.ledger).filePath(executionId + QStringLiteral(".jsonl")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        fail(QStringLiteral("Cannot append ledger %1: %2").arg(file.fileName(), file.errorString()));
    }
    file.write(QJsonDocument(event).toJson(QJsonDocument::Compact));
    file.write("\n");
}

QVector<QJsonObject> readEvents(const Paths& paths)
{
    QVector<QJsonObject> events;
    const QStringList files = QDir(paths.ledger).entryList({QStringLiteral("*.jsonl")}, QDir::Files, QDir::Name);
    for (const QString& name : files) {
        QFile file(QDir(paths.ledger).filePath(name));
        if (!file.open(QIODevice::ReadOnly)) {
            fail(QStringLiteral("Cannot read ledger %1").arg(file.fileName()));
        }
        while (!file.atEnd()) {
            const QByteArray line = file.readLine().trimmed();
            if (line.isEmpty()) {
                continue;
            }
            QJsonParseError error;
            const QJsonDocument document = QJsonDocument::fromJson(line, &error);
            if (error.error != QJsonParseError::NoError || !document.isObject()) {
                fail(QStringLiteral("Invalid ledger event in %1").arg(file.fileName()));
            }
            events.append(document.object());
        }
    }
    return events;
}

QJsonObject startedEvent(const QVector<QJsonObject>& events, const QString& executionId)
{
    for (const QJsonObject& event : events) {
        if (event.value(QStringLiteral("type")) == QLatin1String("execution.started")
                && event.value(QStringLiteral("exec")) == executionId) {
            return event;
        }
    }
    fail(QStringLiteral("Unknown execution: %1").arg(executionId));
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

QJsonObject finding(const QString& id, const QString& severity, const QString& domain, const QString& title, const QString& explanation, const QString& action = {})
{
    QJsonObject value{
        {QStringLiteral("id"), id},
        {QStringLiteral("severity"), severity},
        {QStringLiteral("domain"), domain},
        {QStringLiteral("title"), title},
        {QStringLiteral("explanation"), explanation},
    };
    if (!action.isEmpty()) {
        value.insert(QStringLiteral("suggested_action"), action);
    }
    return value;
}

QString planReference(const ProjectConfig& config, const Paths& paths, const QString& task)
{
    const QString path = expandPath(config.planPath, paths.root);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    int lineNumber = 0;
    while (!file.atEnd()) {
        ++lineNumber;
        if (QString::fromUtf8(file.readLine()).contains(task)) {
            return config.planPath + QStringLiteral("#L") + QString::number(lineNumber);
        }
    }
    return {};
}

QString writeEvidence(const Paths& paths, const QJsonObject& value)
{
    const QByteArray data = QJsonDocument(value).toJson(QJsonDocument::Compact);
    const QString name = QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex()) + QStringLiteral(".json");
    QFile file(QDir(paths.evidence).filePath(name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fail(QStringLiteral("Cannot write evidence %1").arg(file.fileName()));
    }
    file.write(data);
    return QStringLiteral("evidence/") + name;
}

} // namespace

QJsonObject inspectProject(const QString& configPath)
{
    return ProjectConfig::load(configPath).toJson();
}

QJsonObject projectStatus(const QString& configPath)
{
    const ProjectConfig config = ProjectConfig::load(configPath);
    const Paths paths = pathsFor(configPath);
    ensureDirectories(paths);
    const QVector<QJsonObject> events = readEvents(paths);
    QJsonArray repositories;
    QJsonArray findings;
    QHash<QString, QString> remoteBaseShas;

    for (const RepositoryConfig& repository : config.repositories) {
        const QString repositoryPath = expandPath(repository.path, paths.root);
        QJsonObject report{{QStringLiteral("name"), repository.name}, {QStringLiteral("path"), repositoryPath}, {QStringLiteral("base"), repository.base}};
        const ProcessResult fetchResult = git(repositoryPath, {QStringLiteral("fetch"), QStringLiteral("--quiet"), QStringLiteral("origin")});
        if (fetchResult.exitCode != 0) {
            report.insert(QStringLiteral("fetch_error"), fetchResult.error);
            findings.append(finding(QStringLiteral("git.fetch_failed"), QStringLiteral("warning"), QStringLiteral("git"), QStringLiteral("Cannot fetch remote"), repository.name + QStringLiteral(": ") + fetchResult.error, QStringLiteral("Restore remote access, then run status again.")));
        }
        try {
            const QString dirty = gitRequired(repositoryPath, {QStringLiteral("status"), QStringLiteral("--porcelain")});
            const QString head = gitRequired(repositoryPath, {QStringLiteral("rev-parse"), QStringLiteral("HEAD")});
            const QString branch = gitRequired(repositoryPath, {QStringLiteral("branch"), QStringLiteral("--show-current")});
            const QString remoteBaseSha = gitRequired(repositoryPath, {QStringLiteral("rev-parse"), repository.base});
            const QString counts = gitRequired(repositoryPath, {QStringLiteral("rev-list"), QStringLiteral("--left-right"), QStringLiteral("--count"), repository.base + QStringLiteral("...HEAD")});
            const QStringList countParts = counts.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
            const int behind = countParts.value(0).toInt();
            const int ahead = countParts.value(1).toInt();
            report.insert(QStringLiteral("branch"), branch);
            report.insert(QStringLiteral("head"), head);
            report.insert(QStringLiteral("base_sha"), remoteBaseSha);
            report.insert(QStringLiteral("behind_base"), behind);
            report.insert(QStringLiteral("ahead_of_base"), ahead);
            report.insert(QStringLiteral("dirty"), !dirty.isEmpty());
            if (!dirty.isEmpty()) {
                findings.append(finding(QStringLiteral("git.dirty_workspace"), QStringLiteral("warning"), QStringLiteral("git"), QStringLiteral("Workspace has uncommitted changes"), repository.name + QStringLiteral(" is dirty")));
            }
            if (behind > 0) {
                findings.append(finding(QStringLiteral("git.remote_ahead"), QStringLiteral("warning"), QStringLiteral("git"), QStringLiteral("Branch is behind remote base"), repository.name + QStringLiteral(" is ") + QString::number(behind) + QStringLiteral(" commits behind ") + repository.base));
            }
            const QString localBase = repository.base.section(QLatin1Char('/'), 1);
            const ProcessResult localBaseExists = git(repositoryPath, {QStringLiteral("rev-parse"), QStringLiteral("--verify"), localBase});
            if (localBaseExists.exitCode == 0) {
                const QString localCounts = gitRequired(repositoryPath, {QStringLiteral("rev-list"), QStringLiteral("--left-right"), QStringLiteral("--count"), repository.base + QStringLiteral("...") + localBase});
                const int localBehind = localCounts.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts).value(0).toInt();
                if (localBehind > 0) {
                    findings.append(finding(QStringLiteral("git.stale_local_base"), QStringLiteral("warning"), QStringLiteral("git"), QStringLiteral("Local base is behind remote"), localBase + QStringLiteral(" is ") + QString::number(localBehind) + QStringLiteral(" commits behind ") + repository.base));
                }
            }
            remoteBaseShas.insert(repository.name, remoteBaseSha);
        } catch (const std::exception& error) {
            report.insert(QStringLiteral("error"), QString::fromUtf8(error.what()));
        }
        repositories.append(report);
    }

    QSet<QString> evidencedTasks;
    QSet<QString> completedExecutions;
    QSet<QString> executionsWithCommits;
    for (const QJsonObject& event : events) {
        const QString type = event.value(QStringLiteral("type")).toString();
        if (type == QLatin1String("execution.finished")) {
            completedExecutions.insert(event.value(QStringLiteral("exec")).toString());
            if (!event.value(QStringLiteral("commits")).toArray().isEmpty()) {
                executionsWithCommits.insert(event.value(QStringLiteral("exec")).toString());
            }
        }
        if (type == QLatin1String("evidence.recorded") && event.value(QStringLiteral("kind")).toString() != QLatin1String("manual_note")) {
            evidencedTasks.insert(event.value(QStringLiteral("task")).toString());
        }
        if (type == QLatin1String("note") && event.value(QStringLiteral("kind")).toString() == QLatin1String("unresolved") && event.value(QStringLiteral("ref")).isNull()) {
            findings.append(finding(QStringLiteral("context.unresolved_without_ref"), QStringLiteral("info"), QStringLiteral("context"), QStringLiteral("Unresolved note has no reference"), event.value(QStringLiteral("text")).toString()));
        }
    }
    for (const QJsonObject& event : events) {
        if (event.value(QStringLiteral("type")).toString() != QLatin1String("execution.started")) {
            continue;
        }
        const QString executionId = event.value(QStringLiteral("exec")).toString();
        const QString task = event.value(QStringLiteral("task")).toString();
        const bool completed = completedExecutions.contains(executionId);
        const bool hasHandoff = QFileInfo::exists(QDir(paths.handoffs).filePath(executionId + QStringLiteral(".md")));
        if (executionsWithCommits.contains(executionId)) {
            evidencedTasks.insert(task);
        }
        if (event.value(QStringLiteral("plan_ref")).toString().isEmpty()) {
            findings.append(finding(QStringLiteral("plan.execution_without_plan_link"), QStringLiteral("info"), QStringLiteral("plan"), QStringLiteral("Execution has no plan link"), executionId + QStringLiteral(" has no matching task in plan")));
        }
        if (completed && !hasHandoff) {
            findings.append(finding(QStringLiteral("context.no_handoff"), QStringLiteral("warning"), QStringLiteral("context"), QStringLiteral("Completed execution has no handoff"), executionId));
        }
        if (!completed && !hasHandoff) {
            const QDateTime startedAt = QDateTime::fromString(event.value(QStringLiteral("ts")).toString(), Qt::ISODate);
            if (!startedAt.isValid()) {
                findings.append(finding(QStringLiteral("context.invalid_ledger_timestamp"), QStringLiteral("warning"), QStringLiteral("context"), QStringLiteral("Execution has an invalid ledger timestamp"), executionId + QStringLiteral(" has invalid ts: ") + event.value(QStringLiteral("ts")).toString()));
            } else if (startedAt.secsTo(QDateTime::currentDateTimeUtc()) >= 24 * 60 * 60) {
                const qint64 ageSeconds = startedAt.secsTo(QDateTime::currentDateTimeUtc());
                findings.append(finding(QStringLiteral("context.orphaned_execution"), QStringLiteral("warning"), QStringLiteral("context"), QStringLiteral("Execution appears abandoned"), executionId + QStringLiteral(" started ") + QString::number(ageSeconds / 3600) + QStringLiteral(" hours ago without finish or handoff")));
            } else {
                findings.append(finding(QStringLiteral("context.active_execution"), QStringLiteral("info"), QStringLiteral("context"), QStringLiteral("Execution is still active"), executionId + QStringLiteral(" has no finish event or handoff yet")));
            }
        }
        const QString recordedPlanSha = event.value(QStringLiteral("plan_sha1")).toString();
        if (!completed && !recordedPlanSha.isEmpty() && recordedPlanSha != sha1File(expandPath(config.planPath, paths.root))) {
            findings.append(finding(QStringLiteral("plan.changed_during_execution"), QStringLiteral("warning"), QStringLiteral("plan"), QStringLiteral("Plan changed during execution"), task));
        }
        const QString currentBaseSha = remoteBaseShas.value(event.value(QStringLiteral("repo")).toString());
        if (!completed && !currentBaseSha.isEmpty() && currentBaseSha != event.value(QStringLiteral("base_sha")).toString()) {
            findings.append(finding(QStringLiteral("git.stale_worktree_base"), QStringLiteral("warning"), QStringLiteral("git"), QStringLiteral("Worktree base is stale"), executionId + QStringLiteral(" was created from an older ") + event.value(QStringLiteral("base")).toString()));
        }
    }

    QFile plan(expandPath(config.planPath, paths.root));
    if (plan.open(QIODevice::ReadOnly)) {
        const QRegularExpression doneTask(QStringLiteral("^\\s*-\\s*\\[x\\].*(") + config.taskIdPattern + QStringLiteral(")"), QRegularExpression::CaseInsensitiveOption);
        while (!plan.atEnd()) {
            const QString line = QString::fromUtf8(plan.readLine());
            const QRegularExpressionMatch match = doneTask.match(line);
            if (match.hasMatch() && !evidencedTasks.contains(match.captured(1))) {
                findings.append(finding(QStringLiteral("plan.done_without_evidence"), QStringLiteral("warning"), QStringLiteral("plan"), QStringLiteral("Done plan task has no evidence"), match.captured(1)));
            }
        }
    }

    return {{QStringLiteral("project"), config.name}, {QStringLiteral("repositories"), repositories}, {QStringLiteral("findings"), findings}};
}

QJsonObject startExecution(const QString& configPath, const QString& task, const QString& agent, const QString& repositoryName)
{
    const ProjectConfig config = ProjectConfig::load(configPath);
    const Paths paths = pathsFor(configPath);
    ensureDirectories(paths);
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
    gitRequired(repositoryPath, {QStringLiteral("fetch"), QStringLiteral("--quiet"), QStringLiteral("origin")});
    if (!gitRequired(repositoryPath, {QStringLiteral("status"), QStringLiteral("--porcelain")}).isEmpty()) {
        fail(QStringLiteral("Refusing start: repository has uncommitted changes"));
    }
    const QString baseSha = gitRequired(repositoryPath, {QStringLiteral("rev-parse"), repository.base});
    const QString worktreeRoot = expandPath(config.worktreeRoot, paths.root);
    const QString worktree = QDir(worktreeRoot).filePath(task);
    const QString executionId = QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd'T'HHmmss'Z'")) + QLatin1Char('-') + task;
    if (QFileInfo::exists(worktree) || QFileInfo::exists(QDir(paths.ledger).filePath(executionId + QStringLiteral(".jsonl")))) {
        fail(QStringLiteral("Refusing start: worktree or execution already exists"));
    }
    if (!QDir().mkpath(worktreeRoot)) {
        fail(QStringLiteral("Cannot create worktree root: %1").arg(worktreeRoot));
    }
    const QString branch = QStringLiteral("task/") + task;
    gitRequired(repositoryPath, {QStringLiteral("worktree"), QStringLiteral("add"), QStringLiteral("-b"), branch, worktree, repository.base});

    appendEvent(paths, executionId, {
        {QStringLiteral("ts"), nowUtc()}, {QStringLiteral("type"), QStringLiteral("execution.started")}, {QStringLiteral("exec"), executionId},
        {QStringLiteral("task"), task}, {QStringLiteral("agent"), agent}, {QStringLiteral("repo"), repository.name},
        {QStringLiteral("worktree"), worktree}, {QStringLiteral("branch"), branch}, {QStringLiteral("base"), repository.base},
        {QStringLiteral("base_sha"), baseSha}, {QStringLiteral("head_sha"), baseSha}, {QStringLiteral("plan_ref"), planReference(config, paths, task)},
        {QStringLiteral("plan_sha1"), sha1File(expandPath(config.planPath, paths.root))},
    });
    return {{QStringLiteral("exec"), executionId}, {QStringLiteral("worktree"), worktree}, {QStringLiteral("branch"), branch}, {QStringLiteral("base_sha"), baseSha}};
}

QJsonObject finishExecution(const QString& configPath, const QString& executionId, const QString& outcome)
{
    if (outcome != QLatin1String("finished") && outcome != QLatin1String("interrupted") && outcome != QLatin1String("abandoned")) {
        fail(QStringLiteral("Outcome must be finished, interrupted, or abandoned"));
    }
    const ProjectConfig config = ProjectConfig::load(configPath);
    const Paths paths = pathsFor(configPath);
    ensureDirectories(paths);
    const QVector<QJsonObject> events = readEvents(paths);
    const QJsonObject started = startedEvent(events, executionId);
    for (const QJsonObject& event : events) {
        if (event.value(QStringLiteral("type")).toString() == QLatin1String("execution.finished")
                && event.value(QStringLiteral("exec")).toString() == executionId) {
            fail(QStringLiteral("Execution already finished: %1").arg(executionId));
        }
    }
    const QString worktree = started.value(QStringLiteral("worktree")).toString();
    const QString baseSha = started.value(QStringLiteral("base_sha")).toString();
    const QString headSha = gitRequired(worktree, {QStringLiteral("rev-parse"), QStringLiteral("HEAD")});
    const QStringList commitLines = gitRequired(worktree, {QStringLiteral("log"), QStringLiteral("--format=%h%x09%s"), baseSha + QStringLiteral("..HEAD")}).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    const QStringList files = gitRequired(worktree, {QStringLiteral("diff"), QStringLiteral("--name-only"), baseSha + QStringLiteral("..HEAD")}).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    const QString shortstat = gitRequired(worktree, {QStringLiteral("diff"), QStringLiteral("--shortstat"), baseSha + QStringLiteral("..HEAD")});
    int filesChanged = 0;
    int insertions = 0;
    int deletions = 0;
    const QRegularExpression shortstatPattern(QStringLiteral("^(\\d+) files? changed(?:, (\\d+) insertions?\\(\\+\\))?(?:, (\\d+) deletions?\\(-\\))?$"));
    const QRegularExpressionMatch shortstatMatch = shortstatPattern.match(shortstat);
    if (!shortstat.isEmpty() && !shortstatMatch.hasMatch()) {
        fail(QStringLiteral("Unexpected git diff --shortstat output: %1").arg(shortstat));
    }
    if (shortstatMatch.hasMatch()) {
        filesChanged = shortstatMatch.captured(1).toInt();
        insertions = shortstatMatch.captured(2).toInt();
        deletions = shortstatMatch.captured(3).toInt();
    }
    QJsonArray commitJson;
    QJsonArray fileJson;
    for (const QString& commit : commitLines) commitJson.append(commit.section(QLatin1Char('\t'), 0, 0));
    for (const QString& file : files) fileJson.append(file);
    const QString filesRef = writeEvidence(paths, {{QStringLiteral("files"), fileJson}});

    QFile handoff(QDir(paths.handoffs).filePath(executionId + QStringLiteral(".md")));
    if (!handoff.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fail(QStringLiteral("Cannot write handoff: %1").arg(handoff.fileName()));
    }
    QTextStream output(&handoff);
    output << "---\nexec: " << executionId << "\ntask: " << started.value(QStringLiteral("task")).toString() << "\nagent: " << started.value(QStringLiteral("agent")).toString() << "\noutcome: " << outcome << "\nrepo: " << started.value(QStringLiteral("repo")).toString() << "\nworktree: " << worktree << "\nbranch: " << started.value(QStringLiteral("branch")).toString() << "\nbase: " << started.value(QStringLiteral("base")).toString() << "@" << baseSha << "\nrange: " << baseSha << ".." << headSha << "\n---\n\n## Doğrulanmış (Mudflow üretti)\n\nCommits:\n";
    for (const QString& commit : commitLines) output << "- " << commit << '\n';
    output << "\nDeğişen dosyalar: " << filesChanged << " (+" << insertions << " / -" << deletions << ")\n";
    for (const QString& file : files) output << "- " << file << '\n';
    output << "\nEvidence: " << filesRef << '\n';
    output << "\n## Agent notu (zayıf evidence — doğrulanmadı)\n\n";
    bool hasAgentSummary = false;
    for (const QJsonObject& event : events) {
        if (event.value(QStringLiteral("type")).toString() == QLatin1String("evidence.recorded")
                && event.value(QStringLiteral("exec")).toString() == executionId
                && event.value(QStringLiteral("kind")).toString() == QLatin1String("agent_summary")) {
            output << "- " << event.value(QStringLiteral("summary")).toString() << '\n';
            hasAgentSummary = true;
        }
    }
    if (!hasAgentSummary) output << "Yok.\n";
    output << "\n## Açık kalanlar\n\n";
    bool hasUnresolved = false;
    for (const QJsonObject& event : events) {
        if (event.value(QStringLiteral("type")).toString() == QLatin1String("note")
                && event.value(QStringLiteral("exec")).toString() == executionId
                && event.value(QStringLiteral("kind")).toString() == QLatin1String("unresolved")) {
            output << "- [ ] " << event.value(QStringLiteral("text")).toString();
            const QJsonValue reference = event.value(QStringLiteral("ref"));
            output << (reference.isNull() ? "  (ref yok)" : "  (ref: " + reference.toString() + ")") << '\n';
            hasUnresolved = true;
        }
    }
    if (!hasUnresolved) output << "Yok.\n";
    output.flush();
    if (output.status() != QTextStream::Ok) {
        fail(QStringLiteral("Cannot write handoff: %1").arg(handoff.fileName()));
    }
    handoff.close();
    appendEvent(paths, executionId, {{QStringLiteral("ts"), nowUtc()}, {QStringLiteral("type"), QStringLiteral("execution.finished")}, {QStringLiteral("exec"), executionId}, {QStringLiteral("outcome"), outcome}, {QStringLiteral("head_sha"), headSha}, {QStringLiteral("commits"), commitJson}, {QStringLiteral("files_changed"), filesChanged}, {QStringLiteral("insertions"), insertions}, {QStringLiteral("deletions"), deletions}, {QStringLiteral("files_ref"), filesRef}});
    return {{QStringLiteral("exec"), executionId}, {QStringLiteral("outcome"), outcome}, {QStringLiteral("head_sha"), headSha}, {QStringLiteral("handoff"), QStringLiteral("handoffs/") + executionId + QStringLiteral(".md")}};
}

void recordEvidence(const QString& configPath, const QString& executionId, const QString& kind, const QString& summary, const QString& reference)
{
    const QStringList kinds{QStringLiteral("commit"), QStringLiteral("diff"), QStringLiteral("test"), QStringLiteral("files"), QStringLiteral("command"), QStringLiteral("agent_summary"), QStringLiteral("manual_note")};
    if (!kinds.contains(kind) || summary.trimmed().isEmpty()) fail(QStringLiteral("Invalid evidence kind or empty summary"));
    const Paths paths = pathsFor(configPath);
    ensureDirectories(paths);
    const QJsonObject started = startedEvent(readEvents(paths), executionId);
    appendEvent(paths, executionId, {{QStringLiteral("ts"), nowUtc()}, {QStringLiteral("type"), QStringLiteral("evidence.recorded")}, {QStringLiteral("exec"), executionId}, {QStringLiteral("task"), started.value(QStringLiteral("task")).toString()}, {QStringLiteral("kind"), kind}, {QStringLiteral("ref"), reference.isEmpty() ? QJsonValue::Null : QJsonValue(reference)}, {QStringLiteral("summary"), summary}});
}

void recordNote(const QString& configPath, const QString& executionId, const QString& kind, const QString& text, const QString& reference)
{
    const QStringList kinds{QStringLiteral("decision"), QStringLiteral("unresolved"), QStringLiteral("blocker")};
    if (!kinds.contains(kind) || text.trimmed().isEmpty()) fail(QStringLiteral("Invalid note kind or empty text"));
    const Paths paths = pathsFor(configPath);
    ensureDirectories(paths);
    startedEvent(readEvents(paths), executionId);
    appendEvent(paths, executionId, {{QStringLiteral("ts"), nowUtc()}, {QStringLiteral("type"), QStringLiteral("note")}, {QStringLiteral("exec"), executionId}, {QStringLiteral("kind"), kind}, {QStringLiteral("text"), text}, {QStringLiteral("source"), QStringLiteral("human")}, {QStringLiteral("ref"), reference.isEmpty() ? QJsonValue::Null : QJsonValue(reference)}});
}

} // namespace mudflow
