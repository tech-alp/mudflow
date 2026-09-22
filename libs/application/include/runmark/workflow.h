#pragma once

// Use cases. Results come back TYPED: turning them into JSON is the CLI's
// job, into a model the UI's (TC-012). The application layer is not bound to
// any one presentation.

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
    QString preservedRef;      // empty when there was nothing to preserve
    QVector<Finding> warnings;
};

struct FinishResult {
    QString exec;
    QString outcome;
    QString headSha;
    QString preservedRef;
    QString handoff;           // relative to the .runmark root
    // What the worktree looks like now that the execution is closed. Reported
    // at the moment the decision is made, so nobody has to go looking for the
    // standing git.orphaned_worktree finding.
    WorktreeCleanupFacts worktree;
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
