# Mudflow Data Model (v0.1)

Bu doküman `start` / `finish` / `status` üçlüsünün yazdığı ve okuduğu her şeyi tanımlar.
Kod değil, şema. Amaç: C++ yazmadan önce "ne nereye yazılıyor" sorusunun bitmesi.

Kapsam: tek repo, tek plan dosyası, provider interface yok.
Genişleme noktaları en altta işaretli.

---

## 1. Disk düzeni

```text
.mudflow/
├ project.json                                  el ile yazılır
├ ledger/20260918T142231Z-SCMS-042.jsonl        execution başına 1 dosya
├ handoffs/20260918T142231Z-SCMS-042.md         execution başına 1 dosya
└ evidence/<sha1>.json                          büyük payload'lar
```

**Execution başına ayrı dosya** — iki agent aynı anda iki worktree'de çalışırken
yazma çakışması olmaz. Kilit yok, lock dosyası yok.
Dosya adı zaman damgasıyla başladığı için `ls` çıktısı zaten kronolojik;
ayrı index'e gerek yok.

ARCHITECTURE.md'deki `cache/` ve `index/` v0.1'de yok.
Ledger birkaç yüz satır; her `status`'ta baştan okunur.
`status` 200 ms'yi geçerse index eklenir.

Project anchor bir Git worktree içindeyse Mudflow yalnız ürettiği `ledger/`,
`evidence/` ve `handoffs/` yollarını ortak Git dizinindeki `info/exclude`a
idempotent ekler. `.mudflow/project.json` dışlanmaz; proje config'i takip
edilir. Paylaşılan `.gitignore` Mudflow tarafından değiştirilmez.

**Execution ID formatı:**

```text
<UTC ISO8601 compact>-<task_id>
20260918T142231Z-SCMS-042
```

Sıralanabilir, okunabilir, kütüphane gerektirmez.

---

## 2. project.json

Format kararı: TECH_CHOICES.md TC-003 (YAML yerine JSON → Qt dışı sıfır bağımlılık).

```json
{
  "version": 1,
  "name": "scms",

  "worktree_root": "~/worktrees",

  "repos": [
    {
      "name": "scms-core",
      "path": "~/projects/scms-core",
      "base": {"remote": "origin", "branch": "development"}
    }
  ],

  "plan": { "path": "docs/plans/config-migration.md" },

  "task_id_pattern": "SCMS-\\d+"
}
```

- `worktree_root` — ADR-006: project anchor != workspace root
- `repos[].base.remote` + `repos[].base.branch` — ADR-007: verified remote
  base. Git ref'i `remote + "/" + branch` olarak türetilir; eski
  `"origin/main"` string biçimi kabul edilmez.
- `task_id_pattern` — Plan ↔ evidence eşleşmesinin dayandığı konvansiyon. Bu
  pattern olmadan PlanTruthEngine çalışmaz.

v0.1'de `repos` tek elemanlı. Liste olması multi-repo'yu şema değiştirmeden açar.

---

## 3. Ledger — JSONL, append-only

Her satır bir olay. Dört tip var, fazlası yok.

Ortak alanlar: `ts` (UTC ISO8601), `type`, `exec`.

### 3.1 execution.started

`mudflow start <task>` preflight'ı geçtikten sonra yazar.

```json
{
  "ts": "2026-09-18T14:22:31Z",
  "type": "execution.started",
  "exec": "20260918T142231Z-SCMS-042",
  "task": "SCMS-042",
  "agent": "codex",
  "repo": "scms-core",
  "worktree": "~/worktrees/SCMS-042",
  "branch": "task/SCMS-042",
  "workspace_source": "created",
  "repo_dirty": false,
  "preserved_ref": null,
  "base": "origin/development",
  "base_sha": "a1b2c3d4",
  "head_sha": "a1b2c3d4",
  "plan_ref": "docs/plans/config-migration.md#t8"
}
```

`plan_ref` boş olabilir → `plan.execution_without_plan_link` finding'i doğar.

`repo_dirty`: start anında ana repo'nun kirli olup olmadığı (ADR-014).
Bloklamaz, kaydedilir ve `start` çıktısının `warnings` dizisinde döner.

`preserved_ref`: worktree'de commit edilmemiş iş varsa
`refs/mudflow/preserved/<exec>`; yoksa `null`. Ref, geçici index ile üretilen
snapshot commit'ini gösterir; working tree ve stash değiştirilmez.

`workspace_source`: `created` | `adopted`. `created` için `base_sha`, fetch
sonrası remote base'dir. `adopted` için Mudflow'un ölçtüğü
`merge-base(HEAD, base)` değeridir; dış aracın "hangi SHA'dan açtım" iddiası
ledger'a yazılmaz.

