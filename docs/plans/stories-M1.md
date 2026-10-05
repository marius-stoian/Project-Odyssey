# Story plans: M1

Per-story plans for milestone M1, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-020](#us-020)
- [US-021](#us-021)
- [US-022](#us-022)
- [US-023](#us-023)
- [US-024](#us-024)

---

<a id="us-020"></a>

## Plan US-020: Open a window with a steady game loop

Codex v1.3, prompt S-US-020. Design: [M1 Luna design](M1-luna-design.md). Traces to ADR-002, ADR-006, TEC-05.

### Files
| File | Layer | What |
|---|---|---|
| `src/luna/platform/system.{h,cpp}` | Platform | `System` (RAII SDL start/stop), `nowNanoseconds`, `sleepNanoseconds`, `requestQuit` |
| `src/luna/platform/events.h` | Platform | Luna's own event type (Quit for now) |
| `src/luna/platform/window.{h,cpp}` | Platform | `Window`: SDL_Window + SDL_Renderer via unique_ptr with custom deleters; VSync; 480x270 integer-scaled virtual screen |
| `src/luna/engine/fixed_step_clock.{h,cpp}` | Engine | 20 ticks per second accumulator, capped catch-up, `alpha()` |
| `src/luna/engine/frame_stats.{h,cpp}` | Engine | Average frame rate |
| `src/luna/engine/game.h`, `application.{h,cpp}` | Engine | `Game` interface; `run()`: the loop, logging window size, first-frame time, average FPS |
| `src/game/odyssey_game.{h,cpp}` | Game | `OdysseyGame` and its window settings |
| `apps/odysseus/main.cpp` | App | Runs the game; `--quit-after`, `--log-dir` |
| `tests/luna/loop_test.cpp`, `tests/luna/run_game_window.cmake` | Tests | `luna_tests` (Engine identity); end-to-end window test |

### Tests
| Scenario | Test |
|---|---|
| Open | `US-020 Open and close` (ctest, label `window`): runs `odysseus.exe --quit-after 3`, requires "Window opened: 1280x720" and "First frame after N ms" with N < 3000 |
| Steady | `US-020 Steady` (doctest): 60 s of frames at 30, 60 and 144 Hz all give 1199-1200 ticks; stalls do not snowball. Manual: 60-second run, average FPS from the log |
| Close | Same end-to-end test: `--quit-after` pushes the same quit event as the close button; exit code 0, "Window closed by the player", no `[ERROR]` in the log |

### Verification results (2026-09-30)
| Check | Result | Evidence |
|---|---|---|
| Debug and Release builds | Pass, 0 warning lines | build logs |
| ctest Debug and Release | 7/7 both | [windows-debug.txt](../evidence/US-020/windows-debug.txt), [windows-release.txt](../evidence/US-020/windows-release.txt) |
| Open | Pass: window 1280x720, first frame after 279 ms (Release) | [fps-60-seconds.txt](../evidence/US-020/fps-60-seconds.txt) |
| Steady | Pass: **60.0 FPS over 60.0 s (3599 frames), 1200 simulation ticks** on the development PC (60 Hz); unit test proves equal speed at 30/60/144 Hz | same, `US-020 Steady` |
| Close | Pass: clean shutdown, no error in the log | same |

Acceptor verdict (2026-09-30): **ACCEPT**. Manual close-button click by a person is not part of the automated evidence; `--quit-after` uses the identical quit event.

---

<a id="us-021"></a>

## Plan US-021: Control the game through intents

Codex v1.3, prompt S-US-021. Design: [M1 Luna design](M1-luna-design.md). Traces to ARC-03.

### Flow
SDL event -> `luna::platform::translateEvent` (Platform, the only code that knows SDL) -> Luna `Event` (Key, GamepadButton, GamepadAxis) -> `luna::engine::InputMap` (bindings) -> `Intents` for one tick -> `Game::update(const Intents&)`. Game code never sees a key or a button.

### Files
| File | Layer | What |
|---|---|---|
| `src/luna/platform/events.h` | Platform | Luna's Key (physical positions, so WASD works on AZERTY/QWERTZ), GamepadButton (by position: South = Xbox A), GamepadAxis |
| `src/luna/platform/sdl_events.{h,cpp}` | Platform | `translateEvent(SDL_Event)`; stick values scaled to -1..+1 |
| `src/luna/platform/window.{h,cpp}` | Platform | Opens gamepads when plugged in, closes them when removed (RAII) |
| `src/luna/engine/input.{h,cpp}` | Engine | `Intent`, `Intents` (held, pressed once, moveX/moveY), `InputMap` with default bindings and a 0.3 stick dead zone |
| `src/luna/engine/game.h`, `application.cpp` | Engine | `update(const Intents&)`; the loop feeds events to InputMap, logs gamepad connect/disconnect |

### Tests
| Scenario | Test |
|---|---|
| Given the default bindings, when I press W, then Move Up | `US-021 SDL events become Luna events` (SDL W -> Key::W) + `US-021 Default bindings` (Key::W -> MoveUp held and pressed; W and Up share the intent; key repeat ignored) |
| Given a gamepad, when I push the left stick up, then the same Move Up | `US-021 SDL events become Luna events` (SDL LeftY -32768 -> LeftY -1.0) + `US-021 Gamepad` (LeftY -1 -> MoveUp; dead zone; D-pad; unplugging releases) |
| Given the Game layer code, when reviewed, then only intents | `US-021 Game reads only intents`: automated review of `src/game/` and `apps/odysseus/` for SDL, keys, buttons, axes and input headers |

### Verification results (2026-09-30)
| Check | Result | Evidence |
|---|---|---|
| Builds | Debug and Release, 0 warning lines | build logs |
| ctest | 9/9 Debug and Release | [windows-debug.txt](../evidence/US-021/windows-debug.txt), [windows-release.txt](../evidence/US-021/windows-release.txt) |
| All three scenarios | Pass | same |

Limitation: no physical gamepad was connected during verification; the gamepad path is proven with synthetic SDL events through the same translation and bindings. Acceptor verdict (2026-09-30): **ACCEPT**.

---

<a id="us-022"></a>

## Plan US-022: Draw sprites with crisp pixels

Codex v1.3, prompt S-US-022. Design: [M1 Luna design](M1-luna-design.md). Traces to ENV-11, ADR-003. Owner decision D-04 (32x48 px characters, 8 directions).

### Files
| File | Layer | What |
|---|---|---|
| `src/core/geometry.h` | Core | `Point`, `Rect` (whole pixels) |
| `src/luna/platform/window.{h,cpp}`, `events.h`, `sdl_events.cpp` | Platform | Textures (RGBA, nearest-neighbour, alpha blending), `drawTexture`, `presentationRect`, `setSize`, `outputRect`, `readPixels` (whole window, bars included), `saveScreenshot`; `WindowResized` event |
| `src/luna/engine/image.h` | Engine | `Image` (RGBA pixels in memory), `Color` |
| `src/luna/engine/renderer.{h,cpp}` | Engine | `Renderer` interface (ADR-003), `WindowRenderer`, `RecordingRenderer` (tests), `integerScale()` |
| `src/luna/engine/game.h`, `application.{h,cpp}` | Engine | `Game::start(Renderer&)`, `render(Renderer&, alpha)`; logs the pixel scale at start and on every resize; `--screenshot` support |
| `src/game/placeholder_art.{h,cpp}`, `odyssey_game.{h,cpp}` | Game | Code-drawn character sheet (32x48, 8 directions x 4 frames) and tiles (32x32); the hero drawn mid-screen |

### Tests
| Scenario | Test |
|---|---|
| 480x270 at 1920x1080 scales by exactly 4, no blurred pixels | `US-022 Whole-number scale` (math) + `US-022 Crisp pixels` (label `window`): a hidden 1920x1080 window, an 8x8 checkerboard of single art pixels, screen read back: every art pixel is an exact 4x4 block of its own colour |
| Not an exact multiple: largest whole-number scale, letterboxed | Same tests at 1366x768: scale 2, picture 960x540 at (203, 114), black bars around it |

### Verification results (2026-09-30)
| Check | Result | Evidence |
|---|---|---|
| Builds | Debug and Release, 0 warning lines | build logs |
| ctest | 10/10 Debug and Release | [windows-debug.txt](../evidence/US-022/windows-debug.txt), [windows-release.txt](../evidence/US-022/windows-release.txt) |
| 1920x1080 | Pass: scale x4, 1024 screen pixels checked, 0 wrong | [pixel-readback.txt](../evidence/US-022/pixel-readback.txt) |
| 1366x768 | Pass: scale x2, picture at (203, 114), 256 pixels checked, 0 wrong, bars black | same |
| The game | The hero sprite, crisp, at the default 1280x720 window (x2, letterboxed) | [game-1280x720.png](../evidence/US-022/game-1280x720.png) |

Found and fixed during testing: SDL reads back only the picture area when the virtual screen is on; `readPixels` now switches it off for the read so the black bars can be checked too.

Acceptor verdict (2026-09-30): **ACCEPT**.

---

<a id="us-023"></a>

## Plan US-023: Show a tile map with a following camera

Codex v1.3, prompt S-US-023. Design: [M1 Luna design](M1-luna-design.md). Traces to architecture 7.4. Delegated decision D-16 (32x32 tiles).

### Files
| File | Layer | What |
|---|---|---|
| `src/luna/engine/tile_map.{h,cpp}` | Engine | `TileMap`: a 2D grid in one vector (index `y * width + x`), solid tiles (outside the map is solid), `visibleTiles(view)`, `draw()` that draws only visible tiles |
| `src/luna/engine/camera.{h,cpp}` | Engine | `Camera`: follows a target a fraction (0.25) of the way per tick, interpolated between ticks, whole-pixel view, clamped to the world |
| `src/game/test_map.{h,cpp}` | Game | The 64x64 test valley: grass, crossing paths, a pond, scattered rocks (rocks and water solid) |
| `src/game/odyssey_game.{h,cpp}` | Game | Draws the map through the camera, the hero on top; the camera follows the hero |

### Tests
| Scenario | Test |
|---|---|
| Given a 64x64 test map, tiles within the camera view are drawn and tiles outside are skipped | `US-023 Only visible tiles are drawn`: `RecordingRenderer` counts draws: 15x9 when grid-aligned, 16x9 between tiles, all on screen, fewer than 5% of the map |
| Given the character near the centre, when it moves, the camera follows smoothly and stops at the map edges | `US-023 Camera follows and stops at the map edges`: moves part of the way each tick, never backwards or past the target, settles exactly; blended between ticks; clamped at both corners |

### Verification results (2026-09-30)
| Check | Result | Evidence |
|---|---|---|
| Builds | Debug and Release, 0 warning lines | build logs |
| ctest | 10/10 Debug and Release | [windows-debug.txt](../evidence/US-023/windows-debug.txt), [windows-release.txt](../evidence/US-023/windows-release.txt) |
| Scenarios | Pass | [doctest-us023.txt](../evidence/US-023/doctest-us023.txt) |
| The game | The test valley around the hero | [game-map.png](../evidence/US-023/game-map.png) |

Acceptor verdict (2026-09-30): **ACCEPT**. The camera follows a moving character in the game once US-024 makes the hero walk; the behaviour itself is proven here.

---

<a id="us-024"></a>

## Plan US-024: Walk the character around the map

Codex v1.3, prompt S-US-024. Design: [M1 Luna design](M1-luna-design.md). Traces to ENV-11. Owner decision D-04 (32x48, 8 directions); delegated D-17 (8-way movement, same speed diagonally).

### Files
| File | Layer | What |
|---|---|---|
| `src/luna/engine/collision.{h,cpp}` | Engine | `Box`, `moveAndCollide()`: stops flush against solid tiles and the map edge, one axis at a time (slides along walls) |
| `src/luna/engine/input.{h,cpp}`, `application.{h,cpp}` | Engine | Scripted input (`setScripted`, `ScriptedHold`) for automated end-to-end tests |
| `src/game/hero.{h,cpp}` | Game | `Hero`: 8-way movement at `HeroConfig::speedPixelsPerSecond` (96 px/s = 3 tiles/s), feet collision box 20x10, walking animation (8 frames/s, 4 frames), idle facing the last direction, interpolated drawing |
| `src/game/odyssey_game.{h,cpp}`, `test_map.cpp` | Game | The hero walks the test valley; a boulder on the east path; the camera follows the hero |
| `apps/odysseus/main.cpp` | App | `--hold <Intent>:<from>:<to>`; logs where the hero ended |

### Tests
| Scenario | Test |
|---|---|
| Given idle, when I hold Move Right, it walks right with a walking animation at the configured speed | `US-024 Walk right`: 1 s of Move Right moves exactly 96 px right, faces East, all 4 walking frames shown |
| Given a rock to the right, when I walk into it, it stops at the tile edge | `US-024 Stop at a rock` (feet box flush with the rock's left edge, no creeping) + end to end `US-024 Walk to the rock` in the real window |
| Given walking, when I release all movement input, it stops and plays idle facing the last direction | `US-024 Stop and face the last direction` |
| D-17 | `Diagonal walking keeps the same speed`; `Collision stops boxes flush against solid tiles` (Luna) |

### Verification results (2026-09-30)
| Check | Result | Evidence |
|---|---|---|
| Builds | Debug and Release, 0 warning lines | build logs |
| ctest | 12/12 Debug and Release | [windows-debug.txt](../evidence/US-024/windows-debug.txt), [windows-release.txt](../evidence/US-024/windows-release.txt) |
| Unit scenarios | Pass | [doctest-hero.txt](../evidence/US-024/doctest-hero.txt) |
| End to end, real window | Hold Move Right 3 s: the hero walks, the camera follows, the hero stops at x 1142.0 (flush with the boulder at x 1152) and is idle facing East | [walked-to-rock.png](../evidence/US-024/walked-to-rock.png) |

Found and fixed during testing: a pattern in the end-to-end script lost its escaping (`\[ERROR\]` read as "any of E, R, O"); corrected.

Acceptor verdict (2026-09-30): **ACCEPT**.
