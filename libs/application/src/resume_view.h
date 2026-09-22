#pragma once

#include "runmark/facts.h"

#include <QJsonArray>
#include <QJsonObject>

namespace runmark {

// Olculmus resume facts'ini CLI sozlesmesindeki JSON paketine cevirir.
QJsonObject resumePackage(const ResumeFacts& facts, const QJsonArray& gaps);

} // namespace runmark
