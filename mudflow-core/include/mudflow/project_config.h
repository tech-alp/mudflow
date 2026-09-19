#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

namespace mudflow {

struct RepositoryConfig {
    QString name;
    QString path;
    QString base;
};

struct ProjectConfig {
    int version = 1;
    QString name;
    QString worktreeRoot;
    QVector<RepositoryConfig> repositories;
    QString planPath;
    QString taskIdPattern;

    static ProjectConfig load(const QString& path);
    QJsonObject toJson() const;
};

} // namespace mudflow
