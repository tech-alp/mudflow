#include "runmark/workflow.h"

#include <QFile>
#include <QJsonArray>
#include <QProcess>
#include <QTemporaryDir>

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

} // namespace

int main()
{
    QTemporaryDir directory;
    if (!directory.isValid()) return 1;
    const QString root = directory.path();
    const QString repository = root + QStringLiteral("/repo");
    const QString remote = root + QStringLiteral("/remote.git");
    const QString config = root + QStringLiteral("/.runmark/project.json");

    if (!git({QStringLiteral("init"), QStringLiteral("--bare"), remote})
            || !git({QStringLiteral("init"), QStringLiteral("-b"), QStringLiteral("feature/x"), repository})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.invalid")})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("Runmark Test")})
            || !writeFile(repository + QStringLiteral("/file.txt"), "base\n")
            || !git({QStringLiteral("-C"), repository, QStringLiteral("add"), QStringLiteral(".")})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("initial")})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("remote"), QStringLiteral("add"), QStringLiteral("up"), remote})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("push"), QStringLiteral("-u"), QStringLiteral("up"), QStringLiteral("feature/x")})
            || !QDir().mkpath(root + QStringLiteral("/.runmark"))
            || !writeFile(root + QStringLiteral("/plan.md"), "- [ ] MF-1\n")
            || !writeFile(config, R"({"version":1,"name":"test","worktree_root":"worktrees","repos":[{"name":"repo","path":"repo","base":{"remote":"up","branch":"feature/x"}}],"plan":{"path":"plan.md"},"task_id_pattern":"MF-\\d+"})")) return 1;

    try {
        const QJsonObject status = runmark::projectStatus(config);
        if (status.value(QStringLiteral("repositories")).toArray().at(0).toObject().value(QStringLiteral("base")).toString() != QLatin1String("up/feature/x")) return 1;
        const QJsonObject started = runmark::startExecution(config, QStringLiteral("MF-1"), QStringLiteral("codex"), {});
        return started.value(QStringLiteral("worktree")).toString().isEmpty() ? 1 : 0;
    } catch (...) {
        return 1;
    }
}