### 3.2 execution.finished

`mudflow finish` yazar.

```json
{
  "ts": "2026-09-18T16:04:02Z",
  "type": "execution.finished",
  "exec": "20260918T142231Z-SCMS-042",
  "outcome": "finished",
  "head_sha": "f9e8d7c6",
  "commits": ["b2c3d4e", "f9e8d7c"],
  "files_changed": 12,
  "insertions": 340,
  "deletions": 58,
  "files_ref": "evidence/3a7f91c2.json",
  "preserved_ref": null
}
```

`outcome`: `finished` | `interrupted` | `abandoned`

Satırı küçük tutmak için değişen dosya listesi `evidence/` altına ayrı yazılır.

### 3.3 evidence.recorded

```json
{
  "ts": "2026-09-18T15:40:11Z",
  "type": "evidence.recorded",
  "exec": "20260918T142231Z-SCMS-042",
  "task": "SCMS-042",
  "kind": "test",
  "ref": "evidence/8c1d0b44.json",
  "summary": "ctest: 148 passed, 0 failed"
}
```

`kind` TRUST_MODEL.md'deki hiyerarşinin tam karşılığı:

```text
commit        (en güçlü)
diff
test
files
command
agent_summary
manual_note   (en zayıf)
```

Güçlülük derecesi **saklanmaz**, `kind`'dan türetilir.
Tek yerde tanımlı sabit tablo; ledger'a yazılırsa iki kaynak doğar.

### 3.4 note

Karar ve açık maddeler.

```json
{
  "ts": "2026-09-18T15:12:00Z",
  "type": "note",
  "exec": "20260918T142231Z-SCMS-042",
  "kind": "unresolved",
  "text": "Config migration'da eski format okuma desteği kalacak mı?",
  "source": "agent",
  "ref": null
}
```

`kind`: `decision` | `unresolved` | `blocker`
`source`: `agent` | `human`

`ref` null ise → `context.unresolved_without_ref` finding'i (ADR-010).

---

## 4. Invariants

- **Ledger append-only'dir.** Hiçbir olay değiştirilmez veya silinmez; aksi
  halde geçmiş execution kanıtı sonradan yeniden yazılabilir.
- **Teşhis bilgisi kanıt değildir.** Repository raporundaki `error` ve
  `fetch_error` yalnız yönlendirme içindir, ledger'a yazılmaz; aksi halde
  geçici ağ/ortam hatası kalıcı execution gerçeği gibi görünür.
- **Finding'ler türetilir, saklanmaz.** Her `status`, ledger + Git + plan'dan
  yeniden hesaplar; aksi halde bayat finding gerçek durumla çelişir.
- **`base_sha` Mudflow'un kendi ölçümüdür.** Provider'ın söylediği SHA olduğu
  gibi kaydedilmez; aksi halde ADR-002'nin evidence > claim sınırı bozulur.
- **Evidence gücü `kind`'dan türetilir.** Ayrı bir güç alanı saklanmaz; aksi
  halde aynı evidence için iki çelişen otorite oluşur.
- **Handoff ölçülen gerçekleri agent iddialarından ayrı tutar.** Mudflow'un
  ürettiği bölüm ile `agent_summary` aynı başlıkta birleşmez; aksi halde iddia
  doğrulanmış veri gibi okunur.
- **Ledger'daki `base` opak bir gösterim alanıdır, ayrıştırılmaz.** Git ref'i
  her zaman güncel `project.json`'daki `remote` + `branch` alanlarından
  türetilir. Eski kayıtlar `"origin/main"` biçiminde string, yeniler aynı
  biçimde yazılmaya devam eder; ikisi de yalnızca mesajda ve handoff
  frontmatter'ında gösterilir. Aksi halde geçmiş ledger'ı ayrıştırmaya
  çalışan bir refactor, kaldırdığımız "ilk slash'tan kes" tahminini geri
  getirir ve eski kayıtları yanlış yorumlar.
- **Bloklayan ölçümü bozar, uyarı hijyeni bildirir.** ADR-014 uyarınca kanıtı
  güvenilmez yapan durumlar durur, yalnız çalışma hijyenini etkileyenler
  warning olur; aksi halde araç ya kanıtsız devam eder ya da gereksiz engeller.

---

## 5. Finding

TRUST_MODEL.md'deki modelin birebir karşılığı. **Diske yazılmaz** — her
`status` çağrısında ledger + git + plan'dan yeniden hesaplanır.
Kalıcı finding store'u olsaydı cache invalidation sorunu doğardı; bunun yerine
her seferinde baştan üretmek yeterince hızlı.

