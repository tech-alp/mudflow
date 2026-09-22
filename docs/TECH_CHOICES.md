# Runmark Teknoloji Kararları

TC-001–009 mevcut Runmark uygulamasının kararlarını ve yollarını kaydeder.
Runmark/`rmk`, hedef dizin yapısı, Merce ve modules geçişi TC-010 ile
tanımlanır; mevcut kodun taşındığı veya minimum build sürümünün değiştiği
varsayılmaz. Yeni kararlar [ADR-016–019](DECISIONS.md) ile birlikte okunur.

ADR'ler ürün kararlarını tutar (DECISIONS.md). Bu doküman **uygulama
kararlarını** tutar: hangi kütüphane, hangi sınıf, neden, ne zaman değişir.

Her kararın sonunda **geri dönüş maliyeti** yazılı. Pahalı olanlar tartışılır,
ucuz olanlar tartışılmaz.

---

## TC-001 — Tek C++ core, iki ince frontend

İki frontend'in ortak iş mantığı ilkesi korunur. Dizin ve katman yerleşimi
hedefte TC-010 ile güncellenir; aşağıdaki yapı mevcut sürümdür.

```text
core/             tüm mantık, Qt Core, GUI yok
cli/              main() + argüman dispatch        ~200 satır
ui/               QML                              (Phase 3, henüz yok)
```

Nihai hedef Qt/QML desktop olduğu için core zaten C++ olmak zorunda.
CLI'ı başka dilde yazmak core'u ikinci kez yazmak demektir.

Değerlendirilen alternatifler:

- **Atılacak prototip (Go/Python):** hızlı iterasyon, ama "atarız" diye başlayan
  prototip atılmaz; üç ay sonra iki çekirdek bakarsın.
- **Ayrı süreç (desktop → `rmk status --json`):** süreç sınırı cazip ama
  yanlış yerde. Uzun süren işler (çok repo fetch, agent launch, canlı ilerleme)
  bu sınırdan geçmek zorunda kalır.

Süreç sınırı yine de kullanılacak — ama core ile frontend arasında değil,
**core ile provider'lar arasında** (Phase 6).

Geri dönüş maliyeti: **yüksek.** Bu dokümandaki tartışılması gereken tek karar.

---

## TC-002 — Qt Core standart kütüphanedir

v0.1'in ihtiyaç duyduğu her şey Qt Core'da var:

| İhtiyaç | Qt karşılığı |
|---|---|
| git çalıştır, çıktı yakala | `QProcess` |
| JSON oku/yaz | `QJsonDocument` |
| JSONL append | `QFile` (`QIODevice::Append`) |
| Markdown'dan checkbox / başlık / task ID | `QRegularExpression` |
| Dosya, dizin, glob, `~` genişletme | `QDir`, `QFileInfo` |
| ISO8601 zaman damgası | `QDateTime` |
| CLI argüman parse | `QCommandLineParser` |
| Terminal çıktısı | `QTextStream` |

Plan dosyası için gerçek markdown parser'ı **yok**. İhtiyaç duyulan tek şey
checkbox satırı, başlık ve task ID yakalamak; iki regex yeterli.
Tam parser, bu üçünden fazlası gerektiğinde konuşulur.

**Bağımlılık politikası (v0.1): Qt dışı sıfır.**
İlk dış bağımlılık bilinçli bir karar olmalı, kaza olmamalı.

Commit edilmemiş worktree snapshot'ı için `git stash` kullanılmaz: stash
kullanıcıya ait global yığındır ve çalışma alanını değiştirir. Mevcut
`QProcess` ile geçici index + `git commit-tree` kullanılır; yeni bağımlılık
veya provider gerekmez.

---

## TC-003 — Config formatı JSON

`project.yaml` yerine `project.json`.

Tek gerekçe: YAML, v0.1'in tek Qt dışı bağımlılığı olurdu (yaml-cpp).
`QJsonDocument` bedava geliyor. Dosya elle yazılıyor ama ~15 satır;
JSON olması kullanılabilirliği anlamlı ölçüde bozmuyor.

Bedeli açık: yorum satırı yok, tırnak gürültüsü var.
Config 50 satırı geçer veya kullanıcı elle yazmaktan şikâyet ederse YAML'a geçilir.

Geri dönüş maliyeti: **düşük.** Tek okuyucu fonksiyon.

> DATA_MODEL.md §2 bu karara göre güncellendi.

---

## TC-004 — CLI parser: QCommandLineParser

Option, flag, positional argüman, otomatik `--help` / `--version` veriyor.

**Bilinen eksiği: subcommand kavramı yok.** `argv[1]` elle alınıp dispatch
edilir, kalan argümanlar o komuta ait parser'a verilir. ~15 satır.

