#pragma once

#include "runmark/facts.h"
#include "runmark/project_config.h"
#include "paths.h"

#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace runmark {

struct ProcessResult {
    int exitCode;
    QString output;
    QString error;
};

// --- transport: unaware of what it carries (TC-006) ---
ProcessResult git(const QString& repository, const QStringList& arguments);
QString gitRequired(const QString& repository, const QStringList& arguments);
QString gitCommonDir(const QString& repository);

// --- measurement ---
QString baseRef(const RepositoryConfig& repository);
RepoFacts observeRepo(const RepositoryConfig& repository, const QString& repositoryPath);
void observeResumeGit(const ProjectConfig& config, const Paths& paths, ResumeFacts& facts);

// Captures uncommitted work under refs/runmark/preserved/<exec>. Touches
// neither the working tree nor the global stash stack. Empty when clean.
QString preserveWorktree(const QString& worktree, const QString& executionId, const QString& previousRef = {});

// Writes the directories Runmark produces to .git/info/exclude, never to
// the shared .gitignore.
void ensureGitExcludes(const Paths& paths);

} // namespace runmark
