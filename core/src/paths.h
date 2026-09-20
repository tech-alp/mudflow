#pragma once

#include <QString>
#include "mudflow/facts.h"

namespace mudflow {

// .mudflow durum dizininin yerleşimi.
struct Paths {
    QString root;
    QString state;
    QString ledger;
    QString evidence;
    QString handoffs;
};

QString expandPath(const QString& value, const QString& root);
Paths pathsFor(const QString& configPath);
void ensureDirectories(const Paths& paths);
QString sha1File(const QString& path);
FileFacts observePath(const QString& path);
QJsonArray observeInstructions(const QStringList& instructions, const QString& root);

} // namespace mudflow
