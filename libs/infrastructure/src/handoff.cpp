#include "handoff.h"

#include "runmark/error.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
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

void writeHandoff(const Paths& paths, const HandoffInput& input, const Ledger& ledger)
{
    QFile handoff(QDir(paths.handoffs).filePath(input.executionId + QStringLiteral(".md")));
    if (!handoff.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        fail(QStringLiteral("Cannot write handoff: %1").arg(handoff.fileName()));
    }
    QTextStream output(&handoff);
    output << "---\nexec: " << input.executionId
           << "\ntask: " << input.started.task
           << "\nagent: " << input.started.agent
           << "\noutcome: " << input.outcome
           << "\nrepo: " << input.started.repo
           << "\nworktree: " << input.worktree
           << "\nbranch: " << input.started.branch
           << "\nbase: " << input.started.base << "@" << input.baseSha
           << "\nrange: " << input.baseSha << ".." << input.headSha
           << "\n---\n\n## Verified (produced by Runmark)\n\nCommits:\n";
    for (const QString& commit : input.commitLines) output << "- " << commit << '\n';
    output << "\nFiles changed: " << input.filesChanged << " (+" << input.insertions << " / -" << input.deletions << ")\n";
    for (const QString& file : input.files) output << "- " << file << '\n';
    output << "\nEvidence: " << input.filesRef << '\n';
    for (const EvidenceRecorded& evidence : ledger.evidence) {
        if (evidence.exec == input.executionId && evidence.fromRuntime) {
            output << "\nTests (from the " << evidence.runtime << " transcript): " << evidence.summary << '\n';
        }
    }
    if (!input.preservedRef.isEmpty()) output << "\nPreserved uncommitted snapshot: " << input.preservedRef << '\n';

    output << "\n## Agent note (weak evidence \u2014 unverified)\n\n";
    bool hasAgentSummary = false;
    for (const EvidenceRecorded& evidence : ledger.evidence) {
        if (evidence.exec == input.executionId && !evidence.fromRuntime) {
            output << "- " << (evidence.kind == QLatin1String("agent_summary") ? QString() : QLatin1Char('[') + evidence.kind + QStringLiteral("] "))
                   << evidence.summary << '\n';
            hasAgentSummary = true;
        }
    }
    if (!hasAgentSummary) output << "None.\n";

    output << "\n## Open items\n\n";
    bool hasUnresolved = false;
    for (const NoteRecorded& note : ledger.notes) {
        if (note.exec == input.executionId && note.kind == QLatin1String("unresolved")) {
            output << "- [ ] " << note.text;
            output << (note.ref.isEmpty() ? QStringLiteral("  (no ref)") : QStringLiteral("  (ref: ") + note.ref + QStringLiteral(")")) << '\n';
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
