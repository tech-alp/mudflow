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

int lineCount(const QString& path, const QString& expected)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return -1;
    int count = 0;
    while (!file.atEnd()) {
        if (QString::fromUtf8(file.readLine()).trimmed() == expected) ++count;
    }
    return count;
}

} // namespace

int main()
{
    QTemporaryDir directory;
    if (!directory.isValid()) return 1;
    const QString repository = directory.path() + QStringLiteral("/repo");
    const QString remote = directory.path() + QStringLiteral("/remote.git");
    const QString config = repository + QStringLiteral("/.runmark/project.json");

    if (!git({QStringLiteral("init"), QStringLiteral("--bare"), remote})
            || !git({QStringLiteral("init"), QStringLiteral("-b"), QStringLiteral("main"), repository})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("test@example.invalid")})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("Runmark Test")})
            || !QDir().mkpath(repository + QStringLiteral("/.runmark"))
            || !writeFile(repository + QStringLiteral("/plan.md"), "- [ ] MF-1\n")
            || !writeFile(config, R"({"version":1,"name":"test","worktree_root":"worktrees","repos":[{"name":"repo","path":".","base":{"remote":"origin","branch":"main"}}],"plan":{"path":"plan.md"},"task_id_pattern":"MF-\\d+"})")
            || !git({QStringLiteral("-C"), repository, QStringLiteral("add"), QStringLiteral(".")})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("initial")})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("remote"), QStringLiteral("add"), QStringLiteral("origin"), remote})
            || !git({QStringLiteral("-C"), repository, QStringLiteral("push"), QStringLiteral("-u"), QStringLiteral("origin"), QStringLiteral("main")})) return 1;

    try {
        runmark::projectStatus(config);
        if (!writeFile(repository + QStringLiteral("/.runmark/ledger/probe.jsonl"), "{}\n")
                || !gitOutput({QStringLiteral("-C"), repository, QStringLiteral("status"), QStringLiteral("--porcelain")}).isEmpty()) return 1;

        runmark::projectStatus(config);
        const QString commonDirValue = gitOutput({QStringLiteral("-C"), repository, QStringLiteral("rev-parse"), QStringLiteral("--git-common-dir")});
        const QString commonDir = QDir::isAbsolutePath(commonDirValue) ? commonDirValue : QDir(repository).filePath(commonDirValue);
        for (const QString& pattern : {QStringLiteral("/.runmark/ledger/"), QStringLiteral("/.runmark/evidence/"), QStringLiteral("/.runmark/handoffs/"), QStringLiteral("/.runmark/sessions/")}) {
            const int occurrences = lineCount(commonDir + QStringLiteral("/info/exclude"), pattern);
            if (occurrences != 1) return 1;
        }
    } catch (...) {
        return 1;
    }
    return 0;
}
