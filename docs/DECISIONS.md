# Runmark Decisions

ADR-001–015, Runmark adıyla alınan karar geçmişidir; eski komut ve veri
yolları tarihsel bağlamıyla korunur. 22 Eylül 2026 tarihli ADR-016–019 hedef
mimariyi tanımlar. Kabul edilmiş hedef, kodun taşındığı anlamına gelmez.

## ADR-001 — Planning framework değil
Accepted.

Runmark Superpowers, planning-with-files vb. araçları entegre eder.

## ADR-002 — Evidence > agent claim
Accepted.

Commit, diff ve test; agent summary’den daha güçlü evidence’tır.

## ADR-003 — Plugin-first, not everything-is-a-plugin
Accepted.

Core yerleşimi ADR-018 ile güncellendi; ürün kavramlarının sahipliği korunur.

Core:
- ProjectGraph
- GitTruthEngine
- PlanTruthEngine
- ExecutionLedger
- EvidenceStore
- TrustEngine

## ADR-004 — v0.1 external plugin ABI yok
Accepted.

Önce provider contracts stabilize edilmeli.

## ADR-005 — Local-first
Accepted.

Cloud backend gerekmiyor.

## ADR-006 — Project anchor ve workspace root ayrıdır
Accepted.

## ADR-007 — Yeni iş verified remote base’den başlar
Accepted.

Default:

```text
git fetch
→ resolve remote base
→ health check
→ create worktree
```

## ADR-008 — MudIssue bir provider’dır
Accepted.

Runmark Mud’a bağımlı olmamalı.

## ADR-009 — Agent runtime external kalır
Accepted.

Claude/Codex auth Runmark’a taşınmaz.

## ADR-010 — Handoff durable source referanslamalı
Accepted.

## ADR-011 — Qt/QML desktop
Preferred.

Desktop seçimi ADR-017 ile kesinleşti; named modules yönü ADR-019'dadır.

```text
C++20
Qt 6.11+
QML
CMake
```

## ADR-012 — Trust explainable olmalı
Accepted.

Her warning somut evidence göstermeli.

## ADR-013 — Workspace core'da kalır, mevcut worktree sahiplenilir
Accepted.

Runmark workspace yaratabilir; MudIssue, IDE veya elle oluşturulmuş geçerli
worktree'yi de kullanabilir. Workspace'i kimin yarattığı dış mekanizmadır.

Runmark her durumda kendi ölçümünü kaydeder:

```text
fetch remote base
→ workspace/repo ilişkisini doğrula
→ dirty state kontrol et
→ HEAD ve merge-base'i ölç
→ ledger'a kaydet
```

MudIssue zorunlu bağımlılık değildir. v0.1'de workspace provider veya Mud
adapter'ı yoktur.

## ADR-014 — Ne bloklar, ne uyarır
Accepted.

Ana repo'nun kirliliği `start`'ı **engellemez**. Base uzak ref'ten çözüldüğü
için teknik bir engel yoktur; bu bir proje hijyeni tercihiydi ve bloklayıcı
olması insanları aracın dışına itiyordu.

Durum kaybolmaz: `execution.started` olayına `repo_dirty` yazılır ve `start`
çıktısı `warnings` dizisinde `git.dirty_workspace` döndürür.

Bloklayıcı kalanlar:

```text
worktree başka repo'nun   ölçüm yanlış repoyu ölçer
worktree başka execution'a ait
execution zaten var
task, task_id_pattern'e uymuyor
```

Worktree kirliliği artık bloklamaz. Runmark, working tree veya stash'e
dokunmadan geçici index ile snapshot commit'i üretir ve bunu
`refs/runmark/preserved/<exec-id>` altında saklar. Bu yakalama başarısızsa
`start`/`finish` başarısız olur; aksi halde kanıtsız iş üretirdi.

İlke: **ölçümü bozan şey bloklar, hijyen tercihi uyarır.**

---

## ADR-015 — Hook'un çalıştığı ölçülür, varsayılmaz
Accepted.

Kurulu sanılan ama hiç çalışmayan bir SessionStart hook'u, temiz bir projeden
ayırt edilemez: ikisinde de `status` sessizdir. Bu, projenin avladığı sessiz
körlük sınıfının aynısıdır.

`resume --hook` çağrıldığında `.runmark/hook-observed.json` yazılır. Üç sonuç:

```text
hooks_expected yok        → kural değerlendirilmez
beklenti var, gözlem yok  → context.hooks_not_observed (warning)
gözlem var                → sessiz
```

Beklenti `project.json`'da opt-in'dir. Her projede uyarmak, CLI'yi tek başına
kullanan projeye kapatamayacağı bir bulgu üretirdi — ADR-014'teki "hijyen
tercihi uyarır" ilkesi burada uyarının kendisini opt-in yapar.

Gözlem ledger olayı **değildir**. Ledger execution'a bağlı, append-only olay
kaydıdır; hook gözlemi hiçbir execution'a ait değil ve yalnız son değeri
anlamlı. Ayrıca `resume` bir okuma yoludur: oraya olay yazmak `resume`'u
idempotent olmaktan çıkarır ve ledger'ı ölçümle değil trafikle şişirirdi.

