#include "FindingModel.h"

#include <QJsonArray>
#include <QJsonDocument>

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
    // Rule IDs repeat across repositories/tasks. Preserve selection only for
    // the same finding content, never for another instance of the same rule.
    case KeyRole: return QString::fromUtf8(QJsonDocument(QJsonArray{
        row.id, row.domain, row.title, row.explanation, row.suggestedAction}).toJson(QJsonDocument::Compact));
    case IdRole: return row.id;
    case SeverityRole: return row.severity;
    case DomainLabelRole: return domainLabel(row.domain);
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
        {TitleRole, "title"}, {ExplanationRole, "explanation"}, {SuggestedActionRole, "suggestedAction"}, {KeyRole, "findingKey"}, {DomainLabelRole, "domainLabel"}};
}

QString FindingModel::domainLabel(const QString& domain)
{
    if (domain == QLatin1String("git")) return tr("Git");
    if (domain == QLatin1String("plan")) return tr("Plan");
    if (domain == QLatin1String("context")) return tr("Bağlam");
    if (domain == QLatin1String("documentation")) return tr("Dokümantasyon");
    return domain;
}

void FindingModel::reset(const QVector<Row>& rows)
{
    beginResetModel();
    m_rows = rows;
    endResetModel();
}

} // namespace runmark
