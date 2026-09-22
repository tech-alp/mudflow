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

// --- taşıma katmanı: ne taşıdığını bilmez (TC-006) ---
ProcessResult git(const QString& repository, const QStringList& arguments);
QString gitRequired(const QString& repository, const QStringList& arguments);
QString gitCommonDir(const QString& repository);

// --- ölçüm ---
QString baseRef(const RepositoryConfig& repository);
RepoFacts observeRepo(const RepositoryConfig& repository, const QString& repositoryPath);
void observeResumeGit(const ProjectConfig& config, const Paths& paths, ResumeFacts& facts);

// Commit'lenmemiş işi refs/runmark/preserved/<exec> altına yakalar.
// Working tree'ye ve global stash yığınına dokunmaz. Temizse boş döner.
QString preserveWorktree(const QString& worktree, const QString& executionId, const QString& previousRef = {});

// Runmark'un ürettiği dizinleri .git/info/exclude'a yazar, .gitignore'a değil.
void ensureGitExcludes(const Paths& paths);

} // namespace runmark
