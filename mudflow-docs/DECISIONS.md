# Mudflow Decisions

## ADR-001 — Planning framework değil
Accepted.

Mudflow Superpowers, planning-with-files vb. araçları entegre eder.

## ADR-002 — Evidence > agent claim
Accepted.

Commit, diff ve test; agent summary’den daha güçlü evidence’tır.

## ADR-003 — Plugin-first, not everything-is-a-plugin
Accepted.

Core:
- ProjectGraph
- GitTruthEngine
- PlanTruthEngine
- ExecutionLedger
- EvidenceStore
- TrustEngine

## ADR-004 — v0.1 external plugin ABI yok
Accepted.

Önce provider contracts stabilize edilmeli.

## ADR-005 — Local-first
Accepted.

Cloud backend gerekmiyor.

## ADR-006 — Project anchor ve workspace root ayrıdır
Accepted.

## ADR-007 — Yeni iş verified remote base’den başlar
Accepted.

Default:

```text
git fetch
→ resolve remote base
→ health check
→ create worktree
```

## ADR-008 — MudIssue bir provider’dır
Accepted.

Mudflow Mud’a bağımlı olmamalı.

## ADR-009 — Agent runtime external kalır
Accepted.

Claude/Codex auth Mudflow’a taşınmaz.

## ADR-010 — Handoff durable source referanslamalı
Accepted.

## ADR-011 — Qt/QML desktop
Preferred.

```text
C++20
Qt 6.11+
QML
CMake
```

## ADR-012 — Trust explainable olmalı
Accepted.

Her warning somut evidence göstermeli.
