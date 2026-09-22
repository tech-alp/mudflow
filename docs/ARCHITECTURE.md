# Runmark Architecture

Durum: adlandırma uygulandı; katman ve dizin geçişi bekliyor.
Ürün kapsamı [PRD](PRD.md), karar geçmişi [DECISIONS](DECISIONS.md), geçiş
sırası [ROADMAP](ROADMAP.md), mevcut disk sözleşmesi [DATA_MODEL](DATA_MODEL.md).

## Temel karar

Nexroz, aynı çalışma alanındaki kardeş projedir (`../nexroz`,
`Nexroz_Specification_v0.3_Combined.md`); plugin runtime, module sınırları ve
agent runtime tasarımı oradan alınır.

Runmark, Nexroz'un module ve plugin sınırlarını ürünün ihtiyaçlarına uyarlar:
ince plugin runtime, kalıcı ürün servisleri, değiştirilebilir UI ve entegrasyonlar.
Runtime pluginlerin nasıl çalışacağını; ürün servisleri execution, evidence ve
trust kurallarını bilir. CLI ve desktop aynı application katmanını kullanır.

Execution/evidence/trust servislerinin sahibi Runmark'tır. Bunlar korumalı
system plugin olarak sunulabilir; üçüncü parti replacement sözleşmesi yoktur.
Bir ekran kapatıldığında verisi silinmez ve ilgili ürün servisi durmaz.

## Adlandırma

