#pragma once

#include "paths.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace runmark {

QString nowUtc();

// Execution başına ayrı JSONL dosyası — iki agent aynı anda yazarken
// çakışma olmaz, kilit gerekmez (DATA_MODEL.md §1).
void appendEvent(const Paths& paths, const QString& executionId, const QJsonObject& event);
QVector<QJsonObject> readEvents(const Paths& paths);
QJsonObject startedEvent(const QVector<QJsonObject>& events, const QString& executionId);
ResumeFacts observeResumeLedger(const Paths& paths, const QString& taskOrExecution);

// İçerik adresli: aynı payload aynı dosyaya düşer.
QString writeEvidence(const Paths& paths, const QJsonObject& value);

} // namespace runmark
