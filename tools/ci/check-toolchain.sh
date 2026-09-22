#!/bin/sh
set -eu
test "$(uname -s)" = Darwin
test "$(uname -m)" = arm64
: "${LLVM_ROOT:?Set LLVM_ROOT to the LLVM installation}"
: "${QT6_ROOT:?Set QT6_ROOT to the Qt installation}"
"$LLVM_ROOT/bin/clang++" --version | head -n 1 | grep -F 'clang version 23.1.1'
cmake --version | head -n 1 | grep -Fx 'cmake version 4.4.3'
test "$(ninja --version)" = '1.13.2'
test "$("$QT6_ROOT/bin/qtpaths" --qt-version)" = '6.11.1'
