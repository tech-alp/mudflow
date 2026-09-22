#include "StatusViewModel.h"

#include "runmark/workflow.h"

#include <QMetaObject>

#include <exception>
#include <thread>

namespace runmark {

StatusViewModel::StatusViewModel(QObject* parent)
    : QObject(parent)
{
}

void StatusViewModel::setConfigPath(const QString& path)
{
    m_configPath = path;
}

void StatusViewModel::refresh()
{
    if (m_busy) {
        return;
    }
    m_busy = true;
    m_error.clear();
    emit changed();

    const QString configPath = m_configPath;
    std::thread([this, configPath]() {
        QString project;
        QVector<FindingModel::Row> rows;
        QString error;
        try {
            const StatusResult result = projectStatus(configPath);
            project = result.project;
            // The translation lives here, on the module side of the boundary.
            for (const Finding& finding : result.findings) {
                rows.append({finding.id, finding.severity, finding.domain,
                    finding.title, finding.explanation, finding.suggestedAction});
            }
        } catch (const std::exception& failure) {
            // A failure is reported, never swallowed: an empty list would look
            // exactly like a clean project.
            error = QString::fromUtf8(failure.what());
        }
        QMetaObject::invokeMethod(this, [this, project, rows, error]() {
            apply(project, rows, error);
        }, Qt::QueuedConnection);
    }).detach();
}

void StatusViewModel::apply(const QString& project, const QVector<FindingModel::Row>& rows, const QString& error)
{
    m_project = project;
    m_error = error;
    m_findings.reset(rows);
    m_busy = false;
    emit changed();
}

} // namespace runmark
