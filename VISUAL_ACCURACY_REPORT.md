# Visual accuracy report

## Milestone 3 status

The independent shell now has Home/Welcome Hub, Game Library, Game Hub,
Settings, and Control Center captures produced by the headless SDL smoke test.
This confirms rendering and basic screen composition, not pixel-accurate
reproduction. The visual target is the Home/Welcome Hub shown in Sony's
September 2024 rollout material; the exact system software version is
unidentified.

## Screenshots

`nexus_shell_window_smoke` writes build-only captures:

- Home / Welcome Hub: `build/nexus-shell-smoke.bmp`
- Selected Home entry: `build/nexus-shell-smoke.bmp.selected-home.bmp`
- Game Hub: `build/nexus-shell-smoke.bmp.game-hub.bmp`
- Game Library: `build/nexus-shell-smoke.bmp.game-library.bmp`
- Settings: `build/nexus-shell-smoke.bmp.settings.bmp`
- Control Center: `build/nexus-shell-smoke.bmp.control-center.bmp`

These are local build outputs and are not checked-in or redistributed assets.
The same software-rendered smoke path runs in GitHub Actions.

## Screen comparisons

**Home / Welcome Hub.** Reference: the [official Welcome Hub launch
screenshot](https://blog.playstation.com/tachyon/2024/09/2424168de190dae0ea094bf824184b7ec6c53e77.jpg?fit=1024%2C1024)
and [Sony's Welcome Hub documentation](https://www.playstation.com/en-us/support/account/welcome-hub/).
The implementation includes the Games/Media navigation, upper system/profile
access, horizontal app row, selected label, and lower widget area. Its local
sample cards and original vector-wave background are structurally similar but
not Sony artwork. Card positions, spacing, colors, typography, and widget
dimensions are approximations; no pixel measurements were available.

**Game Hub.** Reference: [official PS5 features](https://www.playstation.com/en-th/ps5/ps5-features/)
and [Sony's accessibility guide](https://www.playstation.com/en-us/support/hardware/ps5-accessibility-settings/),
which refer to game hubs and game-specific information. The capture has local
artwork, title, Play action, and Overview/Activities/Trophies/Related sections.
Its layout and content are NEXUS-authored test data; no official reference
frame was available for spacing, color, typography, or focus comparison.

**Game Library.** Reference: [official PS5 features](https://www.playstation.com/en-th/ps5/ps5-features/)
documents game lists/library features. NEXUS currently shows a simple local
sample list and links entries to the independent Game Hub. The card grid is
not claimed to match the console library layout.

**Settings.** References: Sony's [Accessibility settings](https://www.playstation.com/en-us/support/hardware/ps5-accessibility-settings/)
and [Screen and Video resolution guide](https://www.playstation.com/en-us/support/hardware/ps5-4k-resolution-guide/)
confirm menu paths and some settings behavior, but do not provide a measured
full Settings-screen reference. NEXUS uses an independently authored category
list, applies only host settings, and labels unsupported entries. Layout,
icons, colors, and typography are not validated against a Sony screenshot.

**Control Center.** Reference: [Sony's Control Center guide](https://www.playstation.com/en-us/support/games/customize-ps5-control-center/).
NEXUS separates contextual sample cards from the control row and implements
Guide/PS activation, focus, local icon ordering, and an open/close transition.
The Sony page is textual, so overlay dimensions, card placement, colors,
animations, and exact focus visuals remain unverified; NEXUS's 180 ms
ease-out transition is a prototype choice, not a measured Sony value. Hiding controls and
console services are not implemented.

Across all screens, the renderer uses SDL_ttf with a host system font and
original local geometry. No Sony artwork, fonts, or proprietary assets are
used. Headless screenshots were visually inspected for clipping, layout
hierarchy, and readable labels; the Settings capture route was adjusted to
show the actual Settings view. No interactive desktop or physical controller
validation is claimed.

## Next validation

Obtain public, version- and region-specific reference frames for Game Hub,
Settings, and Control Center. Record resolution and context, measure
component positions/colors, and compare them to captures at the same aspect
ratio. Do not claim pixel accuracy until those measurements exist. Follow the
manual desktop test procedure in `DEVELOPMENT.md`; do not redistribute
proprietary assets.
