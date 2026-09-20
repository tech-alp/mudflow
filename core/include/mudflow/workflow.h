#pragma once

#include <QJsonObject>
#include <QString>

namespace mudflow {

QJsonObject inspectProject(const QString& configPath);
QJsonObject projectStatus(const QString& configPath);
QJsonObject startExecution(const QString& configPath, const QString& task, const QString& agent, const QString& repositoryName);
QJsonObject finishExecution(const QString& configPath, const QString& executionId, const QString& outcome);
void recordEvidence(const QString& configPath, const QString& executionId, const QString& kind, const QString& summary, const QString& reference);
void recordNote(const QString& configPath, const QString& executionId, const QString& kind, const QString& text, const QString& reference);

} // namespace mudflow
