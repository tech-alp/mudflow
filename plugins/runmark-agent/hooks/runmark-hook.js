'use strict';

// Forwards the agent runtime's hook JSON to `rmk hook <event>`. Every decision
// (what to record, whether to remind) is made by rmk, where it is tested; this
// script only guards the session and never breaks it.
const { readFileSync } = require('node:fs');
const { execFileSync } = require('node:child_process');
const { isAbsolute } = require('node:path');

// Per-event budget. Stop and prompt-submit run on every turn.
const EVENTS = { 'session-start': 10000, 'prompt-submit': 2500, stop: 4000, 'session-end': 2500 };

try {
  const event = process.argv[2];
  if (!Object.hasOwn(EVENTS, event)) process.exit(0);
  const raw = readFileSync(0, 'utf8');
  const input = JSON.parse(raw);
  if (!input || typeof input.cwd !== 'string' || !isAbsolute(input.cwd) || typeof input.session_id !== 'string') {
    process.exit(0);
  }
  const options = {
    cwd: input.cwd,
    encoding: 'utf8',
    stdio: ['pipe', 'pipe', 'pipe'],
    killSignal: 'SIGKILL',
    maxBuffer: 1024 * 1024,
    windowsHide: true,
  };

  // Checked once per session, not on every turn: an older rmk simply rejects
  // `rmk hook` and the error is swallowed below.
  if (event === 'session-start') {
    const version = execFileSync('rmk', ['--version'], { ...options, stdio: ['ignore', 'pipe', 'pipe'], timeout: 3000 }).trim();
    const minimum = require('../skills/runmark/compatibility.json').minimum_rmk_version;
    const match = /^rmk (0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?$/.exec(version);
    const installed = match ? match.slice(1, 4).map(BigInt) : null;
    const required = minimum.split('.').map(BigInt);
    const difference = installed ? installed.findIndex((part, index) => part !== required[index]) : -1;
    if (!match || (difference === -1 ? Boolean(match[4]) : installed[difference] < required[difference])) {
      process.stderr.write(`Runmark session context skipped: ${match ? version : 'cannot verify CLI version'}; requires >= ${minimum}.\n`);
      process.exit(0);
    }
  }

  // session-start: plain stdout is taken as context by both runtimes.
  // stop: a {"decision":"block"} line asks the agent to record a note first.
  const output = execFileSync('rmk', ['hook', event], { ...options, input: raw, timeout: EVENTS[event] });
  if (output.trim()) process.stdout.write(output);
} catch {
  // Missing CLI, malformed input and command failures must not break the session.
}
