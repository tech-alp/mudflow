#include "plan.h"

#include "paths.h"

#include <QFile>
#include <QRegularExpression>

namespace runmark {

PlanFacts observePlan(const ProjectConfig& config, const QString& root)
{
    PlanFacts facts;
    const QString path = expandPath(config.planPath, root);
    facts.sha1 = sha1File(path);

    QFile plan(path);
    if (!plan.open(QIODevice::ReadOnly)) {
        return facts;
    }
    facts.readable = true;

    // The ID must be a whole token. Without the boundaries the pattern
    // "SCMS-\\d+" also matches inside "SCMS-42-W1", so five work packages would
    // collapse onto one task and the same ID would be reported twice.
    const QString bounded = QStringLiteral("(?<![A-Za-z0-9_-])(") + config.taskIdPattern
        + QStringLiteral(")(?![A-Za-z0-9_-])");
    const QRegularExpression checklistItem(QStringLiteral("^\\s*-\\s*\\[[ xX]\\]"));
    const QRegularExpression anyTask(QStringLiteral("^\\s*-\\s*\\[[ xX]\\].*") + bounded, QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression doneTask(QStringLiteral("^\\s*-\\s*\\[x\\].*") + bounded, QRegularExpression::CaseInsensitiveOption);
    // Loose form: what the old parser would have accepted. A line that matches
    // it but not the bounded form carries an identifier we must not guess at.
    const QRegularExpression looseTask(QStringLiteral("^\\s*-\\s*\\[[ xX]\\].*(") + config.taskIdPattern + QStringLiteral(")"), QRegularExpression::CaseInsensitiveOption);

    while (!plan.atEnd()) {
        const QString line = QString::fromUtf8(plan.readLine());
        if (checklistItem.match(line).hasMatch()) {
            ++facts.checklistCount;
        }
        if (anyTask.match(line).hasMatch()) {
            ++facts.taskCount;
        } else if (checklistItem.match(line).hasMatch()) {
            const QRegularExpressionMatch loose = looseTask.match(line);
            if (loose.hasMatch()) {
                facts.ambiguousTasks.append(line.trimmed());
            }
        }
        const QRegularExpressionMatch match = doneTask.match(line);
        if (match.hasMatch()) {
            facts.doneTasks.append(match.captured(1));
        }
    }
    return facts;
}

QString planReference(const ProjectConfig& config, const QString& root, const QString& task)
{
    QFile file(expandPath(config.planPath, root));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    // Same boundary rule as observePlan: "SCMS-42" must not resolve to the
    // line that actually declares "SCMS-42-W1".
    const QRegularExpression bounded(QStringLiteral("(?<![A-Za-z0-9_-])")
        + QRegularExpression::escape(task) + QStringLiteral("(?![A-Za-z0-9_-])"));
    int lineNumber = 0;
    while (!file.atEnd()) {
        ++lineNumber;
        if (bounded.match(QString::fromUtf8(file.readLine())).hasMatch()) {
            return config.planPath + QStringLiteral("#L") + QString::number(lineNumber);
        }
    }
    return {};
}

} // namespace runmark
