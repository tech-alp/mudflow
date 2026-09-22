# Runmark OD Prototype Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Runmark'ın bağlantılı, Türkçe, Markdown-first desktop demo akışını OD'de üretmek ve doğrulamak.

**Architecture:** Üretim Open Design tarafından yapılır; orkestratör brief'i sağlar ve sonucu kontrol eder. Tek bellek içi durum modeli Liste/Kanban/Belge ile çalışma ekranlarını besler. Runmark native kodu değiştirilmez.

**Tech Stack:** OD `frontend-design`; önerilen agent `claude`, model `opus` (OD'nin döndürdüğü alias; kesin model sürümü varsayılmaz). Prototip HTML/CSS/JS ES modules; backend ve yeni framework kurulumu gerekmiyor. Merce v1.2.0 semantik token referansı.

**Spec:** `docs/superpowers/specs/2026-09-22-desktop-prototype-design.md` — bütün belge, yalnız özeti değil, her üretim brief'ine eklenir.

## Global Constraints

- 1440 × 900 ana ölçü; 1280 × 800 kullanılabilir.
- Türkçe UI; Runmark, rmk, .runmark/, runmark-agent; MudIssue adı korunur.
- Koyu tüm akış; aynı bileşenlerle açık Genel Bakış.
- Gerçek Git/Jira/ajan/dosya yazması yok; açık Demo etiketi.
- Markdown beyanı ≠ gözlenen test ≠ insan kabulü; Jira ayrı kaynak.
- Native PoC salt-okunur Overview + Findings olarak kalır.
- Normal metin kontrastı en az 4.5:1; büyük metin ve UI sınırları en az 3:1.
- Yeni OD projesi; mevcut projeye izinsiz yazma, push, release veya native değişiklik yok.

## Review Focus

1. Dış belge revizyonu kaydetme/undo sırasında kaybolmamalı — görev 2.
2. Yinelenen ID ve Markdown içindeki HTML/script yanlış bağlama veya kod çalışmasına yol açmamalı — görev 2.
3. Proje/senaryo değiştirmek SCMS kayıtlarını başka projeye taşımamalı — görev 1 ve 3.
4. Yeni commit eski testin geçerli sayılmasına veya kabul üretmesine yol açmamalı — görev 3.
5. Dar pencere, klavye ve modal kapanışı ana işlem/focus kaybettirmemeli — görev 1 ve 4.

## Dosya sahipliği

Runmark deposunda yalnız bu plan ve sonuç raporu tutulur:
`docs/superpowers/plans/2026-09-22-od-prototype-review.md`.
OD projesindeki yollar proje köküne göredir:

- `index.html`: tek uygulama giriş noktası.
- `tokens.css`: açık/koyu semantik tokenlar.
- `app.css`: desktop yerleşimi ve ortak bileşen stilleri.
- `app.js`: shell, ekranlar ve etkileşim bağlama.
- `state.mjs`: tek store, fixture, belge revizyonları ve saf geçişler.
- `state.test.mjs`: Node yerleşik test/assert kontrolleri.
- `DESIGN.md`: Merce eşlemesi ve native uyarlama sınırı.
- `README.md`: demo kullanım rotası ve senaryo listesi.

Dosyalar OD ajanı tarafından oluşturulur; orkestratör başarısız/yavaş üretimi
atlamak için elle ikinci prototip yazmaz. OD çalışma süresi boyunca 30–60 saniye
aralıkla durum sorgulanır; yalnız kullanıcı iptal isterse run iptal edilir.

## Merce referansı — doğrulanan eşleme

Yerel kaynak `/Volumes/Mac/SCS/dev/Merce_`, etiket `v1.2.0`, commit
`2e587eb9f5cfc2f3456508646e22d151f45f0b55`.
Etiketteki `Theme/MerceTheme.h` ve `Theme/MerceColors.h` incelendi.

| Prototip CSS rolü | Merce QML rolü |
|---|---|
| --surface-canvas | Theme.colors.surface.canvas |
| --surface-container | Theme.colors.surface.container |
| --surface-raised | Theme.colors.surface.containerRaised |
| --text-primary | Theme.colors.content.primary |
| --text-secondary | Theme.colors.content.secondary |
| --border-subtle | Theme.colors.outline.subtle |
| --focus | Theme.colors.outline.focus |
| --action-primary | Theme.colors.action.primary.container |
| --action-primary-text | Theme.colors.action.primary.content |

Theme ayrıca spacing/radius/size/typography grupları sunar. Runmark desktop
yoğunluk değerleri öneri olarak DESIGN.md'de ayrılır; hazır Merce desktop
profilinin bulunduğu veya web prototipinin doğrudan QML olduğu söylenmez.
Renk değerleri grafit/teal brief'ine uygun seçilip ölçülür; kontrol edilmiş
bir Merce dark manifestinden alınmış gibi gösterilmez.

## Task 1: OD projesi, shell ve ortak durum

**Files:** OD `index.html`, `tokens.css`, `app.css`, `app.js`, `state.mjs`, `state.test.mjs`, `DESIGN.md`.
**Interfaces:** `createState()` başlangıç durumunu; `dispatch(state, action)` yeni durumu döndürür. State alanları `projectId`, `scenario`, `selectedWorkPackageId`, `theme`, `document`, `executions`, `reports`.

- [ ] OD ajan/skill listesini yeniden kontrol et; claude/opus unavailable ise sessiz model değiştirme, bildir.
- [ ] Yeni `Runmark Desktop Prototype` projesi oluştur; dönüşteki tam project ID ile devam et.
- [ ] Tam spec + bu plan + Merce eşleme tablosunu OD brief'ine ekle. İlk run shell, Genel Bakış ve ortak store'u üretsin; landing page/template dashboard istemediğimizi belirt.
- [ ] OD'ye şu testi önce eklet ve çalıştır; uygulanmadan önce FAIL, store tamamlanınca PASS beklenir:

```js
import test from 'node:test';
import assert from 'node:assert/strict';
import {createState, dispatch} from './state.mjs';
test('project switch preserves SCMS selection', () => {
  const initial = createState();
  const empty = dispatch(initial, {type: 'selectProject', id: 'runmark-demo'});
  assert.equal(empty.projectId, 'runmark-demo');
  const restored = dispatch(empty, {type: 'selectProject', id: 'scms'});
  assert.equal(restored.selectedWorkPackageId, initial.selectedWorkPackageId);
});
```

- [ ] `node --test state.test.mjs` sonucunu iste; yapılmadıysa yapılmadı kaydet.
- [ ] Önizlemeyi aç: dört ana navigasyon, proje seçimi, kapatılabilir sağ panel, Cmd+K, tema, boş proje; 1280 × 800'de ana işlem görünür olsun.
- [ ] `get_artifact` ile bütün bundle'ı al, ortak token ve store kullanımını kontrol et. Beklenen çıktı çalışan shell'dir; sonraki run mevcut bundle'ı geliştirir.

## Task 2: Liste / Kanban / Belge ve revizyon güvenliği

**Files:** OD `app.js`, `app.css`, `state.mjs`, `state.test.mjs`.
**Interfaces:** `parsePlan(text)` → `{items, errors}`; `applyPlan(current, draft, expectedRevision)` → `{ok, reason, document}`. `current` alanları `text`, `revision`; item alanları `id`, `title`, `declaredDone`. Hatalı/çatışan uygulama current nesnesini değiştirmez.

- [ ] İkinci run'a aynı spec'i gönder; üç görünüm, kart ayrıntı sekmeleri, Taşı menüsü, ham taslak, Uygula, Geri al ve AI dış değişiklik simülasyonunu iste.
- [ ] Aşağıdaki kontrolleri `state.test.mjs` içine eklet, önce FAIL sonra PASS döngüsüyle doğrulat:

```js
test('stale draft cannot overwrite newer source', () => {
  const current = {text: '- [ ] SCMS-42-W3 — Yeni başlık', revision: 2};
  const result = applyPlan(current, '- [x] SCMS-42-W3 — Eski başlık', 1);
  assert.equal(result.ok, false);
  assert.equal(result.reason, 'conflict');
  assert.deepEqual(result.document, current);
});
test('duplicate IDs reject the document', () => {
  const parsed = parsePlan('- [ ] SCMS-42-W3 — A\n- [x] SCMS-42-W3 — B');
  assert.ok(parsed.errors.length > 0);
});
```

- [ ] Test import'una `parsePlan, applyPlan` eklet; `node --test state.test.mjs` çalıştırılsın.
- [ ] UI'da W3 başlığını değiştir, kimliğin/kanıt bağlantısının korunduğunu üç görünümde kontrol et. W3'ü tamamlandıya taşı; test veya kabul oluşmamalı.
- [ ] Taslak açıkken dış değişiklik ve undo dene; güncel kaynak üzerine yazılmamalı. İlgisiz Markdown paragrafı/yorum değişmeden kalmalı.
- [ ] Belgeye `<img src=x onerror=alert(1)>` metni gir; metin olarak gösterilmeli, kod çalışmamalı. Genel HTML renderer ekleme.

## Task 3: Başlatma, handoff, kanıt ve destek ekranları

**Files:** OD `app.js`, `app.css`, `state.mjs`, `state.test.mjs`, `README.md`.
**Interfaces:** `dispatch` eylemleri `selectScenario`, `startDemo`, `handoffDemo`, `requestReview`, `acceptDemo`, `toggleProjectPlugin`. Kontrollerin sonucu state'de açık `notice` alanında; gerçek dış işlem yok.

- [ ] Üçüncü run'da üç adımlı başlatma, profil içeriği/skill durumları, Çalışma Alanları, Geçmiş, handoff, raporlar, iki eklenti bölümü ve Ayarlar tamamlat.
- [ ] Spec'teki SCMS fixture ve 18/18 + 12/12 raporlarını tek store'dan bağlat. İnsan kabulünü ayrı açık işlem tut.
- [ ] Aşağıdaki test önce FAIL sonra PASS olmalı:

```js
test('remote failure blocks a verified-base start', () => {
  const offline = dispatch(createState(), {type: 'selectScenario', id: 'remote-failed'});
  const started = dispatch(offline, {type: 'startDemo', mode: 'verified-base'});
  assert.deepEqual(started.executions, offline.executions);
  assert.equal(started.notice.code, 'remote-unverified');
});
```

- [ ] `node --test state.test.mjs` çalıştır; test sayısı ve sonuçları sakla.
- [ ] Görünür UI'dan görev → plan → profil → preflight → Claude → kesinti → Codex → rapor → inceleme akışını yürüt.
- [ ] `d42a103` senaryosunda eski 12/12 görünür fakat güncel kanıt sayılmaz; otomatik kabul yok. Hook kapalı senaryosunda sınırlı takip görünür.
- [ ] Eklentiyi SCMS için kapat; Runmark demo ayarı değişmesin, geçmiş silinmesin. Plugin hatasında shell gezinmesi sürsün.
- [ ] Başlamadan önce/kesinti/Codex/inceleme senaryolarını sırayla seç; sayı/başlık/ajan ve son kontrol zamanı tüm ekranlarda tutarlı olsun.

## Task 4: Görsel/etkileşim kabulü ve teslim

**Files:** OD `README.md`, `DESIGN.md`; Runmark `docs/superpowers/plans/2026-09-22-od-prototype-review.md`.
**Interfaces:** Girdi OD run ID, project ID, preview URL ve artifact bundle. Çıktı spec kabul maddeleri 1–14 için PASS/FAIL/NOT RUN raporu.

- [ ] 1440 × 900 ve 1280 × 800'de ana ekranları ayrı okunabilir görüntülerle kontrol et; body yatay taşması, kesilen buton veya ulaşılmaz panel olmamalı.
- [ ] Koyu tüm ekranlar/açık Genel Bakış, kontrast, uzun branch/path, empty/loading/error/offline/limited durumlarını kontrol et.
- [ ] Yalnız klavyeyle Cmd+K, seçim, kart taşıma, dialog aç/kapat ve focus dönüşünü dene. Console hatalarını kaydet.
- [ ] Spec §9 maddelerini sonuç raporuna tek tek geçir. Gerçek ölçüm yoksa PASS yazma; modelin iddiasını bağımsız doğrulama diye sunma.
- [ ] Sorun varsa aynı OD projesinde hedefli düzeltme run'ı iste; çalışan diğer akışları korut ve etkilenen kontrolleri tekrarla.
- [ ] Preview URL, model alias/run ID, dosya dökümü, Merce eşleme sınırı ve kalan bulguları kullanıcıya ver. Native PoC koduna geçme.

## Yürütme kapısı

Kullanıcının seçtiği yöntem OD üzerinden üretimdir; ek yerel üretim ajanı
gerekmiyor. Bu yazılı plan incelenip onaylanmadan OD projesi/run oluşturulmaz.
Onay sonrası executing-plans ile bu dört görev sıralı yürütülür.
