# Mudflow Teknoloji Kararları (v0.1)

ADR'ler ürün kararlarını tutar (DECISIONS.md). Bu doküman **uygulama
kararlarını** tutar: hangi kütüphane, hangi sınıf, neden, ne zaman değişir.

Her kararın sonunda **geri dönüş maliyeti** yazılı. Pahalı olanlar tartışılır,
ucuz olanlar tartışılmaz.

---

## TC-001 — Tek C++ core, iki ince frontend

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
- **Ayrı süreç (desktop → `mudflow status --json`):** süreç sınırı cazip ama
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

**Geçiş tetikleyicisi:** `mudflow project status` gibi **iç içe** komut grupları.
ROADMAP Phase 1'de bunlar var; geldiklerinde manuel dispatch çirkinleşir.
v0.1'in üç komutu (`start` / `finish` / `status`) için gerek yok.

Geri dönüş maliyeti: **çok düşük.** Mantığın tamamı `core/` altında;
parser değişimi tek dosyaya dokunur. Bu yüzden karar üzerinde durulmaz.

---

## TC-005 — Dosya izleme: v0.1'de yok

`mudflow status` çalışır, hesaplar, çıkar. İzlemeye ihtiyacı yok.
İzleme sadece desktop'ın canlı yenilemesi için gerekli — **Phase 3.**

Sırası geldiğinde: `QFileSystemWatcher`.

İzlenecek küme küçük ve recursive değil:

```text
.mudflow/ledger/*.jsonl
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

Watchman'ın çözdüğü problem (devasa ağacın recursive izlenmesi) Mudflow'da yok.
Local-first bir masaüstü uygulamasına daemon kurma yükü bindirmek adoption
maliyeti.

**Watchman'ın gerçekten yeri olduğu senaryo ayrı:** repo çok büyük ve
`git status` yavaşsa, git'in `core.fsmonitor` ayarı Watchman'ı kullanabilir.
Bu **git'e** bağlanır, Mudflow'a değil. Mudflow hiçbir şey bilmeden hızlanır.

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
farklı dil, kamuya açık versiyonlu sözleşme); bunların hiçbiri Mudflow'da yok.
JSON-RPC aynı katmanlamayı verir ve bakılacak bir derleyici bırakmaz.

Geri dönüş maliyeti: **yok** — bugün kod değil, kural.

---

## TC-007 — CLI çıktı sözleşmesi

```text
stdout    sonuç JSON'u        (yalnızca başarıda)
stderr    hata JSON'u         (yalnızca başarısızlıkta)
exit      0 başarı · 1 runtime · 2 kullanım
```

Hata gövdesi:

```json
{ "error": { "code": "runtime", "message": "Cannot read /yok/project.json: ..." } }
```

Neden ayrı akış: stdout "sonuç" kanalıdır, taşıma düzeyindeki başarısızlıkları
taşımaz — TC-006'nın aynı ayrımı. Böylece `mudflow status | jq` boruya hata
sızdırmaz ve desktop, CLI'ı spawn ederse iki kanalı karıştırmadan okur.

`code` şimdilik yalnızca iki değer alıyor: `usage`, `runtime`. Daha ince bir
taksonomi (`git.fetch_failed` gibi) **ilk gerçek tüketici bir koda göre dallanmak
isteyince** eklenir, önce değil.

Bu sözleşme, desktop'ın core'u link mi edeceği yoksa CLI'ı spawn mı edeceği
kararını açık tutar. Sözleşme olmazsa spawn seçeneği sessizce ölür.

Geri dönüş maliyeti: **düşük.** Tek fonksiyon (`emitError`).

---

## TC-008 — Bileşen dizinleri ve CMake bağımlılıkları

Dizin adları proje öneki taşımaz: `core/`, `cli/`, `docs/`, `cmake/`.
Core testleri `core/tests/`, CLI sözleşme testi `cli/tests/` altında tutulur.
Henüz uygulanmayan `ui/` ve `agent/` dizinleri oluşturulmaz.

Header'lar `core/include/mudflow/` altında kalır; include yolu
`<mudflow/...>` olarak korunur. Statik core hedefi `mudflow_core`, alias'ı
`mudflow::core` olur.

Her bileşen doğrudan kullandığı dış bağımlılığı kendi `find_package` çağrısıyla
bulur. Core, `Qt6::Core` bağımlılığını `PUBLIC` aktarır; CLI yalnız
`mudflow::core` linkler. Core'u linklemeyen CLI sözleşme testi Qt'yi kendi bulur.
Kök CMake Qt aramaz ve hedef ya da install kuralı tanımlamaz; proje sürümünü,
seçenekleri, CTest, alt dizinler ve CPack'i yönetir.

C++20 gereksinimi `target_compile_features(mudflow_core PUBLIC cxx_std_20)`
ile tüketicilere aktarılır; global `CMAKE_CXX_STANDARD` kullanılmaz.
Core'u linklemeyen CLI sözleşme testi C++20 gereksinimini kendi hedefinde belirtir.
Kurulum `cli/` içinde `GNUInstallDirs` ile yapılır; dış tüketicisi olmayan
statik core için install kuralı yoktur.

Sürümün tek kaynağı `project(Mudflow VERSION 0.1.0 ...)` olur.
CLI, `cmake/version.h.in` şablonundan `configure_file` ile üretilen başlığı
kullanır. `MUDFLOW_BUILD_CLI` varsayılan olarak açık, gelecekteki UI için
`MUDFLOW_BUILD_UI` kapalıdır; testler `BUILD_TESTING` ile yönetilir.

Geri dönüş maliyeti: **düşük.** Dizin yolları ve CMake bağlantıları değişir;
çalışma zamanı mantığı değişmez.

---

## TC-009 — Çekirdek katmanları: gözlem, değerlendirme, orkestrasyon

```text
GÖZLEM           git'i çalıştır, planı oku, ledger'ı oku, dosya varlığını ölç
                 → RepoFacts · PlanFacts · ExecutionFacts        (saf veri)

