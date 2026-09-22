// Plan kurallarinin sessizce kor kalmadigini dogrular.
// Git repo gerektirmez: repo yolu cozulemeyince status repo raporuna hata yazip
// plan bolumunu yine calistirir.

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

bool hasFinding(const QJsonArray& findings, const QString& id)
{
    for (const QJsonValue& value : findings) {
        if (value.toObject().value(QStringLiteral("id")).toString() == id) {
            return true;
        }
    }
    return false;
}

QJsonArray findingsFor(const QString& config)
{
    return runmark::projectStatus(config).value(QStringLiteral("findings")).toArray();
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
        // 1. Baslik formati ("## [x] MF-1") parser'in gormedigi format: uyarmali.
        if (!writeFile(plan, "## [x] MF-1 done\n")) return 1;
        if (!hasFinding(findingsFor(config), QStringLiteral("plan.no_parsable_tasks"))) return 1;

        // 2. Checklist var ama task_id_pattern tutmuyor: yine uyarmali.
        if (!writeFile(plan, "- [x] birsey\n- [ ] baska birsey\n")) return 1;
        if (!hasFinding(findingsFor(config), QStringLiteral("plan.no_parsable_tasks"))) return 1;

        // 3. Yalnizca acik maddeler: kural kor degil, uyarmamali.
        if (!writeFile(plan, "- [ ] MF-1 henuz baslamadi\n")) return 1;
        const QJsonArray openOnly = findingsFor(config);
        if (hasFinding(openOnly, QStringLiteral("plan.no_parsable_tasks"))
                || hasFinding(openOnly, QStringLiteral("plan.done_without_evidence"))) return 1;

        // 4. Dogru format, kanit yok: asil kural calismali, korluk uyarisi cikmamali.
        if (!writeFile(plan, "- [x] MF-1 kanit yok\n")) return 1;
        const QJsonArray done = findingsFor(config);
        if (!hasFinding(done, QStringLiteral("plan.done_without_evidence"))
                || hasFinding(done, QStringLiteral("plan.no_parsable_tasks"))) return 1;

        // 5. Plan dosyasi yoksa susmamali.
        if (!QFile::remove(plan)) return 1;
        if (!hasFinding(findingsFor(config), QStringLiteral("plan.unreadable"))) return 1;
    } catch (const std::exception&) {
        return 1;
    }
    return 0;
}
