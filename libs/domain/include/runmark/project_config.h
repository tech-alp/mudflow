#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace runmark {

struct RepositoryConfig {
    QString name;
    QString path;
    QString remote;
    QString branch;
};

struct ProjectConfig {
    int version = 1;
    QString name;
    QString worktreeRoot;
    QVector<RepositoryConfig> repositories;
    QString planPath;
    QString taskIdPattern;
    bool hooksExpected = false;   // proje bir ajan hook'u bekliyor mu
    QStringList instructions;

    // Pure validation. To read from disk, use loadProjectConfig in infrastructure.
    static ProjectConfig parse(const QJsonObject& root);
    QJsonObject toJson() const;
};

} // namespace runmark