DEĞERLENDİRME    evaluate(config, facts) → Finding[]             (SAF fonksiyon)

ORKESTRASYON     komutlar: inspect · status · start · finish · evidence · note
```

Dosya karşılığı:

```text
core/src/git.cpp       taşıma (run/git/gitRequired) + ölçüm → RepoFacts
core/src/plan.cpp      ölçüm → PlanFacts
core/src/ledger.cpp    olay yaz/oku, evidence
core/src/paths.cpp     .mudflow yerleşimi, expandPath, sha1
core/src/handoff.cpp   handoff yaz  (resume okuma tarafını buraya ekleyecek)
core/src/rules.cpp     SAF — facts alır, finding döndürür, I/O yok
core/src/workflow.cpp  yalnız orkestrasyon
```

`facts.h` ve `rules.h` public (`core/include/mudflow/`); geri kalan başlıklar
`core/src/` altında çekirdeğe özeldir.

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

## Özet: neyi ne zaman değiştiririz

| Karar | Şimdi | Tetikleyici |
|---|---|---|
| Core dili | C++ / Qt Core | — (pahalı, değişmiyor) |
| Config | JSON | 50 satırı geçerse → YAML |
| CLI parser | QCommandLineParser | İç içe komut grubu → CLI11 |
| Dosya izleme | Yok | Phase 3 → QFileSystemWatcher |
| Recursive izleme | — | FD limiti veya kaynak ağacı izleme ihtiyacı → Watchman |
| Markdown | 2 regex | Checkbox/başlık/ID'den fazlası → parser |
| Qt dışı bağımlılık | Sıfır | Her biri ayrı karar |
| Provider katmanlaması | Kural, kod değil | Phase 6 → JSON-RPC 2.0 over stdio |
| Hata `code` alanı | `usage` / `runtime` | Tüketici koda göre dallanmak isteyince |
| `rules.cpp` saflığı | I/O yok, `now` fact | Bozulursa ayrım kaybolur — yeni gerçek `facts.h`'ye eklenir |
