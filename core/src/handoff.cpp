#include "handoff.h"

#include "error.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

namespace mudflow {

void writeHandoff(const Paths& paths, const HandoffInput& input, const QVector<QJsonObject>& events)
{
    QFile handoff(QDir(paths.handoffs).filePath(input.executionId + QStringLiteral(".md")));
    if (!handoff.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fail(QStringLiteral("Cannot write handoff: %1").arg(handoff.fileName()));
    }
    QTextStream output(&handoff);
    output << "---\nexec: " << input.executionId
           << "\ntask: " << input.started.value(QStringLiteral("task")).toString()
           << "\nagent: " << input.started.value(QStringLiteral("agent")).toString()
           << "\noutcome: " << input.outcome
           << "\nrepo: " << input.started.value(QStringLiteral("repo")).toString()
           << "\nworktree: " << input.worktree
           << "\nbranch: " << input.started.value(QStringLiteral("branch")).toString()
           << "\nbase: " << input.started.value(QStringLiteral("base")).toString() << "@" << input.baseSha
           << "\nrange: " << input.baseSha << ".." << input.headSha
           << "\n---\n\n## Doğrulanmış (Mudflow üretti)\n\nCommits:\n";
    for (const QString& commit : input.commitLines) output << "- " << commit << '\n';
    output << "\nDeğişen dosyalar: " << input.filesChanged << " (+" << input.insertions << " / -" << input.deletions << ")\n";
    for (const QString& file : input.files) output << "- " << file << '\n';
    output << "\nEvidence: " << input.filesRef << '\n';
    if (!input.preservedRef.isEmpty()) output << "\nPreserved uncommitted snapshot: " << input.preservedRef << '\n';

    output << "\n## Agent notu (zayıf evidence — doğrulanmadı)\n\n";
    bool hasAgentSummary = false;
    for (const QJsonObject& event : events) {
        if (event.value(QStringLiteral("type")).toString() == QLatin1String("evidence.recorded")
                && event.value(QStringLiteral("exec")).toString() == input.executionId
                && event.value(QStringLiteral("kind")).toString() == QLatin1String("agent_summary")) {
            output << "- " << event.value(QStringLiteral("summary")).toString() << '\n';
            hasAgentSummary = true;
        }
    }
    if (!hasAgentSummary) output << "Yok.\n";

    output << "\n## Açık kalanlar\n\n";
    bool hasUnresolved = false;
    for (const QJsonObject& event : events) {
        if (event.value(QStringLiteral("type")).toString() == QLatin1String("note")
                && event.value(QStringLiteral("exec")).toString() == input.executionId
                && event.value(QStringLiteral("kind")).toString() == QLatin1String("unresolved")) {
            output << "- [ ] " << event.value(QStringLiteral("text")).toString();
            const QJsonValue reference = event.value(QStringLiteral("ref"));
            output << (reference.isNull() ? "  (ref yok)" : "  (ref: " + reference.toString() + ")") << '\n';
            hasUnresolved = true;
        }
    }
    if (!hasUnresolved) output << "Yok.\n";

    output.flush();
    if (output.status() != QTextStream::Ok) {
        fail(QStringLiteral("Cannot write handoff: %1").arg(handoff.fileName()));
    }
}

} // namespace mudflow
