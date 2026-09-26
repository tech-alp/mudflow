#include "runmark/workflow.h"

#include <QFile>
#include <QProcess>
#include <QTemporaryDir>

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

} // namespace

int main()
{
    QTemporaryDir directory;
    if (!directory.isValid()) return 1;
    const QString root = directory.path();
    const QString repository = root + QStringLiteral("/repo");
    const QString remote = root + QStringLiteral("/remote.git");
    const QString worktree = root + QStringLiteral("/worktrees/MF-1");
    const QString config = root + QStringLiteral("/.runmark/project.json");

    if (!git({QStringLiteral("init"), QStringLiteral("--bare"), remote})
            || !git({QStringLiteral("init"), QStringLiteral("-b"), QStringLiteral("main"), repository})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.invalid")})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("Runmark Test")})
            || !writeFile(repository + QStringLiteral("/tracked.txt"), "base\n")
            || !git({QStringLiteral("-C"), repository, QStringLiteral("add"), QStringLiteral(".")})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("initial")})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("remote"), QStringLiteral("add"), QStringLiteral("origin"), remote})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("push"), QStringLiteral("-u"), QStringLiteral("origin"), QStringLiteral("main")})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("worktree"), QStringLiteral("add"), QStringLiteral("-b"), QStringLiteral("external/MF-1"), worktree, QStringLiteral("origin/main")})
            || !writeFile(worktree + QStringLiteral("/uncommitted.txt"), "preserve me\n")
            || !QDir().mkpath(root + QStringLiteral("/.runmark"))
            || !writeFile(root + QStringLiteral("/plan.md"), "- [ ] MF-1\n")
            || !writeFile(config, R"({"version":1,"name":"test","worktree_root":"worktrees","repos":[{"name":"repo","path":"repo","base":{"remote":"origin","branch":"main"}}],"plan":{"paths":["plan.md"]},"task_id_pattern":"MF-\\d+"})")) return 1;

    const QString statusBefore = gitOutput({QStringLiteral("-C"), worktree, QStringLiteral("status"), QStringLiteral("--porcelain")});
    const QString stashBefore = gitOutput({QStringLiteral("-C"), worktree, QStringLiteral("stash"), QStringLiteral("list")});
    try {
        const runmark::StartResult started = runmark::startExecution(config, QStringLiteral("MF-1"), QStringLiteral("codex"), {});
        const QString executionId = started.exec;
        const QString preservedRef = started.preservedRef;
        if (executionId.isEmpty()
                || preservedRef != QStringLiteral("refs/runmark/preserved/") + executionId
                || gitOutput({QStringLiteral("-C"), worktree, QStringLiteral("rev-parse"), QStringLiteral("--verify"), preservedRef}).isEmpty()
                || gitOutput({QStringLiteral("-C"), worktree, QStringLiteral("show"), preservedRef + QStringLiteral(":uncommitted.txt")}) != QLatin1String("preserve me")
                || gitOutput({QStringLiteral("-C"), worktree, QStringLiteral("status"), QStringLiteral("--porcelain")}) != statusBefore
                || gitOutput({QStringLiteral("-C"), worktree, QStringLiteral("stash"), QStringLiteral("list")}) != stashBefore) return 1;

        QFile ledger(root + QStringLiteral("/.runmark/ledger/") + executionId + QStringLiteral(".jsonl"));
        if (!ledger.open(QIODevice::ReadOnly) || !QString::fromUtf8(ledger.readAll()).contains(QStringLiteral("\"preserved_ref\":\"") + preservedRef + QLatin1Char('"'))) return 1;

        if (!writeFile(worktree + QStringLiteral("/finish-only.txt"), "preserve this too\n")) return 1;
        const QString statusAtFinish = gitOutput({QStringLiteral("-C"), worktree, QStringLiteral("status"), QStringLiteral("--porcelain")});
        const runmark::FinishResult finished = runmark::finishExecution(config, executionId, QStringLiteral("interrupted"));
        QFile handoff(root + QStringLiteral("/.runmark/") + finished.handoff);
        if (finished.preservedRef != preservedRef
                || !handoff.open(QIODevice::ReadOnly)
                || !QString::fromUtf8(handoff.readAll()).contains(preservedRef)
                || gitOutput({QStringLiteral("-C"), worktree, QStringLiteral("show"), preservedRef + QStringLiteral(":finish-only.txt")}) != QLatin1String("preserve this too")
                || gitOutput({QStringLiteral("-C"), worktree, QStringLiteral("status"), QStringLiteral("--porcelain")}) != statusAtFinish
                || gitOutput({QStringLiteral("-C"), worktree, QStringLiteral("stash"), QStringLiteral("list")}) != stashBefore) return 1;
    } catch (...) {
        return 1;
    }
    return 0;
}
