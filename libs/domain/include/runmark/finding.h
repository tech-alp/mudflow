#pragma once

// Değerlendirmenin ürünü. Ölçülen facts'ten türer ve yorum taşır — bu yüzden
// facts.h'ta değil. JSON'a çevrilmesi CLI'nin işidir (TC-007), o yüzden
// burada serileştirme yok.

#include <QString>

namespace runmark {

struct Finding {
    QString id;
    QString severity;          // info | warning | blocking
    QString domain;            // git | plan | context
    QString title;
    QString explanation;
    QString suggestedAction;   // boş olabilir
};

} // namespace runmark
