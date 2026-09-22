#pragma once

#include "paths.h"

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace runmark {

// The measured half of a handoff. An agent's claim never enters here; it is
// written under its own heading from the ledger's agent_summary events
// (ADR-002 / ADR-013).
struct HandoffInput {
    QString executionId;
    QString outcome;
    QJsonObject started;
    QString worktree;
    QString baseSha;
    QString headSha;
    QStringList commitLines;
    QStringList files;
    int filesChanged = 0;
    int insertions = 0;
    int deletions = 0;
    QString filesRef;
    QString preservedRef;
};

// Three headings put ADR-002 inside the format: measured, claimed, open.
void writeHandoff(const Paths& paths, const HandoffInput& input, const QVector<QJsonObject>& events);
void readHandoff(const Paths& paths, ResumeFacts& facts);

} // namespace runmark
