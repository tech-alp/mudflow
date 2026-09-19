#include "mudflow/workflow.h"

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

bool writeFile(const QString& path, const QByteArray& contents)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

bool hasFinding(const QJsonArray& findings, const QString& id)
{
    for (const QJsonValue& value : findings) {
        if (value.toObject().value(QStringLiteral("id")).toString() == id) return true;
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
    const QString config = root + QStringLiteral("/.mudflow/project.json");

    if (!git({QStringLiteral("init"), QStringLiteral("--bare"), remote})
            || !git({QStringLiteral("init"), QStringLiteral("-b"), QStringLiteral("main"), repository})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.invalid")})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("Mudflow Test")})
            || !writeFile(repository + QStringLiteral("/obsolete.txt"), "old\n")
            || !git({QStringLiteral("-C"), repository, QStringLiteral("add"), QStringLiteral("obsolete.txt")})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("initial")})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("remote"), QStringLiteral("add"), QStringLiteral("origin"), remote})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("push"), QStringLiteral("-u"), QStringLiteral("origin"), QStringLiteral("main")})) return 1;

    if (!QDir().mkpath(root + QStringLiteral("/.mudflow"))
            || !writeFile(root + QStringLiteral("/plan.md"), "- [x] MF-1\n")
            || !writeFile(config, R"({"version":1,"name":"test","worktree_root":"worktrees","repos":[{"name":"repo","path":"repo","base":"origin/main"}],"plan":{"path":"plan.md"},"task_id_pattern":"MF-\\d+"})")) return 1;

    try {
        const QJsonObject started = mudflow::startExecution(config, QStringLiteral("MF-1"), QStringLiteral("codex"), {});
        const QString executionId = started.value(QStringLiteral("exec")).toString();
        const QString worktree = started.value(QStringLiteral("worktree")).toString();
        if (executionId.isEmpty() || !git({QStringLiteral("-C"), worktree, QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.invalid")})
                || !git({QStringLiteral("-C"), worktree, QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("Mudflow Test")})
                || !QFile::remove(worktree + QStringLiteral("/obsolete.txt"))
                || !git({QStringLiteral("-C"), worktree, QStringLiteral("add"), QStringLiteral("-u")})
                || !git({QStringLiteral("-C"), worktree, QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("remove obsolete")})) return 1;

        mudflow::recordEvidence(config, executionId, QStringLiteral("test"), QStringLiteral("1 passed"), {});
        mudflow::recordEvidence(config, executionId, QStringLiteral("agent_summary"), QStringLiteral("Removed obsolete file"), {});
        mudflow::recordNote(config, executionId, QStringLiteral("unresolved"), QStringLiteral("Needs follow-up"), {});
        const QString handoff = root + QStringLiteral("/.mudflow/handoffs/") + executionId + QStringLiteral(".md");
        if (!QDir().mkpath(handoff)) return 1;
        try {
            mudflow::finishExecution(config, executionId, QStringLiteral("finished"));
            return 1;
        } catch (const std::exception&) {
        }
        if (!QDir(handoff).removeRecursively()) return 1;
        const QJsonObject finished = mudflow::finishExecution(config, executionId, QStringLiteral("finished"));
        QFile handoffFile(handoff);
        if (!handoffFile.open(QIODevice::ReadOnly)) return 1;
        const QString handoffText = QString::fromUtf8(handoffFile.readAll());
        if (!writeFile(config, R"({"version":1,"name":"test","worktree_root":"worktrees","repos":[{"name":"repo","path":"worktrees/MF-1","base":"origin/main"}],"plan":{"path":"plan.md"},"task_id_pattern":"MF-\\d+"})")) return 1;
        if (!writeFile(root + QStringLiteral("/plan.md"), "- [x] MF-1\nupdated\n")
                || !git({QStringLiteral("-C"), repository, QStringLiteral("commit"), QStringLiteral("--allow-empty"), QStringLiteral("-m"), QStringLiteral("advance base")})
                || !git({QStringLiteral("-C"), repository, QStringLiteral("push")})) return 1;
        const QJsonObject status = mudflow::projectStatus(config);
        const QJsonArray findings = status.value(QStringLiteral("findings")).toArray();
        const QJsonArray repositories = status.value(QStringLiteral("repositories")).toArray();
        if (repositories.isEmpty()) return 1;
        const QJsonObject repositoryStatus = repositories.at(0).toObject();
        if (finished.value(QStringLiteral("outcome")).toString() != QLatin1String("finished")
                || repositoryStatus.value(QStringLiteral("ahead_of_base")).toInt() != 1
                || !handoffText.contains(QStringLiteral("## Doğrulanmış (Mudflow üretti)"))
                || !handoffText.contains(QStringLiteral("## Agent notu (zayıf evidence — doğrulanmadı)"))
                || !handoffText.contains(QStringLiteral("Removed obsolete file"))
                || !handoffText.contains(QStringLiteral("## Açık kalanlar"))
                || !handoffText.contains(QStringLiteral("Needs follow-up"))
                || !hasFinding(findings, QStringLiteral("context.unresolved_without_ref"))
                || hasFinding(findings, QStringLiteral("plan.changed_during_execution"))
                || hasFinding(findings, QStringLiteral("git.stale_worktree_base"))) return 1;

        if (!writeFile(worktree + QStringLiteral("/untracked.txt"), "dirty\n")
                || !git({QStringLiteral("-C"), worktree, QStringLiteral("remote"), QStringLiteral("set-url"), QStringLiteral("origin"), root + QStringLiteral("/missing.git")})) return 1;
        const QJsonArray offlineFindings = mudflow::projectStatus(config).value(QStringLiteral("findings")).toArray();
        if (repositoryStatus.value(QStringLiteral("dirty")).toBool()
                || !hasFinding(offlineFindings, QStringLiteral("git.fetch_failed"))
                || !hasFinding(offlineFindings, QStringLiteral("git.dirty_workspace"))) return 1;
        return 0;
    } catch (const std::exception&) {
        return 1;
    }
    return 1;
}
