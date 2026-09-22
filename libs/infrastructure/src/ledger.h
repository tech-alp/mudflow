#pragma once

#include "paths.h"

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace runmark {

QString nowUtc();

// One JSONL file per execution: two agents writing at once never collide,
// so no lock is needed (DATA_MODEL.md §1).
void appendEvent(const Paths& paths, const QString& executionId, const QJsonObject& event);
QVector<QJsonObject> readEvents(const Paths& paths);
QJsonObject startedEvent(const QVector<QJsonObject>& events, const QString& executionId);
ResumeFacts observeResumeLedger(const Paths& paths, const QString& taskOrExecution);

// Content addressed: the same payload lands in the same file.
QString writeEvidence(const Paths& paths, const QJsonObject& value);

} // namespace runmark
