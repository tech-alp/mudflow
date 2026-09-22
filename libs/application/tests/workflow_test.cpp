#include "runmark/workflow.h"

#include <QFile>
#include <QJsonArray>
#include <QProcess>
#include <QTemporaryDir>

#include <exception>

namespace {

bool git(const QStringList& arguments)
{
    return QProcess::execute(QStringLiteral("git"), arguments) == 0;
}

QString gitOutput(const QStringList& arguments)
{
    QProcess process;
    process.start(QStringLiteral("git"), arguments);
    if (!process.waitForFinished() || process.exitCode() != 0) return {};
    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

bool writeFile(const QString& path, const QByteArray& contents)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

bool hasFinding(const QVector<runmark::Finding>& findings, const QString& id)
{
    for (const runmark::Finding& finding : findings) {
        if (finding.id == id) return true;
    }
    return false;
}

bool hasFindingFor(const QVector<runmark::Finding>& findings, const QString& id, const QString& value)
{
    for (const runmark::Finding& finding : findings) {
        if (finding.id == id && finding.explanation.contains(value)) return true;
    }
    return false;
}

} // namespace

int main()
{
    QTemporaryDir project;
    if (!project.isValid()) return 1;
    const QString root = project.path();
    const QString repository = root + QStringLiteral("/repo");
    const QString remote = root + QStringLiteral("/remote.git");
    const QString config = root + QStringLiteral("/.runmark/project.json");

    if (!git({QStringLiteral("init"), QStringLiteral("--bare"), remote})
            || !git({QStringLiteral("init"), QStringLiteral("-b"), QStringLiteral("main"), repository})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.invalid")})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("Runmark Test")})
            || !writeFile(repository + QStringLiteral("/obsolete.txt"), "old\n")
            || !git({QStringLiteral("-C"), repository, QStringLiteral("add"), QStringLiteral("obsolete.txt")})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("initial")})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("remote"), QStringLiteral("add"), QStringLiteral("origin"), remote})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("push"), QStringLiteral("-u"), QStringLiteral("origin"), QStringLiteral("main")})) return 1;

    if (!QDir().mkpath(root + QStringLiteral("/.runmark"))
            || !writeFile(root + QStringLiteral("/plan.md"), "- [x] MF-1\n")
            || !writeFile(config, R"({"version":1,"name":"test","worktree_root":"worktrees","repos":[{"name":"repo","path":"repo","base":{"remote":"origin","branch":"main"}}],"plan":{"path":"plan.md"},"task_id_pattern":"MF-\\d+"})")) return 1;

    try {
        // ADR-014: kirli ana repo baslatmayi engellemez, uyarir ve ledger'a yazar.
        if (!writeFile(repository + QStringLiteral("/scratch.txt"), "dirty\n")) return 1;
        const runmark::StartResult started = runmark::startExecution(config, QStringLiteral("MF-1"), QStringLiteral("codex"), {});
        if (!hasFinding(started.warnings, QStringLiteral("git.dirty_workspace"))) return 1;
        if (!QFile::remove(repository + QStringLiteral("/scratch.txt"))) return 1;
        const QString executionId = started.exec;
        const QString worktree = started.worktree;
        if (executionId.isEmpty() || !git({QStringLiteral("-C"), worktree, QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.invalid")})
                || !git({QStringLiteral("-C"), worktree, QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("Runmark Test")})
                || !QFile::remove(worktree + QStringLiteral("/obsolete.txt"))
                || !git({QStringLiteral("-C"), worktree, QStringLiteral("add"), QStringLiteral("-u")})
                || !git({QStringLiteral("-C"), worktree, QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("remove obsolete")})) return 1;

        runmark::recordEvidence(config, executionId, QStringLiteral("test"), QStringLiteral("1 passed"), {});
        runmark::recordEvidence(config, executionId, QStringLiteral("agent_summary"), QStringLiteral("Removed obsolete file"), {});
        runmark::recordNote(config, executionId, QStringLiteral("unresolved"), QStringLiteral("Needs follow-up"), {});
        const QString handoff = root + QStringLiteral("/.runmark/handoffs/") + executionId + QStringLiteral(".md");
        if (!QDir().mkpath(handoff)) return 1;
        try {
            runmark::finishExecution(config, executionId, QStringLiteral("finished"));
            return 1;
        } catch (const std::exception&) {
        }
        if (!QDir(handoff).removeRecursively()) return 1;
        const runmark::FinishResult finished = runmark::finishExecution(config, executionId, QStringLiteral("finished"));
        QFile handoffFile(handoff);
        if (!handoffFile.open(QIODevice::ReadOnly)) return 1;
        const QString handoffText = QString::fromUtf8(handoffFile.readAll());

        const runmark::StartResult active = runmark::startExecution(config, QStringLiteral("MF-2"), QStringLiteral("codex"), {});
        const QString activeExecutionId = active.exec;
        const QString activeLedger = root + QStringLiteral("/.runmark/ledger/") + activeExecutionId + QStringLiteral(".jsonl");
        const QVector<runmark::Finding> activeFindings = runmark::projectStatus(config).findings;
        if (!hasFinding(activeFindings, QStringLiteral("context.active_execution"))) return 1;
        QFile activeLedgerFile(activeLedger);
        if (!activeLedgerFile.open(QIODevice::ReadOnly)) return 1;
        QByteArray activeLedgerContents = activeLedgerFile.readAll();
        activeLedgerFile.close();
        const int timestampStart = activeLedgerContents.indexOf("\"ts\":\"");
        if (timestampStart < 0) return 1;
        const int timestampValue = timestampStart + 6;
        activeLedgerContents.replace(timestampValue, 20, "2000-01-01T00:00:00Z");
        if (!writeFile(activeLedger, activeLedgerContents)) return 1;
        const QVector<runmark::Finding> orphanFindings = runmark::projectStatus(config).findings;
        if (!hasFinding(orphanFindings, QStringLiteral("context.orphaned_execution"))) return 1;
        activeLedgerContents.replace("2000-01-01T00:00:00Z", "BOZUK-TARIH");
        if (!writeFile(activeLedger, activeLedgerContents)) return 1;
        const QVector<runmark::Finding> invalidTimestampFindings = runmark::projectStatus(config).findings;
        if (!hasFinding(invalidTimestampFindings, QStringLiteral("context.invalid_ledger_timestamp"))) return 1;

        const QString externalWorktree = root + QStringLiteral("/worktrees/MF-3");
        if (!git({QStringLiteral("-C"), repository, QStringLiteral("worktree"), QStringLiteral("add"), QStringLiteral("-b"), QStringLiteral("external/MF-3"), externalWorktree, QStringLiteral("origin/main")})
                || !git({QStringLiteral("-C"), repository, QStringLiteral("commit"), QStringLiteral("--allow-empty"), QStringLiteral("-m"), QStringLiteral("advance after external worktree")})
                || !git({QStringLiteral("-C"), repository, QStringLiteral("push")})) return 1;
        const QString externalHead = gitOutput({QStringLiteral("-C"), externalWorktree, QStringLiteral("rev-parse"), QStringLiteral("HEAD")});
        const runmark::StartResult adopted = runmark::startExecution(config, QStringLiteral("MF-3"), QStringLiteral("claude"), {});
        if (externalHead.isEmpty()
                || adopted.worktree != externalWorktree
                || adopted.branch != QLatin1String("external/MF-3")
                || adopted.workspaceSource != QLatin1String("adopted")
                || adopted.baseSha != externalHead) return 1;

        if (!writeFile(config, R"({"version":1,"name":"test","worktree_root":"worktrees","repos":[{"name":"repo","path":"worktrees/MF-1","base":{"remote":"origin","branch":"main"}}],"plan":{"path":"plan.md"},"task_id_pattern":"MF-\\d+"})")) return 1;
        if (!writeFile(root + QStringLiteral("/plan.md"), "- [x] MF-1\nupdated\n")
                || !git({QStringLiteral("-C"), repository, QStringLiteral("commit"), QStringLiteral("--allow-empty"), QStringLiteral("-m"), QStringLiteral("advance base")})
                || !git({QStringLiteral("-C"), repository, QStringLiteral("push")})) return 1;
        const runmark::StatusResult status = runmark::projectStatus(config);
        const QVector<runmark::Finding> findings = status.findings;
        const QVector<runmark::RepoFacts> repositories = status.repositories;
        if (repositories.isEmpty()) return 1;
        const runmark::RepoFacts repositoryStatus = repositories.at(0);
        if (finished.outcome != QLatin1String("finished")
                || repositoryStatus.ahead != 1
                || !handoffText.contains(QStringLiteral("## Verified (produced by Runmark)"))
                || !handoffText.contains(QStringLiteral("## Agent note (weak evidence \u2014 unverified)"))
                || !handoffText.contains(QStringLiteral("Removed obsolete file"))
                || !handoffText.contains(QStringLiteral("## Open items"))
                || !handoffText.contains(QStringLiteral("Needs follow-up"))
                || !hasFinding(findings, QStringLiteral("context.unresolved_without_ref"))
                || !hasFinding(findings, QStringLiteral("git.orphaned_worktree"))
                || hasFindingFor(findings, QStringLiteral("plan.changed_during_execution"), QStringLiteral("MF-1"))
                || hasFindingFor(findings, QStringLiteral("git.stale_worktree_base"), executionId)) return 1;

        if (!writeFile(worktree + QStringLiteral("/untracked.txt"), "dirty\n")
                || !git({QStringLiteral("-C"), worktree, QStringLiteral("remote"), QStringLiteral("set-url"), QStringLiteral("origin"), root + QStringLiteral("/missing.git")})) return 1;
        const QVector<runmark::Finding> offlineFindings = runmark::projectStatus(config).findings;
        if (repositoryStatus.dirty
                || !hasFinding(offlineFindings, QStringLiteral("git.fetch_failed"))
                || !hasFinding(offlineFindings, QStringLiteral("git.dirty_workspace"))) return 1;
        return 0;
    } catch (const std::exception&) {
        return 1;
    }
    return 1;
}
