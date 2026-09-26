#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>

namespace runmark {

// A shell command the agent runtime itself recorded, with the exit status the
// runtime observed. Stronger than an agent's claim, weaker than a command
// Runmark ran itself (DATA_MODEL.md §3.3).
struct TranscriptCommand {
    QDateTime at;
    QString command;
    int exitCode = -1;   // -1: the runtime reported a failure without a code
};

struct TranscriptFacts {
    QString path;        // empty when no transcript was found
    QString error;       // non-empty means unknown, never "no commands ran"
    QVector<TranscriptCommand> commands;
};

// A session found by its transcript alone, whether or not any hook ran.
struct TranscriptSession {
    QString runtime;           // claude | codex
    QString id;
    QString cwd;
    QString path;
};

// Transcripts modified since `since`: Claude <config>/projects/*/*.jsonl and
// Codex <home>/sessions/**/rollout-*.jsonl. Codex sub-threads (they carry a
// parent_thread_id, e.g. automatic reviews) are left out: they are not
// sessions a person started.
QVector<TranscriptSession> recentTranscriptSessions(const QDateTime& since);

// The session id the runtime exports to the commands it runs, or empty.
QString sessionIdFromEnvironment(const QString& runtime);

// Shell commands recorded at or after `since` in the runtime's transcript for
// `sessionId`. Transcript formats are undocumented; anything unrecognised
// becomes an error so it surfaces as a finding instead of a clean result.
TranscriptFacts readTranscript(const QString& runtime, const QString& sessionId, const QDateTime& since);

} // namespace runmark