| Alan | Hedef |
|---|---|
| Ürün / repository kökü | Runmark / `runmark/` |
| CLI binary | `rmk` (`rmk.exe` Windows'ta) |
| Uygulama dizinleri | `apps/cli`, `apps/desktop` |
| Kütüphane dizini | `libs/domain`; `runmark-domain` değil |
| CMake target alias | `runmark::domain` |
| C++ namespace / named module | `runmark` / `runmark.domain` |
| QML module | `Runmark.Shell` |
| Workspace verisi | `.runmark/` |
| Preserved Git ref | `refs/runmark/preserved/<exec-id>` |
| Plugin ID | `com.runmark.findings` |
| Route | `runmark://com.runmark.findings/list` |

Tablodaki adlar ve dizin düzeni koda uygulandı: `apps/cli` ile
`libs/{domain,application,infrastructure}` ayrımı RM-1'de yapıldı.

PoC sürecinde geriye uyumluluk aranmadı: önceki ad, veri dizini ve Git
ref'leri hiçbir yerde tutulmaz, migration kodu yazılmaz.

## Katmanlar ve bağımlılıklar

```text
CLI ────────────────────────────────┐
                                    v
Desktop → Merce UI Shell → QML proxy → Application → Domain
                                                → Infrastructure

Plugin Runtime → Plugin API ← System / Feature / Integration plugins
       ├── QML Runtime    → izinli UI katkıları ve proxy'ler
       └── Process Host   → sürümlü IPC ve process gözetimi
```

Bu çizim çağrı akışıdır. Domain runtime, UI ve infrastructure import etmez.
Application'ın dış dünya bağımlılıkları açıkça verilir; plugin sınırındaki
registry, tüm iç kodun kullandığı global bir service locator olmaz.

| Bileşen | Sorumluluk |
|---|---|
| `domain` | Project, Workspace, Execution, Evidence, Finding; saf kurallar |
| `application` | Inspect, status, start, finish, resume, evidence/note akışları |
| `infrastructure` | Git, dosya sistemi, JSON/JSONL, ledger ve handoff I/O |
| `runtime` | Discovery, manifest validation, dependency graph, lifecycle, effects, diagnostics |
| `plugin-api` | Plugin context, sürümlü capability ve kayıt sözleşmeleri |
| `process-host` | Process başlatma, IPC, timeout, crash/restart politikası |
| `qml-runtime` | QML katkılarını oluşturma, proxy erişimi, instance sahipliği |
| `ui-shell` | Merce, navigation, route/tab, inspector, activity ve rescue görünümü |

CLI, QML motoruna veya Merce'ye bağımlı olmaz. Gerekli headless servisler
uygulama başlangıcında bağlanır. Frontend'ler iş kurallarını yeniden yazmaz.

## Hedef repository yapısı

```text
runmark/
├── CMakeLists.txt
├── CMakePresets.json
├── cmake/
│   ├── dependencies.cmake
│   └── toolchains/
├── apps/
│   ├── cli/
│   │   ├── CMakeLists.txt
│   │   └── src/main.cpp
│   └── desktop/
│       ├── CMakeLists.txt
│       ├── src/main.cpp
│       └── assets/
├── libs/
│   ├── domain/
│   ├── application/
│   ├── infrastructure/
│   ├── runtime/
│   ├── plugin-api/
│   ├── process-host/
│   ├── qml-runtime/
│   └── ui-shell/
├── plugins/
│   ├── system/
│   │   └── execution-services/
│   ├── features/
│   │   ├── overview/
│   │   ├── plan/
│   │   ├── worktrees/
│   │   ├── executions/
│   │   └── findings/
│   └── integrations/
│       ├── planning-markdown/
│       ├── mudissue/
│       ├── codex/
│       └── claude-code/
├── integrations/
│   └── agent-clients/
│       └── runmark/
├── schemas/
│   ├── plugin-manifest-v1.schema.json
│   └── plugin-rpc-v1.schema.json
├── tests/
│   ├── integration/
│   ├── compatibility/
│   └── e2e/
├── tools/
└── docs/
    ├── adr/
    ├── architecture/
    └── sdk/
```

Dizinler ilgili özellik uygulanırken oluşturulur. Unit testler ilgili
kütüphanenin `tests/` dizininde; çapraz bileşen testleri kökte tutulur.
Mevcut ADR geçmişi `docs/DECISIONS.md` içindedir; `docs/adr/` gelecekteki
doküman yerleşimidir, ikinci bir karar kaynağı henüz açılmaz.

`plugins/integrations/`, Runmark'ın yüklediği adaptörleri içerir.
`integrations/agent-clients/runmark/`, dış agent uygulamalarına kurulan
skill/hook paketidir. Bugünkü `plugins/runmark-agent` ikinci gruba taşınır;
marketplace tanımları ve hook yolları aynı geçişte güncellenir.

## C++ named modules

C++ named module derleme zamanı sınırıdır; runtime plugin veya QML module
değildir. BMI dosyaları public plugin ABI'si olarak dağıtılmaz.

İlk sınırlar `runmark.domain`, `runmark.application`, `runmark.runtime` olur.
Her sınıf için ayrı module açılmaz; ihtiyaç halinde partition kullanılır:

```text
libs/domain/
├── CMakeLists.txt
├── src/
│   ├── domain.cppm       # export module runmark.domain;
│   ├── facts.cppm        # export module runmark.domain:facts;
│   ├── rules.cppm        # export module runmark.domain:rules;
│   └── rules.cpp
└── tests/
    └── rules_test.cpp
```

- CMake `FILE_SET CXX_MODULES` ve target tabanlı compile features kullanılır.
- Import graph döngüsüz olur; dış API açıkça export edilir.
- QObject/QML köprüsü başlangıçta `.h/.cpp` kalır; module import `.cpp`
  tarafında yapılır. MOC header'ı module içindeki tiplere bağımlı olmaz.
- `import std`, header units ve özel MOC/module helper'ı başlangıç kapsamı dışıdır.
- Baseline C++20 / Qt 6.11+ / CMake 4.4+'dır. `FILE_SET CXX_MODULES` CMake
  3.28'de geldi, Merce 3.30+ ister; 4.4 tabanı ikisini de karşılar ve yerel
  derlemede doğrulanan sürümdür. C++23 / Ninja / LLVM Clang 19.1+ ve Windows
  MSVC hattı teknik doğrulama adaylarıdır; henüz build gereksinimi değildir.
- Conan 2 bu kararla otomatik eklenmez. Araç zinciri macOS/Linux/Windows'ta
  clean ve incremental build, MOC/QML köprüsü ve testler doğrulanınca sabitlenir.

Modules açık API sınırı sağlar; build hızlanması ölçülmeden vaat edilmez.
Teknik referans: [CMake C++ Modules](https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html).

## Plugin modeli

| Tür | Sorumluluk / politika |
|---|---|
| Protected system | Execution/evidence/trust servislerini sunar; normal modda kapatılamaz |
| Feature | Ekran, panel, komut katkısı; kapatılabilir |
| Integration | Plan/task/agent/evidence kaynağına adaptör; kapatılabilir |

Başlangıç system pluginleri statik bağlanabilir. Public native ABI ve
ürün servislerini üçüncü partiyle değiştirme hedeflenmez. Dış entegrasyonlar
sürümlü JSON-RPC 2.0 over stdio üzerinden ayrı process çalışır.

Manifest, IPC payload'ları ve capability sürümleri doğrulanır. Required
service eksikse plugin bekler; dependency cycle aktivasyonu engeller.
Aktivasyon başarısızsa o scope'un kayıtları geri alınır. Provider kapanırken
ona bağımlı consumer'lar önce durdurulur; devam eden çağrılar sonuçlandırılır
veya açık hata ile iptal edilir.

Her plugin instance'ının effect scope'u; listener, command, UI contribution,
service, timer/watcher ve process sahipliğini tutar. Kapanış ters kayıt
sırasında, idempotent ve süre sınırıyla yapılır; bir cleanup hatası kalan
cleanup işlemlerini engellemez. Kalıcı ledger/evidence verisi scope kapanınca
silinmez. Git veya harici API işlemleri için cleanup, otomatik rollback değildir.

## UI ve Merce

Merce ortak tasarım sistemi olarak kullanılır; Runmark için kompakt `desktop`
profili hazırlanır. Mevcut kiosk profilleri değiştirilmez. Merce ve Qt sürümleri
sabitlenir; StyleKit Technology Preview riski yükseltme testleriyle yönetilir.

Shell navigation, route/tab host, context inspector, activity alanı ve temel
rescue görünümünü sağlar. Feature pluginleri bu bölgelere katkı verir;
execution/evidence dosyalarına doğrudan yazmaz, izinli servisleri çağırır.
Theme kontrolü shell'dedir; pluginler ortak tokenları kullanır.

Route plugin owner'a bağlıdır. Plugin kapanınca route kapanır veya fallback
görünümüne geçer. Tab state `workspaceId + route + instanceKey` ile tutulur.
Klavye/focus, erişilebilir ad/rol, açık/koyu tema ve DPI doğrulaması
plugin sınırlarını da kapsar. UI reload/generation mekanizması ayrı teknik
doğrulama gerektirir; yalnız component cache temizlemek yeterli sayılmaz.

## Domain ve persistence

`Project Anchor != Workspace Root`: proje config'i ile execution worktree'si
ayrı yerlerdir. Project, Repository, Workspace, Plan, Task, Execution,
Evidence ve Finding anlamları korunur. Git/plan/task kaynakları source-of-truth
olarak kalır; plugin mimarisi yeni bir task veritabanını zorunlu kılmaz.

ProjectGraph, GitTruthEngine, PlanTruthEngine, ExecutionLedger, EvidenceStore
ve TrustEngine mantıksal ürün sorumluluklarıdır; her biri için ayrı kütüphane
veya sınıf oluşturmak zorunlu değildir. Finding'ler ölçülmüş facts'ten türetilir.

| Ürün sorumluluğu | İşlev |
|---|---|
| ProjectGraph | Multi-repo proje ilişkileri |
| GitTruthEngine | Fetch, ahead/behind, merge-base, stale base, dirty state, worktree ancestry |
| PlanTruthEngine | Plan task'ları ile evidence eşleşmesi |
| ExecutionLedger | Agent çalışmalarının append-oriented kalıcı kaydı |
| EvidenceStore | Diff, commit, build/test, artifact ve review referansları |
| TrustEngine | Somut evidence'a dayanan açıklanabilir findings |

Hedef disk düzeni, mevcut JSON/JSONL sözleşmesini korur:

```text
.runmark/
├── project.json
├── ledger/
├── handoffs/
├── evidence/
└── hook-observed.json
```

Mevcut `.runmark/` dosyaları ve preserved Git ref'leri geçiş yapılana kadar
geçerlidir. Eski/yeni dizin birlikte bulunduğunda seçim, migration, backup ve
rollback davranışı kod geçişinden önce tanımlanıp compatibility testleriyle
doğrulanır. Bu doküman SQLite veya otomatik veri taşıma kararı getirmez.

## Agent start akışı

```text
Resolve project → resolve task → git fetch → Git Truth → Plan Truth
→ previous execution/handoff → minimal context bundle → launch agent
```

## Güvenlik sınırları

- QML gerçek sandbox değildir; proxy erişimi uygulama sözleşmesini sınırlar.
- Ayrı process crash izolasyonu sağlar; OS sandbox olmadan aynı kullanıcının
  dosya/ağ izinlerini ortadan kaldırmaz. İlk sürüm açıkça güvenilen pluginlerle çalışır.
- Workspace açılması plugin çalıştırma izni değildir. Workspace paketleri
  korumalı system plugin ID'lerini override edemez.
- Plugin disable ile native binary unload ayrıdır; ilk native güncellemeler restart gerektirir.
- Agent credential'ları Runmark'a kopyalanmaz; resmî runtime sahipliği korunur.
- Force push, dirty rebase, branch deletion ve merge otomatik yapılmaz.
  Agent'ın tamamlandı iddiası tek başına task doğrulaması değildir.
