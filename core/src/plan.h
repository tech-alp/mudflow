#pragma once

#include "runmark/facts.h"
#include "runmark/project_config.h"

#include <QString>

namespace runmark {

// Plan dosyasını okur. Gerçek markdown parser değil: checkbox satırı ve
// task ID yakalayan iki regex. Fazlası gerekirse TECH_CHOICES TC-002.
PlanFacts observePlan(const ProjectConfig& config, const QString& root);

// Task ID'nin plan içinde geçtiği ilk satır: "<yol>#L<n>". Yoksa boş.
QString planReference(const ProjectConfig& config, const QString& root, const QString& task);

} // namespace runmark
