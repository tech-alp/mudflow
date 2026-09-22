// Pure presentation: typed result -> JSON / markdown. Reads no filesystem,
// no git, no clock; its input is already-measured facts. The DISK side of a
// handoff stays in infrastructure (handoff.cpp), because that one writes.
#include "json.h"

#include <QJsonArray>
#include <QTextStream>

namespace runmark {

static QJsonObject resumePackage(const ResumeFacts& facts, const QJsonArray& gaps);

QJsonObject toJson(const Finding& finding)
{
    QJsonObject value{
        {QStringLiteral("id"), finding.id},
        {QStringLiteral("severity"), finding.severity},
        {QStringLiteral("domain"), finding.domain},
        {QStringLiteral("title"), finding.title},
        {QStringLiteral("explanation"), finding.explanation},
    };
    if (!finding.suggestedAction.isEmpty()) {
        value.insert(QStringLiteral("suggested_action"), finding.suggestedAction);
    }
    return value;
}

QJsonObject toJson(const RepoFacts& facts)
{
    QJsonObject report{
        {QStringLiteral("name"), facts.name},
        {QStringLiteral("path"), facts.path},
        {QStringLiteral("base"), facts.base},
    };
    if (!facts.fetchError.isEmpty()) {
        report.insert(QStringLiteral("fetch_error"), facts.fetchError);
    }
    if (facts.measured) {
        report.insert(QStringLiteral("branch"), facts.branch);
        report.insert(QStringLiteral("head"), facts.head);
        report.insert(QStringLiteral("base_sha"), facts.baseSha);
        report.insert(QStringLiteral("behind_base"), facts.behind);
        report.insert(QStringLiteral("ahead_of_base"), facts.ahead);
        report.insert(QStringLiteral("dirty"), facts.dirty);
    } else {
        report.insert(QStringLiteral("error"), facts.measurementError);
    }
    return report;
}

namespace {

QJsonArray toJsonArray(const QVector<Finding>& findings)
{
    QJsonArray array;
    for (const Finding& finding : findings) array.append(toJson(finding));
    return array;
}

QJsonValue orNull(const QString& value)
{
    return value.isEmpty() ? QJsonValue::Null : QJsonValue(value);
}

} // namespace

QJsonObject toJson(const StatusResult& result)
{
    QJsonArray repositories;
    for (const RepoFacts& repository : result.repositories) repositories.append(toJson(repository));
    return {{QStringLiteral("project"), result.project},
            {QStringLiteral("repositories"), repositories},
            {QStringLiteral("findings"), toJsonArray(result.findings)}};
}

QJsonObject toJson(const StartResult& result)
{
    return {{QStringLiteral("exec"), result.exec}, {QStringLiteral("worktree"), result.worktree},
        {QStringLiteral("branch"), result.branch}, {QStringLiteral("workspace_source"), result.workspaceSource},
        {QStringLiteral("base_sha"), result.baseSha}, {QStringLiteral("preserved_ref"), orNull(result.preservedRef)},
        {QStringLiteral("warnings"), toJsonArray(result.warnings)}};
}

QJsonObject toJson(const FinishResult& result)
{
    return {{QStringLiteral("exec"), result.exec}, {QStringLiteral("outcome"), result.outcome},
        {QStringLiteral("head_sha"), result.headSha}, {QStringLiteral("preserved_ref"), orNull(result.preservedRef)},
        {QStringLiteral("handoff"), result.handoff}};
}

QJsonObject toJson(const ResumeResult& result)
{
    return resumePackage(result.facts, toJsonArray(result.gaps));
}

static QJsonObject resumePackage(const ResumeFacts& facts, const QJsonArray& gaps)
{
    const auto nullable = [](const QString& value) -> QJsonValue {
        return value.isEmpty() ? QJsonValue::Null : QJsonValue(value);
    };
    const auto boolean = [](std::optional<bool> value) -> QJsonValue {
        return value.has_value() ? QJsonValue(*value) : QJsonValue::Null;
    };
    const QString recordedPlan = facts.started.value(QStringLiteral("plan_sha1")).toString();
    const QString recordedHandoff = facts.finished.value(QStringLiteral("handoff_sha1")).toString();
    QJsonArray evidence, claims, withRef, withoutRef;
    for (const QJsonObject& event : facts.events) {
        const QString kind = event.value(QStringLiteral("kind")).toString();
        if (event.value(QStringLiteral("type")) == QLatin1String("evidence.recorded")) {
            if (kind == QLatin1String("test") || kind == QLatin1String("command")) evidence.append(event);
            if (kind == QLatin1String("agent_summary")) claims.append(event);
        }
        if (event.value(QStringLiteral("type")) == QLatin1String("note") && kind == QLatin1String("unresolved")) {
            (event.value(QStringLiteral("ref")).toString().isEmpty() ? withoutRef : withRef).append(event);
        }
    }
    QJsonObject measured = facts.measured;
    for (const QString& key : {QStringLiteral("source"), QStringLiteral("commits"), QStringLiteral("files_changed")}) {
        if (!measured.contains(key)) measured.insert(key, QJsonValue::Null);
    }
    measured.insert(QStringLiteral("evidence"), evidence);
    return {{QStringLiteral("task"), facts.task}, {QStringLiteral("exec"), nullable(facts.exec)},
        {QStringLiteral("plan_ref"), nullable(facts.started.value(QStringLiteral("plan_ref")).toString())},
        {QStringLiteral("plan_sha1"), nullable(recordedPlan)},
        {QStringLiteral("current_plan_sha1"), nullable(facts.planSha1)},
        {QStringLiteral("plan_changed"), recordedPlan.isEmpty() || facts.planSha1.isEmpty()
            ? QJsonValue::Null : QJsonValue(recordedPlan != facts.planSha1)},
        {QStringLiteral("workspace"), QJsonObject{
            {QStringLiteral("worktree"), nullable(facts.started.value(QStringLiteral("worktree")).toString())},
            {QStringLiteral("branch"), nullable(facts.started.value(QStringLiteral("branch")).toString())},
            {QStringLiteral("base"), nullable(facts.started.value(QStringLiteral("base")).toString())},
            {QStringLiteral("base_sha"), nullable(facts.started.value(QStringLiteral("base_sha")).toString())},
            {QStringLiteral("remote_base_sha"), nullable(facts.started.value(QStringLiteral("remote_base_sha")).toString())},
            {QStringLiteral("current_base_sha"), nullable(facts.currentBaseSha)},
            {QStringLiteral("worktree_exists"), boolean(facts.worktree.exists)},
            {QStringLiteral("base_advanced"), boolean(facts.baseAdvanced)}}},
        {QStringLiteral("measured"), measured},
        {QStringLiteral("agent_claims"), QJsonObject{{QStringLiteral("verification"), QStringLiteral("unverified")},
            {QStringLiteral("evidence"), claims}}},
        {QStringLiteral("unresolved"), QJsonObject{{QStringLiteral("with_ref"), withRef}, {QStringLiteral("without_ref"), withoutRef}}},
        {QStringLiteral("instructions"), facts.started.contains(QStringLiteral("instructions"))
            ? facts.started.value(QStringLiteral("instructions")) : QJsonValue::Null},
        {QStringLiteral("preserved_ref"), nullable((facts.finished.isEmpty() ? facts.started : facts.finished).value(QStringLiteral("preserved_ref")).toString())},
        {QStringLiteral("handoff"), QJsonObject{{QStringLiteral("path"), nullable(facts.handoff.path)},
            {QStringLiteral("sha1"), nullable(facts.handoff.sha1)}, {QStringLiteral("recorded_sha1"), nullable(recordedHandoff)},
            {QStringLiteral("verified"), recordedHandoff.isEmpty() || facts.handoff.sha1.isEmpty()
                ? QJsonValue::Null : QJsonValue(recordedHandoff == facts.handoff.sha1)},
            {QStringLiteral("content"), facts.handoff.sha1.isEmpty() ? QJsonValue::Null : QJsonValue(facts.handoffContent)}}},
        {QStringLiteral("gaps"), gaps}};
}

QString resumeMarkdown(const QJsonObject& package)
{
    // This output is meant to be handed to an agent. Whoever wants JSON uses
    // the default; here we use the same headings as handoff.md, otherwise the
    // two formats drift apart.
    QString text;
    QTextStream out(&text);

    const auto str = [](const QJsonValue& value, const QString& fallback = QStringLiteral("—")) {
        return value.isString() ? value.toString() : fallback;
    };
    // Unknown and no are different: null stays "—", false is stated outright.
    const auto tri = [](const QJsonValue& value, const QString& yes, const QString& no) {
        return value.isBool() ? (value.toBool() ? yes : no) : QStringLiteral("unknown");
    };

    const QJsonValue task = package.value(QStringLiteral("task"));
    out << "# Runmark resume";
    if (task.isString() && !task.toString().isEmpty()) out << ": " << task.toString();
    out << "\n\n";

    const QJsonValue exec = package.value(QStringLiteral("exec"));
    if (!exec.isString()) {
        out << (task.isString() && !task.toString().isEmpty()
                ? "No execution recorded for this task.\n\n"
                : "No execution recorded in this project.\n\n");
    } else {
        const QJsonObject workspace = package.value(QStringLiteral("workspace")).toObject();
        out << "exec: " << exec.toString() << '\n'
            << "plan: " << str(package.value(QStringLiteral("plan_ref")))
            << "  (" << tri(package.value(QStringLiteral("plan_changed")),
                            QStringLiteral("CHANGED since the execution"), QStringLiteral("unchanged")) << ")\n"
            << "worktree: " << str(workspace.value(QStringLiteral("worktree")))
            << "  (" << tri(workspace.value(QStringLiteral("worktree_exists")),
                            QStringLiteral("present"), QStringLiteral("MISSING ON DISK")) << ")\n"
            << "branch: " << str(workspace.value(QStringLiteral("branch"))) << '\n'
            << "base: " << str(workspace.value(QStringLiteral("base"))) << '@'
            << str(workspace.value(QStringLiteral("base_sha")))
            << "  (" << tri(workspace.value(QStringLiteral("base_advanced")),
                            QStringLiteral("MOVED since then"), QStringLiteral("up to date")) << ")\n";
        const QJsonValue preserved = package.value(QStringLiteral("preserved_ref"));
        if (preserved.isString()) out << "preserved work: " << preserved.toString() << '\n';
        out << '\n';
    }

    out << "## Verified (produced by Runmark)\n\n";
    const QJsonObject measured = package.value(QStringLiteral("measured")).toObject();
    const QJsonArray commits = measured.value(QStringLiteral("commits")).toArray();
    if (commits.isEmpty()) {
        out << "No commits.\n";
    } else {
        out << "Commits:\n";
        for (const QJsonValue& commit : commits) out << "- " << commit.toString() << '\n';
    }
    const QJsonValue filesChanged = measured.value(QStringLiteral("files_changed"));
    if (filesChanged.isDouble()) out << "\nFiles changed: " << filesChanged.toInt() << '\n';
    const QJsonArray evidence = measured.value(QStringLiteral("evidence")).toArray();
    out << "\nEvidence:\n";
    if (evidence.isEmpty()) {
        out << "None.\n";
    } else {
        for (const QJsonValue& value : evidence) {
            const QJsonObject item = value.toObject();
            out << "- " << str(item.value(QStringLiteral("kind"))) << ": "
                << str(item.value(QStringLiteral("summary"))) << '\n';
        }
    }

    out << "\n## Agent note (weak evidence \u2014 unverified)\n\n";
    const QJsonArray claims = package.value(QStringLiteral("agent_claims")).toObject()
        .value(QStringLiteral("evidence")).toArray();
    if (claims.isEmpty()) {
        out << "None.\n";
    } else {
        for (const QJsonValue& value : claims) out << "- " << str(value.toObject().value(QStringLiteral("summary"))) << '\n';
    }

    out << "\n## Open items\n\n";
    const QJsonObject unresolved = package.value(QStringLiteral("unresolved")).toObject();
    bool anyUnresolved = false;
    for (const QString& key : {QStringLiteral("with_ref"), QStringLiteral("without_ref")}) {
        for (const QJsonValue& value : unresolved.value(key).toArray()) {
            const QJsonObject note = value.toObject();
            const QJsonValue reference = note.value(QStringLiteral("ref"));
            out << "- [ ] " << str(note.value(QStringLiteral("text")))
                << (reference.isString() ? QStringLiteral("  (ref: ") + reference.toString() + QLatin1Char(')')
                                         : QStringLiteral("  (no ref)")) << '\n';
            anyUnresolved = true;
        }
    }
    if (!anyUnresolved) out << "None.\n";

    out << "\n## Instructions\n\n";
    const QJsonValue instructions = package.value(QStringLiteral("instructions"));
    const QJsonArray instructionList = instructions.toArray();
    if (instructionList.isEmpty()) {
        out << (instructions.isNull() ? "Not recorded.\n" : "None.\n");
    } else {
        for (const QJsonValue& value : instructionList) {
            const QJsonObject item = value.toObject();
            const QJsonValue sha1 = item.value(QStringLiteral("sha1"));
            out << "- " << str(item.value(QStringLiteral("name"))) << "  "
                << (sha1.isString() ? QStringLiteral("sha1 ") + sha1.toString().left(12)
                                    : QStringLiteral("UNREADABLE")) << '\n';
        }
    }

    out << "\n## Handoff\n\n";
    const QJsonObject handoff = package.value(QStringLiteral("handoff")).toObject();
    const QJsonValue verified = handoff.value(QStringLiteral("verified"));
    if (!handoff.value(QStringLiteral("path")).isString()) {
        out << "None.\n";
    } else {
        out << "path: " << str(handoff.value(QStringLiteral("path"))) << '\n'
            << "sha1: " << str(handoff.value(QStringLiteral("sha1"))) << '\n';
        if (!handoff.value(QStringLiteral("sha1")).isString()) {
            out << "status: file could not be read\n";
        } else if (!verified.isBool()) {
            out << "status: unverifiable (no sha1 recorded in the ledger)\n";
        } else if (verified.toBool()) {
            out << "status: matches the recorded sha1\n";
        } else {
            out << "status: CHANGED \u2014 recorded sha1 "
                << str(handoff.value(QStringLiteral("recorded_sha1"))) << '\n';
        }
    }

    out << "\n## Gaps\n\n";
    const QJsonArray gaps = package.value(QStringLiteral("gaps")).toArray();
    if (gaps.isEmpty()) {
        out << "None.\n";
    } else {
        for (const QJsonValue& value : gaps) {
            const QJsonObject gap = value.toObject();
            out << "- " << str(gap.value(QStringLiteral("id"))) << " — "
                << str(gap.value(QStringLiteral("explanation"))) << '\n';
        }
    }
    return text;
}

} // namespace runmark
