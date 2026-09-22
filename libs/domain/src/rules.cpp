#include "runmark/rules.h"

#include <QHash>
#include <QJsonArray>
#include <QSet>

namespace runmark {
namespace {

const ExecutionFacts* executionFor(const StatusFacts& facts, const QString& executionId)
{
    for (const ExecutionFacts& execution : facts.executions) {
        if (execution.exec == executionId) return &execution;
    }
    return nullptr;
}

} // namespace

Finding finding(const QString& id, const QString& severity, const QString& domain,
                const QString& title, const QString& explanation, const QString& action)
{
    return {id, severity, domain, title, explanation, action};
}

QVector<Finding> evaluateResume(const ResumeFacts& facts)
{
    QVector<Finding> gaps;
    const auto gap = [&gaps](const QString& id, const QString& domain, const QString& title, const QString& explanation) {
        gaps.append(finding(id, QStringLiteral("warning"), domain, title, explanation));
    };
    if (!facts.ledgerError.isEmpty()) {
        gap(QStringLiteral("context.ledger_unreadable"), QStringLiteral("context"), QStringLiteral("Execution history is unknown"), facts.ledgerError);
        return gaps;
    }
    if (facts.exec.isEmpty()) {
        gap(QStringLiteral("context.no_execution"), QStringLiteral("context"),
            facts.task.isEmpty() ? QStringLiteral("Ledger has no execution") : QStringLiteral("Task has no execution"),
            facts.task.isEmpty() ? QStringLiteral("no execution recorded in this project") : facts.task);
        return gaps;
    }
    if (facts.started.isEmpty()) {
        gap(QStringLiteral("context.missing_start"), QStringLiteral("context"), QStringLiteral("Execution has no start event"), facts.exec);
        return gaps;
    }
    if (!QDateTime::fromString(facts.started.value(QStringLiteral("ts")).toString(), Qt::ISODate).isValid()) {
        gap(QStringLiteral("context.invalid_ledger_timestamp"), QStringLiteral("context"), QStringLiteral("Invalid start timestamp"), facts.exec);
    }
    if (facts.handoff.exists == false) {
        gap(QStringLiteral("context.no_handoff"), QStringLiteral("context"), QStringLiteral("Handoff file is missing"), facts.handoff.path);
    } else if (facts.handoff.sha1.isEmpty()) {
        gap(QStringLiteral("context.handoff_unreadable"), QStringLiteral("context"), QStringLiteral("Handoff cannot be read"), facts.handoff.path + QStringLiteral(": ") + facts.handoff.error);
    } else {
        const QString recorded = facts.finished.value(QStringLiteral("handoff_sha1")).toString();
        if (recorded.isEmpty()) {
            gap(QStringLiteral("context.handoff_unverified"), QStringLiteral("context"), QStringLiteral("Original handoff hash is unknown"), facts.exec);
        } else if (recorded != facts.handoff.sha1) {
            gap(QStringLiteral("context.handoff_changed"), QStringLiteral("context"), QStringLiteral("Handoff changed after finish"), facts.handoff.path);
        }
    }
    if (!facts.worktree.exists.has_value()) {
        gap(QStringLiteral("git.worktree_unknown"), QStringLiteral("git"), QStringLiteral("Worktree existence is unknown"), facts.worktree.error);
    } else if (!*facts.worktree.exists) {
        gap(QStringLiteral("git.worktree_missing"), QStringLiteral("git"), QStringLiteral("Worktree is missing"), facts.worktree.path);
    }
    if (!facts.fetchError.isEmpty()) {
        gap(QStringLiteral("git.fetch_failed"), QStringLiteral("git"), QStringLiteral("Cannot fetch remote base"), facts.fetchError);
    }
    if (!facts.baseAdvanced.has_value()) {
        gap(QStringLiteral("git.base_unknown"), QStringLiteral("git"), QStringLiteral("Base advancement is unknown"),
            facts.baseError.isEmpty() ? QStringLiteral("No verified current base comparison is available") : facts.baseError);
    } else if (*facts.baseAdvanced) {
        gap(QStringLiteral("git.base_advanced"), QStringLiteral("git"), QStringLiteral("Base advanced since execution start"), facts.currentBaseSha);
    }
    const QString recordedPlan = facts.started.value(QStringLiteral("plan_sha1")).toString();
    if (recordedPlan.isEmpty() || facts.planSha1.isEmpty()) {
        gap(QStringLiteral("plan.comparison_unknown"), QStringLiteral("plan"), QStringLiteral("Plan change is unknown"), QStringLiteral("Recorded or current plan SHA1 is unavailable"));
    } else if (recordedPlan != facts.planSha1) {
        gap(QStringLiteral("plan.changed_during_execution"), QStringLiteral("plan"), QStringLiteral("Plan changed since execution start"), facts.task);
    }
    if (facts.started.value(QStringLiteral("plan_ref")).toString().isEmpty()) {
        gap(QStringLiteral("plan.execution_without_plan_link"), QStringLiteral("plan"), QStringLiteral("Execution has no plan link"), facts.exec);
    }
    if (!facts.measurementError.isEmpty()) {
        gap(QStringLiteral("git.measurement_unavailable"), QStringLiteral("git"), QStringLiteral("Commit or file measurement is unavailable"), facts.measurementError);
    }
    const QJsonValue instructions = facts.started.value(QStringLiteral("instructions"));
    if (!instructions.isArray()) {
        gap(QStringLiteral("context.instructions_unknown"), QStringLiteral("context"), QStringLiteral("Instruction provenance was not recorded"), facts.exec);
    } else {
        for (const QJsonValue& instruction : instructions.toArray()) {
            if (instruction.toObject().value(QStringLiteral("sha1")).toString().isEmpty()) {
                gap(QStringLiteral("context.instruction_unreadable"), QStringLiteral("context"), QStringLiteral("Instruction hash was not recorded"), instruction.toObject().value(QStringLiteral("path")).toString());
            }
        }
    }
    for (const QJsonObject& event : facts.events) {
        if (event.value(QStringLiteral("type")) == QLatin1String("note") && event.value(QStringLiteral("kind")) == QLatin1String("unresolved")
                && event.value(QStringLiteral("ref")).toString().isEmpty()) {
            gaps.append(finding(QStringLiteral("context.unresolved_without_ref"), QStringLiteral("info"), QStringLiteral("context"),
                QStringLiteral("Unresolved note has no reference"), event.value(QStringLiteral("text")).toString()));
        }
    }
    return gaps;
}

QVector<Finding> evaluate(const ProjectConfig& config, const StatusFacts& facts)
{
    QVector<Finding> findings;
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

    // --- Ledger scan ---
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

    // --- Per execution ---
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
        // A finished execution whose worktree survives on disk reads as active
        // work. Removal is never automatic (ARCHITECTURE.md "Security"); this
        // only makes it visible.
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
        // Measuring nothing silently looks exactly like measuring a clean
        // result. Tell the two apart.
        if (facts.plan.taskCount == 0) {
            findings.append(finding(QStringLiteral("plan.no_parsable_tasks"), QStringLiteral("warning"), QStringLiteral("plan"),
                QStringLiteral("Plan yields no task candidates"),
                facts.plan.checklistCount == 0
                    ? config.planPath + QStringLiteral(" has no \"- [ ]\" / \"- [x]\" checklist item; plan rules evaluated nothing")
                    : QString::number(facts.plan.checklistCount) + QStringLiteral(" checklist items found in ") + config.planPath + QStringLiteral(" but none matched task_id_pattern ") + config.taskIdPattern,
                QStringLiteral("Write tasks as \"- [x] <TASK-ID> ...\" items, or fix project.task_id_pattern.")));
        }
    }

    // --- Hook ---
    // A hook believed to be installed but never run is indistinguishable from
    // a clean project. Once the expectation is declared, absence is a finding.
    if (config.hooksExpected && !facts.lastHookObserved.has_value()) {
        findings.append(finding(QStringLiteral("context.hooks_not_observed"), QStringLiteral("warning"), QStringLiteral("context"),
            QStringLiteral("No agent hook has been observed"),
            facts.hookError.isEmpty()
                ? QStringLiteral("project.hooks_expected is true but no session start hook has run rmk yet")
                : QStringLiteral("Hook observation cannot be read: ") + facts.hookError,
            QStringLiteral("Install the runmark-agent plugin, then open a new agent session.")));
    }

    return findings;
}

} // namespace runmark
