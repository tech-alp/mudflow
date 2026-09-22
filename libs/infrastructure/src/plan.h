#pragma once

#include "runmark/facts.h"
#include "runmark/project_config.h"

#include <QString>

namespace runmark {

// Reads the plan file. Not a real markdown parser: two regexes, one for the
// checkbox line and one for the task ID. If more is ever needed, TC-002.
PlanFacts observePlan(const ProjectConfig& config, const QString& root);

// First line in the plan where the task ID appears: "<path>#L<n>", else empty.
QString planReference(const ProjectConfig& config, const QString& root, const QString& task);

} // namespace runmark
