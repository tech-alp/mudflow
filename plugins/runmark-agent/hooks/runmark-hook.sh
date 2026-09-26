#!/bin/sh
# Entry point for every Runmark hook: runmark-hook.sh <event>.
# Hooks can run with a narrow PATH: under Codex `env node` was not found and
# exited 127, and `rmk` was invisible too. Add the usual install locations.
# ponytail: fixed list; if node lives elsewhere, add it here.
PATH="$PATH:/usr/bin:/bin:/opt/homebrew/bin:/usr/local/bin:$HOME/.local/bin"
export PATH
# No external commands: under a narrow PATH even `dirname` can be missing.
case "$0" in */*) dir=${0%/*} ;; *) dir=. ;; esac
node=$(command -v node) || exit 0
[ -n "$node" ] || exit 0
exec "$node" "$dir/runmark-hook.js" "$1"
