#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QCommandLineParser>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("Runmark"));
    app.setApplicationName(QStringLiteral("Runmark"));

    QCommandLineParser parser;
    parser.addHelpOption();
    const QCommandLineOption projectOption(
        {QStringLiteral("p"), QStringLiteral("project")},
        QStringLiteral("Path to project.json."), QStringLiteral("path"));
    parser.addOption(projectOption);
    parser.process(app);

    QQmlApplicationEngine engine;
    // Smoke mode waits for the first measurement and reports it, so the test
    // proves the whole chain (QML -> view model -> application), not just that
    // a window opened.
    engine.setInitialProperties({
        {QStringLiteral("configPath"), parser.value(projectOption)},
        {QStringLiteral("themeIndexPath"), QCoreApplication::applicationDirPath()
            + QStringLiteral("/theme/themes/index.json")},
        {QStringLiteral("smoke"), qEnvironmentVariableIsSet("RUNMARK_SMOKE")}});
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("Runmark.Shell", "Shell");
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    return app.exec();
}
