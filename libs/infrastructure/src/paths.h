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
    QString hookObserved;
};

QString expandPath(const QString& value, const QString& root);
Paths pathsFor(const QString& configPath);
void ensureDirectories(const Paths& paths);
QString sha1File(const QString& path);
FileFacts observePath(const QString& path);
QVector<Instruction> observeInstructions(const QStringList& instructions, const QString& root);

// .runmark/hook-observed.json: when the agent hook last ran. Not a ledger
// event -- it belongs to no execution and is written on a read path.
QString readHookObservation(const Paths& paths, std::optional<QDateTime>& lastSeen);
void writeHookObservation(const Paths& paths);

} // namespace runmark
