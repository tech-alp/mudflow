#include "plan.h"

#include "paths.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

namespace runmark {
namespace {

struct PlanFile {
    QString absolute;
    QString display;           // relative to the project root when inside it
};

// Expands project.plan.paths. A glob is allowed in the file name only
// ("docs/superpowers/plans/*.md"); it matches in name order. A plain path is
// kept even when missing, so the reader can report it.
QVector<PlanFile> planFiles(const ProjectConfig& config, const QString& root, QStringList* unmatched)
{
    const QString canonicalRoot = QFileInfo(root).canonicalFilePath();
    const auto display = [&](const QString& absolute) {
        const QString canonical = QFileInfo(absolute).canonicalFilePath();
        return !canonicalRoot.isEmpty() && canonical.startsWith(canonicalRoot + QLatin1Char('/'))
            ? canonical.mid(canonicalRoot.size() + 1) : absolute;
    };
    QVector<PlanFile> files;
    for (const QString& entry : config.planPaths) {
        const QString expanded = expandPath(entry, root);
        const QFileInfo info(expanded);
        if (!info.fileName().contains(QRegularExpression(QStringLiteral("[*?\\[]")))) {
            files.append({expanded, info.exists() ? display(expanded) : entry});
            continue;
        }
        const QStringList matches = info.dir().entryList({info.fileName()}, QDir::Files, QDir::Name);
        if (matches.isEmpty() && unmatched) unmatched->append(entry);
        for (const QString& name : matches) {
            const QString absolute = info.dir().filePath(name);
            files.append({absolute, display(absolute)});
        }
    }
    return files;
}

} // namespace

PlanFacts observePlan(const ProjectConfig& config, const QString& root)
{
    PlanFacts facts;
    // The ID must be a whole token. Without the boundaries the pattern
    // "SCMS-\\d+" also matches inside "SCMS-42-W1", so five work packages would
    // collapse onto one task and the same ID would be reported twice.
    const QString bounded = QStringLiteral("(?<![A-Za-z0-9_-])(") + config.taskIdPattern
        + QStringLiteral(")(?![A-Za-z0-9_-])");
    const QRegularExpression checklistItem(QStringLiteral("^\\s*-\\s*\\[[ xX]\\]"));
    const QRegularExpression checkedItem(QStringLiteral("^\\s*-\\s*\\[[xX]\\]"));
    const QRegularExpression anyTask(QStringLiteral("^\\s*-\\s*\\[[ xX]\\].*") + bounded, QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression doneTask(QStringLiteral("^\\s*-\\s*\\[x\\].*") + bounded, QRegularExpression::CaseInsensitiveOption);
    // Loose form: what the old parser would have accepted. A line that matches
    // it but not the bounded form carries an identifier we must not guess at.
    const QRegularExpression looseTask(QStringLiteral("^\\s*-\\s*\\[[ xX]\\].*(") + config.taskIdPattern + QStringLiteral(")"), QRegularExpression::CaseInsensitiveOption);

    for (const PlanFile& entry : planFiles(config, root, &facts.unreadable)) {
        QFile plan(entry.absolute);
        if (!plan.open(QIODevice::ReadOnly)) {
            facts.unreadable.append(entry.display);
            continue;
        }
        facts.readable = true;
        PlanFileFacts file{entry.display, 0, 0, sha1File(entry.absolute)};
        while (!plan.atEnd()) {
            const QString line = QString::fromUtf8(plan.readLine());
            if (checklistItem.match(line).hasMatch()) {
                ++file.checklistCount;
                if (checkedItem.match(line).hasMatch()) ++file.doneCount;
            }
            if (anyTask.match(line).hasMatch()) {
                ++facts.taskCount;
            } else if (checklistItem.match(line).hasMatch() && looseTask.match(line).hasMatch()) {
                facts.ambiguousTasks.append(line.trimmed());
            }
            const QRegularExpressionMatch match = doneTask.match(line);
            if (match.hasMatch()) {
                facts.doneTasks.append(match.captured(1));
            }
        }
        facts.checklistCount += file.checklistCount;
        facts.files.append(file);
    }
    return facts;
}

QString planReference(const ProjectConfig& config, const QString& root, const QString& task)
{
    // Same boundary rule as observePlan: "SCMS-42" must not resolve to the
    // line that actually declares "SCMS-42-W1".
    const QRegularExpression bounded(QStringLiteral("(?<![A-Za-z0-9_-])")
        + QRegularExpression::escape(task) + QStringLiteral("(?![A-Za-z0-9_-])"));
    for (const PlanFile& entry : planFiles(config, root, nullptr)) {
        QFile file(entry.absolute);
        if (!file.open(QIODevice::ReadOnly)) continue;
        int lineNumber = 0;
        while (!file.atEnd()) {
            ++lineNumber;
            if (bounded.match(QString::fromUtf8(file.readLine())).hasMatch()) {
                return entry.display + QStringLiteral("#L") + QString::number(lineNumber);
            }
        }
    }
    return {};
}

} // namespace runmark
