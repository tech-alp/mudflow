# Runmark Roadmap

Bu doküman hedef kapsam ve sırayı gösterir; maddelerin tamamlandığı anlamına
gelmez. Komut adı `rmk`'dır ve build onu üretir.
İsim ve mimari kararları: [ADR-016–019](DECISIONS.md).

## Phase 0 — Workflow validation
Amaç: modeli gerçek bir SCMS/TVM benzeri akışta doğrulamak.

- project manifest
- normalized task/plan modeli
- execution ledger şeması
- evidence modeli
- Git health checks
- multi-repo örneği

## Phase 1 — Core CLI
- `rmk inspect`
- `rmk status`
- Git health findings
- `rmk start <task>`
- execution begin/finish
- Superpowers provider
- MudIssue provider
- Claude/Codex launch
- Git evidence

## Phase 2 — Planning continuity
- planning-with-files provider
- plan task normalization
- plan ↔ evidence mapping
- stale/unverified task detection
- handoff artifact
- `rmk resume`

## Desktop öncesi — Adlandırma ve mimari geçişi

1. ~~`rmk` executable, `runmark` namespace ve paket/hook adlarını birlikte taşı.~~
   Yapıldı (2026-09-22). PoC olduğu için eski ad, dizin ve ref'ler korunmadı;
   migration kodu yazılmadı, kurulu eski binary ve pluginler söküldü.
2. ~~`apps/cli` ile `libs/domain`, `libs/application`, `libs/infrastructure`
   sınırlarını kur.~~ Yapıldı (RM-1, 2026-09-22).
3. ~~Application katmanı JSON yerine tipli sonuç döndürsün.~~ Yapıldı
   (RM-4, 2026-09-22). `Finding` domain struct'ı oldu, serileştirme
   `apps/cli/src/json.cpp`'ye indi, `domain_purity` testi sınırı tutuyor.
4. ~~Küçük bir named module + QObject/QML köprüsünü macOS'ta
   clean/incremental build ile doğrula.~~ Yapıldı (RM-2, 2026-09-22).
   Ölçülen toolchain: LLVM Clang 23.1.1 / CMake 4.4.3 / Ninja 1.13.2 /
   Qt 6.11.1, C++23. Linux ilerleyen süreçte, Windows en son eklenir;
   mevcut kabul kapsamı macOS'tur.
5. Domain'den başlayarak modules geçişini yap. Henüz kullanılmayan host/SDK
   dizinlerini veya genel amaçlı framework soyutlamalarını oluşturma.

## Phase 3 — Qt/QML Desktop MVP

- Merce entegrasyonu, sürüm sabitleme ve kompakt `desktop` profili
- `apps/desktop`, `libs/ui-shell`; CLI'nin GUI bağımlılığı olmaması
- Minimum plugin runtime/API ve QML katkı host'u
- İlk feature plugin: Findings; activation failure ve cleanup doğrulaması
- Korumalı execution servislerine UI proxy'leri; doğrudan ledger yazımı yok
- Navigation, route/tab, inspector, activity ve temel rescue görünümü
- Gerçek verili Worktrees ekranında büyük liste, klavye/focus, tema ve DPI doğrulaması

Ekranlar:
- Overview
- Plan
- Tasks
- Worktrees
- Executions
- Findings

## Phase 4 — Provider Registry

Phase 3'teki minimum runtime üzerine provider capability sözleşmeleri eklenir:

- IPlanProvider
- ITaskProvider
- IAgentProvider
- IEvidenceProvider
- Sürümlü capability, missing service ve dependency cycle kontrolleri
- Provider kaybında consumer durdurma ve devam eden çağrıları sonuçlandırma
- Scope bazlı resource sahipliği ve ters sırada cleanup

## Phase 5 — Additional providers
Plan:
- OpenSpec
- Spec Kit

Task:
- tk
- GitHub Issues
- Jira

Agent:
- OpenCode
- Gemini CLI

Evidence:
- CTest
- pytest
- npm/pnpm test

## Phase 6 — External Plugin SDK
Provider contracts stabilize olduktan sonra:
- manifest
- discovery
- versioning
- `libs/process-host` ile JSON-RPC 2.0 over stdio
- Gerçek child process ile handshake, timeout, crash/restart testleri
- Versioned manifest ve IPC şemaları; eski sürüm fixture'larıyla compatibility
- Açıkça güvenilen paketler; workspace pluginleri için otomatik çalıştırma yok
- Public SDK, ilk gerçek process adaptörü doğrulandıktan sonra sabitlenir

## Phase 7 — Advanced Trust Engine
- doc freshness
- architecture drift
- plan coverage
- cross-repo execution graph
- configurable trust policies

## Deferred
- cloud sync
- team accounts
- marketplace
- autonomous merge
- workflow designer
- generalized multi-agent orchestration
- third-party execution/evidence/trust service replacement
- public native plugin ABI ve native hot-unload
- OS sandbox olmadan untrusted plugin çalıştırma
