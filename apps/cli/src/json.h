#pragma once

// TC-007'nin çıktı sözleşmesi. Application tipli döner; JSON'a çevirmek
// CLI'nin işidir — başka bir yüzey (desktop) aynı sonuçtan başka bir şey
// üretir ve application ikisini de bilmez (TC-012).

#include "runmark/workflow.h"

#include <QJsonObject>
#include <QString>

namespace runmark {

QJsonObject toJson(const Finding& finding);
QJsonObject toJson(const RepoFacts& facts);
QJsonObject toJson(const StatusResult& result);
QJsonObject toJson(const StartResult& result);
QJsonObject toJson(const FinishResult& result);
QJsonObject toJson(const ResumeResult& result);

// Aynı sonucun ikinci sunumu: agent'ın okuyacağı markdown.
QString resumeMarkdown(const QJsonObject& package);

} // namespace runmark
