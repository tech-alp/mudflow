#include "config_io.h"

#include "runmark/error.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>

namespace runmark {

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
