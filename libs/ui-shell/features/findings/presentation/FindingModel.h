#pragma once

// QML-facing model. Owns no rules: it holds what the application layer
// measured and exposes it by role (TC-012).
//
// ADR-019: a MOC header must not depend on types that live inside a named
// module. `runmark::Finding` reaches this layer through the module, so the row
// below is a plain UI-local copy and the conversion happens in a .cpp that
// imports the module. Including the module shim here breaks the moc build.

#include <QAbstractListModel>
#include <QString>
#include <QVector>
#include <QtQmlIntegration>

namespace runmark {

class FindingModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Owned by StatusViewModel")

public:
    struct Row {
        QString id;
        QString severity;
        QString domain;
        QString title;
        QString explanation;
        QString suggestedAction;
    };

    enum Role { IdRole = Qt::UserRole + 1, SeverityRole, DomainRole, TitleRole, ExplanationRole, SuggestedActionRole, KeyRole, DomainLabelRole };

    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    static QString domainLabel(const QString& domain);

    void reset(const QVector<Row>& rows);

private:
    QVector<Row> m_rows;
};

} // namespace runmark
