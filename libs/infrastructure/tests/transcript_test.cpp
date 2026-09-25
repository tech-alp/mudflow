// Parses transcripts written by real runtimes (sanitised, see
// transcripts/sanitize.py). A runtime that changes its format breaks this
// test, not a user's evidence (ADR-022). Each fixture directory is named after
// the runtime version that produced it.
#include "transcript.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

#include <stdexcept>

namespace {

void check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

const QDateTime epoch = QDateTime::fromSecsSinceEpoch(0, QTimeZone::UTC);

void claude(const QString& fixtures)
{
    const QString id = QStringLiteral("06a72e06-b1ae-49ab-83a5-de70513b6222");
    qputenv("CLAUDE_CONFIG_DIR", (fixtures + "/claude-2.1.282").toUtf8());
    const runmark::TranscriptFacts facts = runmark::readTranscript(QStringLiteral("claude"), id, epoch);
    check(facts.error.isEmpty(), "claude fixture reads");
    check(facts.commands.size() == 3, "claude: three Bash calls");
    check(facts.commands[0].command == "./check.sh" && facts.commands[0].exitCode == 3, "claude: failing command keeps its code");
    check(facts.commands[1].command == "echo ok" && facts.commands[1].exitCode == 0, "claude: passing command is 0");
    check(facts.commands[2].exitCode == 0, "claude: third command");

    // Only runs at or after `since` count.
    const runmark::TranscriptFacts later = runmark::readTranscript(QStringLiteral("claude"), id, facts.commands[1].at);
    check(later.commands.size() == 2, "claude: since filter");

    // A live session's last line may be half written; it must not hide the rest.
    QTemporaryDir live;
    const QString dir = live.path() + "/projects/-fixture";
    check(QDir().mkpath(dir), "live directory");
    check(QFile::copy(fixtures + "/claude-2.1.282/projects/-fixture/" + id + ".jsonl", dir + "/" + id + ".jsonl"), "copy fixture");
    QFile file(dir + "/" + id + ".jsonl");
    check(file.open(QIODevice::Append), "append partial line");
    file.write("{\"type\":\"assistant\",\"message\":{\"content\":[{\"type\":\"tool_u");
    file.close();
    qputenv("CLAUDE_CONFIG_DIR", live.path().toUtf8());
    check(runmark::readTranscript(QStringLiteral("claude"), id, epoch).commands.size() == 3, "claude: partial last line ignored");

    // A file in a shape we do not know is unknown, never "no commands ran".
    check(QFile::remove(dir + "/" + id + ".jsonl"), "remove copy");
    QFile unknown(dir + "/" + id + ".jsonl");
    check(unknown.open(QIODevice::WriteOnly), "write unknown format");
    unknown.write("{\"event\":\"something-else\"}\n");
    unknown.close();
    const runmark::TranscriptFacts foreign = runmark::readTranscript(QStringLiteral("claude"), id, epoch);
    check(!foreign.error.isEmpty() && foreign.commands.isEmpty(), "claude: unknown format is an error");

    check(!runmark::readTranscript(QStringLiteral("claude"), QStringLiteral("no-such-session"), epoch).error.isEmpty(), "claude: missing session is an error");
}

void codex(const QString& fixtures)
{
    qputenv("CODEX_HOME", (fixtures + "/codex-0.156.1").toUtf8());
    const runmark::TranscriptFacts facts = runmark::readTranscript(QStringLiteral("codex"),
        QStringLiteral("01a0d850-53c4-73b2-9507-4fb220a4153c"), epoch);
    check(facts.error.isEmpty(), "codex fixture reads");
    check(facts.commands.size() == 2, "codex: two command executions");
    check(facts.commands[0].command == "./check.sh" && facts.commands[0].exitCode == 3, "codex: failing command keeps its code");
    check(facts.commands[1].command == "echo ok" && facts.commands[1].exitCode == 0, "codex: passing command is 0");
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc != 2) return 2;
    try {
        claude(QString::fromLocal8Bit(argv[1]));
        codex(QString::fromLocal8Bit(argv[1]));
    } catch (const std::exception& error) {
        QTextStream(stderr) << error.what() << '\n';
        return 1;
    }
    return 0;
}
