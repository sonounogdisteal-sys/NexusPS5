# PS5 shell research notes

This is an evidence log, not a complete reverse-engineering specification.
The available public sources confirm some user-visible behavior but do not
reveal enough implementation detail to reproduce every screen faithfully.
Unknown details remain marked unknown instead of being guessed.

## Scope and terminology

- Sony documentation describes the PS5 home screen, Control Center, Game Hub,
  activities, and Welcome hub as user-facing features.
- `SceShellUI.elf` is named in the project brief. Its exact responsibilities,
  dependencies, launch contract, and relationship to other system modules
  have **not** been verified from the sources reviewed here.
- NEXUS intends to implement a new shell, not execute or reproduce Sony
  binaries. No Sony executable, firmware, or proprietary assets are used.

## Verified components

### Control Center

Evidence: [Sony support: Customize the PS5 Control Center](https://www.playstation.com/en-gb/support/games/customize-ps5-control-center/)

| Property | Evidence-based finding |
|---|---|
| Purpose | Quick access to system features without leaving a game. |
| Activation | Pressing the PS button displays it. |
| Position and dimensions | It is displayed over the current game. Exact coordinates and dimensions are unknown from the text documentation. |
| Controls | Home, Switcher, Notifications, Game Base, Music, Sound, Mic, Accessories, Profile, and Power are documented. Availability is context-dependent. |
| Cards | Sony documents cards for trophy progress/challenges/activities, voice chat and broadcast, and music controls. |
| Navigation | The icon/control area is focusable; Options enters rearrangement mode, X places a moved icon, and hidden controls can be restored. |
| Visuals | Exact colors, typography, artwork, icon geometry, and background treatment are not specified in the support article. |
| Animation and return behavior | Not specified by the reviewed text source. |
| Dependencies | Sony console software, active game/context, and corresponding system services; internal service contracts are unknown. |
| Confidence | High for documented names and described interactions; low/unknown for pixel-level and timing details. |

### Welcome hub

Evidence: [Sony support: Customize the Welcome hub](https://www.playstation.com/en-us/support/account/welcome-hub/) and [September 2024 system update announcement](https://blog.playstation.com/2024/09/12/ps5-system-update-adds-welcome-hub-party-share-personalized-3d-audio-profiles-adaptive-controller-charging-and-more/)

| Property | Evidence-based finding |
|---|---|
| Purpose and position | Personalized area available on the PS5 home screen, customized with widgets and backgrounds. |
| Widgets | Friend Activity, Friends Online, Community Activity, Messages, Console Storage, Trophies, PlayStation Store, PlayStation Plus, News, Accessibility, Media Gallery, Battery, and Wishlist are documented; availability may vary by region. |
| Layout | Widgets can be added/removed, moved, resized, and arranged through presets or a custom preset. Controls documented include X, Square, and L1/R1. |
| Background | Can use provided designs, game or Media Gallery images; slideshow and showcase modes are documented. |
| Dimensions and exact placement | The feature supports resizing and positioning, but exact grid dimensions, widget sizes, margins, and screen coordinates are unknown from the reviewed text. |
| Colors, typography, iconography, animation | Not sufficiently specified in reviewed text. The 2024 announcement mentions animated background designs, without specifying their motion. |
| Dependencies | Some widgets/features depend on services, region, account, or system software; details vary. No PlayStation Network service is implemented by NEXUS. |
| Confidence | High for documented widget and customization behavior; medium for rollout context; low/unknown for visual measurements and timing. |

### Home screen and Game Hub

Evidence: [Official PS5 features](https://www.playstation.com/en-th/ps5/ps5-features/) and [PS5: The Ultimate FAQ](https://blog.playstation.com/2020/11/09/ps5-the-ultimate-faq/)

Sony describes game-specific hubs as collecting related content, including
community broadcasts and new content. The features page also documents
activities, challenges, game presets, and game lists. Exact current layout,
selection geometry, animation, and differences across system software
generations are not established by the reviewed text. The Games and Media
sections' pixel-level structure is **unknown**.

## Open research questions

- Exact roles and dependencies of `SceShellUI.elf`; no reliable evidence
  reviewed establishes a complete module boundary or executable launch path.
- Which shell details differ across system software versions and regions.
- Current Games/Media home layout, focus transitions, artwork sizing, and
  backgrounds.
- Detailed Game Hub, profile, trophies, notifications, library, settings, and
  application switcher layouts and interactions.
- Control Center card arrangement, exact visual treatment, transitions, and
  context-dependent controls.
- Typography, color values, animation timing, and accessibility behavior.

## Sources reviewed

- Sony Design, PS5 story: <https://www.sony.com/en/SonyInfo/design/stories/PS5/> —
  fetch returned HTTP 403 in this research environment; content not verified.
- Sony Support, Control Center: <https://www.playstation.com/en-gb/support/games/customize-ps5-control-center/>
- Sony Support, Welcome hub: <https://www.playstation.com/en-us/support/account/welcome-hub/>
- Sony, PS5 features: <https://www.playstation.com/en-th/ps5/ps5-features/>
- PlayStation Blog, Welcome hub system update (2024-09-12):
  <https://blog.playstation.com/2024/09/12/ps5-system-update-adds-welcome-hub-party-share-personalized-3d-audio-profiles-adaptive-controller-charging-and-more/>
- PlayStation Blog, PS5 Ultimate FAQ (2020-11-09, later updated):
  <https://blog.playstation.com/2020/11/09/ps5-the-ultimate-faq/>
- RPCSX public repository: <https://github.com/RPCSX/rpcsx> — repository
  metadata and top-level layout were inspected as technical context only. No
  code or files were copied or used as a runtime dependency.

The research was performed against public pages retrieved on 2026-11-17.
Failed or incomplete retrieval is not treated as evidence.
