---
name: runmark
description: Resume a known Runmark task, record observed evidence, and preserve decisions or unresolved work through the Runmark CLI.
---

Use only the `rmk` CLI for project state. Never read or write its state
files directly. Run from the configured project directory, or pass
`-p "$PROJECT_CONFIG"` using a project config path supplied by the user.

Check `rmk --version`; minimum supported version is in `compatibility.json`.
If the CLI is missing, continue the agent session without Runmark. If its version
is incompatible, explain the required minimum and skip Runmark commands.

Run `rmk resume "$TASK_OR_EXEC" --markdown` for a known task or execution,
or `rmk resume --markdown` with no selector for the most recent execution in
the project. For recording, use the exact execution ID returned by Runmark
(`resume` JSON exposes `exec`); never substitute a task ID or session ID.

The SessionStart hook runs the selector-less form, so a session usually opens
with the latest execution already in context. Absence of that context does not
mean there is no previous execution: the hook stays silent whenever the CLI is
missing, incompatible or fails. Check `resume` yourself before assuming a clean
slate. Read the `gaps` array: it reports what could not be established.

After observing a test, command result, commit or diff, record the actual result:

```sh
rmk evidence "$EXEC" --kind test --summary "<command, result, failures>" --ref "<durable log path>"
```

Use `commit`, `diff`, `test`, `files` or `command` only for observed evidence.
Preserve failed results too. Include a durable `--ref` when available; omit the
option when no source exists. Never invent a measurement or source reference.

Before handing work over, record your own summary separately:

```sh
rmk evidence "$EXEC" --kind agent_summary --summary "<agent account of completed and remaining work>"
```

ADR-002: an agent summary is a claim, not a measurement. Always use
`agent_summary` for your own synthesis, even if it mentions tests; link the real
test evidence separately. CLI success confirms recording, not claim validity.

Record a decision when made, an unresolved question when left open, or a blocker
when progress stops. Use the corresponding `decision`, `unresolved` or `blocker`
kind, with concrete text and a durable `--ref` when available:

```sh
rmk note "$EXEC" --kind unresolved --text "<open question and next action>"
```

These are deliberate agent CLI calls. The SessionStart hook never records
evidence, notes or hook events. Do not create or finish an execution merely
because a session starts. Report CLI errors; do not claim a failed write succeeded.
