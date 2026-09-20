#include "mudflow/rules.h"

#include <QHash>
#include <QSet>

namespace mudflow {
namespace {

const ExecutionFacts* executionFor(const StatusFacts& facts, const QString& executionId)
{
    for (const ExecutionFacts& execution : facts.executions) {
        if (execution.exec == executionId) return &execution;
    }
    return nullptr;
}

} // namespace

QJsonObject finding(const QString& id, const QString& severity, const QString& domain,
                    const QString& title, const QString& explanation, const QString& action)
{
    QJsonObject value{
        {QStringLiteral("id"), id},
        {QStringLiteral("severity"), severity},
        {QStringLiteral("domain"), domain},
        {QStringLiteral("title"), title},
        {QStringLiteral("explanation"), explanation},
    };
    if (!action.isEmpty()) {
        value.insert(QStringLiteral("suggested_action"), action);
    }
    return value;
}

QJsonArray evaluate(const ProjectConfig& config, const StatusFacts& facts)
{
    QJsonArray findings;
    QHash<QString, QString> remoteBaseShas;

    // --- Git ---
    for (const RepoFacts& repository : facts.repos) {
        if (!repository.fetchError.isEmpty()) {
            findings.append(finding(QStringLiteral("git.fetch_failed"), QStringLiteral("warning"), QStringLiteral("git"),
                QStringLiteral("Cannot fetch remote"),
                repository.name + QStringLiteral(": ") + repository.fetchError,
                QStringLiteral("Restore remote access, then run status again.")));
        }
        if (!repository.measured) {
            continue;
        }
        if (repository.dirty) {
            findings.append(finding(QStringLiteral("git.dirty_workspace"), QStringLiteral("warning"), QStringLiteral("git"),
                QStringLiteral("Workspace has uncommitted changes"), repository.name + QStringLiteral(" is dirty")));
        }
        if (repository.behind > 0) {
            findings.append(finding(QStringLiteral("git.remote_ahead"), QStringLiteral("warning"), QStringLiteral("git"),
                QStringLiteral("Branch is behind remote base"),
                repository.name + QStringLiteral(" is ") + QString::number(repository.behind) + QStringLiteral(" commits behind ") + repository.base));
        }
        if (repository.localBaseExists && repository.localBehind > 0) {
            findings.append(finding(QStringLiteral("git.stale_local_base"), QStringLiteral("warning"), QStringLiteral("git"),
                QStringLiteral("Local base is behind remote"),
                repository.localBase + QStringLiteral(" is ") + QString::number(repository.localBehind) + QStringLiteral(" commits behind ") + repository.base));
        }
        remoteBaseShas.insert(repository.name, repository.baseSha);
    }

    // --- Ledger taraması ---
    QSet<QString> evidencedTasks;
    QSet<QString> completedExecutions;
    QSet<QString> executionsWithCommits;
    for (const QJsonObject& event : facts.events) {
        const QString type = event.value(QStringLiteral("type")).toString();
        if (type == QLatin1String("execution.finished")) {
            completedExecutions.insert(event.value(QStringLiteral("exec")).toString());
            if (!event.value(QStringLiteral("commits")).toArray().isEmpty()) {
                executionsWithCommits.insert(event.value(QStringLiteral("exec")).toString());
            }
        }
        if (type == QLatin1String("evidence.recorded") && event.value(QStringLiteral("kind")).toString() != QLatin1String("manual_note")) {
            evidencedTasks.insert(event.value(QStringLiteral("task")).toString());
        }
        if (type == QLatin1String("note") && event.value(QStringLiteral("kind")).toString() == QLatin1String("unresolved") && event.value(QStringLiteral("ref")).isNull()) {
            findings.append(finding(QStringLiteral("context.unresolved_without_ref"), QStringLiteral("info"), QStringLiteral("context"),
                QStringLiteral("Unresolved note has no reference"), event.value(QStringLiteral("text")).toString()));
        }
    }

    // --- Execution başına ---
    for (const QJsonObject& event : facts.events) {
        if (event.value(QStringLiteral("type")).toString() != QLatin1String("execution.started")) {
            continue;
        }
        const QString executionId = event.value(QStringLiteral("exec")).toString();
        const QString task = event.value(QStringLiteral("task")).toString();
        const bool completed = completedExecutions.contains(executionId);
        const ExecutionFacts* execution = executionFor(facts, executionId);
        const bool hasHandoff = execution && execution->hasHandoff;
        if (executionsWithCommits.contains(executionId)) {
            evidencedTasks.insert(task);
        }
        if (event.value(QStringLiteral("plan_ref")).toString().isEmpty()) {
            findings.append(finding(QStringLiteral("plan.execution_without_plan_link"), QStringLiteral("info"), QStringLiteral("plan"),
                QStringLiteral("Execution has no plan link"), executionId + QStringLiteral(" has no matching task in plan")));
        }
        if (completed && !hasHandoff) {
            findings.append(finding(QStringLiteral("context.no_handoff"), QStringLiteral("warning"), QStringLiteral("context"),
                QStringLiteral("Completed execution has no handoff"), executionId));
        }
        const QString executionWorktree = event.value(QStringLiteral("worktree")).toString();
        // Bitmis execution'in worktree'si diskte kalirsa aktif is sanilabilir.
        // Silme otomatik degil (ARCHITECTURE.md "Guvenlik"); yalnizca gorunur yapilir.
        if (completed && !executionWorktree.isEmpty() && execution && execution->worktreeExists) {
            findings.append(finding(QStringLiteral("git.orphaned_worktree"), QStringLiteral("info"), QStringLiteral("git"),
                QStringLiteral("Completed execution still has a worktree"),
                executionWorktree + QStringLiteral(" remains on disk after ") + executionId + QStringLiteral(" (")
                    + event.value(QStringLiteral("workspace_source")).toString() + QStringLiteral(")"),
                QStringLiteral("git worktree remove ") + executionWorktree));
        }
        if (!completed && !hasHandoff) {
            const QDateTime startedAt = QDateTime::fromString(event.value(QStringLiteral("ts")).toString(), Qt::ISODate);
            if (!startedAt.isValid()) {
                findings.append(finding(QStringLiteral("context.invalid_ledger_timestamp"), QStringLiteral("warning"), QStringLiteral("context"),
                    QStringLiteral("Execution has an invalid ledger timestamp"),
                    executionId + QStringLiteral(" has invalid ts: ") + event.value(QStringLiteral("ts")).toString()));
            } else if (startedAt.secsTo(facts.now) >= 24 * 60 * 60) {
                const qint64 ageSeconds = startedAt.secsTo(facts.now);
                findings.append(finding(QStringLiteral("context.orphaned_execution"), QStringLiteral("warning"), QStringLiteral("context"),
                    QStringLiteral("Execution appears abandoned"),
                    executionId + QStringLiteral(" started ") + QString::number(ageSeconds / 3600) + QStringLiteral(" hours ago without finish or handoff")));
            } else {
                findings.append(finding(QStringLiteral("context.active_execution"), QStringLiteral("info"), QStringLiteral("context"),
                    QStringLiteral("Execution is still active"), executionId + QStringLiteral(" has no finish event or handoff yet")));
            }
        }
        const QString recordedPlanSha = event.value(QStringLiteral("plan_sha1")).toString();
        if (!completed && !recordedPlanSha.isEmpty() && recordedPlanSha != facts.plan.sha1) {
            findings.append(finding(QStringLiteral("plan.changed_during_execution"), QStringLiteral("warning"), QStringLiteral("plan"),
                QStringLiteral("Plan changed during execution"), task));
        }
        const QString currentBaseSha = remoteBaseShas.value(event.value(QStringLiteral("repo")).toString());
        if (!completed && !currentBaseSha.isEmpty() && currentBaseSha != event.value(QStringLiteral("base_sha")).toString()) {
            findings.append(finding(QStringLiteral("git.stale_worktree_base"), QStringLiteral("warning"), QStringLiteral("git"),
                QStringLiteral("Worktree base is stale"),
                executionId + QStringLiteral(" was recorded from an older ") + event.value(QStringLiteral("base")).toString()));
        }
    }

    // --- Plan ---
    if (!facts.plan.readable) {
        findings.append(finding(QStringLiteral("plan.unreadable"), QStringLiteral("warning"), QStringLiteral("plan"),
            QStringLiteral("Plan file cannot be read"),
            config.planPath + QStringLiteral(" could not be opened; no plan rule was evaluated"),
            QStringLiteral("Fix project.plan.path in project.json.")));
    } else {
        for (const QString& task : facts.plan.doneTasks) {
            if (!evidencedTasks.contains(task)) {
                findings.append(finding(QStringLiteral("plan.done_without_evidence"), QStringLiteral("warning"), QStringLiteral("plan"),
                    QStringLiteral("Done plan task has no evidence"), task));
            }
        }
        // Sessizce hicbir sey olcmemek, temiz cikmakla ayni seye benzer. Ayirt et.
        if (facts.plan.taskCount == 0) {
            findings.append(finding(QStringLiteral("plan.no_parsable_tasks"), QStringLiteral("warning"), QStringLiteral("plan"),
                QStringLiteral("Plan yields no task candidates"),
                facts.plan.checklistCount == 0
                    ? config.planPath + QStringLiteral(" has no \"- [ ]\" / \"- [x]\" checklist item; plan rules evaluated nothing")
                    : QString::number(facts.plan.checklistCount) + QStringLiteral(" checklist items found in ") + config.planPath + QStringLiteral(" but none matched task_id_pattern ") + config.taskIdPattern,
                QStringLiteral("Write tasks as \"- [x] <TASK-ID> ...\" items, or fix project.task_id_pattern.")));
        }
    }

    return findings;
}

} // namespace mudflow
