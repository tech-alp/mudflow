#include "paths.h"

#include "error.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

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
    return {root, state, QDir(state).filePath(QStringLiteral("ledger")), QDir(state).filePath(QStringLiteral("evidence")),
            QDir(state).filePath(QStringLiteral("handoffs")), QDir(state).filePath(QStringLiteral("hook-observed.json"))};
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

QString readHookObservation(const Paths& paths, std::optional<QDateTime>& lastSeen)
{
    if (!QFileInfo::exists(paths.hookObserved)) {
        return {};
    }
    QFile file(paths.hookObserved);
    if (!file.open(QIODevice::ReadOnly)) {
        return file.errorString();
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return QStringLiteral("Invalid JSON: ") + parseError.errorString();
    }
    const QString ts = document.object().value(QStringLiteral("ts")).toString();
    const QDateTime observed = QDateTime::fromString(ts, Qt::ISODate);
    if (!observed.isValid()) {
        return QStringLiteral("Invalid ts: ") + ts;
    }
    lastSeen = observed;
    return {};
}

void writeHookObservation(const Paths& paths)
{
    // Sessizce vazgec: hook'un asil isi baglam uretmek, bu kayit yan urun.
    if (!QDir().mkpath(paths.state)) return;
    QFile file(paths.hookObserved);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    file.write(QJsonDocument(QJsonObject{{QStringLiteral("ts"),
        QDateTime::currentDateTimeUtc().toString(Qt::ISODate)}}).toJson(QJsonDocument::Compact));
}

QJsonArray observeInstructions(const QStringList& instructions, const QString& root)
{
    QJsonArray result;
    for (const QString& instruction : instructions) {
        const QString path = expandPath(instruction, root);
        const QString sha1 = sha1File(path);
        result.append(QJsonObject{{QStringLiteral("name"), QFileInfo(path).fileName()},
            {QStringLiteral("path"), path},
            {QStringLiteral("sha1"), sha1.isEmpty() ? QJsonValue::Null : QJsonValue(sha1)}});
    }
    return result;
}

} // namespace mudflow
