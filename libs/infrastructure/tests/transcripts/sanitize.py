#!/usr/bin/env python3
"""Turn a real agent transcript into a committable test fixture.

    sanitize.py claude|codex <real transcript> <fixture out>

Allowlist, never blocklist: a transcript also carries the user's global
instructions, memory, plugin context and account data, so only the line
shapes the parser reads (and a few it must skip) survive. Every absolute
path becomes /fixture. Regenerate when a runtime changes its format
(ADR-022), naming the directory after the runtime version.
"""
import json
import re
import sys

PATH = re.compile(r"(/private)?(/tmp|/var/folders|/Users|/home)/[^\s\"'`]*")


def claude(line):
    kind = line.get("type")
    content = (line.get("message") or {}).get("content")
    if kind == "user" and isinstance(content, str):
        return line  # the prompt
    if kind not in ("user", "assistant") or not isinstance(content, list):
        return None
    blocks = []
    for block in content:
        if block.get("type") in ("tool_use", "tool_result", "text"):
            blocks.append(block)
        elif block.get("type") == "thinking":  # keep the shape, not the text
            blocks.append({"type": "thinking", "thinking": "", "signature": ""})
    if not blocks:
        return None
    line["message"]["content"] = blocks
    return line


CODEX_EVENTS = {"task_started", "task_complete", "item_completed"}
CODEX_ITEMS = {"CommandExecution", "AgentMessage", "UserMessage"}
CODEX_RESPONSES = {"custom_tool_call", "custom_tool_call_output", "function_call", "function_call_output"}


def codex(line):
    kind, payload = line.get("type"), line.get("payload") or {}
    if kind == "session_meta":
        keep = ("id", "session_id", "timestamp", "originator", "cli_version")
        line["payload"] = {k: payload[k] for k in keep if k in payload}
        return line
    if kind == "event_msg" and payload.get("type") in CODEX_EVENTS:
        item = payload.get("item")
        if item is not None and item.get("type") not in CODEX_ITEMS:
            return None
        return line
    if kind == "response_item" and payload.get("type") in CODEX_RESPONSES:
        return line
    return None


def main(runtime, source, target):
    keep = claude if runtime == "claude" else codex
    out = []
    for raw in open(source, encoding="utf-8"):
        line = keep(json.loads(raw))
        if line is not None:
            text = PATH.sub("/fixture", json.dumps(line, ensure_ascii=False))
            out.append(text)
    body = "\n".join(out) + "\n"
    for leak in ("techalp", "claude-501", "CLAUDE.md", "AGENTS.md", "credential"):
        assert leak not in body, f"fixture still contains {leak!r}"
    open(target, "w", encoding="utf-8").write(body)


if __name__ == "__main__":
    main(*sys.argv[1:4])
