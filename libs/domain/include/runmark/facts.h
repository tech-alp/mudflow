#pragma once

// Gözlem aşamasının ürettiği saf veri. Buradaki hiçbir alan yorum içermez;
// yalnızca ölçülen gerçekler durur. Kural değerlendirmesi (rules.h) yalnızca
// bunlara bakar, git'e veya dosya sistemine dokunmaz.

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
    QString base;              // remote/branch, gösterim için
    QString branch;
    QString head;
    QString baseSha;
    int behind = 0;
    int ahead = 0;
    bool dirty = false;
    QString localBase;         // yerel base branch adı
    bool localBaseExists = false;
    int localBehind = 0;
    QString fetchError;        // boş değilse fetch başarısız — teşhis, kanıt değil
    QString measurementError;  // boş değilse ölçüm yarıda kaldı
    bool measured = false;
};

struct ExecutionFacts {
    QString exec;
    bool hasHandoff = false;
    bool worktreeExists = false;
};

struct PlanFacts {
    bool readable = false;
    int checklistCount = 0;    // "- [ ]" / "- [x]" satır sayısı
    int taskCount = 0;         // bunlardan task_id_pattern eşleşenler
    QStringList doneTasks;     // "- [x]" işaretli task ID'leri, dosya sırasında
    QString sha1;
};

struct StatusFacts {
    // "Şimdi" de bir gözlemdir. Fact olarak taşınınca rules saf kalır ve
    // 24 saat sınırı ledger tarihini geri almadan test edilebilir.
    QDateTime now;
    QVector<RepoFacts> repos;
    QVector<QJsonObject> events;
    QVector<ExecutionFacts> executions;
    PlanFacts plan;
    // Hook'un son calistigi an. Yoksa nullopt: "hic gorulmedi" ile "okunamadi"
    // ayrimini hookError tasir.
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
