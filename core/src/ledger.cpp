#include "ledger.h"

#include "error.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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

ResumeFacts observeResumeLedger(const Paths& paths, const QString& taskOrExecution)
{
    ResumeFacts facts;
    facts.task = taskOrExecution;
    QVector<QJsonObject> events;
    try {
        const FileFacts ledger = observePath(paths.ledger);
        if (!ledger.exists.has_value() || (ledger.exists == true
                && (!QFileInfo(paths.ledger).isDir() || !QFileInfo(paths.ledger).isReadable()
                    || !QFileInfo(paths.ledger).isExecutable()))) {
            fail(QStringLiteral("Cannot list ledger directory: ") + paths.ledger);
        }
        events = readEvents(paths);
    } catch (const std::exception& error) {
        facts.ledgerError = QString::fromUtf8(error.what());
        return facts;
    }
    // IDs carry UTC time; lexical order is the ledger's documented chronology.
    // Bos secici "en son execution" demektir: oturum baslangicinda hangi task'ta
    // oldugunu bilmeyen bir cagiran icin tek anlamli varsayilan bu.
    for (const QJsonObject& event : events) {
        const QString exec = event.value(QStringLiteral("exec")).toString();
        if (!taskOrExecution.isEmpty() && exec == taskOrExecution) {
            facts.exec = exec;
            break;
        }
        if (event.value(QStringLiteral("type")) != QLatin1String("execution.started")) continue;
        const bool matches = taskOrExecution.isEmpty()
            || event.value(QStringLiteral("task")) == taskOrExecution;
        if (matches && exec > facts.exec) {
            facts.exec = exec;
        }
    }
    if (facts.exec.isEmpty()) return facts;
    for (const QJsonObject& event : events) {
        if (event.value(QStringLiteral("exec")) != facts.exec) continue;
        facts.events.append(event);
        if (event.value(QStringLiteral("type")) == QLatin1String("execution.started")) facts.started = event;
        if (event.value(QStringLiteral("type")) == QLatin1String("execution.finished")) facts.finished = event;
    }
    if (!facts.started.isEmpty()) facts.task = facts.started.value(QStringLiteral("task")).toString();
    return facts;
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
