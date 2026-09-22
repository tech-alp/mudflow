# Runmark — Etkileşimli Desktop Prototipi

Tarih: 2026-09-22
Durum: Kullanıcı incelemesine hazır; uygulama onayı değildir.

## 1. Amaç ve onaylanan sınır

Runmark, AI destekli geliştirmede plan, Git durumu, çalışma geçmişi ve
kanıtları bağlayan local-first geliştirme uygulamasıdır. Kullanıcı açılışta
ne yapıldığını, neyin eksik olduğunu ve hangi işlemle devam edeceğini anlamalıdır.
Sohbet uygulaması, Jira kopyası, IDE veya landing page değildir.

Kullanıcı kararı: tüm ana kullanım akışı etkileşimli demo olarak tasarlanır;
native macOS PoC yalnız Overview + Findings dilimini uygular. Tasarımda bir
yeteneğin gösterilmesi, Runmark backend'inde uygulanmış olduğu anlamına gelmez.
Linux daha sonra, Windows en son gelir.

Teslimat birbirine bağlı, okunabilir ölçekte ekranları olan OD prototipidir;
bağımsız dashboard görselleri veya küçültülmüş ekran kolajı değildir.
UI Türkçedir. Kod, branch, dosya, komut ve ürün adları çevrilmez.
Ürün Runmark, CLI `rmk`, veri dizini `.runmark/`, ajan bağlantısı
`runmark-agent` olarak adlandırılır. MudIssue ayrı ürün olarak adını korur.

## 2. Yaklaşım ve mimari sınır

Seçilen yaklaşım: OD içinde ortak durum ve bileşenlerle tam demo akışı.
Yalnız Overview tasarımı handoff'u sınamaz; doğrudan tam QML uygulaması ise
etkileşim değişikliklerini pahalılaştırır. Bu yüzden prototip üretim kodu değildir.

Native hedef:

```text
rmk CLI ──────────────────┐
                          ├─ Application → Domain / Infrastructure
Merce shell → QML proxy ──┘
    └─ Findings plugin → izinli application proxy
```

UI, CLI JSON çıktısını parse etmez; ledger/Git/trust kurallarını tekrar yazmaz.
Execution, evidence ve trust servisleri korumalıdır. Değiştirilebilir özellik
ve entegrasyon pluginleri bu servislerin izinli sözleşmelerini kullanır.
Plugin olması, ayrı ana navigasyon sayfası olması demek değildir.
Plugin kapanması geçmişi silmez; ilgili görünüm son bilinen durum veya fallback gösterir.

Demo yalnız bellek içi durum kullanır. Gerçek Git, Jira, dosya, ajan, terminal,
credential veya plugin kurulumu işlemi yapmaz. Sabit ve küçük “Demo” göstergesi
bulunur; simüle başlatma/etkinleştirme işlemleri gerçek başarı gibi sunulmaz.

## 3. Roller ve durumların anlamı

- İş kaynağı: Jira, GitHub Issues, MudIssue veya yerel görev; hepsi zorunlu değildir.
- Planlama yöntemi: Superpowers; çalışma notları: planning-with-files.
- Ajan: Claude Code veya Codex. İş kaynaklarıyla aynı sağlayıcı sütununa konmaz.
- Çalışma profili: seçilmiş talimatların sürümlü görüntüsü; Skillapse referanslıdır.
- Runmark: görev, plan, repo/worktree, execution ve evidence bağlantısını tutar.

Jira durumu ve yerel çalışma aşaması ayrı etiketlerdir. Demo boyunca Jira
`In Progress` kalabilir; Runmark “İnceleme bekliyor” olabilir. İnsan kabulü de
Jira'yı otomatik kapatmaz.

Aşamalar: Planlama → Onaylandı → Uygulama → Kontrol → İnceleme → Kabul edildi.
Execution durumu (çalışıyor, ara verildi, kesildi, durduruldu) aşamadan ayrıdır.
“Ara ver”, “Durdur” ve “İncelemeye gönder” farklı işlemlerdir; menü içinde sunulur.
Normal ajan cevabının bitmesi veya test başarısı işi kendiliğinden bitirmez.
Küçük işlerde gereksiz belge zorunluluğu yoktur; eksik kayıt başarı sayılmaz.

