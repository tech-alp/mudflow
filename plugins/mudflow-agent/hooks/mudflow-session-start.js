#!/usr/bin/env node
'use strict';

const { readFileSync } = require('node:fs');
const { execFileSync } = require('node:child_process');
const { isAbsolute } = require('node:path');

try {
  const input = JSON.parse(readFileSync(0, 'utf8'));
  if (!input || typeof input.cwd !== 'string' || !isAbsolute(input.cwd)) {
    process.exit(0);
  }

  const version = execFileSync('mudflow', ['--version'], {
    cwd: input.cwd,
    encoding: 'utf8',
    stdio: ['ignore', 'pipe', 'pipe'],
    timeout: 3000,
    killSignal: 'SIGKILL',
    maxBuffer: 64 * 1024,
    windowsHide: true,
  }).trim();
  const minimum = require('../skills/mudflow/compatibility.json').minimum_mudflow_version;
  const match = /^mudflow (0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)(-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?$/.exec(version);
  if (!match) {
    process.stderr.write(`Mudflow session context skipped: cannot verify CLI version; requires >= ${minimum}.\n`);
    process.exit(0);
  }
  const installed = match.slice(1, 4).map(BigInt);
  const required = minimum.split('.').map(BigInt);
  const difference = installed.findIndex((part, index) => part !== required[index]);
  if (difference === -1 ? Boolean(match[4]) : installed[difference] < required[difference]) {
    process.stderr.write(`Mudflow session context skipped: ${version}; requires >= ${minimum}.\n`);
    process.exit(0);
  }

  // Argument-less resume means "latest execution": selection lives in core, and
  // the hook must not infer it from Git, findings or files (TC-006).
  // --hook records that this ran, so a hook that silently stops firing becomes
  // a status finding instead of looking like a clean project.
  const context = execFileSync('mudflow', ['resume', '--markdown', '--hook'], {
    cwd: input.cwd,
    encoding: 'utf8',
    stdio: ['ignore', 'pipe', 'pipe'],
    timeout: 10000,
    killSignal: 'SIGKILL',
    maxBuffer: 1024 * 1024,
    windowsHide: true,
  });
  // Duz stdout iki runtime'da da baglam olarak alinir. Claude'un JSON bicimi
  // (hookSpecificOutput.additionalContext) da calisir ama Codex onu almaz;
  // tek bicim tutmak ayrismayi kaldiriyor.
  if (context.trim()) {
    process.stdout.write(context);
  }
} catch {
  // Missing CLI, malformed input and command failures must not break the session.
}
