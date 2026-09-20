#pragma once

#include "paths.h"

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace mudflow {

// Handoff'un ölçülen kısmı. Agent iddiası buraya girmez — o, ledger'daki
// agent_summary olaylarından ayrı başlıkta yazılır (ADR-002 / ADR-013).
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

// Üç başlık ADR-002'yi formatın içine gömer: ölçülen, iddia edilen, açık kalan.
void writeHandoff(const Paths& paths, const HandoffInput& input, const QVector<QJsonObject>& events);
void readHandoff(const Paths& paths, ResumeFacts& facts);
QJsonObject resumePackage(const ResumeFacts& facts, const QJsonArray& gaps);

} // namespace mudflow
