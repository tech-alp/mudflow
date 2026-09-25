#include "handoff.h"

#include "runmark/error.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTextStream>

namespace runmark {

void readHandoff(const Paths& paths, ResumeFacts& facts)
{
    // A ledger ID is data, never a relative filesystem path.
    if (facts.exec.contains(QLatin1Char('/')) || facts.exec.contains(QStringLiteral(".."))) {
        facts.handoff.error = QStringLiteral("Invalid execution ID for handoff path");
        return;
    }
    facts.handoff = observePath(QDir(paths.handoffs).filePath(facts.exec + QStringLiteral(".md")));
    QFile file(facts.handoff.path);
    if (!file.open(QIODevice::ReadOnly)) {
        facts.handoff.error = file.errorString();
        return;
    }
    const QByteArray content = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        facts.handoff.error = file.errorString();
        return;
    }
    facts.handoff.sha1 = QString::fromLatin1(QCryptographicHash::hash(content, QCryptographicHash::Sha1).toHex());
    facts.handoffContent = QString::fromUtf8(content);
}

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
           << "\n---\n\n## Verified (produced by Runmark)\n\nCommits:\n";
    for (const QString& commit : input.commitLines) output << "- " << commit << '\n';
    output << "\nFiles changed: " << input.filesChanged << " (+" << input.insertions << " / -" << input.deletions << ")\n";
    for (const QString& file : input.files) output << "- " << file << '\n';
    output << "\nEvidence: " << input.filesRef << '\n';
    for (const QJsonObject& event : events) {
        if (event.value(QStringLiteral("type")).toString() == QLatin1String("evidence.recorded")
                && event.value(QStringLiteral("exec")).toString() == input.executionId
                && event.value(QStringLiteral("source")).toString() == QLatin1String("runtime")) {
            output << "\nTests (from the " << event.value(QStringLiteral("runtime")).toString() << " transcript): "
                   << event.value(QStringLiteral("summary")).toString() << '\n';
        }
    }
    if (!input.preservedRef.isEmpty()) output << "\nPreserved uncommitted snapshot: " << input.preservedRef << '\n';

    output << "\n## Agent note (weak evidence \u2014 unverified)\n\n";
    bool hasAgentSummary = false;
    for (const QJsonObject& event : events) {
        if (event.value(QStringLiteral("type")).toString() == QLatin1String("evidence.recorded")
                && event.value(QStringLiteral("exec")).toString() == input.executionId
                && event.value(QStringLiteral("source")).toString() != QLatin1String("runtime")) {
            const QString kind = event.value(QStringLiteral("kind")).toString();
            output << "- " << (kind == QLatin1String("agent_summary") ? QString() : QLatin1Char('[') + kind + QStringLiteral("] "))
                   << event.value(QStringLiteral("summary")).toString() << '\n';
            hasAgentSummary = true;
        }
    }
    if (!hasAgentSummary) output << "None.\n";

    output << "\n## Open items\n\n";
    bool hasUnresolved = false;
    for (const QJsonObject& event : events) {
        if (event.value(QStringLiteral("type")).toString() == QLatin1String("note")
                && event.value(QStringLiteral("exec")).toString() == input.executionId
                && event.value(QStringLiteral("kind")).toString() == QLatin1String("unresolved")) {
            output << "- [ ] " << event.value(QStringLiteral("text")).toString();
            const QJsonValue reference = event.value(QStringLiteral("ref"));
            output << (reference.isNull() ? "  (no ref)" : "  (ref: " + reference.toString() + ")") << '\n';
            hasUnresolved = true;
        }
    }
    if (!hasUnresolved) output << "None.\n";

    output.flush();
    if (output.status() != QTextStream::Ok) {
        fail(QStringLiteral("Cannot write handoff: %1").arg(handoff.fileName()));
    }
}

} // namespace runmark
