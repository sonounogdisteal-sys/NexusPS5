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

## Reconstruction target

The visual target is the PS5 Home/Welcome Hub interface presented in Sony's
September 2024 Welcome Hub rollout announcement and its accompanying public
screenshot. That identifies the feature generation, not an exact firmware
build: the source does not identify a system software version number or
region-specific build. NEXUS does not claim to reproduce a particular firmware
revision. Later features described on currently maintained support pages,
such as system-wide console-generation appearance themes, are not treated as
part of the 2024 reference unless independently verified in that reference.

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
| Navigation | The icon/control area is focusable; Options enters rearrangement mode, X places a moved icon, and controls can be moved to a Hidden Controls area. Some controls cannot be hidden. |
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
| Layout | Widgets can be added/removed, moved, resized, and arranged through presets or a custom preset. Sony documents X to toggle visibility, Square to begin a move, arrows to position, X to place, and L1/R1 to resize. |
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

The [Welcome Hub launch article screenshot](https://blog.playstation.com/tachyon/2024/09/2424168de190dae0ea094bf824184b7ec6c53e77.jpg?fit=1024%2C1024)
was visually inspected at 1024x576. It shows Games/Media at the upper left,
system indicators at the upper right, a horizontal row of application tiles,
a selected Welcome tile and label, and a dark illustrated background with
widget cards occupying much of the lower area. This is one Welcome Hub view,
not evidence that every PS5 Home state has this exact layout. Its component
positions, dimensions, and color values remain approximate until measured from
version- and region-specific source captures.

### Settings and accessibility

Sony's [accessibility settings support page](https://www.playstation.com/en-us/support/hardware/ps5-accessibility-settings/)
confirms Settings > Accessibility and documents display/text, controller,
zoom, screen-reader, caption, and audio accessibility options. Sony's
[resolution support page](https://www.playstation.com/en-us/support/hardware/ps5-4k-resolution-guide/)
confirms Settings > Screen and Video > Video Output and describes display
resolution options. The Control Center support article also directs users to
Settings > Sound > Audio Output. These sources document navigation and
behavior, not a measured Settings screen layout. NEXUS uses the requested
category names, implements only host settings it can apply, and labels
console/guest settings as unsupported.

### Prototype decisions and limitations

The shell uses separate Games and Media navigation, a horizontal row of
sample entries, selected-entry artwork, a local Game Library and Game Hub,
Settings, and a Welcome Hub-like set of local widgets. Widget editing supports
visibility, grid position, size, and original local backgrounds with file
persistence. Welcome Hub controls follow the publicly documented interaction
pattern, but NEXUS's grid and screen positions are its own approximation.
Artwork and catalog entries are original local samples, not Sony or game
assets. Play explicitly reports that guest execution is unavailable.

The Control Center prototype keeps documented controls distinct from
contextual cards and supports PS/Guide activation, horizontal focus, local
icon ordering, and an animated open/close transition. Hiding controls and
service-dependent actions are not implemented. Its exact card ordering,
overlay dimensions, transparency, and transition timing are **not verified**
against an official interface screenshot. They remain subject to visual
reconstruction after collecting a version- and region-specific official
reference.

## Graphics and input foundation

Research: [SDL2 Video](https://wiki.libsdl.org/SDL2/CategoryVideo),
[SDL2 2D Renderer](https://wiki.libsdl.org/SDL2/CategoryRender),
[SDL2 Vulkan helpers](https://wiki.libsdl.org/SDL2/CategoryVulkan), and
[SDL2 Game Controller](https://wiki.libsdl.org/SDL2/CategoryGameController).
SDL2 provides cross-platform window/event abstractions, standardized gamepad
buttons, a 2D renderer, and helper functions for creating Vulkan surfaces.
NEXUS currently uses its SDL 2D renderer only; Vulkan is not initialized.
The initial headless renderer path permits UI tests without a GPU.

GLFW was considered: it provides cross-platform window/input and explicit
Vulkan surface helpers ([GLFW input guide](https://www.glfw.org/docs/latest/input),
[Vulkan reference](https://www.glfw.org/docs/latest/group__vulkan.html)); it
would leave the 2D UI renderer and text rasterization to separate libraries.
SDL3 was also considered; its [video](https://wiki.libsdl.org/SDL3/CategoryVideo),
[Vulkan](https://wiki.libsdl.org/SDL3/CategoryVulkan), and
[gamepad](https://wiki.libsdl.org/SDL3/CategoryGamepad) APIs provide similar
cross-platform primitives. SDL2 was selected for this milestone because its
2D renderer, controller abstraction, Vulkan window helpers, and software
rendering path can cover the first UI without a Vulkan SDK/GPU. The tested
Codespaces packages are SDL2 2.30.0 and SDL_ttf 2.22.0. The tradeoff is that
the current renderer is only a UI prototype; a dedicated Vulkan backend still
needs implementation.

Official UI reference used for feature names and interaction:
[PS5 features](https://www.playstation.com/en-th/ps5/ps5-features/),
[Control Center](https://www.playstation.com/en-gb/support/games/customize-ps5-control-center/),
and [Welcome hub](https://www.playstation.com/en-us/support/account/welcome-hub/).
These pages document behavior but do not provide measured design tokens or
component geometry. The official video linked by the features page is
<https://www.youtube.com/watch?v=wLUvxgY2wd8>; its video content has not been
frame-analyzed for this implementation.

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
- Sony Support, Accessibility settings: <https://www.playstation.com/en-us/support/hardware/ps5-accessibility-settings/>
- Sony Support, Screen and Video resolution: <https://www.playstation.com/en-us/support/hardware/ps5-4k-resolution-guide/>
- Sony, PS5 features: <https://www.playstation.com/en-th/ps5/ps5-features/>
- Sony, Welcome Hub screenshot in the September 2024 system update announcement:
  <https://blog.playstation.com/tachyon/2024/09/2424168de190dae0ea094bf824184b7ec6c53e77.jpg?fit=1024%2C1024>
- PlayStation Blog, Welcome hub system update (2024-09-12):
  <https://blog.playstation.com/2024/09/12/ps5-system-update-adds-welcome-hub-party-share-personalized-3d-audio-profiles-adaptive-controller-charging-and-more/>
- PlayStation Blog, PS5 Ultimate FAQ (2020-11-09, later updated):
  <https://blog.playstation.com/2020/11/09/ps5-the-ultimate-faq/>
- RPCSX public repository: <https://github.com/RPCSX/rpcsx> — repository
  metadata and top-level layout were inspected as technical context only. No
  code or files were copied or used as a runtime dependency.

The research was performed against public pages retrieved on 2026-11-17.
Failed or incomplete retrieval is not treated as evidence.
