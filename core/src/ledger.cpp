#include "ledger.h"

#include "error.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>

namespace mudflow {

QString nowUtc()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}

void appendEvent(const Paths& paths, const QString& executionId, const QJsonObject& event)
{
    QFile file(QDir(paths.ledger).filePath(executionId + QStringLiteral(".jsonl")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        fail(QStringLiteral("Cannot append ledger %1: %2").arg(file.fileName(), file.errorString()));
    }
    file.write(QJsonDocument(event).toJson(QJsonDocument::Compact));
    file.write("\n");
}

QVector<QJsonObject> readEvents(const Paths& paths)
{
    QVector<QJsonObject> events;
    const QStringList files = QDir(paths.ledger).entryList({QStringLiteral("*.jsonl")}, QDir::Files, QDir::Name);
    for (const QString& name : files) {
        QFile file(QDir(paths.ledger).filePath(name));
        if (!file.open(QIODevice::ReadOnly)) {
            fail(QStringLiteral("Cannot read ledger %1").arg(file.fileName()));
        }
        while (!file.atEnd()) {
            const QByteArray line = file.readLine().trimmed();
            if (line.isEmpty()) {
                continue;
            }
            QJsonParseError error;
            const QJsonDocument document = QJsonDocument::fromJson(line, &error);
            if (error.error != QJsonParseError::NoError || !document.isObject()) {
                fail(QStringLiteral("Invalid ledger event in %1").arg(file.fileName()));
            }
            events.append(document.object());
        }
    }
    return events;
}

QJsonObject startedEvent(const QVector<QJsonObject>& events, const QString& executionId)
{
    for (const QJsonObject& event : events) {
        if (event.value(QStringLiteral("type")) == QLatin1String("execution.started")
                && event.value(QStringLiteral("exec")) == executionId) {
            return event;
        }
    }
    fail(QStringLiteral("Unknown execution: %1").arg(executionId));
}

QString writeEvidence(const Paths& paths, const QJsonObject& value)
{
    const QByteArray data = QJsonDocument(value).toJson(QJsonDocument::Compact);
    const QString name = QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex()) + QStringLiteral(".json");
    QFile file(QDir(paths.evidence).filePath(name));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fail(QStringLiteral("Cannot write evidence %1").arg(file.fileName()));
    }
    file.write(data);
    return QStringLiteral("evidence/") + name;
}

} // namespace mudflow
