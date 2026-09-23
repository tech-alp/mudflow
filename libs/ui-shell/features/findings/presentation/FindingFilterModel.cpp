#include "FindingFilterModel.h"
#include "FindingModel.h"

#include <QMap>

namespace runmark {
FindingFilterModel::FindingFilterModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    setSortRole(FindingModel::PriorityRole);
    sort(0);
    connect(this, &QAbstractItemModel::modelReset, this, &FindingFilterModel::reconcileSelection);
    connect(this, &QAbstractItemModel::rowsInserted, this, &FindingFilterModel::reconcileSelection);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &FindingFilterModel::reconcileSelection);
    connect(this, &QAbstractItemModel::dataChanged, this, &FindingFilterModel::reconcileSelection);
}

bool FindingFilterModel::filterAcceptsRow(int row, const QModelIndex& parent) const
{
    const auto item = sourceModel()->index(row, 0, parent);
    if (!m_showInfo && m_query.isEmpty() && m_domain.isEmpty()
            && item.data(FindingModel::PriorityRole).toInt() == 2) return false;
    if (!m_domain.isEmpty() && item.data(FindingModel::DomainRole).toString() != m_domain)
        return false;
    for (const int role : {FindingModel::IdRole, FindingModel::TitleRole,
             FindingModel::ExplanationRole, FindingModel::DomainRole, FindingModel::DomainLabelRole, FindingModel::SuggestedActionRole,
             FindingModel::DisplayTitleRole, FindingModel::SummaryRole, FindingModel::NextStepRole}) {
        if (item.data(role).toString().contains(m_query, Qt::CaseInsensitive)) return true;
    }
    return false;
}

int FindingFilterModel::countSeverity(int priority) const
{
    int count = 0;
    if (!sourceModel()) return count;
    for (int row = 0; row < sourceModel()->rowCount(); ++row) {
        const auto item = sourceModel()->index(row, 0);
        if (priority < 0 || item.data(FindingModel::PriorityRole).toInt() == priority)
            count += item.data(FindingModel::OccurrencesRole).toInt();
    }
    return count;
}
int FindingFilterModel::totalCount() const { return countSeverity(-1); }
int FindingFilterModel::infoCount() const { return countSeverity(2); }
int FindingFilterModel::warningCount() const { return countSeverity(1); }
int FindingFilterModel::criticalCount() const { return countSeverity(0); }
void FindingFilterModel::setShowInfo(bool value)
{
    if (m_showInfo == value) return;
    beginFilterChange();
    m_showInfo = value;
    endFilterChange(Direction::Rows);
    reconcileSelection();
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
            counts[sourceModel()->index(row, 0).data(FindingModel::DomainRole).toString()] += sourceModel()->index(row, 0).data(FindingModel::OccurrencesRole).toInt();
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
