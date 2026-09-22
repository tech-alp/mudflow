# Runmark desktop / Merce

Merce v1.2.0 is pinned to commit `2e587eb9f5cfc2f3456508646e22d151f45f0b55`.
Its Theme, Foundation, Style and Controls QML modules/plugins are linked statically.
CLI-only builds never fetch Merce or require Qt Quick.

```sh
cmake --preset dev -DRUNMARK_BUILD_UI=ON
cmake --build --preset dev
ctest --preset dev
cmake --build build/dev --target runmark_ui_shell_qmllint
```

Set `LLVM_ROOT` and `QT6_ROOT` for the existing presets. Offline builds can use
`-DFETCHCONTENT_SOURCE_DIR_MERCE=/path/to/Merce` with the pinned checkout.

`theme/profiles/desktop.json` is Runmark's separate schema-v2 profile: 28/32/40 px
controls, 14 px body text and 12 px page padding. Merce's cart/maintenance/ops
profiles are unchanged. The shell uses `MerceStyle`, semantic colors and `MButton`.
The existing smoke test checks desktop light/dark and kiosk/desktop round trips.

Merce 1.2 accepts external theme sources only from the filesystem. CMake copies
`theme/themes/index.json` and `theme/profiles/desktop.json` beside the build binary
and installs that same `theme/` tree beside `runmark-desktop`. Missing or invalid
profile data stops startup with a nonzero exit instead of silently using kiosk
metrics. Ship the binary and this tree together.

Qt remains dynamically linked. Desktop Qt runtime bundling/signing is not part
of RM-8; an installed development binary needs a matching Qt 6.11 SDK, for example
`DYLD_FRAMEWORK_PATH="$QT6_ROOT/lib"` on macOS. Installation smoke was checked with
that SDK at scale factors 1 and 2, without a filesystem Merce QML import path.
