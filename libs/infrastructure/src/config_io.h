#pragma once

#include "runmark/project_config.h"

#include <QString>

namespace runmark {

// project.json'u okur ve ProjectConfig::parse ile dogrular. Dosya sistemi
// burada; dogrulama kurallari domain'de (TC-009).
ProjectConfig loadProjectConfig(const QString& path);

} // namespace runmark
