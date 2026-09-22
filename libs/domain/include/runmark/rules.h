#pragma once

#include "runmark/facts.h"
#include "runmark/finding.h"
#include "runmark/project_config.h"

#include <QVector>

namespace runmark {

// Produces findings. PURE: reads no filesystem, no git, no clock.
// The same facts always yield the same findings, so a test is a plain call.
QVector<Finding> evaluate(const ProjectConfig& config, const StatusFacts& facts);
QVector<Finding> evaluateResume(const ResumeFacts& facts);

// One place owns the finding shape (TRUST_MODEL.md); start warnings use it too.
Finding finding(const QString& id, const QString& severity, const QString& domain,
                const QString& title, const QString& explanation, const QString& action = {});

} // namespace runmark
