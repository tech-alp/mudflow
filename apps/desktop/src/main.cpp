#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCommandLineParser>
#include <QDir>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Runmark"));

    QCommandLineParser parser;
    parser.addHelpOption();
    const QCommandLineOption projectOption(
        {QStringLiteral("p"), QStringLiteral("project")},
        QStringLiteral("Path to project.json."), QStringLiteral("path"),
        QDir::current().filePath(QStringLiteral(".runmark/project.json")));
    parser.addOption(projectOption);
    parser.process(app);

    QQmlApplicationEngine engine;
    // Smoke mode waits for the first measurement and reports it, so the test
    // proves the whole chain (QML -> view model -> application), not just that
    // a window opened.
    engine.setInitialProperties({
        {QStringLiteral("configPath"), parser.value(projectOption)},
        {QStringLiteral("smoke"), qEnvironmentVariableIsSet("RUNMARK_SMOKE")}});
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("Runmark.Shell", "Shell");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    return app.exec();
}
