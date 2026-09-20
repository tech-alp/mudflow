#include "paths.h"

#include "error.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace mudflow {

QString expandPath(const QString& value, const QString& root)
{
    if (value == QLatin1String("~")) {
        return QDir::homePath();
    }
    if (value.startsWith(QLatin1String("~/"))) {
        return QDir::home().filePath(value.mid(2));
    }
    return QFileInfo(value).isAbsolute() ? QDir::cleanPath(value) : QDir(root).absoluteFilePath(value);
}

Paths pathsFor(const QString& configPath)
{
    QDir configDirectory = QFileInfo(configPath).absoluteDir();
    if (!configDirectory.cdUp()) {
        fail(QStringLiteral("Project config must be inside .mudflow"));
    }
    const QString root = configDirectory.absolutePath();
    const QString state = QDir(root).filePath(QStringLiteral(".mudflow"));
    return {root, state, QDir(state).filePath(QStringLiteral("ledger")), QDir(state).filePath(QStringLiteral("evidence")), QDir(state).filePath(QStringLiteral("handoffs"))};
}

void ensureDirectories(const Paths& paths)
{
    if (!QDir().mkpath(paths.ledger) || !QDir().mkpath(paths.evidence) || !QDir().mkpath(paths.handoffs)) {
        fail(QStringLiteral("Cannot create .mudflow state directories"));
    }
}

QString sha1File(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(&file);
    return QString::fromLatin1(hash.result().toHex());
}

} // namespace mudflow
