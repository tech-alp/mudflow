# Runmark Desktop UX

Hedef desktop tasarımıdır; shell ve plugin runtime henüz uygulanmadı.

2026-09-22: Yeni etkileşimli prototip için
[Runmark tasarım sözleşmesi](superpowers/specs/2026-09-22-desktop-prototype-design.md)
kullanıcı incelemesindedir. Aşağıdaki eski navigasyon ve örnekler tarihsel
taslaktır; yeni prototipe doğrudan taşınmaz. Onaylanan kapsam tam demo akışı,
native PoC ise salt-okunur Overview + Findings dilimidir.

## Tasarım sistemi ve shell

Merce kullanılır. Runmark'a özel kompakt `desktop` profili; semantic renk,
tipografi, spacing ve kontrol boyutlarını sağlar. Kiosk profilleri korunur.

Shell; workspace seçimi, ana navigasyon, route/tab host, context inspector,
activity alanı ve temel rescue görünümünü sahiplenir. Feature pluginler bu
bölgelere sayfa, panel ve komut katkısı verir. Theme yönetimi shell'dedir.
Execution/evidence/trust servislerine izinli proxy üzerinden erişilir.

Route örneği: `runmark://com.runmark.findings/list`. Plugin durdurulduğunda
ilgili görünüm kontrollü kapanır veya fallback'e geçer; kalıcı kayıt silinmez.

Klavye erişimi, plugin sınırında focus devamlılığı, erişilebilir ad/rol ve
renk dışı durum göstergeleri zorunludur. İlk doğrulama gerçek verili Worktrees
ekranında büyük liste, açık/koyu tema ve farklı DPI değerlerini kapsar.
CLI komutu `rmk`'dır; UI gezinmesi ve görsel durum CLI katmanına taşınmaz.

## UX hedefi

Runmark açıldığında birkaç saniyede şu sorular cevaplanmalı:

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