Ciddi alternatif **CLI11** (header-only, gerçek subcommand, iç içe gruplar,
validator, daha iyi help çıktısı).

**Geçiş tetikleyicisi:** `rmk project status` gibi **iç içe** komut grupları.
İç içe gruplar gerekirse parser yeniden değerlendirilir; mevcut düz komut
yapısı ve hedef `rmk` adlandırması tek başına yeni parser gerektirmez.

Geri dönüş maliyeti: **çok düşük.** Mantığın tamamı `core/` altında;
parser değişimi tek dosyaya dokunur. Bu yüzden karar üzerinde durulmaz.

---

## TC-005 — Dosya izleme: v0.1'de yok

`rmk status` çalışır, hesaplar, çıkar. İzlemeye ihtiyacı yok.
İzleme sadece desktop'ın canlı yenilemesi için gerekli — **Phase 3.**

Sırası geldiğinde: `QFileSystemWatcher`.

İzlenecek küme küçük ve recursive değil:

```text
.runmark/ledger/*.jsonl
.git/HEAD, .git/refs/**, .git/packed-refs
plan dosyası
```

Kaynak ağacı izlenmiyor — dirty state `git status` çalıştırılarak öğreniliyor,
dosya başına bildirime gerek yok.

### Watchman neden değil

| | Watchman | QFileSystemWatcher |
|---|---|---|
| Bağımlılık | Daemon kurulumu + yaşam döngüsü | Yok |
| Recursive izleme | Var — asıl gücü | Yok, dizinler elle eklenir |
| Ölçek | 100k+ dosya | Onlarca yol |
| FD limiti | Sorun değil | macOS/kqueue: yol başına 1 FD |
| Dağıtım | Kullanıcı daemon kuracak | Yok |

Watchman'ın çözdüğü problem (devasa ağacın recursive izlenmesi) Runmark'da yok.
Local-first bir masaüstü uygulamasına daemon kurma yükü bindirmek adoption
maliyeti.

**Watchman'ın gerçekten yeri olduğu senaryo ayrı:** repo çok büyük ve
`git status` yavaşsa, git'in `core.fsmonitor` ayarı Watchman'ı kullanabilir.
Bu **git'e** bağlanır, Runmark'a değil. Runmark hiçbir şey bilmeden hızlanır.

Geri dönüş maliyeti: **yok** — henüz kod yazılmıyor.

---

## TC-006 — Katmanlama: taşıyan katman taşıdığını bilmez

Provider'ı **çalıştıran** kod, provider'ın **ne dediğini** bilmez.

```text
Alt katman (taşıma)      başlat · çağır · cevabı al · zaman aşımı · hata · durdur
Üst katman (anlam)       planları listele · task durumu güncelle · testi çalıştır
```

Gerekçe: bu iki katman farklı hızlarda değişir. Taşıma neredeyse hiç değişmez;
provider API'si ilk aylarda sürekli değişir. Ayrı tutulmazlarsa üstteki her
değişiklik alttakine dokunur ve her yeni provider tipi tesisatı kopyalatır.

Referans: Vicinae'nin `manager.fig`'i aynı ayrımı yapıyor — manager eklentiyi
load/unload eder ve mesajı iletir, ama *"payload'ın neyden yapıldığını bilmez"*;
asıl API ayrı bir spec'te (`tsapi.fig`) durur.

### Bugün ne demek

**Hiçbir yeni soyutlama değil.** Provider'lar hâlâ düz C++ sınıfı (ADR-004).
Kural sadece şu: "provider'ı bul, çağır, hatasını yakala" ile
"plan dosyasını parse et" aynı fonksiyonda olmasın.

Mevcut kodda uygulanmış hali: `run()` / `git()` / `gitRequired()` taşıma,
çağıranlar yorumlama. TC-007'deki hata sözleşmesi de aynı ayrımın örneği —
çekirdek yapısal hata üretir, çıktı biçimine CLI karar verir.

### Sırası geldiğinde (Phase 6)

Süreç sınırı eklemek **alt katmanı** değiştirmek olur; üst katman değişmez.

Ve o gün kendi IDL'ini yazma — **JSON-RPC 2.0 over stdio** yeterli.
Vicinae figura'yı kendine özgü kısıtları için yazdı (üçüncü parti eklentiler,
farklı dil, kamuya açık versiyonlu sözleşme); bunların hiçbiri Runmark'da yok.
JSON-RPC aynı katmanlamayı verir ve bakılacak bir derleyici bırakmaz.

Geri dönüş maliyeti: **yok** — bugün kod değil, kural.

---

## TC-007 — CLI çıktı sözleşmesi