## 4. Ortak shell ve görsel sözleşme

- Üst: proje seçici, `Cmd+K` komut menüsü, yerel/bağlantı durumu ve Demo etiketi.
- Sol: Genel Bakış, Planlar ve Görevler, Çalışma Alanları, Çalışma Geçmişi.
- Sol alt: Eklentiler, Ayarlar. Plugin kurmak otomatik navigasyon eklemez.
- İçerik: tek ana çalışma yüzeyi; bağlama göre sekme ve açılır ayrıntı.
- Sağ: kapatılabilir “Şu anki çalışma” paneli. Kapatınca alan içeriğe döner.
- Her ekranda duruma göre tek belirgin ana işlem: Başla, Devam et veya İncele.

1440 × 900 ana ölçü; 1280 × 800'de navigasyon ve ana işlem görünür kalır.
Dar ölçüde sağ panel kapatılabilir; tablolar sayfayı yatay taşırmaz, teknik
kolonlar ayrıntıya taşınır. Uzun path/branch kısaltılır; tam değer tooltip ve
kopyalama ile erişilir. Kaydırma içerik bölgelerindedir.

Merce `v1.2.0` referanstır; Runmark kompakt desktop profili kiosk profilini
değiştirmez. Nihai native kontroller Qt Quick/QML olacaktır. OD web teknolojisi
kullanabilir; hover-only işlemler, web sitesi hero'su veya mobil düzen kullanamaz.

Grafit taban, düşük doygunluklu yüzeyler, kontrollü teal/mavi vurgu; ince
ayırıcılar, ölçülü köşe yarıçapı, 14–16 px gövde; kod/path/commit monospace.
Grafik, güven yüzdesi, gradient, glassmorphism, AI parıltısı ve kart yığını yoktur.

Ortak semantik token grupları: yüzey, metin, ayırıcı, vurgu, focus, durum;
tipografi, spacing, radius ve kontrol yüksekliği. Bunlar önerilen tasarım
rolleridir, Merce API isimleri olduğu iddia edilmez. Uygulama planı öncesinde
v1.2.0 kaynakları incelenip mevcut token/bileşenlere eşleme çıkarılır; eksikler
Runmark desktop uyarlaması olarak açıkça ayrılır, Merce'de varmış gibi sunulmaz.
Koyu tema tüm akışta; aynı token/bileşenlerle Genel Bakış açık tema karşılığı.

Klavye ile tüm etkileşimler erişilir; görünür focus, erişilebilir ad/rol,
Escape ile panel/modal kapatma ve açan kontrole focus dönüşü zorunludur.
Dialog açıkken focus içinde tutulur. Uyarılar ikon + metin taşır; normal metin
kontrast hedefi en az 4.5:1, büyük metin ve UI sınırları en az 3:1'dir.

## 5. Ekranlar

### Genel Bakış

Ana alan aktif plan ve beş anlamlı iş paketidir. Satırda ad, çalışma durumu,
kanıt/kontrol durumu ve gerekiyorsa kısa eyleme dönük uyarı bulunur. Büyük
istatistik kartı yoktur. Aynı uyarı birden fazla panelde tekrarlanmaz.
Sağ panel görev, ajan, repo/worktree, son kontrol noktası ve sonraki işlemi gösterir.
“Devam et” aynı görevin kesilmiş çalışmasını açar; yeni görev yaratmaz.

### Planlar ve Görevler

Sol plan/iş paketi listesi, merkez seçili iş. Her checkbox bağımsız görev değildir.
Sekmeler: Özet, Plan ve Belgeler, Değişiklikler, Kontroller, Geçmiş.
Özet: amaç, kabul kriterleri, kaynak, çalışma aşaması, açık konu, sonraki adım.
Plan: spec/plan yolu, bölüm, kayıtlı içerikle değişiklik karşılaştırması, Kaynağı aç.
Kaynağı aç demo belge görüntüleyicisini açar; belgenin tamamı başka ekranlara kopyalanmaz.
Değişiklikler: repo/dosya listesi ve kısa demo diff; gerçek editör/terminal yoktur.
Kontroller: komut, exit code, execution, repo ve commit/çalışma ağacı kimliği,
rapor, gözlenen sonuç/ajan bildirimi ayrımı. Eski kod sonucuna açık uyarı verilir.
Dosyanın yaşı tek başına değişmiş/yanlış olduğu anlamına gelmez.

