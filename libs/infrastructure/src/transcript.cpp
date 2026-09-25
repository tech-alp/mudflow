#include "transcript.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace runmark {
namespace {

QString configRoot(const char* variable, const QString& fallback)
{
    const QString value = qEnvironmentVariable(variable);
    return value.isEmpty() ? QDir::home().filePath(fallback) : value;
}

// Claude: <config>/projects/<encoded cwd>/<session>.jsonl. The encoded cwd is
// not reproduced; the session id alone is unique.
QString findClaudeTranscript(const QString& sessionId)
{
    const QDir projects(configRoot("CLAUDE_CONFIG_DIR", QStringLiteral(".claude")) + QStringLiteral("/projects"));
    for (const QString& project : projects.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString candidate = projects.filePath(project + QLatin1Char('/') + sessionId + QStringLiteral(".jsonl"));
        if (QFile::exists(candidate)) return candidate;
    }
    return {};
}

// Codex: <home>/sessions/YYYY/MM/DD/rollout-<time>-<thread>.jsonl.
QString findCodexTranscript(const QString& sessionId)
{
    QDirIterator it(configRoot("CODEX_HOME", QStringLiteral(".codex")) + QStringLiteral("/sessions"),
        {QStringLiteral("rollout-*-") + sessionId + QStringLiteral(".jsonl")}, QDir::Files, QDirIterator::Subdirectories);
    return it.hasNext() ? it.next() : QString();
}

QString resultText(const QJsonValue& content)
{
    if (content.isString()) return content.toString();
    QString text;
    for (const QJsonValue& part : content.toArray()) text += part.toObject().value(QStringLiteral("text")).toString();
    return text;
}

// Claude pairs an assistant tool_use with a later user tool_result by id. A
// failing Bash result carries "Exit code N"; a passing one carries no code.
bool parseClaudeLine(const QJsonObject& line, QHash<QString, TranscriptCommand>& pending, QVector<TranscriptCommand>& out)
{
    const QJsonValue content = line.value(QStringLiteral("message")).toObject().value(QStringLiteral("content"));
    if (!line.contains(QStringLiteral("timestamp"))) return false;
    const QDateTime at = QDateTime::fromString(line.value(QStringLiteral("timestamp")).toString(), Qt::ISODateWithMs);
    for (const QJsonValue& value : content.toArray()) {
        const QJsonObject block = value.toObject();
        const QString type = block.value(QStringLiteral("type")).toString();
        if (type == QLatin1String("tool_use") && block.value(QStringLiteral("name")).toString() == QLatin1String("Bash")) {
            pending.insert(block.value(QStringLiteral("id")).toString(),
                {at, block.value(QStringLiteral("input")).toObject().value(QStringLiteral("command")).toString(), -1});
        } else if (type == QLatin1String("tool_result")) {
            auto it = pending.find(block.value(QStringLiteral("tool_use_id")).toString());
            if (it == pending.end()) continue;
            TranscriptCommand command = it.value();
            pending.erase(it);
            if (!block.value(QStringLiteral("is_error")).toBool()) {
                command.exitCode = 0;
            } else {
                static const QRegularExpression code(QStringLiteral("^Exit code (\\d+)"));
                const QRegularExpressionMatch match = code.match(resultText(block.value(QStringLiteral("content"))));
                if (match.hasMatch()) command.exitCode = match.captured(1).toInt();
            }
            out.append(command);
        }
    }
    return true;
}

bool parseCodexLine(const QJsonObject& line, QVector<TranscriptCommand>& out)
{
    if (!line.contains(QStringLiteral("payload"))) return false;
    const QJsonObject item = line.value(QStringLiteral("payload")).toObject().value(QStringLiteral("item")).toObject();
    if (item.value(QStringLiteral("type")).toString() != QLatin1String("CommandExecution")) return true;
    // ["/bin/zsh", "-lc", "<script>"]: the script is what the agent wrote.
    const QJsonArray argv = item.value(QStringLiteral("command")).toArray();
    QStringList parts;
    for (const QJsonValue& part : argv) parts.append(part.toString());
    const bool shellWrapped = parts.size() == 3 && (parts[1] == QLatin1String("-lc") || parts[1] == QLatin1String("-c"));
    const QJsonValue exitCode = item.value(QStringLiteral("exit_code"));
    out.append({QDateTime::fromString(line.value(QStringLiteral("timestamp")).toString(), Qt::ISODateWithMs),
        shellWrapped ? parts[2] : parts.join(QLatin1Char(' ')),
        exitCode.isDouble() ? exitCode.toInt() : -1});
    return true;
}

} // namespace

QString sessionIdFromEnvironment(const QString& runtime)
{
    const QString id = qEnvironmentVariable(runtime == QLatin1String("codex") ? "CODEX_THREAD_ID" : "CLAUDE_CODE_SESSION_ID");
    // The id becomes part of a file name; anything but [A-Za-z0-9-] is refused.
    static const QRegularExpression safe(QStringLiteral("^[A-Za-z0-9-]+$"));
    return safe.match(id).hasMatch() ? id : QString();
}

TranscriptFacts readTranscript(const QString& runtime, const QString& sessionId, const QDateTime& since)
{
    TranscriptFacts facts;
    facts.path = runtime == QLatin1String("codex") ? findCodexTranscript(sessionId) : findClaudeTranscript(sessionId);
    if (facts.path.isEmpty()) {
        facts.error = QStringLiteral("No %1 transcript found for session %2").arg(runtime, sessionId);
        return facts;
    }
    QFile file(facts.path);
    if (!file.open(QIODevice::ReadOnly)) {
        facts.error = file.errorString();
        return facts;
    }
    QHash<QString, TranscriptCommand> pending;
    QVector<TranscriptCommand> all;
    int recognised = 0;
    while (!file.atEnd()) {
        // A line still being written by a live session fails to parse; skip it.
        const QJsonObject line = QJsonDocument::fromJson(file.readLine()).object();
        if (line.isEmpty()) continue;
        const bool ok = runtime == QLatin1String("codex") ? parseCodexLine(line, all) : parseClaudeLine(line, pending, all);
        if (ok) ++recognised;
    }
    if (recognised == 0) {
        facts.error = QStringLiteral("Unrecognised %1 transcript format: %2").arg(runtime, facts.path);
        return facts;
    }
    for (const TranscriptCommand& command : all) {
        if (command.at.isValid() && command.at >= since) facts.commands.append(command);
    }
    return facts;
}

} // namespace runmark
