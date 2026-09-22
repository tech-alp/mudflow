# Runmark Provider Model

Bu doküman provider hedeflerini tanımlar; bütün adaptörlerin veya registry'nin
uygulanmış olduğunu göstermez. Güncel sıra [ROADMAP](ROADMAP.md), katman
sınırları [ARCHITECTURE](ARCHITECTURE.md) içindedir.

## Amaç

External araçları entegre et, ama core kavramlarını değiştirmelerine izin verme.

Runmark şunları sahiplenir:

```text
Project
Workspace
Plan
Task
Execution
Evidence
Finding
```

## IPlanProvider
Örnek:
- Superpowers
- planning-with-files
- Spec Kit
- OpenSpec

## ITaskProvider
Örnek:
- MudIssue
- tk
- Jira
- GitHub Issues

## IAgentProvider
Örnek:
- Claude Code
- Codex
- OpenCode
- Gemini CLI

## IEvidenceProvider
Örnek:
- Git
- CTest
- pytest
- npm test
- custom script

## Capability modeli

Her provider her şeyi desteklemek zorunda değil.

Örnek:

```yaml
provider: mudissue
capabilities:
  list: true
  read: true
  create: true
  update: true
```

## İlk provider hedefleri

Ürünle birlikte geliştirilmesi hedeflenen provider'lar:

Plan:
- Superpowers
- planning-with-files

Task:
- MudIssue

Agent:
- Claude Code
- Codex

Evidence:
- Git
- manual test evidence

## Plugin sınırları ve evrimi

- Protected system pluginler Runmark'ın execution/evidence/trust servislerini
  sunar; üçüncü parti alternatifleriyle değiştirme sözleşmesi yoktur.
- Feature pluginler bu servislerin görünümüdür; ekranı kapatmak veriyi silmez.
- Integration pluginler harici plan/task/agent/evidence kaynaklarını normalize
  eder. Jira task durumu tek başına Runmark'ın doğrulanmış tamamlanma kararı değildir.
- Önce built-in adaptörler ve minimum runtime, ardından sürümlü provider
  registry, sonrasında gerçek process adaptörü ve public SDK geliştirilir.
- Dış adaptör sözleşmesi JSON-RPC 2.0 over stdio; dahili C++ named modules
  plugin ABI'si değildir. Public C++ shared-library yükleme modeli açılmaz.
- Başlangıçta açıkça güvenilen pluginler çalıştırılır. Process ayrımı crash
  izolasyonudur; OS sandbox olmadan güvenilmeyen kodu sınırlama garantisi vermez.

Runtime adaptörleri `plugins/integrations/`; dış agent uygulamalarına kurulan
skill/hook paketleri `integrations/agent-clients/runmark/` altında hedeflenir.
Mevcut `plugins/mudflow-agent/` henüz taşınmadı.
