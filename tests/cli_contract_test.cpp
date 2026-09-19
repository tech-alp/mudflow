#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>

namespace {

bool writeFile(const QString& path, const QByteArray& contents)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
}

bool hasError(const QByteArray& output, const QString& code)
{
    const QJsonDocument document = QJsonDocument::fromJson(output);
    return document.isObject() && document.object().value(QStringLiteral("error")).toObject().value(QStringLiteral("code")).toString() == code;
}

bool run(const QString& executable, const QStringList& arguments, int expectedExitCode, QByteArray* standardOutput, QByteArray* standardError)
{
    QProcess process;
    process.start(executable, arguments);
    return process.waitForStarted(5000)
        && process.waitForFinished(5000)
        && process.exitCode() == expectedExitCode
        && ((*standardOutput = process.readAllStandardOutput()), true)
        && ((*standardError = process.readAllStandardError()), true);
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) return 1;
    const QString executable = QString::fromLocal8Bit(argv[1]);
    QByteArray standardOutput;
    QByteArray standardError;

    if (!run(executable, {QStringLiteral("nonsense")}, 2, &standardOutput, &standardError)
            || !standardOutput.isEmpty()
            || !hasError(standardError, QStringLiteral("usage"))) return 1;

    if (!run(executable, {QStringLiteral("--project"), QStringLiteral("/does/not/exist/project.json"), QStringLiteral("status")}, 1, &standardOutput, &standardError)
            || !standardOutput.isEmpty()
            || !hasError(standardError, QStringLiteral("runtime"))) return 1;

    QTemporaryDir directory;
    if (!directory.isValid()
            || !writeFile(directory.filePath(QStringLiteral("project.json")), R"({"version":1,"name":"test","worktree_root":"worktrees","repos":[{"name":"repo","path":".","base":"origin/main"}],"plan":{"path":"plan.md"},"task_id_pattern":"MF-\\d+"})")) return 1;
    if (!run(executable, {QStringLiteral("--project"), directory.filePath(QStringLiteral("project.json")), QStringLiteral("inspect")}, 0, &standardOutput, &standardError)
            || !standardError.isEmpty()
            || !QJsonDocument::fromJson(standardOutput).isObject()) return 1;

    return 0;
}
