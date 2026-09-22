#pragma once

// The product of evaluation. Derived from measured facts and carrying a
// judgement -- which is why it does not live in facts.h. Turning it into JSON
// is the CLI's job (TC-007), so there is no serialisation here.

#include <QString>

namespace runmark {

struct Finding {
    QString id;
    QString severity;          // info | warning | blocking
    QString domain;            // git | plan | context
    QString title;
    QString explanation;
    QString suggestedAction;   // may be empty
};

} // namespace runmark
