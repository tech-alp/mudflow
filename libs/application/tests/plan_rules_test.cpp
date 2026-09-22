// Verifies that the plan rules never go silently blind. Needs no git repo:
// when the repo path cannot be resolved, status records the error in the repo
// report and still runs the plan section.

#include "runmark/workflow.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QTemporaryDir>

#include <exception>

namespace {

bool writeFile(const QString& path, const QByteArray& contents)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly | QIODevice::Truncate) && file.write(contents) == contents.size();
}

bool hasFinding(const QVector<runmark::Finding>& findings, const QString& id)
{
    for (const runmark::Finding& finding : findings) {
        if (finding.id == id) {
            return true;
        }
    }
    return false;
}

QVector<runmark::Finding> findingsFor(const QString& config)
{
    return runmark::projectStatus(config).findings;
}

} // namespace

int main()
{
    QTemporaryDir project;
    if (!project.isValid()) return 1;
    const QString root = project.path();
    const QString config = root + QStringLiteral("/.runmark/project.json");
    const QString plan = root + QStringLiteral("/plan.md");

    if (!QDir().mkpath(root + QStringLiteral("/.runmark"))
            || !writeFile(config, R"({"version":1,"name":"t","worktree_root":"w","repos":[{"name":"r","path":"missing-repo","base":{"remote":"origin","branch":"main"}}],"plan":{"path":"plan.md"},"task_id_pattern":"MF-\\d+"})")) return 1;

    try {
        // 1. Heading form ("## [x] MF-1") is a shape the parser cannot see: warn.
        if (!writeFile(plan, "## [x] MF-1 done\n")) return 1;
        if (!hasFinding(findingsFor(config), QStringLiteral("plan.no_parsable_tasks"))) return 1;

        // 2. Checklist present but no task_id_pattern match: warn again.
        if (!writeFile(plan, "- [x] birsey\n- [ ] baska birsey\n")) return 1;
        if (!hasFinding(findingsFor(config), QStringLiteral("plan.no_parsable_tasks"))) return 1;

        // 3. Only open items: the rule is not blind, so it must stay quiet.
        if (!writeFile(plan, "- [ ] MF-1 henuz baslamadi\n")) return 1;
        const QVector<runmark::Finding> openOnly = findingsFor(config);
        if (hasFinding(openOnly, QStringLiteral("plan.no_parsable_tasks"))
                || hasFinding(openOnly, QStringLiteral("plan.done_without_evidence"))) return 1;

        // 4. Correct form, no evidence: the real rule fires, the blindness one does not.
        if (!writeFile(plan, "- [x] MF-1 kanit yok\n")) return 1;
        const QVector<runmark::Finding> done = findingsFor(config);
        if (!hasFinding(done, QStringLiteral("plan.done_without_evidence"))
                || hasFinding(done, QStringLiteral("plan.no_parsable_tasks"))) return 1;

        // 5. A missing plan file must not be silent.
        if (!QFile::remove(plan)) return 1;
        if (!hasFinding(findingsFor(config), QStringLiteral("plan.unreadable"))) return 1;
    } catch (const std::exception&) {
        return 1;
    }
    return 0;
}
