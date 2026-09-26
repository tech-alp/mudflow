#include "paths.h"

#include "runmark/error.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

namespace runmark {

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
        fail(QStringLiteral("Project config must be inside .runmark"));
    }
    const QString root = configDirectory.absolutePath();
    const QString state = QDir(root).filePath(QStringLiteral(".runmark"));
    return {root, state, QDir(state).filePath(QStringLiteral("ledger")), QDir(state).filePath(QStringLiteral("evidence")),
            QDir(state).filePath(QStringLiteral("handoffs")), QDir(state).filePath(QStringLiteral("sessions"))};
}

void ensureDirectories(const Paths& paths)
{
    if (!QDir().mkpath(paths.ledger) || !QDir().mkpath(paths.evidence) || !QDir().mkpath(paths.handoffs)) {
        fail(QStringLiteral("Cannot create .runmark state directories"));
    }
}

QString sha1File(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QCryptographicHash hash(QCryptographicHash::Sha1);
    if (!hash.addData(&file) || file.error() != QFileDevice::NoError) return {};
    return QString::fromLatin1(hash.result().toHex());
}

FileFacts observePath(const QString& path)
{
    FileFacts facts;
    facts.path = path;
    if (path.isEmpty()) {
        facts.error = QStringLiteral("No path was recorded");
        return facts;
    }
    const QFileInfo info(path);
    if (info.exists()) {
        facts.exists = true;
    } else {
        // An inaccessible parent is not proof that its child is absent.
        QDir parent = info.absoluteDir();
        while (!parent.exists() && parent.cdUp()) {}
        if (QFileInfo(parent.absolutePath()).isReadable() && QFileInfo(parent.absolutePath()).isExecutable()) {
            facts.exists = false;
        } else {
            facts.error = QStringLiteral("Cannot inspect parent directory: ") + parent.absolutePath();
        }
    }
    return facts;
}

QVector<Instruction> observeInstructions(const QStringList& instructions, const QString& root)
{
    QVector<Instruction> result;
    for (const QString& instruction : instructions) {
        const QString path = expandPath(instruction, root);
        result.append({QFileInfo(path).fileName(), path, sha1File(path)});
    }
    return result;
}

} // namespace runmark
