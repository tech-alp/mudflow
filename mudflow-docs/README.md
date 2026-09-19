# Mudflow

> **Mudflow, AI ile geliştirilen projelerde planın, kodun, Git state’inin ve agent context’inin birbirinden kopmasını engelleyen local-first execution control plane’dir.**

## Temel fikir

Mudflow dört farklı “truth” katmanını bir araya getirir:

1. **Plan Truth** — plan ne diyor?
2. **Git Truth** — branch/worktree/remote gerçekte ne durumda?
3. **Execution Truth** — gerçekten ne değişti, ne test edildi?
4. **Context Truth** — sonraki agent neyi gerçekten biliyor?

Mudflow; Superpowers, planning-with-files, MudIssue, Git, Claude Code ve Codex gibi mevcut araçları değiştirmez. Bunları bağlar ve execution state’i doğrular.

## İlk hedef

İlk sürümün cevaplaması gereken soru:

> **“Şu an bu projede devam etmek güvenli mi?”**

## İlk entegrasyonlar

- Superpowers
- planning-with-files
- MudIssue
- Git
- Claude Code
- Codex

## Teknoloji yönü

- C++20
- Qt 6.11+
- QML
- CMake
- local-first
- CLI/process adapter’ları
- kaynak sistemleri source-of-truth olarak koruma

## Dokümanlar

- PRD.md
- ROADMAP.md
- ARCHITECTURE.md
- DATA_MODEL.md
- TECH_CHOICES.md
- MVP.md
- PROVIDERS.md
- TRUST_MODEL.md
- UX.md
- DECISIONS.md
