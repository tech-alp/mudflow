# Mudflow Architecture

## Temel ilke

Mudflow **plugin-first**, ama **everything-is-a-plugin** değildir.

Core’da kalacaklar:

```text
ProjectGraph
ExecutionLedger
PlanTruthEngine
GitTruthEngine
EvidenceStore
TrustEngine
```

## High-level architecture

```text
Qt/QML Desktop
      ↓
Application Services
      ↓
Mudflow Core
      ↓
Provider Registry
 ┌────┼────┬─────┐
Plan Task Agent Evidence
```

## Domain modeli

### Project
Mantıksal ürün/workspace.

### Repository
Canonical repo bilgisi.

### Workspace
Aktif execution alanı/worktree.

Önemli:

```text
Project Anchor != Workspace Root
```

Örnek:

```text
Project:
~/projects/scms

Workspace:
~/worktrees/SCMS-42
```

### Plan
Normalized planning artifact.

### Task
Execution’a bağlanan iş birimi.

### Execution
Append-oriented çalışma kaydı.

### Evidence
Commit, diff, test, changed-file, document, decision, agent-run vb.

## Core engine’ler

### ProjectGraph
Multi-repo ilişkilerini tutar.

### GitTruthEngine
- fetch
- ahead/behind
- merge-base
- stale base
- dirty state
- worktree ancestry

### PlanTruthEngine
Plan task ↔ evidence eşleşmesini değerlendirir.

### ExecutionLedger
Agent çalışmalarını kalıcı kayda bağlar.

### EvidenceStore
Kanıt referanslarını indeksler.

### TrustEngine
Explainable finding üretir.

## Persistence

Önerilen:

```text
.mudflow/
├ project.yaml
├ ledger/
├ handoffs/
├ cache/
└ index/
```

Git, plan, spec ve task kaynakları source-of-truth olarak kalır.

## Agent start flow

```text
Resolve project
→ resolve task
→ git fetch
→ Git Truth
→ Plan Truth
→ previous execution/handoff
→ minimal context bundle
→ launch agent
```

## Güvenlik

Otomatik yapılmamalı:
- force push
- dirty rebase
- branch deletion
- merge
- task done marking purely from AI claim
