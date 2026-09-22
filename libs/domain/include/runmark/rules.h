#pragma once

#include "runmark/facts.h"
#include "runmark/project_config.h"

#include <QJsonArray>
#include <QJsonObject>

namespace runmark {

// Finding üretir. SAF: dosya sistemi, git veya saat okumaz.
// Aynı facts her zaman aynı finding dizisini verir — testi düz fonksiyon çağrısı.
QJsonArray evaluate(const ProjectConfig& config, const StatusFacts& facts);
QJsonArray evaluateResume(const ResumeFacts& facts);

// Finding şeması tek yerde (TRUST_MODEL.md). start uyarıları da bunu kullanır.
QJsonObject finding(const QString& id, const QString& severity, const QString& domain,
                    const QString& title, const QString& explanation, const QString& action = {});

} // namespace runmark
