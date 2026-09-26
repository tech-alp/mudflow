#pragma once

#include "runmark/facts.h"
#include "runmark/project_config.h"

#include <QString>

namespace runmark {

// Reads the plan files (project.plan.paths). Not a real markdown parser: a
// checkbox regex and a task-ID regex. If more is ever needed, md4c (TC-002).
PlanFacts observePlan(const ProjectConfig& config, const QString& root);

// First line across the plan files where the task ID appears:
// "<path>#L<n>", else empty.
QString planReference(const ProjectConfig& config, const QString& root, const QString& task);

} // namespace runmark