```text
stdout    sonuç JSON'u        (yalnızca başarıda)
stderr    hata JSON'u         (yalnızca başarısızlıkta)
exit      0 başarı · 1 runtime · 2 kullanım
```

`resume --markdown` açıkça istendiğinde stdout Markdown'dur; varsayılan JSON
ve stderr/exit sözleşmesi değişmez. Paket içindeki eksiklikler runtime hatası
değil, exit 0 sonucunun `gaps` alanıdır.

Hata gövdesi:

```json
{ "error": { "code": "runtime", "message": "Cannot read /yok/project.json: ..." } }
```

Neden ayrı akış: stdout "sonuç" kanalıdır, taşıma düzeyindeki başarısızlıkları
taşımaz — TC-006'nın aynı ayrımı. Böylece `rmk status | jq` boruya hata
sızdırmaz ve desktop, CLI'ı spawn ederse iki kanalı karıştırmadan okur.

`code` şimdilik yalnızca iki değer alıyor: `usage`, `runtime`. Daha ince bir
taksonomi (`git.fetch_failed` gibi) **ilk gerçek tüketici bir koda göre dallanmak
isteyince** eklenir, önce değil.

Bu sözleşme, desktop'ın core'u link mi edeceği yoksa CLI'ı spawn mı edeceği
kararını açık tutar. Sözleşme olmazsa spawn seçeneği sessizce ölür.

Geri dönüş maliyeti: **düşük.** Tek fonksiyon (`emitError`).

---

## TC-008 — Bileşen dizinleri ve CMake bağımlılıkları

Mevcut build düzeni. Tek `core` hedefi RM-1'de üçe bölündü.

Dizin adları proje öneki taşımaz: `apps/`, `libs/`, `docs/`, `cmake/`.
Her kütüphanenin testi kendi `tests/` dizininde, CLI sözleşme testi
`apps/cli/tests/` altındadır. Henüz uygulanmayan `ui/` dizini oluşturulmaz.

```text
libs/domain          runmark::domain           Qt6::Core disinda bagimlilik yok
libs/infrastructure  runmark::infrastructure  → runmark::domain
libs/application     runmark::application     → runmark::infrastructure
apps/cli (rmk)                                → runmark::application
```

Bağımlılık tek yönlüdür; ters bir kenar yoktur. `domain` public header'ları
`libs/domain/include/runmark/`; include yolu `<runmark/...>` korunur.
`infrastructure` kendi `src/` dizinini PUBLIC yapar — bu dışa açılan bir API
değil, yalnız üst katmanın gördüğü iç başlıklardır.

Her bileşen doğrudan kullandığı dış bağımlılığı kendi `find_package` çağrısıyla
bulur. Core, `Qt6::Core` bağımlılığını `PUBLIC` aktarır; CLI yalnız
`runmark::core` linkler. Core'u linklemeyen CLI sözleşme testi Qt'yi kendi bulur.
Kök CMake Qt aramaz ve hedef ya da install kuralı tanımlamaz; proje sürümünü,
seçenekleri, CTest, alt dizinler ve CPack'i yönetir.

C++20 gereksinimi `target_compile_features(runmark_core PUBLIC cxx_std_20)`
ile tüketicilere aktarılır; global `CMAKE_CXX_STANDARD` kullanılmaz.
Core'u linklemeyen CLI sözleşme testi C++20 gereksinimini kendi hedefinde belirtir.
Kurulum `cli/` içinde `GNUInstallDirs` ile yapılır; dış tüketicisi olmayan
statik core için install kuralı yoktur.

Sürümün tek kaynağı kök CMake'deki `project(... VERSION ...)` olur.
CLI, `cmake/version.h.in` şablonundan `configure_file` ile üretilen başlığı
kullanır. `RUNMARK_BUILD_CLI` varsayılan olarak açık, gelecekteki UI için
`RUNMARK_BUILD_UI` kapalıdır; testler `BUILD_TESTING` ile yönetilir.

Geri dönüş maliyeti: **düşük.** Dizin yolları ve CMake bağlantıları değişir;
çalışma zamanı mantığı değişmez.

---

## TC-009 — Çekirdek katmanları: gözlem, değerlendirme, orkestrasyon

```text
GÖZLEM           git'i çalıştır, planı oku, ledger'ı oku, dosya varlığını ölç
                 → RepoFacts · PlanFacts · ExecutionFacts        (saf veri)

DEĞERLENDİRME    evaluate(config, facts) → Finding[]             (SAF fonksiyon)

ORKESTRASYON     komutlar: inspect · status · start · finish · resume · evidence · note
```

Dosya karşılığı:

```text
libs/infrastructure/src/git.cpp        taşıma (run/git/gitRequired) + ölçüm → RepoFacts
libs/infrastructure/src/plan.cpp       ölçüm → PlanFacts
libs/infrastructure/src/ledger.cpp     olay yaz/oku, evidence
libs/infrastructure/src/paths.cpp      .runmark yerleşimi, expandPath, sha1
libs/infrastructure/src/handoff.cpp    handoff oku/yaz — yalnız disk
libs/infrastructure/src/config_io.cpp  project.json'u oku, ProjectConfig::parse'a ver
libs/domain/src/rules.cpp              SAF — facts alır, finding döndürür, I/O yok
libs/domain/src/project_config.cpp     SAF — JSON nesnesini doğrular, dosya açmaz
libs/application/src/workflow.cpp      yalnız orkestrasyon, TİPLİ sonuç döner
apps/cli/src/json.cpp                  SAF — tipli sonuç → JSON / Markdown
libs/ui-shell/src/StatusViewModel.cpp  use case → model; iş kuralı yok
libs/ui-shell/src/FindingModel.cpp     QAbstractListModel, rol eşlemesi
apps/desktop/src/main.cpp              QGuiApplication + QQmlApplicationEngine
```

Config okuma ile doğrulama RM-1'de ayrıldı: `ProjectConfig::parse` saf ve
domain'de, dosyayı açan `loadProjectConfig` infrastructure'da. Aksi halde
domain `QFile`'a bağımlı kalırdı ve bu ayrımın kendisi bozulurdu.

Aynı gerekçeyle `handoff.cpp` ikiye bölündü: `resumePackage` ve
`resumeMarkdown` saf sunum olduğu için ayrıldı, dosya yazan
`writeHandoff`/`readHandoff` infrastructure'da kaldı. RM-4'te sunum
`apps/cli/src/json.cpp`'ye indi — JSON, application'ın değil CLI'nin biçimi.

### Neden

Kural değerlendirmesi ölçümden ayrılmadığı sürece saf mantığı test etmenin tek
yolu tüm boru hattını çalıştırmaktı: plan regex'lerini sınamak için sahte bir
git reposu kurmak gerekiyordu. Ayrıldıktan sonra `core/tests/rules_test.cpp`
git, dosya sistemi ve saat olmadan çalışır.

Aynı ayrımı `DATA_MODEL.md` invariant'ı zaten söylüyordu: *finding'ler
türetilir, saklanmaz.* Kod artık o cümleye uyuyor.

### "Şimdi" de bir gözlemdir

`StatusFacts::now` taşınır, `rules.cpp` saat okumaz. Bu olmadan fonksiyon saf
olmazdı; olduğu için 24 saatlik terk-edilmişlik sınırı ledger tarihini geri
almadan test edilebiliyor.

### Sınır

`rules.cpp` içine `QFile`, `QProcess` veya `QDateTime::current*` girerse ayrım
bozulmuştur. Bir kuralın yeni bir gerçeğe ihtiyacı varsa o gerçek `facts.h`'ye
eklenir ve gözlem aşamasında doldurulur.

Geri dönüş maliyeti: **orta.** Davranış birebir korundu (bölme öncesi binary
ile çıktı karşılaştırması yapıldı), ama geri birleştirmek testleri kaybettirir.

---

## TC-010 — Runmark hedef düzenine aşamalı geçiş

Durum: hedef karar; implementation bekliyor.

- Ürün Runmark, CLI executable `rmk`; dizin adları `apps/cli`, `apps/desktop`
  ve `libs/domain` gibi sorumluluk adlarıdır. `runmark-` dizin öneki kullanılmaz.
- Teknik namespace'ler `runmark::domain`, `runmark.domain`, `Runmark.Shell`
  biçimindedir. CMake target'ları bağımlılık ve compile features'ı taşır.
- `core` domain/application/infrastructure olarak ayrılır. Runtime ürün
  kurallarını bilmez; korumalı ürün servisleri ile değiştirilebilir feature ve
  integration pluginleri ayrı tutulur. İç bağımlılıklar açıkça verilir.
- Named modules dahili API sınırıdır; public plugin ABI'si değildir.
  QObject/QML köprüleri başlangıçta `.h/.cpp`; module import `.cpp` içindedir.
- C++20 korunur; CMake tabanı 4.4'tür (`FILE_SET CXX_MODULES` 3.28+, Merce 3.30+). C++23 / Ninja / LLVM Clang ve Windows MSVC hattı modules
  doğrulamasında değerlendirilir. Üç OS sonucu olmadan yeni baseline ilan edilmez.
