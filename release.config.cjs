module.exports = {
  branches: ['main'],
  tagFormat: 'v${version}',
  plugins: [
    ['./tools/release/node_modules/@semantic-release/commit-analyzer/index.js', { preset: 'conventionalcommits' }],
    ['./tools/release/node_modules/@semantic-release/release-notes-generator/index.js', { preset: 'conventionalcommits' }],
    ['./tools/release/node_modules/@semantic-release/exec/index.js', {
      verifyConditionsCmd: 'node tools/release/release.mjs verify',
      prepareCmd: 'node tools/release/release.mjs prepare ${nextRelease.version}',
    }],
    ['./tools/release/node_modules/@semantic-release/github/index.js', {
      assets: [
        'build/artifacts/runmark-${nextRelease.version}-macos-arm64.tar.gz',
        'build/artifacts/runmark-${nextRelease.version}-macos-arm64.tar.gz.sha256',
      ],
      successComment: false,
      failComment: false,
      failTitle: false,
      releasedLabels: false,
    }],
  ],
};
