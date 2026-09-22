#!/bin/sh
# Official macOS ARM64 release, independent of the runner's Homebrew snapshot.
set -eu
test "$(uname -s)" = Darwin
test "$(uname -m)" = arm64
: "${RUNNER_TEMP:?Set RUNNER_TEMP to a writable temporary directory}"
: "${GITHUB_ENV:?Set GITHUB_ENV to the environment output file}"

llvm_stage=$(mktemp -d "$RUNNER_TEMP/runmark-llvm.XXXXXX")
archive="$llvm_stage/LLVM-23.1.1-macOS-ARM64.tar.xz"
curl --fail --silent --show-error --location --retry 3 --output "$archive" \
  https://github.com/llvm/llvm-project/releases/download/llvmorg-23.1.1/LLVM-23.1.1-macOS-ARM64.tar.xz
printf '%s  %s\n' \
  64220f1c99132ef7e580447781b84f96fbba6862a43a8f6522b423052cd67502 \
  "$archive" | shasum -a 256 --check
mkdir "$llvm_stage/toolchain"
tar -xJf "$archive" -C "$llvm_stage/toolchain" --strip-components=1
test -x "$llvm_stage/toolchain/bin/clang++"
test -x "$llvm_stage/toolchain/bin/clang-scan-deps"
printf 'LLVM_ROOT=%s\n' "$llvm_stage/toolchain" >> "$GITHUB_ENV"
