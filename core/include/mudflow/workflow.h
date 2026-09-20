#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace mudflow {

QJsonObject inspectProject(const QString& configPath);
QJsonObject projectStatus(const QString& configPath);
QJsonObject startExecution(const QString& configPath, const QString& task, const QString& agent, const QString& repositoryName, const QStringList& instructions = {});
QJsonObject resumeExecution(const QString& configPath, const QString& taskOrExecution, bool observedByHook = false);
QString resumeMarkdown(const QJsonObject& package);
QJsonObject finishExecution(const QString& configPath, const QString& executionId, const QString& outcome);
void recordEvidence(const QString& configPath, const QString& executionId, const QString& kind, const QString& summary, const QString& reference);
void recordNote(const QString& configPath, const QString& executionId, const QString& kind, const QString& text, const QString& reference);

} // namespace mudflow