### Çalışmaya Başla

Üç adım; Geri/İleri durum kaybetmez, iptal hiçbir çalışma yaratmaz:

1. Görev ve çalışma alanı: SCMS-42, cart-server + device-agent, mevcut worktree
   veya yeni worktree, proje tabanı `development`.
2. Ajan ve profil: Claude Code/Codex, C++ / Qt geliştirme ve İçeriği incele.
3. Hazırlık kontrolü: remote erişimi, seçilen taban commit, yerel çakışma,
   görev/plan bağlantısı, ajan hazırlığı ve hook gözlemi ayrı satırlardır.

Demo açıklaması: “Yerel development, origin/development'tan 14 commit geride.
Yeni worktree, kontrol edilen remote commit'inden oluşturulacak.
Yerel development değiştirilmeyecek.” Bu fark cart-server'a aittir;
device-agent ayrıca kontrol edilir, aynı 14 commit ona kopyalanmaz.

Başlatma yalnız ilgili tüm repo tabanları doğrulandığında “Kontrol edilen
tabandan başlat” ile simüle edilir. Remote erişilemezse yeni verified-base
başlatma kapalıdır; yeniden deneme ve iptal bulunur. Mevcut çalışma korunur,
son kontrol zamanı gösterilir; risk açıklaması sonrası açık kullanıcı tercihiyle
devam edilebilir, “güncel” iddiası yoktur. Otomatik rebase/merge yapılmaz.
Çakışan worktree için başka alan seçimi gerekir; hiçbir değişiklik silinmez.

### Çalışma Alanları

SCMS altında cart-server, device-agent, cart-app, barcode-service listelenir.
İlk iki repo SCMS-42'ye bağlı; diğerlerinde aktif görev yoktur.
Satır: repo, görev/worktree, branch, yerel değişiklik, remote karşılaştırması,
son kontrol zamanı. Ayrıntı: commit, base, ayrışma ve dosyalar.
Proje kökü `/demo/SCMS` ile worktree kökü `/demo/worktrees/SCMS-42` ayrıdır.
Yerel development geriliği ile feature branch ayrışması farklı açıklanır.

### Çalışma Geçmişi ve Ajan Değiştir

Sohbet değil olay zaman çizelgesi: başlatma, plan onayı, kod kaydı, test,
kesinti, devralma, inceleme isteği ve insan incelemesi.
Seçili execution görev, ajan, profil görüntüsü, worktree, kod, test, karar ve
açık konuları gösterir. Claude ve Codex aynı göreve bağlı ayrı execution'lardır.

Ajan değiştir paneli Claude Code → Codex, aynı görev ve aynı worktree'leri
açıklar. Aktarım: plan, kayıtlı özet, değişiklikler, testler, açık kararlar,
sonraki adım. Eksik kaynak “Kayıt eksik”; ajan özeti “Ajan bildirimi”dir.
“Bütün bağlam aktarıldı” denmez. Onay sonrası “Demo: Codex ile devam senaryosu”
başlar; canlı süreç oluşturulduğu iddia edilmez.

### Eklentiler ve Ayarlar

İki bölüm: Uygulama Eklentileri (Jira, GitHub Issues, MudIssue, Test Raporları)
ve Ajan Bağlantıları (Claude Code/Codex içindeki runmark-agent entegrasyonu).
Ajan bağlantısı Qt/QML plugin'i değildir. Ayrıntıda Genel, Proje ayarları,
İzinler, Tanılama sekmeleri; kurulum, proje etkinliği, araç hazırlığı, güven/onay,
son kontrol ve gözlenen hook ayrı alanlardır. Gerçek destek kapsamı demo etiketlidir.

