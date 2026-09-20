#include "mudflow/project_config.h"
#include "mudflow/version.h"
#include "mudflow/workflow.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

#include <exception>

namespace {

// stdout = sonuc, stderr = hata. Ikisi de JSON; sinyal exit kodu.
int emitError(const QString& code, const QString& message, int exitCode)
{
    const QJsonObject payload{{QStringLiteral("error"), QJsonObject{
        {QStringLiteral("code"), code},
        {QStringLiteral("message"), message},
    }}};
    QTextStream(stderr) << QJsonDocument(payload).toJson(QJsonDocument::Indented);
    return exitCode;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("mudflow"));
    app.setApplicationVersion(QStringLiteral(MUDFLOW_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Local-first execution continuity CLI"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption projectOption(
        {QStringLiteral("p"), QStringLiteral("project")},
        QStringLiteral("Path to project.json."),
        QStringLiteral("path"),
        QDir::current().filePath(QStringLiteral(".mudflow/project.json")));
    parser.addOption(projectOption);
    const QCommandLineOption agentOption(QStringLiteral("agent"), QStringLiteral("Agent: codex or claude."), QStringLiteral("agent"), QStringLiteral("codex"));
    const QCommandLineOption repositoryOption(QStringLiteral("repo"), QStringLiteral("Repository name."), QStringLiteral("name"));
    const QCommandLineOption outcomeOption(QStringLiteral("outcome"), QStringLiteral("Finish outcome."), QStringLiteral("outcome"), QStringLiteral("finished"));
    const QCommandLineOption kindOption(QStringLiteral("kind"), QStringLiteral("Evidence or note kind."), QStringLiteral("kind"));
    const QCommandLineOption summaryOption(QStringLiteral("summary"), QStringLiteral("Evidence summary."), QStringLiteral("text"));
    const QCommandLineOption textOption(QStringLiteral("text"), QStringLiteral("Note text."), QStringLiteral("text"));
    const QCommandLineOption referenceOption(QStringLiteral("ref"), QStringLiteral("Durable source reference."), QStringLiteral("reference"));
    const QCommandLineOption instructionOption(QStringLiteral("instruction"), QStringLiteral("Instruction path, added to project instructions; repeatable."), QStringLiteral("path"));
    const QCommandLineOption markdownOption(QStringLiteral("markdown"), QStringLiteral("Render resume as Markdown."));
    parser.addOptions({agentOption, repositoryOption, outcomeOption, kindOption, summaryOption, textOption, referenceOption, instructionOption, markdownOption});
    parser.addPositionalArgument(QStringLiteral("command"), QStringLiteral("inspect, status, start, finish, resume, evidence, or note."));
    parser.addPositionalArgument(QStringLiteral("argument"), QStringLiteral("Task or execution ID, depending on command."), QStringLiteral("[argument]"));
    parser.process(app);

    const QStringList arguments = parser.positionalArguments();
    try {
        const QString configPath = parser.value(projectOption);
        QJsonObject result;
        if (arguments == QStringList{QStringLiteral("inspect")}) {
            result = mudflow::inspectProject(configPath);
        } else if (arguments == QStringList{QStringLiteral("status")}) {
            result = mudflow::projectStatus(configPath);
        } else if (arguments.size() == 2 && arguments.constFirst() == QLatin1String("start")) {
            result = mudflow::startExecution(configPath, arguments.constLast(), parser.value(agentOption), parser.value(repositoryOption), parser.values(instructionOption));
        } else if (arguments.size() == 2 && arguments.constFirst() == QLatin1String("resume")) {
            result = mudflow::resumeExecution(configPath, arguments.constLast());
            if (parser.isSet(markdownOption)) {
                QTextStream(stdout) << mudflow::resumeMarkdown(result);
                return 0;
            }
        } else if (arguments.size() == 2 && arguments.constFirst() == QLatin1String("finish")) {
            result = mudflow::finishExecution(configPath, arguments.constLast(), parser.value(outcomeOption));
        } else if (arguments.size() == 2 && arguments.constFirst() == QLatin1String("evidence") && parser.isSet(kindOption) && parser.isSet(summaryOption)) {
            mudflow::recordEvidence(configPath, arguments.constLast(), parser.value(kindOption), parser.value(summaryOption), parser.value(referenceOption));
            result = {{QStringLiteral("recorded"), QStringLiteral("evidence")}};
        } else if (arguments.size() == 2 && arguments.constFirst() == QLatin1String("note") && parser.isSet(kindOption) && parser.isSet(textOption)) {
            mudflow::recordNote(configPath, arguments.constLast(), parser.value(kindOption), parser.value(textOption), parser.value(referenceOption));
            result = {{QStringLiteral("recorded"), QStringLiteral("note")}};
        } else {
            return emitError(QStringLiteral("usage"),
                QStringLiteral("Usage: mudflow <inspect|status|start|finish|resume|evidence|note> [argument] [options]"), 2);
        }
        QTextStream(stdout) << QJsonDocument(result).toJson(QJsonDocument::Indented);
        return 0;
    } catch (const std::exception& error) {
        return emitError(QStringLiteral("runtime"), QString::fromUtf8(error.what()), 1);
    }
}
