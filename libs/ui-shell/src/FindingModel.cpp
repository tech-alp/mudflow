#include "FindingModel.h"

namespace runmark {

int FindingModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant FindingModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size()) {
        return {};
    }
    const Row& row = m_rows.at(index.row());
    switch (role) {
    case IdRole: return row.id;
    case SeverityRole: return row.severity;
    case DomainRole: return row.domain;
    case TitleRole: return row.title;
    case ExplanationRole: return row.explanation;
    case SuggestedActionRole: return row.suggestedAction;
    default: return {};
    }
}

QHash<int, QByteArray> FindingModel::roleNames() const
{
    return {{IdRole, "findingId"}, {SeverityRole, "severity"}, {DomainRole, "domain"},
        {TitleRole, "title"}, {ExplanationRole, "explanation"}, {SuggestedActionRole, "suggestedAction"}};
}

void FindingModel::reset(const QVector<Row>& rows)
{
    beginResetModel();
    m_rows = rows;
    endResetModel();
}

} // namespace runmark