Proje bazında etkinleştirme simülasyonu diğer projeyi etkilemez. Devre dışı
kalınca geçmiş korunur, yeni veri durur ve “Son bilinen durum” gösterilir.
Hook yoksa “Sınırlı takip: otomatik kontrollerin bir kısmı kullanılamıyor.”
İzin listesi güvenlik sandbox garantisi değildir. Marketplace veya installer yoktur.
Ayarlar yalnız tema ve demo profil incelemesini kapsar; sahte kapsamlı ayarlar yoktur.

## 6. Profiller ve skill durumları

Profiller: C++ / Qt geliştirme, Hata düzeltme, Teknik araştırma.
İçerik: Superpowers (yöntem), planning-with-files (notlar), proje test kuralları,
Runmark (görev bağlantısı ve çalışma kontrolleri). İkinci skill marketplace yoktur.
Bulundu → Seçildi → Hazırlandı → Kullanımı gözlendi ayrı durumlar; sonuncusu
yalnız demo gözlem kaydı varsa gösterilir. Aşamayla ilgisiz skill aktif gösterilmez.
Sürüm/içerik kimliği profil ayrıntısında; execution başlangıcındaki profil
görüntüsü sonradan sessizce değişmez. Yeni profil yalnız yeni çalışmaya uygulanır.
Aynı plan dosyasını iki güncelleyici sahiplenirse “Çakışabilecek talimatlar”
açıklaması ve birini seçme seçeneği sunulur.

## 7. Tutarlı demo verisi ve geçişler

Tek ortak demo store kullanılır; ekran başına bağımsız fixture tutulmaz.
Proje SCMS; görev `SCMS-42 — Config migration`; Jira `In Progress`.
Profil `C++ / Qt geliştirme`, demo snapshot `cpp-qt-demo-1`.
Plan `docs/superpowers/plans/config-migration.md`; notlar
`task_plan.md`, `findings.md`, `progress.md`. Örnek yollar/commitler demodur.

| İş paketi | Kesinti anındaki durum | Kanıt |
|---|---|---|
| Eski config biçimini incele | İncelendi | Kayıtlı analiz; insan kabulü değildir |
| Yeni şemaya dönüştür | İnceleme bekliyor | Diff + cart-server testi |
| Device-agent doğrulamasını ekle | Uygulama | Diff var, test eksik |
| Geri alma davranışını test et | Bekliyor | Rapor yok |
| Sonuçları incele | Bekliyor | İnsan incelemesi yok |

İlgili branch iki repoda da `task/SCMS-42`; repo kimliği zorunludur.
cart-server kayıtlı kodu `c42a101`, raporu 18/18 başarılı, exit 0.
device-agent kesinti kodu `d42a101`, test raporu yok. Codex demo devamında
`d42a102` koduna 12/12 başarılı rapor ekler; geri alma kapsamı bu raporda açıkça
gösterilir. cart-server kodu değişmedikçe 18/18 raporu geçerli kalır.
Toplam sayı gerekmedikçe gösterilmez; test başarısı kabul durumunu değiştirmez.

Demo kontrolü durumları: Başlamadan önce, Claude kesintisi (varsayılan),
Codex devamı, İnceleme bekliyor. Baştan oynat kullanıcıyı ilk duruma döndürür.
Akış: görevi aç → plan → profil → hazırlık → Claude → kayıtlı ilerleme →
kesinti → Codex devralma → test raporu → incelemeye gönder.
İnsan kabulü ayrı ve açık demo işlemi; otomatik tetiklenmez.

Ek senaryolar aynı store üstünden seçilir:
- Hook kapalı: kurulu/etkin/gözlenen ayrımı, sınırlı takip; eski kayıt korunur.
- Remote başarısız: son başarılı kontrol zamanı kalır, güncellik bilinmiyor.
- Kod değişti: device-agent `d42a103`, eski 12/12 raporu silinmez ama yeni koda
  doğrulama sayılmaz; inceleme öncesi eksik kontrol gösterilir.
- Plan değişti: kayıtlı plan içeriğiyle fark; tarihinden hata türetilmez.
- Plugin hata: shell ayakta, ilgili katkı fallback, yeniden deneme/ayrıntı.

