import {test} from 'node:test';
import assert from 'node:assert/strict';
import {mkdtempSync, mkdirSync, writeFileSync, rmSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join} from 'node:path';
import {spawnSync} from 'node:child_process';

test('toolchain gate accepts only the pinned versions and explains mismatches', () => {
  const root = mkdtempSync(join(tmpdir(), 'rmk-toolchain-test-'));
  const bin = join(root, 'bin');
  mkdirSync(bin);
  const executable = (name, output) => writeFileSync(join(bin, name),
    `#!/bin/sh\nprintf '%s\\n' '${output}'\n`, {mode: 0o755});
  const run = () => spawnSync('/bin/sh', ['tools/ci/check-toolchain.sh'], {
    encoding: 'utf8', env: {...process.env, PATH: `${bin}:/usr/bin:/bin`,
      LLVM_ROOT: root, QT6_ROOT: root},
  });
  try {
    writeFileSync(join(bin, 'uname'), '#!/bin/sh\ncase "$1" in -s) echo Darwin;; -m) echo arm64;; esac\n', {mode: 0o755});
    executable('cmake', 'cmake version 4.4.3');
    executable('ninja', '1.13.2');
    executable('qtpaths', '6.11.1');
    for (const banner of ['clang version 23.1.1', 'Homebrew clang version 23.1.1']) {
      executable('clang++', banner);
      assert.equal(run().status, 0);
    }
    for (const version of ['23.1.0', '23.1.10', '23.1.1-rc1']) {
      executable('clang++', `clang version ${version}`);
      const result = run();
      assert.notEqual(result.status, 0);
      assert.match(result.stderr, /LLVM: expected 23\.1\.1, got /);
    }
    executable('clang++', 'clang version 23.1.1');
    executable('ninja', '1.13.3');
    assert.match(run().stderr, /Ninja: expected 1\.13\.2, got 1\.13\.3/);
  } finally {
    rmSync(root, {recursive: true, force: true});
  }
});