- Merce yalnız desktop katmanında eklenir; Qt/Merce sürümleri sabitlenir.
  CLI için Qt Core bağımlılık sınırı korunur. TC-002'nin yeni bağımlılığı
  gerekçelendirme ilkesi devam eder; desktop için Merce ADR-017 ile seçilmiştir.
- `.runmark/` veri dizinidir ve adlandırma geçişinde birlikte taşındı.
  PoC sürecinde geriye uyumluluk aranmadı; eski dizin ve ref'ler korunmaz.
  CLI adı değişti, JSON/exit sözleşmesi değişmedi — bunu cli_contract testi tutar.

Tek hedef ağaç [ARCHITECTURE.md](ARCHITECTURE.md) içindedir. RM-1'de
uygulanan eşleme (ayrıntısı TC-008'de):

| Durum | Taşıma |
|---|---|
| ✅ RM-1 | `cli/` → `apps/cli/`; `core/` → `libs/{domain,application,infrastructure}` |
| ✅ RM-1 | `project_config` parse/load ve `handoff` sunum/disk ayrımı |
| Bekliyor | `plugins/runmark-agent/` → `integrations/agent-clients/runmark/` |

Geri dönüş maliyeti: **orta/yüksek.** Dizin taşıma tek başına düşük maliyetlidir;
module toolchain, plugin lifecycle ve veri migration sözleşmeleri değildir.
Geçiş [ROADMAP](ROADMAP.md) sırasıyla, mevcut CLI regression testleri korunarak yapılır.

## Özet: neyi ne zaman değiştiririz

| Karar | Şimdi | Tetikleyici |
|---|---|---|
| Core dili | C++ / Qt Core | — (pahalı, değişmiyor) |
| Config | JSON | 50 satırı geçerse → YAML |
| CLI parser | QCommandLineParser | İç içe komut grubu → CLI11 |
| Dosya izleme | Yok | Phase 3 → QFileSystemWatcher |
| Recursive izleme | — | FD limiti veya kaynak ağacı izleme ihtiyacı → Watchman |
| Markdown | 2 regex | Checkbox/başlık/ID'den fazlası → parser |
| Qt dışı bağımlılık | CLI'da sıfır | Desktop için Merce seçildi (ADR-017) |
| Provider katmanlaması | Kural, kod değil | Phase 6 → JSON-RPC 2.0 over stdio |
| Hata `code` alanı | `usage` / `runtime` | Tüketici koda göre dallanmak isteyince |
| `rules.cpp` saflığı | I/O yok, `now` fact | Bozulursa ayrım kaybolur — yeni gerçek `facts.h`'ye eklenir |
| Ürün / CLI | Kodda Runmark / `rmk` | ADR-016 → Runmark / `rmk` |
| Katmanlar | `apps/`, `libs/` (RM-1) | Plugin sınırları bekliyor |
| Named modules | Yok | Üç OS toolchain doğrulaması ardından aşamalı geçiş |

---

## TC-011 — Üçüncü parti bağımlılıkları

Durum: karar. Tarih: 2026-09-22.

TC-002 "Qt Core standart kütüphanedir" diyor; bu madde yeni bir bağımlılık
gerçekten gerektiğinde **nasıl** alınacağını tanımlar.

### Karar merdiveni

İlk tutan basamakta dur:

```text
1. Qt Core yetiyor mu?                    → yeni bağımlılık yok (TC-002)
2. Hedef platformlarda zaten var mı?      → find_package(X REQUIRED)
3. Kaynağı bizim mi?                      → export set ekle, find_package(X CONFIG)
4. Tek dosya, stabil, küçük mü?           → vendor et, sürümü yaz
5. Hiçbiri değil                          → FetchContent, sabitlenmiş tag
```

`FetchContent` son çaredir çünkü bağımlılığın CMake'i bizim ağacımızda
çalışır: global değişkenleri, `add_subdirectory` çağrıları ve hedef adları
bizim projemize sızar. `find_package(CONFIG)` bu sızıntıyı kapatır —
tükettiğimiz tek şey import edilmiş hedeflerdir.

### Kurallar

- **Her bileşen kendi `find_package`'ını çağırır** (TC-008). Kök CMake
  bağımlılık dağıtmaz.
- **Bağımlılık hedefe bağlanır, dizine değil.** `target_link_libraries` ve
  `target_compile_features`; `set(CMAKE_CXX_STANDARD ...)` veya
  `include_directories()` gibi global çağrı kullanılmaz.
- **Namespace'li hedef zorunludur.** `Foo::Bar` yazım hatasında CMake hata
  verir; `FooBar` sessizce düz bir isim sayılır ve link satırına öylece geçer.
- **Desktop bağımlılığı CLI'ye sızamaz.** `apps/cli` yalnız Qt Core görür;
  bunu bileşen sınırları tutar (TC-008).
- **Sürüm sabitlenir.** Tag veya sürüm aralığı; `main` takip edilmez.
- **Sistem paket yöneticisi zorunlu kılınmaz.** Conan/vcpkg tek bir
  bağımlılık için eklenmez; gerekirse ayrı bir kararla gelir.
- **`add_subdirectory` ile gelen bağımlılığın seçenekleri önce ve FORCE ile
  yazılır, `block(SCOPE_FOR VARIABLES)` içinde.** Sonra yazılırsa bağımlılık
  kendi varsayılanını cache'e kilitlemiş olur; `block()` olmadan ise
  `BUILD_SHARED_LIBS` gibi ezmeler ana projeye sızar.

### Merce nasıl alınır

Merce ayrı bir depodur: `https://github.com/tech-alp/Merce.git`, v1.2.0,
`cmake_minimum_required(VERSION 3.30)`, `Qt6::LabsStyleKit` (Technology
Preview) ister.

Gömülmek üzere tasarlanmış — 3. basamağın nasıl görünmesi gerektiğinin örneği:

```cmake
# Merce/CMakeLists.txt
add_library(Merce::Controls ALIAS MerceControls)   # ve Core/Theme/Style/...
option(MERCE_BUILD_TESTS "" ${PROJECT_IS_TOP_LEVEL})
option(BUILD_MERCE_PLAYGROUND "" OFF)
if(PROJECT_IS_TOP_LEVEL)
    include(cmake/InstallMerce.cmake)
endif()
block(SCOPE_FOR VARIABLES)          # FetchContent seçenek ezmeleri sızmasın
    set(BUILD_PLAYGROUND OFF)
    FetchContent_MakeAvailable(QtToastify)
endblock()
```

Kaynaktaki yorum niyeti söylüyor: *"Keep the public target names identical for
FetchContent/add_subdirectory and installed find_package consumers."* Aynı
hedef adı iki tüketim yolunda da geçerli; tüketici hangi yolu seçtiğini
`target_link_libraries` satırında göstermek zorunda kalmıyor.

**Referans tüketici: `cart/app`.** QML modüllerini uygulamanın içine gömdüğü
için `find_package` değil submodule + `add_subdirectory` kullanıyor:

```cmake
set(MERCE_BUILD_NOTIFICATIONS ON  CACHE BOOL "" FORCE)
set(MERCE_BUILD_TESTS         OFF CACHE BOOL "" FORCE)
set(BUILD_MERCE_PLAYGROUND    OFF CACHE BOOL "" FORCE)
block(SCOPE_FOR VARIABLES)
    set(BUILD_SHARED_LIBS OFF)
    add_subdirectory(3rdparty/Merce)
endblock()
```

Submodule sürümü sabitliyor (`v1.1.0-38-gb16812e`), `block()` `BUILD_SHARED_LIBS`
ezmesinin ana projeye sızmasını engelliyor, seçenekler `add_subdirectory`'den
**önce** FORCE ile yazılıyor — sonra yazılırsa Merce kendi varsayılanını
kilitlemiş olur.

**Runmark için: FetchContent.** Submodule değil, çünkü Runmark'ın Merce'yi
yerel olarak düzenleme ihtiyacı yok ve temiz klon + configure tek adımda
çalışmalı — unutulmuş bir `git submodule update --init` sessizce eksik
bağımlılık demektir.

```cmake
include(FetchContent)

block(SCOPE_FOR VARIABLES)
    # Merce modülleri qt_add_qml_module ile STATIC/SHARED yazmadan tanımlı,
    # yani BUILD_SHARED_LIBS'i izliyorlar. Statik = QML modülleri ikilinin
    # içinde; çalışma anında import path veya plugin dizini yok.
    set(BUILD_SHARED_LIBS OFF)
    set(MERCE_BUILD_NOTIFICATIONS OFF)
    set(MERCE_ENABLE_FONTAWESOME OFF)

    FetchContent_Declare(Merce
        GIT_REPOSITORY https://github.com/tech-alp/Merce.git
        GIT_TAG 2e587eb9f5cfc2f3456508646e22d151f45f0b55   # v1.2.0
        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(Merce)
endblock()

target_link_libraries(runmark_ui_shell PUBLIC
    Merce::Theme Merce::Foundation Merce::Style Merce::Controls)
```

Neden bu biçim:

- **`GIT_TAG` bir SHA, tag değil.** Tag taşınabilir; SHA taşınmaz. Yorumda
  hangi sürüm olduğu yazılır. `GIT_SHALLOW` kullanılmaz: CMake, shallow
  clone ile yalnız branch/tag destekler; commit SHA desteklemez.
  Tek bir başarılı deneme bu kombinasyonu güvenilir bir sözleşme yapmaz.
  Kaynak: [CMake Git seçenekleri](https://cmake.org/cmake/help/latest/module/ExternalProject.html#git).
- **`EXCLUDE_FROM_ALL`** (CMake 3.28+): Merce hedefleri `all`'a girmez,
  yalnız link edilenler derlenir.
- **Seçenekler düz `set()`, `CACHE ... FORCE` değil.** Merce
  `cmake_minimum_required(VERSION 3.30)` diyor, yani CMP0077 NEW: `option()`
  normal değişkeni onurlandırır. Cache kirlenmez, ikinci bir projede aynı
  cache farklı değer beklemez.
- **`block(SCOPE_FOR VARIABLES)`** bu ezmelerin bizim ağacımıza sızmasını
  engeller — özellikle `BUILD_SHARED_LIBS`.
- `MERCE_BUILD_TESTS` ve `BUILD_MERCE_PLAYGROUND` yazılmaz; ikisi de
  `PROJECT_IS_TOP_LEVEL`'a bağlı ve gömülüyken zaten kapalı.

Bedeli **configure anında ağ**. Çevrimdışı veya Merce'yi yanında geliştirirken
kaçış yolu CMake'in kendi mekanizması:

```sh
cmake -B build -DFETCHCONTENT_SOURCE_DIR_MERCE=/yol/Merce   # yerel kopyayi kullan
cmake -B build -DFETCHCONTENT_FULLY_DISCONNECTED=ON         # hic ag yok
```

`find_package(Merce CONFIG)` de çalışır ve Merce onu destekler; ama Merce'nin
ayrıca kurulmuş olmasını gerektirir ve statik gömme avantajını verir.

Desktop yazılana kadar bağımlılık eklenmez.

### Neden

Bir bağımlılığı yanlış almak, onu hiç almamaktan pahalıdır: `add_subdirectory`
ile gelen global ayar sessizce bizim derleme bayraklarımızı değiştirir ve
sorun aylar sonra başka bir hedefte ortaya çıkar. Sızıntı olmayan tek yol
import edilmiş hedeflerdir.

---

## TC-012 — DDD ve Qt/QML entegrasyonu

Durum: application tarafı RM-4'te uygulandı; ui-shell desktop ile gelecek.
Tarih: 2026-09-22.

### Sorun

QML `QObject`, `Q_PROPERTY` ve sinyal ister. Domain bunların hiçbirini
bilmemelidir. Kolay yol — domain struct'larına `Q_GADGET` serpmek — derlenir
ama domain'i moc'a ve arayüzün veri şekline bağlar; TC-009'un ayırdığı şeyi
geri birleştirir.

### Application tipli döner (RM-4)

Önceden JSON döndürüyordu:

```cpp
QJsonObject projectStatus(const QString& configPath);   // RM-4 oncesi
```

JSON, CLI'nin **sunum biçimidir** (TC-007). Application'a gömülü kalırsa
desktop de CLI'nin çıktı şekline mahkûm olur: QML her açılışta JSON parse
eder ve domain'de tipli olan şey string'e düşer.

Şimdi:

```cpp
// libs/application
struct StatusResult { QString project; QVector<RepoFacts> repositories; QVector<Finding> findings; };
struct StartResult  { QString exec, worktree, branch, workspaceSource, baseSha, preservedRef;
                      QVector<Finding> warnings; };
struct FinishResult { QString exec, outcome, headSha, preservedRef, handoff; };
struct ResumeResult { ResumeFacts facts; QVector<Finding> gaps; };
```

`Finding` artık domain'de struct (`runmark/finding.h`); `QJsonObject` değil.
Serileştirme `apps/cli/src/json.cpp`'de: `toJson()` aşırı yüklemeleri ve
`resumeMarkdown`. Aynı sonucun iki sunumu yan yana, ikisi de CLI'nin.

`apps/cli` bunu JSON'a çevirir, `libs/ui-shell` tipleri doğrudan kullanır.
ARCHITECTURE'ın "CLI ve desktop aynı application katmanını kullanır" cümlesi
ancak böyle doğru olur. `Finding` de `QJsonObject` olmaktan çıkıp struct olur;
`finding()` fabrikası tek yerde olduğu için dönüşüm dardır.

### Katmanlar

```text
libs/domain          Facts, Finding, rules       QObject yok, moc yok
libs/infrastructure  git, fs, ledger             → domain
libs/application     use case'ler, tipli sonuç   → infrastructure
libs/ui-shell        ViewModel + Model + QML     → application + Merce
apps/cli             JSON serileştirme           → application
apps/desktop         QGuiApplication             → ui-shell
```

`apps/cli` Quick, QML veya Merce'yi **hiç görmez**; bunu TC-011'in
"desktop bağımlılığı CLI'ye sızamaz" kuralı ve bileşen sınırları tutar.

### QML köprüsü yalnız ui-shell'de

```cpp
class StatusViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(FindingModel* findings READ findings CONSTANT)
public slots:
    void refresh();     // application use case'ini cagirir, kural icermez
};
```

```cmake
qt_add_qml_module(runmark_ui_shell
    URI Runmark.Shell
    VERSION 1.0
    SOURCES StatusViewModel.cpp FindingModel.cpp
    QML_FILES Shell.qml StatusPage.qml)
target_link_libraries(runmark_ui_shell PUBLIC runmark::application Merce::Controls)
```

ViewModel iş kuralı taşımaz: use case'i çağırır, sonucu modele döker. Bir kural
ViewModel'e sızarsa CLI ile desktop ayrışır ve bunu ancak aynı sorunun iki
yüzeyde farklı cevaplanması ortaya çıkarır — yani geç.

### İki somut kısıt

**`status` bloklar.** `git fetch` saniyeler sürer; QML thread'inde çağrılırsa
arayüz donar. ViewModel işi worker'a atar (`QtConcurrent::run` +
`QFutureWatcher`) ve `busy` / `error` state'ini yayınlar. Bu, CLI'de hiç var
olmayan tek gereksinimdir; application API'si senkron kalır, eşzamansızlık
ui-shell'in işidir.

RM-7'de üç şey ölçülerek öğrenildi:

- `qt_add_qml_module` modül taramasını kapatıyor; `runmark.domain`'i import eden
  hedefte `CXX_SCAN_FOR_MODULES ON` gerekiyor.
- ADR-019'un "MOC header'ı module tiplerine bağımlı olmasın" kuralı teorik
  değil: `FindingModel.h` `runmark/finding.h`'ı dahil edince moc derlemesi
  `module 'runmark.domain' not found` ile düşüyor. Model bu yüzden kendi
  düz `Row` tipini taşır, çeviri `.cpp`'de yapılır.
- `qt_standard_project_setup()` çağrılmadığında QML kaynak prefix'i `/` olur
  ama engine yalnız `qrc:/qt/qml` altına bakar. Modül derlenir, hiçbir uyarı
  çıkmaz, çalışma anında bulunamaz.

**Qt politikaları açıkça sabitlenir.** `qt_standard_project_setup()`
çağrılmaz — dizin çapında `AUTOMOC` açardı (TC-011). Bunun yerine tek tek:

```text
QTP0003  köke     qt_add_library BUILD_SHARED_LIBS'i onurlandırır (Qt6Core)
QTP0001  ui-shell QML modülü kaynak prefix'i :/qt/qml (Qt6Qml)
QTP0004  ui-shell QML alt dizinleri için qmldir üretimi (Qt6Qml)
QTP0002  yok      yalnız Android
```

Bir politika, onu **tanımlayan modül bulunmadan** set edilemez: kökte yalnız
`Qt6::Core` arandığı için `qt_policy(SET QTP0001 NEW)` orada
"not a known Qt policy" hatası verir. QML politikaları bu yüzden
`libs/ui-shell` içindedir.

QTP0001 NEW, yukarıdaki prefix sorununu resmi yoldan çözer; elle
`RESOURCE_PREFIX` vermeye gerek kalmaz. QTP0003 RM-8 için gerekli: Merce
statik gömülürken `BUILD_SHARED_LIBS` ayarı ancak NEW davranışında dikkate
alınır.

Ayrıca `QT_QML_OUTPUT_DIRECTORY` ayarlanmazsa Qt, modülün çıktı dizini hedef
yoluyla bitmediği için uyarır ve `qmllint` modülü bulamaz.

**Domain'de `Q_OBJECT` / `Q_GADGET` yok.** `domain_purity` testi (RM-4)
`libs/domain` kaynaklarını tarar ve `QFile|QProcess|QDir|QTextStream|
QDateTime::current|Q_OBJECT|Q_GADGET` geçen dosya bulursa düşer. Yorum
satırları çıkarılır, yoksa yasağı anlatan yorumun kendisi ihlal sayılırdı.
Çeviri ViewModel'in işidir.

### Neden

Domain'i QML'e açmak ilk gün en kısa yoldur ve ikinci gün geri alınamaz:
arayüzün istediği her alan domain tipine bir `Q_PROPERTY` ekler, domain
arayüzün şekline göre büyümeye başlar. Çeviri katmanı bu baskıyı ui-shell'de
tutar; domain neyi ölçtüğüne göre şekillenmeye devam eder.
