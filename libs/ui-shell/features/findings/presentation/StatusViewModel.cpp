#include "StatusViewModel.h"

#include "runmark/workflow.h"

#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QSettings>
#include <QtConcurrentRun>

#include <exception>

namespace runmark {
namespace {
struct Measurement {
    QString project;
    QUrl folder;
    QUrl plan;
    QVector<FindingModel::Row> rows;
    QString error;
};
}

StatusViewModel::StatusViewModel(QObject* parent)
    : QObject(parent)
{
}

void StatusViewModel::restoreProject(const QString& explicitPath)
{
    if (m_busy) return;
    QString path = explicitPath;
    if (path.isEmpty()) {
        if (!qEnvironmentVariableIsSet("RUNMARK_SMOKE"))
            path = QSettings().value(QStringLiteral("desktop/lastProject")).toString();
        const QString local = QDir::current().filePath(QStringLiteral(".runmark/project.json"));
        if (path.isEmpty() && QFileInfo::exists(local)) path = local;
    }
    setConfigPath(path);
    refresh();
}

void StatusViewModel::openFolder(const QUrl& folder)
{
    if (m_busy) return;
    if (!folder.isLocalFile() || !QFileInfo(folder.toLocalFile()).isDir()) {
        m_error = tr("Yerel bir proje klasörü seçin.");
        emit changed();
        return;
    }
    const QString path = QDir(folder.toLocalFile()).filePath(".runmark/project.json");
    const QFileInfo config(path);
    if (config.exists() || config.isSymLink()) {
        openProject(QUrl::fromLocalFile(path));
    } else {
        emit setupRequested(folder, QFileInfo(folder.toLocalFile()).fileName());
    }
}

void StatusViewModel::createProject(const QUrl& folder, const QString& name, const QString& remote,
    const QString& branch, const QString& plan, const QString& taskPrefix)
{
    if (m_busy) return;
    if (!folder.isLocalFile()) {
        emit setupFailed(tr("Yerel bir proje klasörü seçin."));
        return;
    }
    struct Result { QString path; QString error; };
    m_busy = true;
    emit changed();
    auto* watcher = new QFutureWatcher<Result>(this);
    connect(watcher, &QFutureWatcher<Result>::finished, this, [this, watcher] {
        const auto result = watcher->result();
        watcher->deleteLater();
        m_busy = false;
        emit changed();
        if (!result.error.isEmpty()) {
            emit setupFailed(result.error);
            return;
        }
        emit setupCreated();
        openProject(QUrl::fromLocalFile(result.path));
    });
    watcher->setFuture(QtConcurrent::run([folder, name, remote, branch, plan, taskPrefix] {
        Result result;
        try {
            result.path = initializeProject(folder.toLocalFile(), name.trimmed(), remote.trimmed(),
                branch.trimmed(), plan.trimmed(), taskPrefix.trimmed());
        } catch (const std::exception& failure) {
            result.error = QString::fromUtf8(failure.what());
        }
        return result;
    }));
}

void StatusViewModel::openProject(const QUrl& url)
{
    if (m_busy) return;
    if (!url.isLocalFile()) {
        m_error = tr("Yerel bir proje dosyası seçin.");
        emit changed();
        return;
    }
    setConfigPath(url.toLocalFile());
    refresh();
}

void StatusViewModel::setConfigPath(const QString& path)
{
    if (m_busy) return;
    m_configPath = path.isEmpty() ? QString() : QFileInfo(path).absoluteFilePath();
    m_project.clear();
    m_measuredAt = {};
    m_findings.setSourceLocations({}, {});
    m_error.clear();
    m_findings.reset({});
    emit configPathChanged();
}

void StatusViewModel::refresh()
{
    if (m_busy) return;
    if (m_configPath.isEmpty()) {
        emit changed();
        return;
    }
    m_busy = true;
    m_error.clear();
    emit changed();

    // The worker owns only copied data; destroying the view disconnects the
    // watcher without letting a background operation access a deleted QObject.
    auto* watcher = new QFutureWatcher<Measurement>(this);
    connect(watcher, &QFutureWatcher<Measurement>::finished, this, [this, watcher]() {
        const auto result = watcher->result();
        watcher->deleteLater();
        m_findings.setSourceLocations(result.folder, result.plan);
        apply(result.project, result.rows, result.error);
    });
    watcher->setFuture(QtConcurrent::run([configPath = m_configPath]() {
        Measurement measurement;
        try {
            const StatusResult result = projectStatus(configPath);
            QDir root = QFileInfo(configPath).absoluteDir();
            root.cdUp();
            measurement.folder = QUrl::fromLocalFile(root.absolutePath());
            // The first readable plan file; paths come back relative to the root.
            const QString planPath = result.plan.files.isEmpty() ? QString() : root.filePath(result.plan.files.first().path);
            if (!planPath.isEmpty() && QFileInfo(planPath).isFile() && QFileInfo(planPath).isReadable())
                measurement.plan = QUrl::fromLocalFile(QFileInfo(planPath).absoluteFilePath());
            measurement.project = result.project;
            for (const Finding& finding : result.findings) {
                measurement.rows.append({finding.id, finding.severity, finding.domain,
                    finding.title, finding.explanation, finding.suggestedAction});
            }
        } catch (const std::exception& failure) {
            measurement.error = QString::fromUtf8(failure.what());
        }
        return measurement;
    }));
}

void StatusViewModel::apply(const QString& project, const QVector<FindingModel::Row>& rows, const QString& error)
{
    m_project = project;
    m_error = error;
    m_findings.reset(rows);
    m_busy = false;
    if (error.isEmpty()) m_measuredAt = QDateTime::currentDateTime();
    if (error.isEmpty() && !qEnvironmentVariableIsSet("RUNMARK_SMOKE"))
        QSettings().setValue(QStringLiteral("desktop/lastProject"), m_configPath);
    emit changed();
}

} // namespace runmark
