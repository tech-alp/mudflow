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

    static ProjectConfig load(const QString& path);
    QJsonObject toJson() const;
};

} // namespace runmark