İkinci proje Runmark demo boş durumdur; SCMS görevleri oraya taşınmaz.
Proje değiştirip dönünce SCMS seçimi/ilerlemesi korunur. Senaryo sıfırlama tüm
ilgili raporları, olayları ve rozetleri birlikte geri alır. Sabit demo saati ve
tek olay listesi kullanılır; ekranlarda farklı “son kontrol” zamanları üretilmez.

## 8. Boş, yükleniyor ve hata davranışı

Boş proje: açıklama ve “Demo SCMS projesine geç” işlemi; uydurma görev yoktur.
Yükleniyor: içerik alanında kısa süreli durum, focus kaybolmaz ve çift işlem engellenir.
Hata: neden + yeniden dene; önceki veri varsa “Son bilinen durum” olarak kalır.
Bağlantısız: yerel gezinme sürer; remote/Jira güncelliği bilinmiyor gösterilir.
Eksik kanıt: rapor yok mesajı; ajan beyanı yerine gözlenen sonuç uydurulmaz.
İncelemeye gönder kritik kontrol açar; eksik raporda Kontroller'e yönlendirir.
Her dosya işleminde modal yoktur; kapatma veya ara verme sonsuz hook döngüsüne girmez.

## 9. Teslimatlar ve kabul kontrolü

OD projesi ve açılabilir etkileşimli önizleme; ortak token/bileşenler;
tek demo durum modeli; senaryo seçimi; Merce eşleme notu; kısa demo kullanım rehberi.
Görseller varsa yalnız referanstır; bu belgeyle çelişen eski davranış taşınmaz.
Bu oturumda görsel referans sağlanmadı, incelenmiş gibi davranılmaz.

Doğrulama üretim backend testinden ayrıdır:
1. Ana akış baştan sona yalnız görünür UI işlemleriyle yürür.
2. Görev seçimi, tüm ayrıntı sekmeleri, üç adım, profil, hazırlık ayrıntısı,
   handoff, rapor ve proje bazlı plugin etkinleştirme gerçekten etkileşimlidir.
3. Tüm ekranlarda görev/ajan/branch/rapor aynı store'dan gelir.
4. Hook kapalı ve remote hatalı senaryolar sahte yeşil başarı üretmez.
5. Yeni kod eski testi güncel kanıt saymaz; başarılı test insan kabulü yapmaz.
6. 1440 × 900 ve 1280 × 800'de taşma ve kesilen ana işlem yoktur.
7. Klavye sırası, Cmd+K, Escape, focus dönüşü ve kontrast kontrol edilir.
8. Koyu tüm ekranlar + aynı bileşenlerle açık Genel Bakış incelenebilir.
9. Console/runtime hataları kontrol edilir; çalıştırılmamış kontrol geçti denmez.
10. Plugin devre dışı/hata durumunda shell ve geçmiş korunur.

Native PoC kabulü ayrı kalır: macOS Merce shell, gerçek application verili
salt-okunur Overview/Findings, yenileme, plugin activation failure/cleanup ve
paketlenmiş .app smoke testi. Diğer demo ekranlarının native uygulanması bu
tasarımın teslim şartı değildir. Büyük Worktrees listesi/DPI gibi mevcut roadmap
hedefleri daha sonraki desktop genişlemesidir; tamamlandı diye işaretlenmez.

## 10. Süreç ve kapsam dışı

Bu belge incelenip onaylandıktan sonra Superpowers writing-plans ile uygulama
planı hazırlanır. Plan incelemesi ve yürütme tercihi sonrasında OD projesi
oluşturulur; önce shell + ana akış, ardından destek ekranları ve hata durumları.
OD üretimi sırasında mevcut bir tasarımın üstüne izinsiz yazılmaz.

Kapsam dışı: native ürün implementasyonu, gerçek agent/Jira/Git çağrıları,
marketplace, process plugin SDK, workflow designer, cloud sync, auth aktarımı,
otomatik merge/rebase, release/push ve Linux/Windows uygulaması.

İlgili kaynaklar: [UX](../../UX.md), [Mimari](../../ARCHITECTURE.md),
[Teknoloji kararları](../../TECH_CHOICES.md), [Roadmap](../../ROADMAP.md).
