---
name: mudflow
description: Resume a known Mudflow task, record observed evidence, and preserve decisions or unresolved work through the Mudflow CLI.
---

Use only the `mudflow` CLI for project state. Never read or write its state
files directly. Run from the configured project directory, or pass
`-p "$PROJECT_CONFIG"` using a project config path supplied by the user.

Check `mudflow --version`; minimum supported version is in `compatibility.json`.
If the CLI is missing, continue the agent session without Mudflow. If its version
is incompatible, explain the required minimum and skip Mudflow commands.

For a known task or execution, run `mudflow resume "$TASK_OR_EXEC" --markdown`
before continuing work. For recording, use the exact execution ID returned by
Mudflow (`resume` JSON exposes `exec`); never substitute a task ID or session ID.
If no task is known, ask for it. The current CLI cannot select an active task
globally. The SessionStart pilot therefore exits silently without injecting
context; absence of context does not mean there is no previous execution.

After observing a test, command result, commit or diff, record the actual result:

```sh
mudflow evidence "$EXEC" --kind test --summary "<command, result, failures>" --ref "<durable log path>"
```

Use `commit`, `diff`, `test`, `files` or `command` only for observed evidence.
Preserve failed results too. Include a durable `--ref` when available; omit the
option when no source exists. Never invent a measurement or source reference.

Before handing work over, record your own summary separately:

```sh
mudflow evidence "$EXEC" --kind agent_summary --summary "<agent account of completed and remaining work>"
```

ADR-002: an agent summary is a claim, not a measurement. Always use
`agent_summary` for your own synthesis, even if it mentions tests; link the real
test evidence separately. CLI success confirms recording, not claim validity.

Record a decision when made, an unresolved question when left open, or a blocker
when progress stops. Use the corresponding `decision`, `unresolved` or `blocker`
kind, with concrete text and a durable `--ref` when available:

```sh
mudflow note "$EXEC" --kind unresolved --text "<open question and next action>"
```

These are deliberate agent CLI calls. The SessionStart hook never records
evidence, notes or hook events. Do not create or finish an execution merely
because a session starts. Report CLI errors; do not claim a failed write succeeded.
