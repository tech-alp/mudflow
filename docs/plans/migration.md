# Runmark Mimari Geçiş Planı

Adlandırma geçişi (ROADMAP adım 1) kapandı. Kalan maddeler ROADMAP'teki
"Desktop öncesi — Adlandırma ve mimari geçişi" adımlarının task karşılığıdır.

Görevler `- [ ]` / `- [x]` checklist maddesi olarak yazılır ve task ID taşır.
Format `project.json` içindeki `task_id_pattern` ile eşleşmezse `rmk status`
`plan.no_parsable_tasks` uyarısı verir — kurallar sessizce kör kalmaz.

- [ ] RM-1 — apps/cli ve libs sınırları

  `cli/` → `apps/cli/`; `core/` → `libs/domain`, `libs/application`,
  `libs/infrastructure`. CLI JSON/exit sözleşmesini ve mevcut veri
  sözleşmesini regression testleriyle koru.

- [ ] RM-2 — Named module doğrulaması

  Küçük bir named module ve QObject/QML köprüsünü macOS/Linux/Windows'ta
  clean ve incremental build ile doğrula; compiler/CMake/Ninja baseline'ını
  ölçüm sonrası sabitle.

- [ ] RM-3 — Domain'den başlayarak modules geçişi

  `runmark.domain` ile başla. Henüz kullanılmayan host/SDK dizinleri veya
  genel amaçlı framework soyutlamaları oluşturma.
