#include "git.h"

#include "error.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryFile>

#include <stdexcept>

namespace mudflow {
namespace {

ProcessResult run(const QString& program, const QStringList& arguments, const QProcessEnvironment& environment = QProcessEnvironment::systemEnvironment())
{
    QProcess process;
    process.setProgram(program);
    process.setArguments(arguments);
    process.setProcessEnvironment(environment);
    process.start();
    if (!process.waitForStarted(5000)) {
        fail(QStringLiteral("Cannot start %1: %2").arg(program, process.errorString()));
    }
    if (!process.waitForFinished(60000)) {
        process.kill();
        fail(QStringLiteral("Timed out: %1 %2").arg(program, arguments.join(QLatin1Char(' '))));
    }
    return {process.exitCode(), QString::fromUtf8(process.readAllStandardOutput()).trimmed(), QString::fromUtf8(process.readAllStandardError()).trimmed()};
}

QString gitWithIndexRequired(const QString& repository, const QStringList& arguments, const QString& indexPath)
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("GIT_INDEX_FILE"), indexPath);
    QStringList gitArguments{QStringLiteral("-C"), repository};
    gitArguments.append(arguments);
    const ProcessResult result = run(QStringLiteral("git"), gitArguments, environment);
    if (result.exitCode != 0) {
        fail(QStringLiteral("git -C %1 %2: %3").arg(repository, arguments.join(QLatin1Char(' ')), result.error));
    }
    return result.output;
}

int leftRightCount(const QString& output, int index)
{
    return output.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts).value(index).toInt();
}

} // namespace

ProcessResult git(const QString& repository, const QStringList& arguments)
{
    QStringList gitArguments{QStringLiteral("-C"), repository};
    gitArguments.append(arguments);
    return run(QStringLiteral("git"), gitArguments);
}

QString gitRequired(const QString& repository, const QStringList& arguments)
{
    const ProcessResult result = git(repository, arguments);
    if (result.exitCode != 0) {
        fail(QStringLiteral("git -C %1 %2: %3").arg(repository, arguments.join(QLatin1Char(' ')), result.error));
    }
    return result.output;
}

QString gitCommonDir(const QString& repository)
{
    const QString commonDir = gitRequired(repository, {QStringLiteral("rev-parse"), QStringLiteral("--git-common-dir")});
    const QString absolutePath = QDir::isAbsolutePath(commonDir) ? QDir::cleanPath(commonDir) : QDir::cleanPath(QDir(repository).absoluteFilePath(commonDir));
    const QString canonicalPath = QFileInfo(absolutePath).canonicalFilePath();
    return canonicalPath.isEmpty() ? absolutePath : canonicalPath;
}

QString baseRef(const RepositoryConfig& repository)
{
    return repository.remote + QLatin1Char('/') + repository.branch;
}

RepoFacts observeRepo(const RepositoryConfig& repository, const QString& repositoryPath)
{
    RepoFacts facts;
    facts.name = repository.name;
    facts.path = repositoryPath;
    facts.base = baseRef(repository);

    // Fetch ayrı sarılır: başarısızlığı diğer ölçümleri iptal etmemeli.
    // Dirty state ve yerel base geriliği ağ gerektirmez.
    const ProcessResult fetchResult = git(repositoryPath, {QStringLiteral("fetch"), QStringLiteral("--quiet"), repository.remote});
    if (fetchResult.exitCode != 0) {
        facts.fetchError = fetchResult.error;
    }

    try {
        const QString dirty = gitRequired(repositoryPath, {QStringLiteral("status"), QStringLiteral("--porcelain")});
        facts.dirty = !dirty.isEmpty();
        facts.head = gitRequired(repositoryPath, {QStringLiteral("rev-parse"), QStringLiteral("HEAD")});
        facts.branch = gitRequired(repositoryPath, {QStringLiteral("branch"), QStringLiteral("--show-current")});
        facts.baseSha = gitRequired(repositoryPath, {QStringLiteral("rev-parse"), facts.base});
        const QString counts = gitRequired(repositoryPath, {QStringLiteral("rev-list"), QStringLiteral("--left-right"), QStringLiteral("--count"), facts.base + QStringLiteral("...HEAD")});
        facts.behind = leftRightCount(counts, 0);
        facts.ahead = leftRightCount(counts, 1);

        facts.localBase = repository.branch;
        const ProcessResult localBaseExists = git(repositoryPath, {QStringLiteral("rev-parse"), QStringLiteral("--verify"), facts.localBase});
        if (localBaseExists.exitCode == 0) {
            facts.localBaseExists = true;
            facts.localBehind = leftRightCount(
                gitRequired(repositoryPath, {QStringLiteral("rev-list"), QStringLiteral("--left-right"), QStringLiteral("--count"), facts.base + QStringLiteral("...") + facts.localBase}), 0);
        }
        facts.measured = true;
    } catch (const std::exception& error) {
        facts.measurementError = QString::fromUtf8(error.what());
    }
    return facts;
}

