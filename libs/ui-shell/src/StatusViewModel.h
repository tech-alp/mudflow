#pragma once

// The QML bridge. Holds no rules: it calls an application use case and hands
// the typed result to a model (TC-012). Every judgement stays in the domain.

#include "FindingModel.h"

#include <QObject>
#include <QString>
#include <QtQmlIntegration>

namespace runmark {

class StatusViewModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString project READ project NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(QString error READ error NOTIFY changed)
    Q_PROPERTY(runmark::FindingModel* findings READ findings CONSTANT)

public:
    explicit StatusViewModel(QObject* parent = nullptr);

    QString project() const { return m_project; }
    bool busy() const { return m_busy; }
    QString error() const { return m_error; }
    FindingModel* findings() { return &m_findings; }

    // Absolute path to project.json. Set once before refresh().
    Q_INVOKABLE void setConfigPath(const QString& path);
    // Runs off the GUI thread: status shells out to git fetch, which can take
    // seconds. The application API stays synchronous; the wait lives here.
    Q_INVOKABLE void refresh();

signals:
    void changed();

private:
    void apply(const QString& project, const QVector<FindingModel::Row>& rows, const QString& error);

    QString m_configPath;
    QString m_project;
    QString m_error;
    bool m_busy = false;
    FindingModel m_findings;
};

} // namespace runmark
