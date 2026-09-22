#pragma once

// Plain data produced by the observation phase. No field here carries a
// judgement -- only what was measured. Rule evaluation (rules.h) reads these
// and nothing else: it never touches git or the filesystem.

#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <optional>

namespace runmark {

struct RepoFacts {
    QString name;
    QString path;
    QString base;              // remote/branch, for display only
    QString branch;
    QString head;
    QString baseSha;
    int behind = 0;
    int ahead = 0;
    bool dirty = false;
    QString localBase;         // name of the local base branch
    bool localBaseExists = false;
    int localBehind = 0;
    QString fetchError;        // non-empty means fetch failed: a diagnosis, not evidence
    QString measurementError;  // non-empty means measurement stopped halfway
    bool measured = false;
};

struct ExecutionFacts {
    QString exec;
    bool hasHandoff = false;
    bool worktreeExists = false;
};

struct PlanFacts {
    bool readable = false;
    int checklistCount = 0;    // number of "- [ ]" / "- [x]" lines
    int taskCount = 0;         // of those, the ones matching task_id_pattern
    QStringList doneTasks;     // task IDs marked "- [x]", in file order
    QString sha1;
};

struct StatusFacts {
    // "Now" is an observation too. Carrying it as a fact keeps the rules pure
    // and lets the 24-hour threshold be tested without rewinding ledger dates.
    QDateTime now;
    QVector<RepoFacts> repos;
    QVector<QJsonObject> events;
    QVector<ExecutionFacts> executions;
    PlanFacts plan;
    // When the hook last ran. Absent means nullopt; hookError separates
    // "never observed" from "could not be read".
    std::optional<QDateTime> lastHookObserved;
    QString hookError;
};

struct FileFacts {
    QString path;
    std::optional<bool> exists;
    QString sha1;
    QString error;
};

struct ResumeFacts {
    QString task;
    QString exec;
    QString ledgerError;
    QJsonObject started;
    QJsonObject finished;
    QVector<QJsonObject> events;
    FileFacts handoff;
    QString handoffContent;
    FileFacts worktree;
    QString planSha1;
    QString currentBaseSha;
    std::optional<bool> baseAdvanced;
    QString baseError;
    QString fetchError;
    QJsonObject measured;
    QString measurementError;
};

} // namespace runmark