QJsonObject toJson(const RepoFacts& facts)
{
    QJsonObject report{
        {QStringLiteral("name"), facts.name},
        {QStringLiteral("path"), facts.path},
        {QStringLiteral("base"), facts.base},
    };
    if (!facts.fetchError.isEmpty()) {
        report.insert(QStringLiteral("fetch_error"), facts.fetchError);
    }
    if (facts.measured) {
        report.insert(QStringLiteral("branch"), facts.branch);
        report.insert(QStringLiteral("head"), facts.head);
        report.insert(QStringLiteral("base_sha"), facts.baseSha);
        report.insert(QStringLiteral("behind_base"), facts.behind);
        report.insert(QStringLiteral("ahead_of_base"), facts.ahead);
        report.insert(QStringLiteral("dirty"), facts.dirty);
    } else {
        report.insert(QStringLiteral("error"), facts.measurementError);
    }
    return report;
}

QString preserveWorktree(const QString& worktree, const QString& executionId, const QString& previousRef)
{
    if (gitRequired(worktree, {QStringLiteral("status"), QStringLiteral("--porcelain")}).isEmpty()) return {};

    QTemporaryFile temporaryIndex;
    if (!temporaryIndex.open()) fail(QStringLiteral("Cannot create temporary Git index"));
    const QString indexPath = temporaryIndex.fileName();
    temporaryIndex.close();
    if (!QFile::remove(indexPath)) fail(QStringLiteral("Cannot prepare temporary Git index"));

    gitWithIndexRequired(worktree, {QStringLiteral("read-tree"), QStringLiteral("HEAD")}, indexPath);
    gitWithIndexRequired(worktree, {QStringLiteral("add"), QStringLiteral("-A")}, indexPath);
    const QString tree = gitWithIndexRequired(worktree, {QStringLiteral("write-tree")}, indexPath);
    QStringList commitArguments{QStringLiteral("commit-tree"), tree, QStringLiteral("-p"), QStringLiteral("HEAD")};
    if (!previousRef.isEmpty()) {
        const ProcessResult previous = git(worktree, {QStringLiteral("rev-parse"), QStringLiteral("--verify"), previousRef});
        if (previous.exitCode == 0) {
            commitArguments.append({QStringLiteral("-p"), previous.output});
        }
    }
    commitArguments.append({QStringLiteral("-m"), QStringLiteral("mudflow: preserve uncommitted work for ") + executionId});
    const QString preservedObject = gitRequired(worktree, commitArguments);
    const QString preservedRef = QStringLiteral("refs/mudflow/preserved/") + executionId;
    gitRequired(worktree, {QStringLiteral("update-ref"), preservedRef, preservedObject});
    return preservedRef;
}

void ensureGitExcludes(const Paths& paths)
{
    const ProcessResult insideWorktree = git(paths.root, {QStringLiteral("rev-parse"), QStringLiteral("--is-inside-work-tree")});
    if (insideWorktree.exitCode != 0 || insideWorktree.output != QLatin1String("true")) {
        return;
    }
    const QString repositoryRoot = QDir::cleanPath(QDir(paths.root).absoluteFilePath(
        gitRequired(paths.root, {QStringLiteral("rev-parse"), QStringLiteral("--show-cdup")})));
    const QString commonDir = gitCommonDir(paths.root);
    const QString statePath = QDir::cleanPath(QDir(repositoryRoot).relativeFilePath(paths.state));
    const QStringList patterns{
        QLatin1Char('/') + statePath + QStringLiteral("/ledger/"),
        QLatin1Char('/') + statePath + QStringLiteral("/evidence/"),
        QLatin1Char('/') + statePath + QStringLiteral("/handoffs/"),
    };

    QFile exclude(QDir(commonDir).filePath(QStringLiteral("info/exclude")));
    QByteArray contents;
    if (exclude.exists()) {
        if (!exclude.open(QIODevice::ReadOnly)) {
            fail(QStringLiteral("Cannot read Git exclude file: %1").arg(exclude.fileName()));
        }
        contents = exclude.readAll();
        exclude.close();
    }
    QSet<QString> existing;
    for (const QByteArray& line : contents.split('\n')) {
        existing.insert(QString::fromUtf8(line).trimmed());
    }
    QStringList missing;
    for (const QString& pattern : patterns) {
        if (!existing.contains(pattern)) missing.append(pattern);
    }
    if (missing.isEmpty()) return;

    if (!exclude.open(QIODevice::WriteOnly | QIODevice::Append)) {
        fail(QStringLiteral("Cannot write Git exclude file: %1").arg(exclude.fileName()));
    }
    if (!contents.isEmpty() && !contents.endsWith('\n')) exclude.write("\n");
    for (const QString& pattern : missing) {
        exclude.write(pattern.toUtf8());
        exclude.write("\n");
    }
}

} // namespace mudflow