Yalnız "hiç görüldü mü" ölçülür. "En son ne zaman" tutulur ama henüz kural
üretmez; hook'un sonradan bozulduğunu yakalamak ayrı bir karardır.

## ADR-016 — Runmark adı, rmk CLI ve sorumluluk bazlı dizinler

Accepted. Adlandırma 2026-09-22'de uygulandı; dizin geçişi bekliyor.

Ürün adı Runmark, CLI executable adı `rmk` olur. Workspace verisi `.runmark/`
altında tutulur. Uygulamalar `apps/cli` ve `apps/desktop`; kütüphaneler
`libs/domain`, `libs/runtime` gibi sorumluluk adlarıyla yerleşir. Dizinlere
`runmark-` öneki eklenmez. Ürün kimliği `runmark::domain`, `runmark.domain`,
`Runmark.Shell`, `com.runmark.findings` gibi teknik namespace'lerde kullanılır.

Gerekçe: kısa terminal komutu ve ürün adına bağlı olmayan okunabilir dosya düzeni.

Bedeli birlikte taşınması gereken sözleşmelerdi ve hepsi aynı anda taşındı:
binary adı, `--version` çıktısının ilk kelimesi, agent plugin'inin
`minimum_rmk_version` anahtarı, hook'un çağırdığı komut, marketplace adları,
veri dizini ve preserved Git ref namespace'i. Biri atlanırsa hook sessizce
hiçbir bağlam enjekte etmez.

PoC sürecinde geriye uyumluluk aranmadı: eski ad, eski dizin ve eski ref'ler
korunmaz, migration kodu yazılmaz. Kurulu eski binary ve pluginler söküldü.

## ADR-017 — Desktop tasarım sistemi Merce

Accepted — hedef; entegrasyon bekliyor. Tarih: 2026-09-22.

Runmark desktop, Merce kullanır. Kompakt `desktop` profili hazırlanır; kiosk
profilleri değiştirilmez. Shell ve feature görünümleri ortak semantic tokenları
kullanır; QGravityUI ikinci bir runtime tasarım bağımlılığı olarak eklenmez.

Gerekçe: merkezi token/style yönetimi ve mevcut tasarım altyapısının kullanımı.
Veri ekranlarının model/view davranışları Runmark'ta geliştirilir.
Bedeli: Merce'nin Qt.labs.StyleKit Technology Preview bağımlılığı nedeniyle
Qt/Merce sürümleri sabitlenir; yükseltmede focus, tema, DPI ve veri ekranları
doğrulanır. Merce CLI/headless bağımlılığı olmaz.

## ADR-018 — İnce runtime, korumalı ürün servisleri, değiştirilebilir pluginler

Accepted — hedef; runtime henüz yok. Tarih: 2026-09-22.
ADR-003'ün core yerleşimini günceller; ADR-004'ün public native ABI sınırını korur.

Domain/application/infrastructure ürün davranışını; runtime ise manifest,
bağımlılık, lifecycle ve effect sahipliğini yönetir. Execution/evidence/trust
servisleri Runmark'a aittir ve korumalı system plugin üzerinden sunulabilir.
Üçüncü parti ürün servisi replacement sözleşmesi açılmaz. UI ve dış sistem
adaptörleri değiştirilebilir. Ekranı kaldırmak veriyi veya ürün servisini silmez.

Gerekçe: yeni ekran/entegrasyon eklerken doğrulama kuralları korunur; CLI ve
desktop aynı application akışlarını kullanır. Bedeli: activation rollback,
dependency kaybı, devam eden çağrılar ve effect cleanup test edilmelidir.
Process izolasyonu sandbox; resource cleanup harici işlem rollback'i sayılmaz.

## ADR-019 — Dahili C++ named modules, ayrı plugin sözleşmesi

Accepted — mimari yön; toolchain doğrulaması bekliyor. Tarih: 2026-09-22.

Dahili API sınırları C++ named modules ile ifade edilir; ilk adaylar
`runmark.domain`, `runmark.application`, `runmark.runtime` olur. CMake
`FILE_SET CXX_MODULES` kullanılır. QObject/QML köprüsü başlangıçta klasik
header/source dosyalarında kalır. BMI public plugin ABI'si değildir.

Gerekçe: açık export yüzeyi ve döngüsüz bağımlılıklar. Bedeli: compiler/BMI,
dependency scanning ve MOC entegrasyonu; build hızlanması garanti değildir.
CMake tabanı 4.4'e çekildi — `FILE_SET CXX_MODULES` 3.28'de geldi, Merce 3.30+
ister. C++23, Ninja ve LLVM Clang/MSVC kombinasyonu adaydır; macOS, Linux ve
Windows clean/incremental build doğrulaması sonrası seçilir.
Mevcut C++20 build değiştirilmedi. `import std`, header units, Conan ve özel
MOC helper'ı bu kararla otomatik eklenmez.
