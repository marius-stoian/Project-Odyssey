# Story plans: M8b

Per-story plans for milestone M8b, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-230](#us-230)
- [US-231](#us-231)
- [US-232](#us-232)
- [US-233](#us-233)
- [US-234](#us-234)

---

<a id="us-230"></a>

## Plan US-230: Luna's SDL_GPU renderer

Assembly plan v2.2, prompt S-US-230. Design: [M8b renderer design](M8b-renderer-design.md) section 3 and [ADR-021](../adr/ADR-021-sdl-gpu-renderer.md). Traces to ARC-11, ADR-021, ADR-003.

### What was built
| File | What |
|---|---|
| `src/luna/platform/backend.h` | `RenderBackend`: what the Window draws with (name, clear, present, textures, draw, presentation rectangle, output size, read pixels); `wholeStepArea`; `kTexelNudge`; the two factory functions |
| `src/luna/platform/sdl_renderer_backend.cpp` | the old SDL_Renderer drawing, moved from the Window; it now draws onto a texture of the virtual-screen size and enlarges that into the window (so the pictures equal the GPU's) |
| `src/luna/platform/gpu_backend.cpp` | the GPU backend: SDL_GPU device and swapchain (VSync), a 480 x 270 virtual screen texture, a nearest-neighbour sampler, textures uploaded through transfer buffers, quads batched in drawing order (a new draw call only when the texture or blend mode changes), Normal and Add pipelines, a blit pass that enlarges the virtual screen into the window by a whole number with black bars, screenshots through a download buffer |
| `src/luna/platform/shaders/*.hlsl` | `sprite.vert`, `sprite.frag`, `blit.vert`, `blit.frag` |
| `CMakeLists.txt` | finds the Windows SDK's `dxc.exe`, compiles the four shaders to DXIL headers at build time, defines `LUNA_GPU`; `-DLUNA_GPU=OFF` or no `dxc.exe` leaves the GPU backend out |
| `src/luna/platform/window.{h,cpp}` | the Window keeps the OS window, events and gamepads and asks a backend to draw; `RendererChoice` (Auto, Gpu, Sdl); Auto falls back to SDL_Renderer with the reason logged; `backendName()` |
| `src/luna/engine/application.{h,cpp}`, `apps/odysseus/main.cpp` | `--renderer auto\|gpu\|sdl`; the log line "Window opened: ... renderer gpu (direct3d12)" |
| `docs/adr/ADR-021-sdl-gpu-renderer.md` | the decision, and the shader-compiler options with trade-offs |

Not changed: the `Renderer` interface and everything in `src/game/` (scenario "Game code": `git diff --stat qa -- src/game` is empty). Deviation from the Codex prompt, recorded in ADR-021: the GPU code is a backend inside Platform, not a `GpuRenderer` class in Engine, because the Engine layer may not include SDL (Charter rule 2); shaders therefore live in `src/luna/platform/shaders/` (Charter rule 11 allows Platform and Engine).

Technical choices (Dominus): the Windows SDK's `dxc.exe` is the shader compiler (ADR-021). Both backends draw through a virtual-screen texture, which made the pictures identical and changed the old path in one visible way: a sprite drawn at three quarters of its size (the children) now has even 2 x 2 pixel blocks instead of uneven ones. A `1/512` texel nudge in both backends settles pixels that fall exactly between two texels.

### Tests
| Scenario | Test |
|---|---|
| Same picture | `US-230 Same picture: the demo level and the camp are drawn the same by the GPU and by SDL_Renderer` (`tests/game/renderer_test.cpp`: each level after 60 ticks, 1280 x 720 window, 921,600 pixels compared: **0 different** for the demo level and the camp; the scene is also checked against itself); `US-230 Alpha, additive and scaled draws are identical on both renderers` (translucent, additive, three halves, three quarters, three times, stretched unevenly, half off the screen: 0 different); the real game's screenshots of both renderers are byte-identical files (`docs/evidence/US-230/`) |
| Fallback | `US-230 Fallback: a renderer that cannot start is replaced by SDL_Renderer, and Auto says which one it uses`; a build made with `-DLUNA_GPU=OFF` was configured, built and its window tests run: Auto logged "The GPU renderer cannot start (this build has no GPU backend ...); using SDL_Renderer instead" and played |
| Game code | no file of `src/game/` is in this story's diff |
| Crisp pixels | `US-230 Crisp pixels with each renderer`: the US-022 checkerboard test on 1920x1080, 1366x768 and 1280x720 with each renderer asked for by name |

Where there is no GPU renderer (CI runners) the pixel comparisons say so and pass; they run on the owner's PC and on any machine with Direct3D 12.

### Manual checks (owner, on your PC)
1. Start the game as before: the log (`Documents`-side `logs` folder) says `renderer gpu (direct3d12)`; the picture looks exactly as before.
2. `odysseus.exe --renderer sdl`: the old renderer; the picture is the same (the children now have even pixels).
3. `odysseus.exe --renderer gpu --screenshot a.bmp --quit-after 3` and the same with `sdl`: the two screenshots are the same picture.
4. Resize the window, go full screen (Settings) and back: both renderers keep crisp pixels and black bars.

Evidence: `docs/evidence/US-230/` (the demo level and the camp, GPU and SDL_Renderer; each pair is byte-identical).

### Notes for the next stories
- US-231 changes the virtual size to 960 x 540 and adds window modes and zoom; both backends read the size from `WindowSettings`.
- M8c adds light and shadow passes: more pipelines and render targets in `gpu_backend.cpp`, and the `Renderer` interface grows the calls it needs.

---

<a id="us-231"></a>

## Plan US-231: 960x540 presentation and window modes

Architect: Mraw (Solution Architect and QA). Prompt S-US-231 v2.4; requirements v2.6 ENV-18 and US-231; owner decisions D-42 and D-44. This is the implementation plan for this story only.

### Outcome and boundaries

Render a 960x540 virtual image through either existing backend, present it with Whole or Fill scaling, and apply and save the four windowed choices, borderless full screen and exclusive full screen from Settings. A missing settings file starts windowed at 1280x720 with Whole scaling; the saved defaults also include camera zoom 2x, UI scale 1x and lighting Medium.

US-232 owns the actual camera zoom, UI scale, mouse wheel and key controls, font scaling and pointer-to-world mapping. US-233 owns every screen and editor layout at the new size. US-231 changes the output and saved values, not those later behaviors. The existing Settings panel needs only enough layout work to expose and operate this story's controls; the full layout pass remains US-233.

### Current seams and concrete changes

| File | Change |
|---|---|
| `src/luna/engine/application.h` | `AppConfig` default virtual size becomes 960x540; initial resolution, mode and scaling pass through a small window configuration value. The current type is `AppConfig`, although the milestone design calls it `ApplicationSettings`. |
| `src/core/presentation.{h,cpp}` | Pure presentation maths returning a destination rectangle for Whole or Fill. Platform backends and Engine input mapping can both depend on Core without reversing a layer boundary. Whole uses the largest integer scale that fits; Fill preserves the 16:9 aspect ratio with fractional scale. For screens smaller than 960x540, a contained fractional fallback avoids clipping. The output rectangle is in drawable pixels. |
| `src/luna/platform/window.{h,cpp}` | Add a mode enum (Windowed, Borderless, Exclusive) and scaling mode to `WindowSettings`; apply the requested mode and windowed size with SDL3, sync the window, and report the actual drawable size. Select an available exclusive display mode deterministically, or report an actionable failure if none is compatible. OS calls stay inside Platform. |
| `src/luna/platform/backend.h`, `sdl_renderer_backend.cpp`, `gpu_backend.cpp` | Both backends get the same presentation policy and virtual dimensions. Each direct `wholeStepArea` decision is replaced by one shared rectangle. Keep nearest-neighbour sampling, black bars outside Whole, and identical screenshots for the same mode. |
| `src/luna/engine/game.h`, `application.cpp` | `WindowChange` grows from the fullscreen boolean to mode, size and scaling. One requested change is applied per tick, and pointer presentation is refreshed from the actual drawable size after changes and resizes. |
| `src/luna/engine/input.{h,cpp}` | Map high-DPI mouse coordinates through the exact presentation rectangle and virtual dimensions. The current integer-only `setPointerArea` cannot represent Fill's fractional scaling; use a ratio and reject black-bar coordinates. |
| `src/game/settings.{h,cpp}`, `odyssey_game.cpp`, `apps/odysseus/main.cpp` | Version 2 settings fields, defaults and migration; issue the expanded `WindowChange` whenever Settings changes. `volume` and `statistics` remain intact. Use the 960x540 virtual size in the game configuration. After `setSaveDirectory` has loaded the chosen file, pass its saved mode, size and scaling into `AppConfig` before `run` opens the window. |
| `src/game/run_flow.{h,cpp}` | Replace the fullscreen toggle and three sizes with the owner-decided mode and four size choices, plus Whole/Fill. A selection applies and saves immediately. Keep the Settings controls usable within the present panel until US-233's layout pass. |
| `docs/guides/` | Document each new settings field and show a complete version 2 example, including the mode and scaling names and first-start defaults. |

The common virtual-size constants live in Luna and feed `AppConfig` and `WindowSettings`. Existing tests may still pass explicit 480x270 values where they verify legacy renderer behavior; production defaults and the application path use 960x540.

### Settings format and migration

Version 2 object: `version: 2`, `resolution: {width, height, mode, scaling}`, `cameraZoom: 2`, `uiScale: 1`, `lighting: "Medium"`, plus existing `volume` and `statistics`. Every field round-trips. The four listed sizes are the selectable windowed choices; the last windowed size persists while in either full screen mode.

The current unversioned file is version 1: a valid `fullscreen: true` converts to borderless and false to windowed; a listed width and height is kept, otherwise 1280x720 is used when the old pair is outside the new choices; valid volume and statistics are kept. New fields receive D-44 defaults. Successfully migrated files are rewritten as version 2. Malformed data follows the existing repair-and-note behavior; an unsupported future version is reported, not silently downgraded.

The settings file belongs to Game because it also stores game preferences. Luna receives typed values through the existing `WindowChange` boundary. Startup loads it before the first visible frame so the selected mode, scaling and size appear immediately.

### Presentation cases

| Drawable area | Whole destination | Fill destination |
|---|---|---|
| 1920x1080 | 1920x1080 at 2x, no bars | 1920x1080 |
| 3840x2160 | 3840x2160 at 4x, no bars | 3840x2160 |
| 2560x1440 | 1920x1080 at (320,180), black bars | 2560x1440 at 2.666...x |
| 1280x720 | 960x540 at (160,90), black bars | 1280x720 |

Compute in drawable pixels, not window points, and recalculate after display, DPI, resize or mode changes. Pointer hit testing stays aligned with the exact rectangle used for drawing. Nearest sampling makes Whole pixel blocks uniform; Fill is the owner's optional use of all available screen area.

### Verification and evidence

1. Unit tests for the table above, letterbox centering on odd dimensions, very small windows, and pointer mapping at Whole and fractional Fill, including clicks on bars.
2. Settings tests: missing file, version 2 round trip of every field, version 1 migration from both fullscreen values, an old unsupported size, malformed fields, and preservation of volume/statistics.
3. Window integration tests on both backends for each mode and all four windowed choices, with actual drawable size and presentation rectangles checked. Headless tests cover the maths; display-dependent mode tests run on the owner's PC.
4. Run `pwsh tools/verify.ps1 -Story US-231`; record Debug warning count, test count and determinism result. After merge, confirm the qa Release CI result separately.
5. On the owner's PC, launch with no settings file, select each mode and windowed size, select Whole and Fill on 2560x1440, restart and confirm persistence. Capture GPU screenshots with `--renderer gpu --screenshot` at 1920x1080, 3840x2160 and 2560x1440; compare representative GPU and fallback pictures and save results in `docs/evidence/US-231/`. Record unavailable display modes explicitly rather than claiming a pass.

### Design review

D-44 resolves the window choices, scaling default, control location and first-start values. No new owner design decision is identified. The design document's broad note about removing old 480x270 literals applies to the virtual output path in this story; camera drawing and screen layouts move in US-232 and US-233 by their explicit prompts. The Drive requirements master sync noted in `Handover.md` remains a source-management gate for the assembly lead; this plan uses the reviewed local v2.6 content and does not change that gate.

---

<a id="us-232"></a>

## US-232 Camera zoom and UI scale: plan

**Design (D-44, ENV-18).** The game is laid out on a 960 x 540 screen with two whole-number scales on top:

- **Camera zoom** (1x or 2x, default 2x, so the world looks as it always did): the camera shows `960 / zoom` x `540 / zoom` world pixels around the hero (2x: 15 x 8.4 tiles, 1x: 30 x 17 tiles).
- **UI scale** (1x or 2x, default 1x): the interface is laid out in `960 / scale` x `540 / scale` of its own pixels.

**How (Engine).** `luna::engine::ScaledRenderer` wraps the real renderer and turns every draw into a whole-number block, so pixel art and the 5x7 font stay crisp. `OdysseyGame::render` draws the world through one ScaledRenderer (zoom) and the interface through another (UI scale). `Camera::setViewSize` changes the view and keeps its centre. `scaledPointer` maps the pointer into each picture's pixels; `update` makes a world pointer and a UI pointer before anything reads it, so aiming, right clicks and menus hit what they show (pointer mapping, US-121).

**Controls.** Settings screen: "Camera zoom" and "UI scale" buttons. In play: the keys + and - (also on the number pad) and the mouse wheel zoom; UI scale stays in Settings. Both are saved in `settings.json` (`cameraZoom`, `uiScale`).

**Tests.** `tests/luna/zoom_test.cpp` (scaled drawing, camera view, pointer at both zooms); `tests/game/aiming_test.cpp` ("US-232 The pointer hits the same world spot at any zoom", "US-232 Zoom keys and the wheel"). Older game tests run at zoom 1x (`setViewScales(1, 1)`) to keep world-relative pointer positions simple.

**Manual checks (owner's PC, GPU renderer).**
1. Start the game: the world looks as before (zoom 2x). `-` or scroll down: 30 x 17 tiles around the hero; `+` or scroll up: back.
2. Settings (Esc), UI scale 2x: HUD, hotbar and panels double in size, still crisp; back to 1x.
3. Aim and click at both zooms: the sword and spear go where the cursor is.

Screenshots (GPU renderer, `--screenshot`): `docs/evidence/US-232/zoom-2x.bmp`, `zoom-1x.bmp`, `ui-2x.bmp`.

**Not in this story.** The run screens and editors still use the old fixed layouts; US-233 lays them out for 960 x 540. The Editor ignores zoom (stays 1x) and uses the UI scale only for its labels.

---

<a id="us-233"></a>

## US-233 Every screen at the new size: plan

**Finding.** After US-231 and US-232 most of the game lays itself out from the interface size (`uiWidth()` x `uiHeight()` = 960 x 540 divided by the UI scale): HUD, hotbar, clan HUD, tutorial line, interaction panel, overlay and mode label. The run screens were the exception: `RunFlow` drew a fixed 420 x 250 panel at (30, 10) and a 480 x 270 shade. The Editor already anchors its toolbar, palette, properties and settings panels to the 960 x 540 screen size it is given.

**Change.** `RunFlow::build` now lays the panel out from the interface size: centred, at most 600 wide, as tall as its content (min 120, max 420), with the shade covering the whole interface. Paragraphs wrap to the panel width (`fitChars()`) instead of a fixed 62 characters. This covers New Game, Privacy, Focus, Event, Mantle, the menu (Bag, Skills, Dominion, Settings), Craft, Barter, the context menu, Talk (dialogue panel) and the end screen.

**Tests.** `US-233 Screens fit the interface at both UI scales` (menu_test.cpp): every menu tab at UI scale 1x and 2x keeps its widgets on screen and non-overlapping. Existing screenshot-style tests (draw counts, widget ids, text) pass unchanged. Old saves and levels are untouched: no format changed, and the determinism and save tests pass.

**Manual checks (owner's PC, GPU renderer).**
1. New Game: the panel is centred, text on three or four lines, nothing overlapping. UI scale 2x (Settings): same screen, larger and still crisp.
2. Open the menu (Esc), visit all four tabs; Settings shows zoom and UI scale rows inside the panel.
3. Talk to a clan member: the dialogue panel uses the width.
4. F2 Editor: toolbar, palettes and properties panels do not overlap.

Contact sheet for the owner's approval: `docs/evidence/US-233/contact-sheet.png` (HUD, Editor, Privacy and New Game at UI 1x and 2x); single screenshots are beside it.

**Not changed.** The Editor keeps its own 1x layout and ignores the UI scale (its panels are already placed from the 960 x 540 size); a UI-scaled Editor would be its own story.

---

<a id="us-234"></a>

## US-234 Frame budget at the new size: plan and method

**Target (D-06).** 60 FPS at 1080p on High lighting on the minimum PC: 6-core CPU, 16 GB, RX 6600 / RTX 3060 class GPU (8 GB), DirectX 12. Only the development PC (RX 7900 XTX, 32 GB, 6-core CPU) can be measured; the minimum PC's numbers are the measured ones scaled by a documented ratio.

**What was added.**
- F3 overlay, two lines: `FPS, frame ms, CPU: tick ms (worst), draw ms` and `GPU ms, people`. The tick is the simulation step; draw is the CPU time to record the frame.
- GPU time (Luna Platform, `gpu_backend.cpp`): SDL_GPU has no timestamp queries, so with timing on (F3 or `--perf`) `present()` lets the card finish the last frame (`SDL_WaitForGPUIdle`), sends the scene on its own and waits for its fence. The time is the card's work for the frame, not the wait for the monitor. This slows the frame a little, so it is off unless asked. The SDL renderer cannot measure it and shows `GPU n/a`.
- `--perf` (overlay on, GPU measured, a session-log line each minute with totals since the start) and `--people N` (the clan starts with N people; for performance runs, past the data file's limit of 200).

**Method.** Release build, GPU renderer, 1920x1080 window (`settings.json` in the save folder), camera zoom 2x, UI 1x, camp level, `--clan --people 500 --perf --quit-after 600`. The people stand at the camp, so all are on screen. Evidence: `docs/evidence/US-234/perf-run-10min.txt`.

**Result on the dev PC (10 minutes, 35,920 frames).** 59.9 FPS average (the monitor's 60 Hz is the ceiling), frame 16.71 ms average; 32 frames (0.09 %) over 20 ms. CPU per frame: draw 0.14 ms (max 0.7), simulation tick 0.12 to 0.43 ms average; GPU 0.20 ms average (max 4.4). The slow frames come from the daily autosave (a tick of 25 to 35 ms once per in-game day, 500 people), not from drawing.

**Scaling to the minimum PC (documented ratio).**
- GPU: the RX 6600 is about 3 to 3.5 times slower than the RX 7900 XTX in raster work; we use 4x: 0.20 ms becomes 0.8 ms (worst 4.4 ms becomes 18 ms, one frame in 36,000).
- CPU: same core count, mid-range clock; we use 2x: draw 0.28 ms, tick 0.3 ms.
- Total about 1.4 ms of a 16.7 ms frame, so 60 FPS holds with a wide margin. These are estimates, not measurements on that hardware; the first run on a real RX 6600 / RTX 3060 PC replaces them.

**Finding for later.** With 500 people the autosave stalls one frame (25 to 35 ms) at the end of each in-game day. Not part of this story; it can move to a background write when it matters (recorded in the Milestone file).

**Manual checks (owner's PC).** Press F3 in the game: two lines of CPU and GPU times appear; with the fallback renderer (`--renderer sdl`) the GPU shows n/a.
