#pragma once

#include <QSortFilterProxyModel>
#include <QVariantList>
#include <QVariantMap>
#include <QtQmlIntegration>

namespace runmark {
class FindingFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY changed)
    Q_PROPERTY(QString domain READ domain WRITE setDomain NOTIFY changed)
    Q_PROPERTY(QString selectedKey READ selectedKey WRITE setSelectedKey NOTIFY changed)
    Q_PROPERTY(QVariantMap selectedFinding READ selectedFinding NOTIFY changed)
    Q_PROPERTY(QVariantList domains READ domains NOTIFY changed)
    Q_PROPERTY(bool showInfo READ showInfo WRITE setShowInfo NOTIFY changed)
    Q_PROPERTY(int infoCount READ infoCount NOTIFY changed)
    Q_PROPERTY(int warningCount READ warningCount NOTIFY changed)
    Q_PROPERTY(int criticalCount READ criticalCount NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY changed)
public:
    explicit FindingFilterModel(QObject* parent = nullptr);
    QString query() const { return m_query; }
    QString domain() const { return m_domain; }
    QString selectedKey() const { return m_selectedKey; }
    int count() const { return rowCount(); }
    int totalCount() const;
    int infoCount() const;
    int warningCount() const;
    int criticalCount() const;
    bool showInfo() const { return m_showInfo; }
    void setShowInfo(bool value);
    QVariantMap selectedFinding() const;
    QVariantList domains() const;
    void setQuery(const QString& value);
    void setDomain(const QString& value);
    void setSelectedKey(const QString& value);
    Q_INVOKABLE QString keyAt(int row) const;
signals:
    void changed();
protected:
    bool filterAcceptsRow(int row, const QModelIndex& parent) const override;
private:
    int countSeverity(int priority) const;
    void reconcileSelection();
    bool m_showInfo = true;
    QString m_query;
    QString m_domain;
    QString m_selectedKey;
};
}
