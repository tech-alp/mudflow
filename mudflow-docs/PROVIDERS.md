# Mudflow Provider Model

## Amaç

External araçları entegre et, ama core kavramlarını değiştirmelerine izin verme.

Mudflow şunları sahiplenir:

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

## v0.1

Built-in provider’lar:

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

## Plugin evrimi

v0.1:
- compiled providers

v0.2:
- provider registry

v0.3+:
- external process provider
- JSON-RPC / process protocol
- manifest

İlk sürümde C++ ABI plugin sistemi kurulmamalı.
