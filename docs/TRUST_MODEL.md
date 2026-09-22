# Runmark Trust Model

## İlke

Runmark opaque “83% healthy” skoru vermek yerine explainable findings üretir.

Bu kurallar korumalı ürün servislerine aittir. Feature pluginler findings'i
gösterir; entegrasyonlar ölçüm/evidence sağlar. Bir plugin'in veya agent'ın
tamamlandı iddiası, bu değerlendirme kurallarının yerine geçmez.

## Truth domains

### Plan Truth
Plan state gerçek evidence ile destekleniyor mu?

State:
```text
verified
partially_verified
unverified
stale
blocked
unknown
```

### Git Truth
Workspace beklenen base ve remote ile uyumlu mu?

Kontroller:
- fetch
- base resolve
- ahead/behind
- merge-base
- dirty state
- detached HEAD
- stale worktree base

### Execution Truth
Gerçekte ne oldu?

Evidence önceliği:
1. commit
2. diff
3. test/build result
4. changed files
5. command result
6. agent summary
7. manual note

### Context Truth
Yeni agent güvenle devam edebilir mi?

- active task known
- plan section linked
- worktree known
- previous execution known
- unresolved items known
- latest diff available
- handoff exists

## Finding modeli

```text
id
severity
domain
title
explanation
evidence[]
suggested_action
```

Severity:
```text
info
warning
blocking
```

## Safe-to-start

Örnek:

```text
READY TO START

✓ remote fetched
✓ origin/development resolved
✓ worktree clean
✓ plan task found

Warnings:
! architecture.md may be stale
```
