#!/bin/sh
set -eu
plugin_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec node - "$plugin_root" <<'NODE'
'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { spawnSync } = require('node:child_process');
const plugin = process.argv[2];
const repo = path.resolve(plugin, '../..');
const json = file => JSON.parse(fs.readFileSync(file, 'utf8'));
let failures = 0;
function check(name, test) {
  try { test(); console.log(`PASS ${name}`); }
  catch (error) { failures++; console.error(`FAIL ${name}: ${error.message}`); }
}
function scan(directory) {
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    const file = path.join(directory, entry.name);
    if (entry.isDirectory()) scan(file);
    else assert(!fs.readFileSync(file, 'utf8').includes('.mudflow/'), file);
  }
}
check('TC-007: hooks/ and skills/ contain no .mudflow/ string', () => {
  scan(path.join(plugin, 'hooks'));
  scan(path.join(plugin, 'skills'));
});
check('four plugin JSON files parse', () => {
  for (const file of ['.claude-plugin/plugin.json', '.codex-plugin/plugin.json',
    'hooks/hooks.json', 'skills/mudflow/compatibility.json']) json(path.join(plugin, file));
});
check('single SessionStart hook and manifest contracts', () => {
  const claude = json(path.join(plugin, '.claude-plugin/plugin.json'));
  const codex = json(path.join(plugin, '.codex-plugin/plugin.json'));
  assert.deepEqual(Object.keys(claude).sort(), ['author', 'description', 'name', 'version']);
  assert.equal(claude.name, 'mudflow-agent');
  assert.equal(codex.name, claude.name);
  assert.equal(codex.hooks, './hooks/hooks.json');
  for (const field of ['displayName', 'shortDescription', 'longDescription', 'developerName', 'category']) {
    assert.equal(typeof codex.interface[field], 'string');
    assert(codex.interface[field].trim());
  }
  assert(Array.isArray(codex.interface.capabilities));
  assert(Array.isArray(codex.interface.defaultPrompt));
  assert(codex.interface.defaultPrompt.length > 0 && codex.interface.defaultPrompt.length <= 3);
  assert(codex.interface.defaultPrompt.every(prompt => typeof prompt === 'string' && prompt.length <= 128));
  const hooks = json(path.join(plugin, 'hooks/hooks.json')).hooks;
  assert.deepEqual(Object.keys(hooks), ['SessionStart']);
  assert.equal(hooks.SessionStart.length, 1);
  const session = hooks.SessionStart[0];
  assert.equal(session.matcher, 'startup|resume');
  assert.equal(session.hooks.length, 1);
  assert.equal(session.hooks[0].type, 'command');
  assert.equal(session.hooks[0].timeout, 10);
  for (const field of ['command', 'commandWindows']) {
    assert.equal(session.hooks[0][field], 'node "${CLAUDE_PLUGIN_ROOT}/hooks/mudflow-session-start.js"');
  }
});
for (const runtime of ['claude', 'codex']) {
  check(`${runtime} marketplace JSON and source directory`, () => {
    const file = runtime === 'claude' ? '.claude-plugin/marketplace.json' : '.agents/plugins/marketplace.json';
    const market = json(path.join(repo, file));
    assert(market.plugins.some(entry => entry.name === 'mudflow-agent'));
    for (const entry of market.plugins) {
      const source = runtime === 'claude' ? entry.source : entry.source.path;
      assert(source.startsWith('./') && !source.split('/').includes('..'));
      assert(fs.statSync(path.resolve(repo, source)).isDirectory());
      if (runtime === 'codex') {
        assert.equal(entry.source.source, 'local');
        assert.equal(entry.policy.installation, 'AVAILABLE');
        assert.equal(entry.policy.authentication, 'ON_INSTALL');
        assert(entry.category);
      }
    }
  });
}
check('root CMakeLists.txt contains no plugins', () => {
  assert(!fs.readFileSync(path.join(repo, 'CMakeLists.txt'), 'utf8').includes('plugins'));
});
check('skill frontmatter and ADR-002 recording guidance', () => {
  const skill = fs.readFileSync(path.join(plugin, 'skills/mudflow/SKILL.md'), 'utf8');
  assert.match(skill, /^---\nname: mudflow\ndescription: .+\n---\n/);
  for (const text of ['mudflow evidence', 'mudflow note', '--kind agent_summary', 'ADR-002', 'claim, not a measurement']) {
    assert(skill.includes(text), text);
  }
});

