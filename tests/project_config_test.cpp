#include "mudflow/project_config.h"

#include <QFile>
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
        "repos": [{"name": "mudflow", "path": ".", "base": "origin/main"}],
        "plan": {"path": "mudflow-docs/ROADMAP.md"},
        "task_id_pattern": "MF-\\d+"
    })");
    file.close();

    try {
        const mudflow::ProjectConfig config = mudflow::ProjectConfig::load(path);
        if (config.name != QLatin1String("mudflow")
                || config.repositories.size() != 1
                || config.toJson().value(QStringLiteral("version")).toInt() != 1) {
            return 1;
        }
    } catch (const std::exception&) {
        return 1;
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
        return 0;
    }
}
