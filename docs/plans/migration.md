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

- [x] RM-3 — Domain'den başlayarak modules geçişi

  `runmark.domain` ile başla. Henüz kullanılmayan host/SDK dizinleri veya
  genel amaçlı framework soyutlamaları oluşturma.

  `runmark.domain` uygulandı; mevcut header yolları import köprüsüdür.
  Üretim macOS baseline'ı C++23 + LLVM + Ninja oldu.

- [x] RM-6 — Plan task kimliği tam token olmalı

  `plan.ambiguous_task_id`: pattern yalnız daha uzun bir kimliğin parçasına
  uyuyorsa satır hiçbir task'a bağlanmaz. Tahmini eşleme kanıtı yanlış task'a
  bağlıyordu.

- [x] RM-7 — Desktop iskeleti: ui-shell ve salt okunur Findings

  `libs/ui-shell` (FindingModel + StatusViewModel, `Runmark.Shell` QML modülü)
  ve `apps/desktop`. Gerçek `projectStatus()` sonucunu gösterir, JSON parse
  etmez (TC-012). `apps/cli` GUI bağımlılığı kazanmamalı — mekanik kontrol.
  Merce bu adımda yok; düz Qt Quick Controls ile yapılır.

- [ ] RM-8 — Merce entegrasyonu ve kompakt desktop profili

  TC-011'deki FetchContent bloğu, `Merce::*` hedefleri, statik linkleme.
  Sürüm SHA ile sabit. Kiosk profili değişmez.

- [x] RM-5 — macOS CI, paketleme ve manuel release altyapısı

  Sabit toolchain, build/test/hook gates, taşınabilir CLI arşivi ve paket
  smoke testi. Sürüm semantic-release tarafından build'e aktarılır.
  Yayın yalnız manuel main workflow'undan; Linux/Windows ve desktop imzalama sonra.

  Kullanıcı onayıyla kapsam 2026-09-22'de kapatıldı: CI ve paketleme tamamlandı,
  yayın yapılmadı. Main `29c6c95` için [hosted CI](https://github.com/tech-alp/runmark/actions/runs/35736952337)
  tamamen geçti. Release environment yalnız main, onaylayıcı tech-alp;
  self-review açık, admin bypass varsayılanı açık. Yayın hazırlığı RM-9'a taşındı.
  Ayrıntılar: [release rehberi](../RELEASING.md).

- [ ] RM-9 — İlk yayın hazırlığı ve release doğrulaması

  Main branch protection ve zorunlu macOS CI kontrolünü yapılandır.
  Lisans kararı kullanıcı tarafından ertelendi; LICENSE ve gözden geçirilmiş
  THIRD_PARTY_NOTICES yayın öncesi tamamlanmalı. Daha önce yayın yok;
  ilk sürüm/bootstrap politikası ayrıca kararlaştırılmalı, 0.3.0 kod sabitinden
  geçmiş yayın tag'i uydurulmamalı. Ardından onaylı dry-run ve gerçek release
  doğrulaması yap. Bu görev RM-8 desktop çalışmasını engellemez.
