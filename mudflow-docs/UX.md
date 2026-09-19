# Mudflow Desktop UX

## UX hedefi

Mudflow açıldığında birkaç saniyede şu sorular cevaplanmalı:

1. Ne yapıyoruz?
2. Ne gerçekten bitti?
3. Ne stale/unsafe?
4. Aktif çalışma nerede?
5. Başka agent devam edebilir mi?

## Navigation

```text
Overview
Plan
Tasks
Worktrees
Executions
Evidence
Findings
```

## Ana ekran

### En büyük alan: Plan Progress

```text
Config Migration Plan

8 / 13 verified
2 in progress
1 blocked
2 unverified
```

### Sağ panel: Active Execution

```text
SCMS-042
Agent: Codex
Worktree: .worktrees/SCMS-042
Branch: task/SCMS-042
```

### Git

```text
✓ remote fetched
⚠ 4 commits behind latest base
```

### Findings

```text
⚠ T8 has no test evidence
⚠ architecture.md may be stale
```

## Plan ekranı

Sol:
- plan outline

Sağ:
- status
- verification
- executions
- evidence
- related repos

## Worktrees ekranı

Her worktree:
- task
- repo
- branch
- base
- divergence
- dirty state
- agent

## Executions ekranı

Chronological ledger:

```text
14:22 Codex       SCMS-042 finished
13:01 Claude Code SCMS-042 interrupted
```

## Start Task akışı

```text
Task
→ repo(s)
→ agent
→ preflight
→ create verified worktree
→ start
```

## Görsel yön

- developer control surface
- dense but calm
- limited color
- actionable warnings
- no AI sparkle
- dark/light first-class
