# Runmark

> **Runmark, AI ile geliştirilen projelerde planın, kodun, Git state’inin ve agent context’inin birbirinden kopmasını engelleyen local-first execution control plane’dir.**

Ürünün önceki adı Runmark'dur. Hedef CLI `rmk`, workspace veri dizini
`.runmark/` olacaktır. Kod geçişi henüz yapılmadığından çalışan komut ve veri
örnekleri [README](../README.md) ve [DATA_MODEL.md](DATA_MODEL.md) içinde
eski adlarla korunur. Hedef mimari, uygulanmış özellik listesi değildir.

## Temel fikir

Runmark dört farklı “truth” katmanını bir araya getirir:

1. **Plan Truth** — plan ne diyor?
2. **Git Truth** — branch/worktree/remote gerçekte ne durumda?
3. **Execution Truth** — gerçekten ne değişti, ne test edildi?
4. **Context Truth** — sonraki agent neyi gerçekten biliyor?

Runmark; Superpowers, planning-with-files, MudIssue, Git, Claude Code ve Codex gibi mevcut araçları değiştirmez. Bunları bağlar ve execution state’i doğrular.

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

- Mevcut kod C++20; hedef C++ named modules, araç zinciri teknik doğrulamaya bağlı
- Qt 6.11+
- QML + Merce tasarım sistemi; kompakt desktop profili
- CMake target'larıyla domain/application/infrastructure ve runtime ayrımı
- Değiştirilebilir UI ve entegrasyon pluginleri; korumalı execution/evidence/trust servisleri
- local-first
- CLI/process adapter’ları
- kaynak sistemleri source-of-truth olarak koruma

## Dokümanlar

- [PRD](PRD.md)
- [Roadmap](ROADMAP.md)
- [Architecture ve hedef dizin yapısı](ARCHITECTURE.md)
- [Mevcut veri modeli](DATA_MODEL.md)
- [Teknoloji kararları](TECH_CHOICES.md)
- [MVP](MVP.md)
- [Provider modeli](PROVIDERS.md)
- [Trust modeli](TRUST_MODEL.md)
- [Desktop UX](UX.md)
- [Mimari kararlar](DECISIONS.md)
