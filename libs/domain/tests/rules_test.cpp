// evaluate() saf olduğu için bu test git reposu, dosya sistemi veya saat
// kurmaz. Facts elle inşa edilir, finding'ler doğrudan kontrol edilir.

#include "runmark/rules.h"

#include <QJsonArray>
#include <QJsonObject>

namespace {

bool has(const QVector<runmark::Finding>& findings, const QString& id)
{
    for (const runmark::Finding& finding : findings) {
        if (finding.id == id) return true;
    }
    return false;
}

runmark::ProjectConfig config()
{
    runmark::ProjectConfig c;
    c.name = QStringLiteral("t");
    c.planPath = QStringLiteral("plan.md");
    c.taskIdPattern = QStringLiteral("MF-\\d+");
    return c;
}

QJsonObject startedEvent(const QString& exec, const QString& task, const QString& ts)
{
    return {{QStringLiteral("type"), QStringLiteral("execution.started")},
            {QStringLiteral("exec"), exec}, {QStringLiteral("task"), task},
            {QStringLiteral("ts"), ts}, {QStringLiteral("repo"), QStringLiteral("r")},
            {QStringLiteral("plan_ref"), QStringLiteral("plan.md#L1")},
            {QStringLiteral("base_sha"), QStringLiteral("aaa")},
            {QStringLiteral("worktree"), QStringLiteral("/w/MF-1")}};
}

} // namespace

