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

## Project selection and presentation

`Shell.qml` owns the window and project picker. Findings QML lives in
`features/findings/qml/`; its models and ViewModel live in
`features/findings/presentation/`. They remain in the same `Runmark.Shell` module.

Startup resolves `--project` first, then the last successfully measured project
from QSettings, then an existing `.runmark/project.json` in the working directory.
With none available, the window offers project selection instead of measuring a
nonexistent path. A missing remembered project shows an error and can be replaced
through **Proje seç**. **Yeniden dene** retries the selected path.

Selection is disabled while a measurement is running. Qt Concurrent keeps the
worker independent of the ViewModel lifetime; closing the window cannot leave a
worker calling a deleted QObject. This adds only the installed Qt Concurrent
module to the UI target. CLI dependencies are unchanged.

`desktop_project_workflow` tests selection, persistence, retry, launch precedence,
closing during measurement, and error layout at 1000/480 px widths. Test settings
are isolated; the existing `RUNMARK_SMOKE` mode neither restores nor saves user
preferences.

## Findings workspace

The shell follows `docs/assets/design-references/01-overview.png`; the Findings
list/detail composition follows `07-findings.png`. Only the implemented Findings
route is exposed. Overview content and evidence execution actions remain separate.

- `shell/`: responsive sidebar, project header and workspace layout.
- `components/`: Merce-backed severity badges and empty states.
- `features/findings/qml/`: page, filter bar, list delegate and detail panel.
- `features/findings/presentation/FindingFilterModel`: Qt proxy filtering, counts
  and selection. Repeated rule IDs are distinguished by finding content; selection
  survives reordering, and clears if its record disappears or is filtered out.

Search uses Qt 6.11's **Basic.SearchField**, not a custom text input. StyleKit 6.11
has no SearchField implementation; the stock Basic control receives Merce palette
values. `Ctrl/Cmd+F` focuses it. The list supports arrow keys and Enter; Escape or
**Listeye dön** returns from compact details. Below 900 px page width, details
replace the list. Below 1000 px window width, the sidebar is hidden.

The desktop profile loads before the workspace style is constructed, avoiding
cached kiosk font sizes. Light/dark switching keeps the same desktop metrics.
Stock Basic scroll controls avoid the always-visible StyleKit scroll indicators.
The GUI test writes fixture-based previews to `build/dev/Testing/desktop-captures/`.

## Project folder setup

The native folder picker opens `<folder>/.runmark/project.json` automatically.
If absent, `ProjectSetupDialog` collects a project name, existing Git remote,
base branch, readable plan file and task prefix (e.g. `PROJ` for `PROJ-1`).
Initialization validates the repository root and config, then creates the file
exclusively; existing or malformed configurations are never overwritten.
Cancelling creates nothing. The selected folder must already be a Git repository;
the setup does not create a remote or plan. Worktrees default to
`~/worktrees/<folder-name>`. Existing `AGENTS.md` is registered automatically.
The initialization use case runs off the UI thread, and success opens the project.

## Findings presentation

The domain and CLI retain the original rules and text. The desktop model adds
Turkish presentation roles (title, summary, impact, next step), preserves the raw
record in an expandable technical section, and groups reference-less notes
without discarding their original text. Counts represent underlying findings,
not grouped rows. Critical and warning findings sort before information.
Information starts collapsed, but explicit search/domain filters include it.
The last successful measurement time is shown; a failed refresh still displays
its error and does not advance that time. Plan links open the current readable
plan, not a claimed historical evidence source; Git links open the project folder.
No action silently modifies Git or marks a historical note resolved.
