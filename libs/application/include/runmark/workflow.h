#pragma once

// Use case'ler. Sonuçlar TİPLİ döner; JSON'a çevirmek CLI'nin, model'e
// çevirmek UI'nin işidir (TC-012). Application bir sunum biçimine bağlı değil.

#include "runmark/facts.h"
#include "runmark/finding.h"
#include "runmark/project_config.h"

#include <QString>
#include <QStringList>
#include <QVector>

namespace runmark {

struct StatusResult {
    QString project;
    QVector<RepoFacts> repositories;
    QVector<Finding> findings;
};

struct StartResult {
    QString exec;
    QString worktree;
    QString branch;
    QString workspaceSource;
    QString baseSha;
    QString preservedRef;      // boşsa kaydedilecek iş yoktu
    QVector<Finding> warnings;
};

struct FinishResult {
    QString exec;
    QString outcome;
    QString headSha;
    QString preservedRef;
    QString handoff;           // .runmark köküne göre
};

struct ResumeResult {
    ResumeFacts facts;
    QVector<Finding> gaps;
};

ProjectConfig inspectProject(const QString& configPath);
StatusResult projectStatus(const QString& configPath);
StartResult startExecution(const QString& configPath, const QString& task, const QString& agent, const QString& repositoryName, const QStringList& instructions = {});
ResumeResult resumeExecution(const QString& configPath, const QString& taskOrExecution, bool observedByHook = false);
FinishResult finishExecution(const QString& configPath, const QString& executionId, const QString& outcome);
void recordEvidence(const QString& configPath, const QString& executionId, const QString& kind, const QString& summary, const QString& reference);
void recordNote(const QString& configPath, const QString& executionId, const QString& kind, const QString& text, const QString& reference);

} // namespace runmark
