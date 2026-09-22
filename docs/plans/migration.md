# Runmark Mimari Geçiş Planı

Adlandırma geçişi (ROADMAP adım 1) kapandı. Kalan maddeler ROADMAP'teki
"Desktop öncesi — Adlandırma ve mimari geçişi" adımlarının task karşılığıdır.

Görevler `- [ ]` / `- [x]` checklist maddesi olarak yazılır ve task ID taşır.
Format `project.json` içindeki `task_id_pattern` ile eşleşmezse `rmk status`
`plan.no_parsable_tasks` uyarısı verir — kurallar sessizce kör kalmaz.

- [x] RM-1 — apps/cli ve libs sınırları

  `cli/` → `apps/cli/`; `core/` → `libs/domain`, `libs/application`,
  `libs/infrastructure`. CLI JSON/exit sözleşmesini ve mevcut veri
  sözleşmesini regression testleriyle koru.

- [x] RM-4 — Application tipli sonuç döndürsün

  `projectStatus` ve kardeşleri `QJsonObject` yerine struct döndürsün; JSON
  serileştirme `apps/cli`'ye insin. CLI sözleşmesi değişmez (TC-012).

- [ ] RM-2 — Named module doğrulaması

  Küçük bir named module ve QObject/QML köprüsünü macOS/Linux/Windows'ta
  clean ve incremental build ile doğrula; compiler/CMake/Ninja baseline'ını
  ölçüm sonrası sabitle.

  macOS deneyi geçti; Linux/Windows ölçümleri bekliyor.
  Kaynak, komutlar ve sonuçlar: [module spike](../../tools/module-spike/README.md).

- [ ] RM-3 — Domain'den başlayarak modules geçişi

  `runmark.domain` ile başla. Henüz kullanılmayan host/SDK dizinleri veya
  genel amaçlı framework soyutlamaları oluşturma.
