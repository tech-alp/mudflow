# Mudflow PRD

## 1. Ürün tanımı

Mudflow, AI ile geliştirilen projelerde plan, kod, Git state ve agent context’in birbirinden kopmasını engelleyen local-first execution control plane’dir.

Mudflow yeni bir planning framework dayatmaz. Mevcut artifact’ları okur, execution state’i modeller ve kanıtlarla doğrular.

## 2. Ana problemler

### Plan drift
Plan görevi tamamlandı görünebilir ama:
- kod gerçekten yazıldı mı,
- test edildi mi,
- commit var mı,
- iş yarım mı kaldı,
- plan artık eski mi

bilinmeyebilir.

### Git drift
Local `main` veya `development` geride kalabilir. Yeni branch eski base’den açılabilir.

### Agent handoff belirsizliği
Claude Code’dan Codex’e geçince:
- ne yapıldı,
- hangi kararlar alındı,
- hangi testler geçti,
- ne kaldı

tam güvenle bilinmeyebilir.

### Multi-repo context kaybı
SCMS/TVM gibi sistemlerde tek feature birden fazla repo ve servise dokunabilir.

## 3. Hedef kullanıcı

- Birden fazla AI coding agent kullanan geliştiriciler
- Git worktree/branch akışı kullananlar
- Spec/plan tabanlı çalışanlar
- Multi-repo projelerde çalışanlar
- Local-first tooling isteyenler

## 4. Jobs to be done

### JTBD-1
Projeye döndüğümde neyin gerçekten tamamlandığını bilmek.

### JTBD-2
Yeni task başlatmadan önce base branch’in güncel olduğunu doğrulamak.

### JTBD-3
Claude → Codex gibi agent geçişlerinde context’i güvenilir aktarmak.

### JTBD-4
Plan task’larının gerçekten kanıtlanmış olup olmadığını görmek.

### JTBD-5
Multi-repo bir sistemi tek proje olarak anlamak.

## 5. Ürün sütunları

### Plan Truth
Plan ile gerçek execution evidence eşleşiyor mu?

### Git Truth
Branch/worktree doğru ve güncel base’den mi geliyor?

### Execution Ledger
Task + agent + worktree + commits + diff + tests + decisions kaydı.

### Context Continuity
Yeni agent durable kaynaklardan kompakt context alır.

## 6. Project Trust

Opaque bir yüzde skoru yerine explainable findings gösterilir.

Örnek:

```text
PLAN
⚠ T8 done görünüyor ama test evidence yok.

GIT
⚠ Worktree origin/development’tan 21 commit geride.

DOCS
⚠ architecture.md son üç mimari commit’ten daha eski.

CONTEXT
✓ Handoff var.
⚠ Bir unresolved decision kalıcı artifact’a bağlı değil.
```

## 7. Ana akış

```text
Project aç
→ repo/plan/task discovery
→ git fetch
→ truth checks
→ active task
→ verified worktree
→ agent launch
→ execution ledger
→ evidence
→ review
```

## 8. Non-goals

Mudflow ilk aşamada:
- Jira replacement değildir
- cloud PM tool değildir
- chat ürünü değildir
- AI coding agent değildir
- CI platformu değildir
- workflow DSL değildir

## 9. Başarı ölçütü

Aşağıdaki sorular için yeniden analiz ihtiyacını ciddi azaltmalıdır:

- “Nerede kalmıştık?”
- “Plan güncel mi?”
- “Bu task gerçekten bitti mi?”
- “Claude ne yaptı?”
- “Codex güvenle devam edebilir mi?”
- “Bu branch latest development’tan mı açıldı?”
