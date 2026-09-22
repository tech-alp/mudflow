# Runmark brand theme

Source: `runmark.brand.json`, seed `#5B8DEF`. Change the seed there and regenerate
both manifests; do not edit generated `themes/runmark.*.json` manually.

The generator uses the pinned Merce v1.2.0 BrandDerivation and its existing
Material Color Utilities HCT palettes. Runmark overrides the stock greenish
neutral surfaces with seed-hue neutrals (chroma 8), and uses accent tone 42 in
light mode and 76 in dark mode. Surface, text and outline tones are defined in
`tools/runmark-theme/main.cpp`. The concept image is directional; its hex labels
are not the generated token contract.

Error, warning and success keep Merce's independent semantic colors. Info uses
a fixed cyan palette to distinguish it from the blue action color, as required
by Merce's status-proximity validator. Neutral badges follow the neutral palette.
The existing `desktop` metrics and all bundled kiosk profiles remain unchanged.

Generate and check (no changes to the main desktop build tree):

```sh
cmake -S tools/runmark-theme -B /tmp/runmark-theme-build -G Ninja \
  -DMERCE_SOURCE=/path/to/Merce-v1.2.0 \
  -DCMAKE_PREFIX_PATH="$QT6_ROOT"
cmake --build /tmp/runmark-theme-build
/tmp/runmark-theme-build/runmark-theme --write \
  libs/ui-shell/theme/runmark.brand.json libs/ui-shell/theme/themes
/tmp/runmark-theme-build/runmark-theme --check \
  libs/ui-shell/theme/runmark.brand.json libs/ui-shell/theme/themes
```

Both modes pass Merce's resolved-theme schema, contrast and status-proximity
validation before any output is written. `--check` also rejects stale generated
files. No generator or extra dependency runs on the device.

`themes/index.json` registers brand `runmark` with modes `light` and `dark`;
CMake copies both manifests alongside that index, including in installation.
After the existing `Theme.addThemeSource(...)` and `Theme.reloadThemes()`:

```qml
Theme.setContext("runmark", "dark", "desktop")
// Or: Theme.setContext("runmark", "light", "desktop")
```

Shell.qml activates the Runmark dark theme at startup; the header switches
between Runmark light and dark while retaining the desktop profile.
