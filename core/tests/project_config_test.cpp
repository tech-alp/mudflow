#include "mudflow/project_config.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>

#include <exception>

int main()
{
    QTemporaryDir directory;
    if (!directory.isValid()) {
        return 1;
    }

    const QString path = directory.filePath(QStringLiteral("project.json"));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return 1;
    }
    file.write(R"({
        "version": 1,
        "name": "mudflow",
        "worktree_root": "~/worktrees",
        "repos": [{"name": "mudflow", "path": ".", "base": {"remote": "origin", "branch": "main"}}],
        "plan": {"path": "docs/ROADMAP.md"},
        "task_id_pattern": "MF-\\d+"
    })");
    file.close();

    try {
        const mudflow::ProjectConfig config = mudflow::ProjectConfig::load(path);
        if (config.name != QLatin1String("mudflow")
                || config.repositories.size() != 1
                || !config.instructions.isEmpty()
                || config.toJson().value(QStringLiteral("repos")).toArray().at(0).toObject().value(QStringLiteral("base")).toObject().value(QStringLiteral("remote")).toString() != QLatin1String("origin")
                || config.toJson().value(QStringLiteral("version")).toInt() != 1) {
            return 1;
        }
    } catch (const std::exception&) {
        return 1;
    }

    // Optional instructions must be a list of non-empty paths.
    const QJsonObject valid = mudflow::ProjectConfig::load(path).toJson();
    for (const QJsonValue& instructions : {QJsonValue("AGENTS.md"), QJsonValue(QJsonValue::Null),
            QJsonValue(QJsonArray{42}), QJsonValue(QJsonArray{""})}) {
        QJsonObject invalid = valid;
        invalid.insert(QStringLiteral("instructions"), instructions);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return 1;
        file.write(QJsonDocument(invalid).toJson());
        file.close();
        try {
            mudflow::ProjectConfig::load(path);
            return 1;
        } catch (const std::exception&) {
        }
    }

    file.setFileName(directory.filePath(QStringLiteral("invalid.json")));
    if (!file.open(QIODevice::WriteOnly) || file.write(R"({"version": 2})") < 0) {
        return 1;
    }
    file.close();
    try {
        mudflow::ProjectConfig::load(file.fileName());
        return 1;
    } catch (const std::exception&) {
    }

    file.setFileName(directory.filePath(QStringLiteral("legacy.json")));
    if (!file.open(QIODevice::WriteOnly) || file.write(R"({"version":1,"name":"test","worktree_root":"w","repos":[{"name":"r","path":".","base":"origin/main"}],"plan":{"path":"p"},"task_id_pattern":"T-\\d+"})") < 0) {
        return 1;
    }
    file.close();
    try {
        mudflow::ProjectConfig::load(file.fileName());
        return 1;
    } catch (const std::exception& error) {
        return QString::fromUtf8(error.what()).contains(QStringLiteral("project.repos[].base must be an object with non-empty remote and branch")) ? 0 : 1;
    }
}
