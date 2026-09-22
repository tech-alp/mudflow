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

- [x] RM-2 — Named module doğrulaması (macOS)

  Küçük bir named module ve QObject/QML köprüsünü macOS'ta
  clean ve incremental build ile doğrula; compiler/CMake/Ninja baseline'ını
  ölçüm sonrası sabitle.

  macOS deneyi geçti. Kullanıcı kararı (2026-09-22): önce macOS,
  ilerleyen süreçte Linux, en son Windows. Son iki platform RM-2/RM-3 kapısı değildir.
  Ölçülen macOS toolchain: LLVM Clang 23.1.1, CMake 4.4.3,
  Ninja 1.13.2, Qt 6.11.1; deney C++23 kullanır.
  Kaynak, komutlar ve sonuçlar: [module spike](../../tools/module-spike/README.md).

- [ ] RM-3 — Domain'den başlayarak modules geçişi

  `runmark.domain` ile başla. Henüz kullanılmayan host/SDK dizinleri veya
  genel amaçlı framework soyutlamaları oluşturma.