const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'mudflow-hook-test-'));
try {
  const log = path.join(temporary, 'calls.jsonl');
  const bin = path.join(temporary, 'bin');
  fs.mkdirSync(bin);
  fs.writeFileSync(path.join(bin, 'mudflow'), `#!${process.execPath}
const fs = require('node:fs');
fs.appendFileSync(process.env.CALL_LOG, JSON.stringify(process.argv.slice(2)) + '\\n');
if (process.env.FAKE_MODE === 'timeout') setTimeout(() => {}, 10000);
else if (process.env.FAKE_MODE === 'failure') { console.error('CLI failed'); process.exit(1); }
else if (process.argv.includes('--version')) console.log(process.env.FAKE_VERSION);
else process.stdout.write(process.env.FAKE_RESUME || '');
`, { mode: 0o755 });
  const valid = JSON.stringify({ cwd: repo, session_id: 'x' });
  function run(input = valid, env = {}) {
    fs.writeFileSync(log, '');
    const result = spawnSync(process.execPath, [path.join(plugin, 'hooks/mudflow-session-start.js')], {
      input, encoding: 'utf8', timeout: 6000,
      env: { ...process.env, PATH: bin, CALL_LOG: log, FAKE_VERSION: 'mudflow 0.1.0', FAKE_MODE: '', ...env },
    });
    assert.ifError(result.error);
    assert.equal(result.status, 0);
    result.calls = fs.readFileSync(log, 'utf8').trim().split('\n').filter(Boolean).map(JSON.parse);
    return result;
  }
  check('empty, malformed and invalid input: silent exit 0, no CLI call', () => {
    for (const input of ['', '{', 'null', '[]', '{}', '{"cwd":42}', '{"cwd":"relative"}']) {
      const result = run(input);
      assert.equal(result.stderr, '');
      assert.deepEqual(result.calls, []);
      assert.equal(result.stdout, '');
    }
  });
  check('missing CLI: silent exit 0', () => {
    const result = run(valid, { PATH: temporary });
    assert.equal(result.stderr, '');
    assert.deepEqual(result.calls, []);
  });
  check('TC-006: compatible CLI injects selector-less resume, writes nothing', () => {
    for (const version of ['0.1.0', '0.1.0+build.1', '0.10.0', '1.0.0', '0.2.0-rc.1']) {
      const result = run(valid, { FAKE_VERSION: `mudflow ${version}`, FAKE_RESUME: '# Mudflow resume: MF-1\n' });
      assert.equal(result.stderr, '');
      // Hook yalnizca cagirir; secici vermez, karar vermez (TC-006).
      assert.deepEqual(result.calls, [['--version'], ['resume', '--markdown']]);
      assert.deepEqual(JSON.parse(result.stdout), {
        hookSpecificOutput: { additionalContext: '# Mudflow resume: MF-1\n' } });
    }
  });
  check('empty resume output: no injection', () => {
    const result = run(valid, { FAKE_RESUME: '   \n' });
    assert.equal(result.stderr, '');
    assert.equal(result.stdout, '');
    assert.deepEqual(result.calls, [['--version'], ['resume', '--markdown']]);
  });
  check('incompatible or unknown version: explicit minimum on stderr, exit 0', () => {
    for (const version of ['mudflow 0.0.9', 'mudflow 0.1.0-rc.1', 'unknown']) {
      const result = run(valid, { FAKE_VERSION: version });
      assert.match(result.stderr, /requires >= 0\.1\.0/);
      assert.deepEqual(result.calls, [['--version']]);
      assert.equal(result.stdout, '');
    }
  });
  check('CLI failure and timeout: silent exit 0', () => {
    for (const mode of ['failure', 'timeout']) {
      const result = run(valid, { FAKE_MODE: mode });
      assert.equal(result.stderr, '');
      assert.deepEqual(result.calls, [['--version']]);
      assert.equal(result.stdout, '');
    }
  });
} finally {
  fs.rmSync(temporary, { recursive: true, force: true });
}
console.log(`RESULT ${failures} failure(s)`);
process.exitCode = failures ? 1 : 0;
NODE