int main()
{
    const QDateTime now = QDateTime::fromString(QStringLiteral("2026-09-20T12:00:00Z"), Qt::ISODate);

    // 1. Git: fetch patlasa bile offline hesaplanabilenler susmaz.
    {
        runmark::StatusFacts f;
        f.now = now;
        f.plan.readable = true;
        f.plan.taskCount = 1;
        runmark::RepoFacts repo;
        repo.name = QStringLiteral("r");
        repo.base = QStringLiteral("origin/main");
        repo.fetchError = QStringLiteral("network down");
        repo.measured = true;
        repo.dirty = true;
        repo.behind = 3;
        f.repos.append(repo);
        const QVector<runmark::Finding> findings = runmark::evaluate(config(), f);
        if (!has(findings, QStringLiteral("git.fetch_failed"))
                || !has(findings, QStringLiteral("git.dirty_workspace"))
                || !has(findings, QStringLiteral("git.remote_ahead"))) return 1;
    }

    // 2. Ölçüm yarıda kaldıysa ölçüme dayalı kural üretilmez.
    {
        runmark::StatusFacts f;
        f.now = now;
        f.plan.readable = true;
        f.plan.taskCount = 1;
        runmark::RepoFacts repo;
        repo.name = QStringLiteral("r");
        repo.measurementError = QStringLiteral("boom");
        repo.dirty = true;   // ölçülmediği için kullanılmamalı
        f.repos.append(repo);
        if (has(runmark::evaluate(config(), f), QStringLiteral("git.dirty_workspace"))) return 1;
    }

    // 3. 24 saat sınırı — "şimdi" fact olduğu için ledger tarihi geri alınmadan test edilir.
    {
        runmark::StatusFacts f;
        f.now = now;
        f.plan.readable = true;
        f.plan.taskCount = 1;
        f.events.append(startedEvent(QStringLiteral("E1"), QStringLiteral("MF-1"), QStringLiteral("2026-09-20T11:00:00Z")));
        f.executions.append({QStringLiteral("E1"), false, false});
        const QVector<runmark::Finding> fresh = runmark::evaluate(config(), f);
        if (!has(fresh, QStringLiteral("context.active_execution"))
                || has(fresh, QStringLiteral("context.orphaned_execution"))) return 1;

        f.events[0] = startedEvent(QStringLiteral("E1"), QStringLiteral("MF-1"), QStringLiteral("2026-09-19T11:00:00Z"));
        const QVector<runmark::Finding> stale = runmark::evaluate(config(), f);
        if (!has(stale, QStringLiteral("context.orphaned_execution"))
                || has(stale, QStringLiteral("context.active_execution"))) return 1;

        f.events[0] = startedEvent(QStringLiteral("E1"), QStringLiteral("MF-1"), QStringLiteral("bozuk"));
        if (!has(runmark::evaluate(config(), f), QStringLiteral("context.invalid_ledger_timestamp"))) return 1;
    }

    // 4. Plan körlüğü: okunabilir ama task yok → uyarmalı.
    {
        runmark::StatusFacts f;
        f.now = now;
        f.plan.readable = true;
        f.plan.checklistCount = 3;
        f.plan.taskCount = 0;
        if (!has(runmark::evaluate(config(), f), QStringLiteral("plan.no_parsable_tasks"))) return 1;

        f.plan.readable = false;
        if (!has(runmark::evaluate(config(), f), QStringLiteral("plan.unreadable"))) return 1;
    }

    // 5. done_without_evidence, kanıt varsa susmalı.
    {
        runmark::StatusFacts f;
        f.now = now;
        f.plan.readable = true;
        f.plan.taskCount = 1;
        f.plan.doneTasks = {QStringLiteral("MF-1")};
        if (!has(runmark::evaluate(config(), f), QStringLiteral("plan.done_without_evidence"))) return 1;

        f.events.append({{QStringLiteral("type"), QStringLiteral("evidence.recorded")},
                         {QStringLiteral("task"), QStringLiteral("MF-1")},
                         {QStringLiteral("kind"), QStringLiteral("test")}});
        if (has(runmark::evaluate(config(), f), QStringLiteral("plan.done_without_evidence"))) return 1;

        // manual_note en zayıf kanıt: tek başına "done"u doğrulamaz.
        f.events[0] = QJsonObject{{QStringLiteral("type"), QStringLiteral("evidence.recorded")},
                                  {QStringLiteral("task"), QStringLiteral("MF-1")},
                                  {QStringLiteral("kind"), QStringLiteral("manual_note")}};
        if (!has(runmark::evaluate(config(), f), QStringLiteral("plan.done_without_evidence"))) return 1;
    }

    // Resume: deterministic facts, no git/filesystem/clock access.
    {
        runmark::ResumeFacts f;
        f.task = QStringLiteral("MF-1");
        if (!has(runmark::evaluateResume(f), QStringLiteral("context.no_execution"))) return 1;
        f.ledgerError = QStringLiteral("permission denied");
        if (!has(runmark::evaluateResume(f), QStringLiteral("context.ledger_unreadable"))
                || has(runmark::evaluateResume(f), QStringLiteral("context.no_execution"))) return 1;
        f.ledgerError.clear();
        f.exec = QStringLiteral("E1");
        f.started = startedEvent(f.exec, f.task, QStringLiteral("2026-09-20T11:00:00Z"));
        f.started.insert(QStringLiteral("plan_sha1"), QStringLiteral("plan"));
        f.started.insert(QStringLiteral("instructions"), QJsonArray{});
        f.planSha1 = QStringLiteral("plan");
        f.finished.insert(QStringLiteral("handoff_sha1"), QStringLiteral("handoff"));
        f.handoff.sha1 = QStringLiteral("handoff");
        // Yollar gercekten dolu gelir; bos birakmak bos explanation uretirdi.
        f.handoff.path = QStringLiteral("/p/.runmark/handoffs/E1.md");
        f.worktree.path = QStringLiteral("/w/MF-1");
        f.handoff.exists = true;
        f.worktree.exists = true;
        f.baseAdvanced = false;
        if (!runmark::evaluateResume(f).isEmpty()) return 1;

        f.handoff.exists = false;
        f.handoff.sha1.clear();
        f.worktree.exists = false;
        f.baseAdvanced = true;
        f.currentBaseSha = QStringLiteral("bbb");
        f.planSha1 = QStringLiteral("changed");
        const QVector<runmark::Finding> missing = runmark::evaluateResume(f);
        for (const QString& id : {QStringLiteral("context.no_handoff"), QStringLiteral("git.worktree_missing"),
                QStringLiteral("git.base_advanced"), QStringLiteral("plan.changed_during_execution")}) {
            if (!has(missing, id)) return 1;
        }
        // Alanlarin varligini artik tip garanti ediyor; kalan risk bos birakmak.
        for (const runmark::Finding& gap : missing) {
            if (gap.id.isEmpty() || gap.severity.isEmpty() || gap.domain.isEmpty()
                    || gap.title.isEmpty() || gap.explanation.isEmpty()) return 1;
        }

        f.handoff.exists.reset();
        f.worktree.exists.reset();
        f.baseAdvanced.reset();
        f.planSha1.clear();
        f.fetchError = QStringLiteral("offline");
        f.measurementError = QStringLiteral("missing git objects");
        f.started.remove(QStringLiteral("instructions"));
        const QVector<runmark::Finding> unknown = runmark::evaluateResume(f);
        for (const QString& id : {QStringLiteral("context.handoff_unreadable"), QStringLiteral("git.worktree_unknown"),
                QStringLiteral("git.base_unknown"), QStringLiteral("git.fetch_failed"), QStringLiteral("plan.comparison_unknown"),
                QStringLiteral("git.measurement_unavailable"), QStringLiteral("context.instructions_unknown")}) {
            if (!has(unknown, id)) return 1;
        }
        if (has(unknown, QStringLiteral("git.worktree_missing")) || has(unknown, QStringLiteral("context.no_handoff"))
                || has(unknown, QStringLiteral("plan.changed_during_execution")) || has(unknown, QStringLiteral("git.base_advanced"))) return 1;
        f.handoff.exists = true;
        f.handoff.sha1 = QStringLiteral("edited");
        if (!has(runmark::evaluateResume(f), QStringLiteral("context.handoff_changed"))) return 1;
        f.finished.remove(QStringLiteral("handoff_sha1"));
        if (!has(runmark::evaluateResume(f), QStringLiteral("context.handoff_unverified"))) return 1;
        f.started.insert(QStringLiteral("instructions"), QJsonArray{QJsonObject{{QStringLiteral("path"), QStringLiteral("missing.md")}, {QStringLiteral("sha1"), QJsonValue::Null}}});
        if (!has(runmark::evaluateResume(f), QStringLiteral("context.instruction_unreadable"))) return 1;
    }

    // 6. Hook körlüğü: beklenti yazıldıysa gözlem yokluğu bulgudur; beklenti
    //    yoksa sessiz kalmalı, aksi halde CLI'yi tek başına kullanan proje
    //    kapatamayacağı bir uyarı görür.
    {
        runmark::StatusFacts f;
        f.now = now;
        f.plan.readable = true;
        f.plan.taskCount = 1;
        runmark::ProjectConfig expects = config();
        expects.hooksExpected = true;
        if (has(runmark::evaluate(config(), f), QStringLiteral("context.hooks_not_observed"))) return 1;
        if (!has(runmark::evaluate(expects, f), QStringLiteral("context.hooks_not_observed"))) return 1;
        f.lastHookObserved = now.addSecs(-3600);
        if (has(runmark::evaluate(expects, f), QStringLiteral("context.hooks_not_observed"))) return 1;
        // Bozuk kayıt "görüldü" sayılmamalı; sebep açıklamada durmalı.
        f.lastHookObserved.reset();
        f.hookError = QStringLiteral("Invalid ts: soon");
        const QVector<runmark::Finding> broken = runmark::evaluate(expects, f);
        if (!has(broken, QStringLiteral("context.hooks_not_observed"))) return 1;
        bool explained = false;
        for (const runmark::Finding& f2 : broken) {
            if (f2.id == QLatin1String("context.hooks_not_observed")) {
                explained = f2.explanation.contains(QStringLiteral("Invalid ts: soon"));
            }
        }
        if (!explained) return 1;
    }

    return 0;
}
