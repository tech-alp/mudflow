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
        apply(result.project, result.rows, result.error);
    });
    watcher->setFuture(QtConcurrent::run([configPath = m_configPath]() {
        Measurement measurement;
        try {
            const StatusResult result = projectStatus(configPath);
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
    if (error.isEmpty() && !qEnvironmentVariableIsSet("RUNMARK_SMOKE"))
        QSettings().setValue(QStringLiteral("desktop/lastProject"), m_configPath);
    emit changed();
}

} // namespace runmark
