#include "FindingFilterModel.h"
#include "FindingModel.h"

#include <QMap>

namespace runmark {
FindingFilterModel::FindingFilterModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    setSortRole(FindingModel::DomainRole);
    sort(0);
    connect(this, &QAbstractItemModel::modelReset, this, &FindingFilterModel::reconcileSelection);
    connect(this, &QAbstractItemModel::rowsInserted, this, &FindingFilterModel::reconcileSelection);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &FindingFilterModel::reconcileSelection);
    connect(this, &QAbstractItemModel::dataChanged, this, &FindingFilterModel::reconcileSelection);
}

bool FindingFilterModel::filterAcceptsRow(int row, const QModelIndex& parent) const
{
    const auto item = sourceModel()->index(row, 0, parent);
    if (!m_domain.isEmpty() && item.data(FindingModel::DomainRole).toString() != m_domain)
        return false;
    for (const int role : {FindingModel::IdRole, FindingModel::TitleRole,
             FindingModel::ExplanationRole, FindingModel::DomainRole, FindingModel::SuggestedActionRole}) {
        if (item.data(role).toString().contains(m_query, Qt::CaseInsensitive)) return true;
    }
    return false;
}

void FindingFilterModel::setQuery(const QString& value)
{
    if (m_query == value) return;
    beginFilterChange();
    m_query = value;
    endFilterChange(Direction::Rows);
    reconcileSelection();
}

void FindingFilterModel::setDomain(const QString& value)
{
    if (m_domain == value) return;
    beginFilterChange();
    m_domain = value;
    endFilterChange(Direction::Rows);
    reconcileSelection();
}

QString FindingFilterModel::keyAt(int row) const
{
    return index(row, 0).data(FindingModel::KeyRole).toString();
}

void FindingFilterModel::setSelectedKey(const QString& value)
{
    m_selectedKey = value;
    reconcileSelection();
}

QVariantMap FindingFilterModel::selectedFinding() const
{
    if (m_selectedKey.isEmpty()) return {};
    for (int row = 0; row < rowCount(); ++row) {
        if (keyAt(row) != m_selectedKey) continue;
        QVariantMap result;
        const auto roles = roleNames();
        for (auto role = roles.cbegin(); role != roles.cend(); ++role)
            result.insert(QString::fromUtf8(role.value()), index(row, 0).data(role.key()));
        return result;
    }
    return {};
}

QVariantList FindingFilterModel::domains() const
{
    QMap<QString, int> counts;
    if (sourceModel()) {
        for (int row = 0; row < sourceModel()->rowCount(); ++row)
            ++counts[sourceModel()->index(row, 0).data(FindingModel::DomainRole).toString()];
    }
    QVariantList result;
    for (auto it = counts.cbegin(); it != counts.cend(); ++it)
        result.append(QVariantMap{{"value", it.key()}, {"label", FindingModel::domainLabel(it.key())}, {"count", it.value()}});
    return result;
}

void FindingFilterModel::reconcileSelection()
{
    if (!m_selectedKey.isEmpty() && selectedFinding().isEmpty()) m_selectedKey.clear();
    emit changed();
}
}
