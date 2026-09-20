# Mudflow Decisions

## ADR-001 — Planning framework değil
Accepted.

Mudflow Superpowers, planning-with-files vb. araçları entegre eder.

## ADR-002 — Evidence > agent claim
Accepted.

Commit, diff ve test; agent summary’den daha güçlü evidence’tır.

## ADR-003 — Plugin-first, not everything-is-a-plugin
Accepted.

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

Mudflow Mud’a bağımlı olmamalı.

## ADR-009 — Agent runtime external kalır
Accepted.

Claude/Codex auth Mudflow’a taşınmaz.

## ADR-010 — Handoff durable source referanslamalı
Accepted.

## ADR-011 — Qt/QML desktop
Preferred.

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

Mudflow workspace yaratabilir; MudIssue, IDE veya elle oluşturulmuş geçerli
worktree'yi de kullanabilir. Workspace'i kimin yarattığı dış mekanizmadır.

Mudflow her durumda kendi ölçümünü kaydeder:

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

Worktree kirliliği artık bloklamaz. Mudflow, working tree veya stash'e
dokunmadan geçici index ile snapshot commit'i üretir ve bunu
`refs/mudflow/preserved/<exec-id>` altında saklar. Bu yakalama başarısızsa
`start`/`finish` başarısız olur; aksi halde kanıtsız iş üretirdi.

İlke: **ölçümü bozan şey bloklar, hijyen tercihi uyarır.**

---

## ADR-015 — Hook'un çalıştığı ölçülür, varsayılmaz
Accepted.

Kurulu sanılan ama hiç çalışmayan bir SessionStart hook'u, temiz bir projeden
ayırt edilemez: ikisinde de `status` sessizdir. Bu, projenin avladığı sessiz
körlük sınıfının aynısıdır.

`resume --hook` çağrıldığında `.mudflow/hook-observed.json` yazılır. Üç sonuç:

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

