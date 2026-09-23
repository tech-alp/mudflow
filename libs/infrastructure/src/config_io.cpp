#include "config_io.h"

#include "runmark/error.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>

namespace runmark {

void createProjectConfig(const QString& path, const ProjectConfig& config)
{
    const auto validated = ProjectConfig::parse(config.toJson());
    if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        fail(QStringLiteral("Cannot create project directory"));
    QFile file(path);
    // NewOnly also rejects existing symlinks and concurrent creation.
    if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly))
        fail(QStringLiteral("Cannot create %1: %2").arg(path, file.errorString()));
    const auto data = QJsonDocument(validated.toJson()).toJson();
    if (file.write(data) != data.size() || !file.flush()) {
        const auto error = file.errorString();
        file.remove();
        fail(QStringLiteral("Cannot write project config: ") + error);
    }
}

ProjectConfig loadProjectConfig(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        fail(QStringLiteral("Cannot read %1: %2").arg(path, file.errorString()));
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        fail(QStringLiteral("Invalid project config %1: %2").arg(path, parseError.errorString()));
    }
    return ProjectConfig::parse(document.object());
}

} // namespace runmark
