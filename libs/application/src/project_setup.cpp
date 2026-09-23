#include "runmark/workflow.h"
#include "runmark/error.h"
#include "config_io.h"
#include "git.h"
#include "paths.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

namespace runmark {
QString initializeProject(const QString& folder, const QString& name, const QString& remote,
    const QString& branch, const QString& plan, const QString& taskPrefix)
{
    const QString root = QFileInfo(folder).canonicalFilePath();
    if (root.isEmpty() || !QFileInfo(root).isDir())
        fail(QStringLiteral("Geçerli bir proje klasörü seçin."));
    const QString gitRoot = gitRequired(root, {"rev-parse", "--show-toplevel"});
    if (QFileInfo(gitRoot).canonicalFilePath() != root)
        fail(QStringLiteral("Git deposunun kök klasörünü seçin."));
    if (!QRegularExpression(QStringLiteral("^[A-Za-z][A-Za-z0-9_]*$")).match(taskPrefix).hasMatch())
        fail(QStringLiteral("Görev öneki harfle başlamalı; harf, rakam ve alt çizgi içerebilir."));
    const QStringList remotes = gitRequired(root, {"remote"}).split('\n', Qt::SkipEmptyParts);
    if (!remotes.contains(remote))
        fail(QStringLiteral("Depoda bu remote tanımlı değil: ") + remote);
    gitRequired(root, {"check-ref-format", "refs/heads/" + branch});
    const QFileInfo planFile(expandPath(plan, root));
    if (!planFile.isFile() || !planFile.isReadable())
        fail(QStringLiteral("Okunabilir bir plan dosyası belirtin."));
    ProjectConfig config;
    config.name = name.trimmed();
    config.worktreeRoot = QDir::home().filePath("worktrees/" + QFileInfo(root).fileName());
    config.taskIdPattern = QRegularExpression::escape(taskPrefix) + QStringLiteral("-\\d+");
    config.planPath = QDir(root).relativeFilePath(planFile.absoluteFilePath());
    config.repositories.append({config.name, QStringLiteral("."), remote, branch});
    if (QFileInfo::exists(QDir(root).filePath("AGENTS.md")))
        config.instructions.append(QStringLiteral("AGENTS.md"));
    const QString path = QDir(root).filePath(".runmark/project.json");
    createProjectConfig(path, config);
    return path;
}
} // namespace runmark
