#pragma once

#include "runmark/project_config.h"

#include <QString>

namespace runmark {

// Reads project.json and validates it through ProjectConfig::parse. The
// filesystem lives here; the validation rules live in the domain (TC-009).
void createProjectConfig(const QString& path, const ProjectConfig& config);

ProjectConfig loadProjectConfig(const QString& path);

} // namespace runmark
