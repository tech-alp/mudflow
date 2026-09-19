# Mudflow Roadmap

## Phase 0 — Workflow validation
Amaç: modeli gerçek bir SCMS/TVM benzeri akışta doğrulamak.

- project manifest
- normalized task/plan modeli
- execution ledger şeması
- evidence modeli
- Git health checks
- multi-repo örneği

## Phase 1 — Core CLI
- `mudflow inspect`
- `mudflow project status`
- `mudflow git health`
- `mudflow start <task>`
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
- `mudflow resume`

## Phase 3 — Qt/QML Desktop MVP
Ekranlar:
- Overview
- Plan
- Tasks
- Worktrees
- Executions
- Findings

## Phase 4 — Provider Registry
- IPlanProvider
- ITaskProvider
- IAgentProvider
- IEvidenceProvider

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
- process/JSON-RPC

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
