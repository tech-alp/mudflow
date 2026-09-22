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

    const QRegularExpression checklistItem(QStringLiteral("^\\s*-\\s*\\[[ xX]\\]"));
    const QRegularExpression anyTask(QStringLiteral("^\\s*-\\s*\\[[ xX]\\].*(") + config.taskIdPattern + QStringLiteral(")"), QRegularExpression::CaseInsensitiveOption);
    const QRegularExpression doneTask(QStringLiteral("^\\s*-\\s*\\[x\\].*(") + config.taskIdPattern + QStringLiteral(")"), QRegularExpression::CaseInsensitiveOption);

    while (!plan.atEnd()) {
        const QString line = QString::fromUtf8(plan.readLine());
        if (checklistItem.match(line).hasMatch()) {
            ++facts.checklistCount;
        }
        if (anyTask.match(line).hasMatch()) {
            ++facts.taskCount;
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
    int lineNumber = 0;
    while (!file.atEnd()) {
        ++lineNumber;
        if (QString::fromUtf8(file.readLine()).contains(task)) {
            return config.planPath + QStringLiteral("#L") + QString::number(lineNumber);
        }
    }
    return {};
}

} // namespace runmark
