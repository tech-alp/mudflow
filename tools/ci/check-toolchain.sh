#!/bin/sh
set -eu
test "$(uname -s)" = Darwin
test "$(uname -m)" = arm64
: "${LLVM_ROOT:?Set LLVM_ROOT to the LLVM installation}"
: "${QT6_ROOT:?Set QT6_ROOT to the Qt installation}"
require_version() {
    if [ "$2" != "$3" ]; then
        printf '%s: expected %s, got %s\n' "$1" "$3" "$2" >&2
        exit 1
    fi
    printf '%s: %s\n' "$1" "$2"
}
clang_version=$("$LLVM_ROOT/bin/clang++" --version | sed -n '1s/.*clang version \([^ ]*\).*/\1/p')
require_version LLVM "$clang_version" 23.1.1
require_version CMake "$(cmake --version | sed -n '1s/cmake version //p')" 4.4.3
require_version Ninja "$(ninja --version)" 1.13.2
require_version Qt "$("$QT6_ROOT/bin/qtpaths" --qt-version)" 6.11.1