```json
{
  "id": "git.stale_worktree_base",
  "severity": "warning",
  "domain": "git",
  "title": "Worktree base 21 commit geride",
  "explanation": "task/SCMS-042 branch'i a1b2c3d'den açıldı, origin/development şu an 9f0e1d2.",
  "evidence": [
    {"kind": "command", "ref": "git merge-base task/SCMS-042 origin/development"}
  ],
  "suggested_action": "git -C ~/worktrees/SCMS-042 rebase origin/development"
}
```

`id` rastgele değil, `<domain>.<rule>` biçiminde sabit anahtar.
Bu sayede Phase 7'deki "configurable trust policies" bedava gelir —
kullanıcı kural id'siyle susturur.

`severity`: `info` | `warning` | `blocking`
`domain`: `git` | `plan` | `execution` | `context`

### v0.1 kural kataloğu

MVP.md §8'in birebir karşılığı. Dokuz kural, fazlası yok.

| id | severity |
|---|---|
| `git.remote_ahead` | warning |
| `git.stale_local_base` | warning |
| `git.dirty_workspace` | warning |
| `git.stale_worktree_base` | warning |
| `plan.done_without_evidence` | warning |
| `plan.execution_without_plan_link` | info |
| `plan.changed_during_execution` | warning |
| `context.no_handoff` | warning |
| `context.active_execution` | info |
| `context.orphaned_execution` | warning |
| `context.invalid_ledger_timestamp` | warning |
| `context.unresolved_without_ref` | info |

`blocking` v0.1'de kullanılmıyor. Severity alanı yine de üç değerli —
ilk blocking kural geldiğinde şema değişmesin.

---

## 6. Handoff

`mudflow finish` üretir. Okuyucusu bir sonraki **agent**, o yüzden format
markdown; makine alanları frontmatter'da.

`.mudflow/handoffs/20260918T142231Z-SCMS-042.md`

```markdown
---
exec: 20260918T142231Z-SCMS-042
task: SCMS-042
agent: codex
outcome: interrupted
repo: scms-core
worktree: ~/worktrees/SCMS-042
branch: task/SCMS-042
base: origin/development@a1b2c3d4
range: a1b2c3d4..f9e8d7c6
---

## Doğrulanmış (Mudflow üretti)

Commits:
- b2c3d4e  config: introduce ConfigV2 reader
- f9e8d7c  config: migrate settings loader

Değişen dosyalar: 12 (+340 / -58)
- src/config/config_v2.cpp
- src/config/loader.cpp
- ...

Test: ctest 148 passed, 0 failed  (15:40)

Preserved uncommitted snapshot: refs/mudflow/preserved/20260918T142231Z-SCMS-042

## Agent notu (zayıf evidence — doğrulanmadı)

Eski format okuma yolu loader.cpp'de duruyor, henüz silinmedi.

## Açık kalanlar

- [ ] Eski format desteği kalacak mı?  (ref yok)
- [ ] Migration testi eksik  (ref: docs/plans/config-migration.md#t9)
```

Üç başlık tesadüf değil, ADR-002'yi formatın içine gömer:
Mudflow'un git'ten ölçtüğü ile agent'ın iddia ettiği aynı bölümde durmaz.

---

## 7. Handoff policy — Handoff'u kim üretir

**Melez.**

- Mudflow mekanik bölümü üretir: SHA aralığı, commit'ler, değişen dosyalar,
  kaydedilmiş test sonuçları. Ölçüm, iddia değil.
- Agent'ın özeti ayrı başlıkta, `agent_summary` kind'ıyla ve
  "doğrulanmadı" etiketiyle durur.
- Açık maddeler `note` olaylarından gelir; `ref`'i olmayan işaretlenir.

Böylece ADR-002 (evidence > agent claim) ile ADR-010 (durable source referansı)
birlikte sağlanır ve handoff hiçbir zaman saf agent çıktısı olmaz.

---

## 8. v0.1'de bilerek yok

| Yok | Ne zaman eklenir |
|---|---|
| `cache/`, `index/` | `status` 200 ms'yi geçince |
| Finding persistence | Geçmiş finding trendi istenirse |
| Multi-repo çapraz execution | `repos` ikinci elemanı geldiğinde |
| Provider interface | İkinci plan provider yazılırken |
| Test runner entegrasyonu | Elle `evidence.recorded` yazmak yetmeyince |
| Ledger şema versiyonu | İlk breaking change'de (`project.json` `version` var) |
