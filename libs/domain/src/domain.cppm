module;

#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <optional>

export module runmark.domain;

export namespace runmark {

struct RepositoryConfig {
    QString name;
    QString path;
    QString remote;
    QString branch;
};

struct ProjectConfig {
    int version = 1;
    QString name;
    QString worktreeRoot;
    QVector<RepositoryConfig> repositories;
    QString planPath;
    QString taskIdPattern;
    bool hooksExpected = false;   // proje bir ajan hook'u bekliyor mu
    QStringList instructions;

    // Pure validation. To read from disk, use loadProjectConfig in infrastructure.
    static ProjectConfig parse(const QJsonObject& root);
    QJsonObject toJson() const;
};

} // namespace runmark

// Plain data produced by the observation phase. No field here carries a
// judgement -- only what was measured. Rule evaluation (rules.h) reads these
// and nothing else: it never touches git or the filesystem.


export namespace runmark {

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
    QString agent;
    // Timestamp of the newest event recorded for this execution. An execution
    // with no finish event is a status value; this is the second signal. A
    // session that has been silent for hours is not the same as one that wrote
    // a minute ago, and only the second one is worth staying away from.
    // Invalid when the ledger carried no usable timestamp.
    QDateTime lastActivity;
};

// Measured state of a finished execution's worktree. Runmark never removes a
// worktree on its own -- an uncommitted change or an unmerged branch would be
// gone with no record of it -- so it reports what it measured and leaves the
// decision outside.
struct WorktreeCleanupFacts {
    QString path;
    bool exists = false;
    bool clean = false;        // no uncommitted change
    bool merged = false;       // branch is an ancestor of the base
    QString error;             // non-empty when a check could not run at all
};

struct PlanFacts {
    bool readable = false;
    int checklistCount = 0;    // number of "- [ ]" / "- [x]" lines
    int taskCount = 0;         // of those, the ones matching task_id_pattern
    QStringList doneTasks;     // task IDs marked "- [x]", in file order
    // Checklist lines where the pattern matched only part of a longer
    // identifier, e.g. "SCMS-42" inside "SCMS-42-W1". Guessing which task
    // they mean would silently bind evidence to the wrong one.
    QStringList ambiguousTasks;
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
    // Newest event timestamp for this execution; the second signal a reader
    // needs before assuming an unfinished execution is nobody's.
    QDateTime lastActivity;
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

// The product of evaluation. Derived from measured facts and carrying a
// judgement -- which is why it does not live in facts.h. Turning it into JSON
// is the CLI's job (TC-007), so there is no serialisation here.


export namespace runmark {

struct Finding {
    QString id;
    QString severity;          // info | warning | blocking
    QString domain;            // git | plan | context
    QString title;
    QString explanation;
    QString suggestedAction;   // may be empty
};

} // namespace runmark

export namespace runmark {

// Produces findings. PURE: reads no filesystem, no git, no clock.
// The same facts always yield the same findings, so a test is a plain call.
QVector<Finding> evaluate(const ProjectConfig& config, const StatusFacts& facts);
QVector<Finding> evaluateResume(const ResumeFacts& facts);

// One place owns the finding shape (TRUST_MODEL.md); start warnings use it too.
Finding finding(const QString& id, const QString& severity, const QString& domain,
                const QString& title, const QString& explanation, const QString& action = {});

} // namespace runmark
