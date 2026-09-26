#pragma once

#include <QDateTime>
#include <QString>
#include <optional>
#include "runmark/facts.h"

namespace runmark {

// Layout of the .runmark state directory.
struct Paths {
    QString root;
    QString state;
    QString ledger;
    QString evidence;
    QString handoffs;
    QString sessions;       // one JSONL file per agent session
};

QString expandPath(const QString& value, const QString& root);
Paths pathsFor(const QString& configPath);
void ensureDirectories(const Paths& paths);
QString sha1File(const QString& path);
FileFacts observePath(const QString& path);
QVector<Instruction> observeInstructions(const QStringList& instructions, const QString& root);


} // namespace runmark
