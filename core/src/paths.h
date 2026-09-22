#pragma once

#include <QDateTime>
#include <QString>
#include <optional>
#include "runmark/facts.h"

namespace runmark {

// .runmark durum dizininin yerleşimi.
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
QJsonArray observeInstructions(const QStringList& instructions, const QString& root);

// .runmark/hook-observed.json: ajan hook'unun son calistigi an. Ledger olayi
// degil; bir execution'a ait degil ve okuma yolunda yazilir.
QString readHookObservation(const Paths& paths, std::optional<QDateTime>& lastSeen);
void writeHookObservation(const Paths& paths);

} // namespace runmark
