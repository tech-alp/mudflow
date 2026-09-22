#include "runmark/project_config.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QRegularExpression>

#include <stdexcept>

namespace runmark {
namespace {

[[noreturn]] void fail(const QString& message)
{
    throw std::runtime_error(message.toStdString());
}

QString requiredString(const QJsonObject& object, const char* key, const QString& context)
{
    const QJsonValue value = object.value(QLatin1String(key));
    if (!value.isString() || value.toString().trimmed().isEmpty()) {
        fail(context + QLatin1String(".") + QLatin1String(key) + QLatin1String(" must be a non-empty string"));
    }
    return value.toString();
}

} // namespace

// PURE: opens no file. Reading is loadProjectConfig's job in infrastructure,
// which is what keeps the domain free of QFile (TC-009).
ProjectConfig ProjectConfig::parse(const QJsonObject& root)
{
    const QJsonValue version = root.value(QStringLiteral("version"));
    if (!version.isDouble() || version.toInt() != 1) {
        fail(QStringLiteral("version must be 1"));
    }

    ProjectConfig config;
    config.name = requiredString(root, "name", QStringLiteral("project"));
    config.worktreeRoot = requiredString(root, "worktree_root", QStringLiteral("project"));
    config.taskIdPattern = requiredString(root, "task_id_pattern", QStringLiteral("project"));
    if (!QRegularExpression(config.taskIdPattern).isValid()) {
        fail(QStringLiteral("project.task_id_pattern must be a valid regular expression"));
    }

    const QJsonValue hooks = root.value(QStringLiteral("hooks_expected"));
    if (!hooks.isUndefined()) {
        if (!hooks.isBool()) fail(QStringLiteral("project.hooks_expected must be a boolean"));
        config.hooksExpected = hooks.toBool();
    }

    const QJsonValue plan = root.value(QStringLiteral("plan"));
    if (!plan.isObject()) {
        fail(QStringLiteral("project.plan must be an object"));
    }
    config.planPath = requiredString(plan.toObject(), "path", QStringLiteral("project.plan"));

    const QJsonValue instructions = root.value(QStringLiteral("instructions"));
    if (!instructions.isUndefined()) {
        if (!instructions.isArray()) fail(QStringLiteral("project.instructions must be an array of paths"));
        for (const QJsonValue& instruction : instructions.toArray()) {
            if (!instruction.isString() || instruction.toString().trimmed().isEmpty()) {
                fail(QStringLiteral("project.instructions items must be non-empty paths"));
            }
            config.instructions.append(instruction.toString());
        }
    }

    const QJsonValue repositories = root.value(QStringLiteral("repos"));
    if (!repositories.isArray() || repositories.toArray().isEmpty()) {
        fail(QStringLiteral("project.repos must be a non-empty array"));
    }
    for (const QJsonValue& value : repositories.toArray()) {
        if (!value.isObject()) {
            fail(QStringLiteral("Every project.repos item must be an object"));
        }
        const QJsonObject repository = value.toObject();
        const QJsonValue base = repository.value(QStringLiteral("base"));
        if (!base.isObject()) {
            fail(QStringLiteral("project.repos[].base must be an object with non-empty remote and branch"));
        }
        const QJsonObject baseObject = base.toObject();
        config.repositories.append({
            requiredString(repository, "name", QStringLiteral("project.repos[]")),
            requiredString(repository, "path", QStringLiteral("project.repos[]")),
            requiredString(baseObject, "remote", QStringLiteral("project.repos[].base")),
            requiredString(baseObject, "branch", QStringLiteral("project.repos[].base")),
        });
    }

    return config;
}

QJsonObject ProjectConfig::toJson() const
{
    QJsonArray repositoriesJson;
    for (const RepositoryConfig& repository : repositories) {
        repositoriesJson.append(QJsonObject{
            {QStringLiteral("name"), repository.name},
            {QStringLiteral("path"), repository.path},
            {QStringLiteral("base"), QJsonObject{{QStringLiteral("remote"), repository.remote}, {QStringLiteral("branch"), repository.branch}}},
        });
    }

    return {
        {QStringLiteral("version"), version},
        {QStringLiteral("name"), name},
        {QStringLiteral("worktree_root"), worktreeRoot},
        {QStringLiteral("repos"), repositoriesJson},
        {QStringLiteral("plan"), QJsonObject{{QStringLiteral("path"), planPath}}},
        {QStringLiteral("task_id_pattern"), taskIdPattern},
        {QStringLiteral("hooks_expected"), hooksExpected},
        {QStringLiteral("instructions"), QJsonArray::fromStringList(instructions)},
    };
}

} // namespace runmark
