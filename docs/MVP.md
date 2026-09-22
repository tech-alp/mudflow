# Runmark MVP

Ürün adı Runmark, hedef CLI `rmk`'dır. Aşağıdaki maddeler kapsam hedefidir;
tamamlanmış özellik listesi değildir. Build `rmk` üretir.
Adlandırma/modules geçişi ve Merce desktop sırası [ROADMAP](ROADMAP.md)'tedir.

## Ana amaç

> **“Devam etmek güvenli mi?” sorusunu cevaplamak.**

## v0.1

### 1. Multi-repo project
Bir manifest ile birden fazla repo tek proje olarak modellenir.

### 2. Git Health
Her repo için:
- branch
- SHA
- remote SHA
- ahead/behind
- dirty state
- worktree
- base
- merge-base

Yeni worktree default olarak verified remote base’den açılır.

### 3. Superpowers provider
Okunacak yollar:
```text
docs/superpowers/specs/
docs/superpowers/plans/
```

### 4. planning-with-files provider
```text
task_plan.md
findings.md
progress.md
```

### 5. MudIssue provider
- list
- read
- update state

### 6. Agent launch
- Claude Code
- Codex

### 7. Execution Ledger
- task
- agent
- repo/worktree
- start/end
- start/end SHA
- changed files
- commits
- tests
- unresolved notes

### 8. MVP Findings

Git:
- remote ahead
- stale local base
- dirty workspace
- stale worktree base

Plan:
- done ama evidence yok
- execution var ama plan link yok
- plan execution sırasında değişmiş

Context:
- previous run var ama handoff yok
- unresolved item durable artifact’a bağlı değil

## Out of scope
- Jira
- GitHub Issues
- cloud
- team accounts
- arbitrary external plugins
- autonomous merge
- complex Kanban
