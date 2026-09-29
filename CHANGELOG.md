# Changelog

Record every pull request's full change set here before opening or updating it.
Entries describe the final changes and their verification; update an entry when
its PR changes rather than leaving an outdated description.

## Luna Physics added to the requirements and the Codex (Dominus, Anima) — 2026-09-30

**State:** On `qa`. Owner decisions of 2026-09-30: Luna gets its own physics, core in the MVP, full 3D math, built right after M1.

- Requirements v1.5 (Drive, mirrored to `docs/project/requirements/`): PHY-01..PHY-06 (Luna Physics, hit detection, ballistics, rigid-body dynamics, element physics and chemistry, later-Age physics), ARC-10 (Physics layer), ARC-01 now six layers, ADR-016 and ADR-017 recorded, MVP-12 decided and MVP-13 added, architecture risk and cut-list rows, milestone M1b (9 weeks likely) with epic E10 and stories US-025..US-029; later milestones shifted 9 weeks (MVP likely 72 weeks, 49 stories).
- Backlog workbook: M1b in the timeline and Gantt, E10, US-025..US-029, decision statuses, MVP-13, kill-gate rows corrected to M2.
- Codex v1.4 (Anima): Charter rules 1, 3, 9 and new rule 10 (deterministic fixed-point physics, SI units, 1 tile = 1 m); K-M1b, S-US-025..S-US-029, X-M1b; P-003.
- Repo: `docs/adr/ADR-017-luna-physics.md`, ADR index, README layer tables, status (M1b next; US-011 paused on its branch), decisions (D-15), design-doc note.

## US-010 / S-US-010: Advance a seeded world clock (Claude) — 2026-09-30

**State:** Accepted; merged into `qa`. First story of M2 (console clan simulator).

- Core: `Pcg32` random streams, `Hasher` (FNV-1a 64).
- Simulation: `DataError`, JSON content loading (nlohmann-json 3.12, D-13), `Calendar` (2400 ticks/day, 7 days/season), `GameClock` (pause/1x/2x/4x), `World` (ticks, daily weather, `hash()`).
- `assets/data/sim/calendar.json`; `odysseus_headless --seed --days --data`.
- Tests: `odysseus_sim_tests` (new, Simulation identity): Calendar, Determinism, Speed control, data validation. `tools/verify.ps1`: the tester's standard build-and-test run with evidence.
- Verification: 0 warnings; ctest 13/13 Debug and Release.

## US-024 / S-US-024: Walk the character around the map (Claude) — 2026-09-30

**State:** Accepted; merged into `qa`. Completes M1 (Luna walking skeleton).

- Luna Engine: `moveAndCollide()` tile collision (flush stops, wall sliding); scripted input (`InputMap::setScripted`, `RunOptions::holds`).
- Game: `Hero` (8-way movement, 96 px/s, feet collision box, walking animation, idle facing the last direction, interpolated drawing); boulder on the east path; camera follows the hero.
- `odysseus.exe --hold <Intent>:<from>:<to>` scripted play; the final hero position is logged.
- Tests: `odysseus_game_tests` (new, Game identity: `US-024 Walk right`, `Stop at a rock`, `Stop and face the last direction`, diagonal speed), Luna collision tests, end to end `US-024 Walk to the rock` (label `window`).
- Verification: 0 warnings; ctest 12/12 Debug and Release; real window: hero stops at x 1142.0 facing East.

## US-023 / S-US-023: Show a tile map with a following camera (Claude) — 2026-09-30

**State:** Accepted; merged into `qa`.

- Luna Engine: `TileMap` (2D grid in one vector, solid tiles, `visibleTiles`, draws only what the camera sees), `Camera` (smooth follow, interpolation, whole pixels, clamped to the world).
- Game: the 64x64 test valley (`test_map.cpp`); the map drawn through the camera with the hero on top.
- Tests: `US-023 Only visible tiles are drawn`, `US-023 Camera follows and stops at the map edges`, TileMap grid test.
- Verification: 0 warnings; ctest 10/10 Debug and Release; screenshot `docs/evidence/US-023/game-map.png`.

## US-022 / S-US-022: Draw sprites with crisp pixels (Claude) — 2026-09-30

**State:** Accepted; merged into `qa`.

- Core: `Point`, `Rect`. Luna Platform: textures (nearest-neighbour, alpha), `drawTexture`, `presentationRect`, `outputRect`, `setSize`, `readPixels` (whole window), `saveScreenshot`, `WindowResized` event.
- Luna Engine: `Image`/`Color`, `Renderer` interface with `WindowRenderer` and `RecordingRenderer`, `integerScale()`; `Game::start(Renderer&)` and `render(Renderer&, alpha)`; pixel scale logged at start and on resize.
- Game: code-drawn placeholder art (hero 32x48 in 8 directions x 4 frames; grass, path, rock, water tiles 32x32); the hero drawn mid-screen.
- `odysseus.exe --screenshot <file.bmp>` saves the last frame.
- Tests: `US-022 Whole-number scale`, `luna_window_tests` (`US-022 Crisp pixels`, label `window`: real hidden window, pixel readback).
- Verification: 0 warnings; ctest 10/10 Debug and Release; 1920x1080 x4 with 0 wrong pixels; 1366x768 x2 letterboxed at (203, 114).

## US-021 / S-US-021: Control the game through intents (Claude) — 2026-09-30

**State:** Accepted; merged into `qa`.

- Luna Platform: Luna `Key` (physical positions), `GamepadButton`, `GamepadAxis` events; `translateEvent()` from SDL; gamepads opened/closed on plug/unplug.
- Luna Engine: `Intent`, `Intents` (held, pressed once, moveX/moveY), `InputMap` with default bindings (WASD/arrows, E/Space/Enter, Esc; stick with 0.3 dead zone, D-pad, South, Start); `Game::update(const Intents&)`.
- Tests: `luna_platform_tests` (new, Platform identity), `US-021 Default bindings`, `US-021 Gamepad`, `US-021 Game reads only intents` (automated review).
- Verification: 0 warnings; ctest 9/9 Debug and Release. No physical gamepad available: proven with synthetic SDL events.

## US-020 / S-US-020: Open a window with a steady game loop (Claude) — 2026-09-30

**State:** Accepted; merged into `qa`. First story of M1 (Luna engine).

- Luna Platform: `System` (SDL start/stop), `Window` (SDL_Window + SDL_Renderer, VSync, 480x270 integer-scaled virtual screen), clock, `requestQuit`, Luna events.
- Luna Engine: `FixedStepClock` (20 ticks/s, capped catch-up, interpolation alpha), `FrameStats`, `Game` interface, `run()` loop with logging.
- Game: `OdysseyGame` and its window settings. `odysseus.exe` opens the window; `--quit-after <s>`, `--log-dir <folder>`.
- Tests: `luna_tests` (Engine identity, `US-020 Steady`), end-to-end `US-020 Open and close` (label `window`).
- Docs: `docs/plans/M1-luna-design.md`, `docs/plans/US-020.md`, evidence, teach-back; delegated decisions D-16 (32x32 tiles), D-17 (8-way movement).
- Verification: 0 warnings; ctest 7/7 Debug and Release; 60-second run: 60.0 FPS, 1200 ticks, first frame 279 ms.

## US-004 / S-US-004: Log what happens and stop on broken assumptions (Claude) — 2026-09-30

**State:** Accepted; merged into `qa`.

- `src/core/log.{h,cpp}`: `LogSession` (RAII) writes `session-YYYYMMDD-HHMMSS-mmm-NN.log` with UTC-timestamped lines, keeps the last 5 session logs, never touches other files; `logInfo` / `logWarning` / `logError`.
- `src/core/assertions.{h,cpp}`: `ODYSSEUS_ASSERT(condition, message)` logs `Assertion failed: ... at file:line` and breaks into the debugger in Debug; compiles away in Release without unused-variable warnings.
- `src/luna/platform/user_paths.{h,cpp}`: `luna::platform::userDataDirectory()` via SDL3 `SDL_GetPrefPath` (Charter rule 2), SDL3 linked PRIVATE to Platform only.
- `vcpkg.json`: add `sdl3` (3.4.16, D-13). `CMakeLists.txt`: new sources, SDL3, `us004_assert_probe`.
- `apps/odysseus/main.cpp`: one log session per run in `%APPDATA%\Project Odyssey\Odysseus\logs`.
- Tests: `tests/core/log_test.cpp` (US-004 Log file, Rotation, Assert), `tests/core/assert_probe.cpp`.
- Docs: plan, evidence (`docs/evidence/US-004/`), teach-back, README "Logs".
- Verification: Debug and Release 0 warnings; ctest 5/5 both; US-004 doctest 3 cases / 24 assertions; end to end: 7 runs leave 5 logs; cdb stops at `assert_probe.cpp @ 13`.

## QA integration of ChatGPT's work (Claude) — 2026-09-30 — branch `qa`

**State:** Merged into `qa`; GitHub CI green on `qa` ([run 36635345962](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36635345962)). `qa` merges into `main` at the M0 exit review.

### Integration
- New branch `qa` from `main` @ `98e45ca`; ChatGPT's work recreated as `story/US-003` @ `f3c3d26` on its base `4488238` (all 90 uploaded files verified identical) and merged with `--no-ff`.
- `README.md`: merge conflict resolved; keeps main's layout table (Luna, `docs/project/`) plus ChatGPT's "Layer boundaries" section; lists `cmake/` and `tests/architecture/`.

### Fixes
- `tests/architecture/run_probe.cmake`: accept MSBuild's `fatal  error C1083` spelling (two spaces) by matching the error code. On Windows, 2 of 5 tests had failed although every forbidden include was rejected; no check was weakened.

### Verification (owner's PC, Visual Studio Community 2026, MSVC 19.51)
- Debug and Release builds: exit 0, 0 warning lines. ctest 5/5 in both, AddressSanitizer on in Debug. Evidence: `docs/evidence/US-003/windows-*.txt`.
- GitHub Actions (Windows runner) on `qa`: green, 5/5 in Debug and Release, 1 min 50 s.

### Documentation and tracking
- `docs/plans/US-003.md`, `docs/reports/US-003-2026-09-30.md`, `docs/learning-journal.md` (US-003 teach-back), `docs/status.md` (US-003 Done), ADR-016 status.
- `docs/decisions.md`: D-01 and D-03 Decided by the owner; standing owner instructions (delegated decisions, `qa` branch rule, Milestone-<n>.md after every story).
- `docs/README.md`: new index of the docs folder. `docs/codex-issues.md`: CI-005 for Anima.
- `Milestone-2.md`: progress snapshot AP-003.

### Clean-up
- Removed the ChatGPT upload folder, its identical zip and the duplicate local checkpoint (kept as `docs/reports/local-checkpoint-2026-09-29.md`).
- Removed `.gitkeep` placeholders in `src/game`, `src/sim`, `src/luna/engine`, `src/luna/platform` and `tools/`, which now contain files.

## US-003 / S-US-003: Enforce the layer rules in the build (ChatGPT) — 2026-09-29

**State:** Built by ChatGPT on local `story/US-003` (GitHub push refused, HTTP 403).
Imported unchanged as `f3c3d26` and merged into `qa` on 2026-09-30; Windows
verification and one test-harness fix in the QA entry above. **Done.**

### Build and source

- `CMakeLists.txt`: replace shared source-root includes with five layer targets;
  link only downward; identify consumers privately; route game/headless through
  Game/Simulation; register separate, serialized architecture CTest scenarios.
- `cmake/LayerRules.cmake`: expose each layer's own headers through a narrow
  forwarding include tree and run architecture validation on every build.
- `cmake/ValidateLayerIncludes.cmake`: check header boundary coverage and normalized
  include directions; restrict SDL3 to Platform and reject uncheckable macro includes.
- `src/core/boundary.h`, `src/core/version.h`: protect Core headers with a single
  source-layer identity check while keeping the existing version API.
- `src/luna/platform/{boundary.h,layer.h,layer.cpp}` and
  `src/luna/engine/{boundary.h,layer.h,layer.cpp}`: add empty Luna scaffolds with
  guards rejecting Simulation/Game dependencies and invalid consumers.
- `src/sim/{boundary.h,layer.h,layer.cpp}` and
  `src/game/{boundary.h,layer.h,layer.cpp}`: add empty, guarded Simulation/Game
  scaffolds; relative and absolute paths cannot bypass the include boundaries.

### Tests

- `tests/architecture/CMakeLists.txt`: 14 real compiler probes for five allowed
  edges and forbidden Simulation/Luna includes, including relative/absolute paths.
- `tests/architecture/layer_rules_test.cpp`: the three named acceptance scenarios
  plus guard completeness; quote diagnostic arguments safely in test commands.
- `tests/architecture/run_probe.cmake`: require the expected compiler/include
  diagnostic for rejected probes rather than accepting arbitrary build failures.
- `tests/architecture/run_validator.cmake`: clean controls and six violations
  injected after configure, proving validation runs on subsequent builds.
- `docs/evidence/US-003/`: preserve the tests-first baseline and supplementary
  Debug/Release test output.

### Documentation and tracking

- `docs/reports/local-checkpoint-2026-09-29.md`: save the local code location,
  resume point, current playability and outstanding owner requests.
- `docs/plans/US-003.md`: implementation plan, tests-first evidence, local results,
  pending acceptance/Windows checks and a teach-back draft awaiting acceptance.
- `docs/adr/ADR-016-layer-boundary-enforcement.md`, `docs/adr/README.md`: record
  the enforcement pattern and index it; no new project library was added.
- `README.md`: explain the five layer targets and architecture include checks.
- `docs/status.md`: keep US-003 Blocked by required Windows CI/integration access.
- `Milestone.md`: prepend AP-002 with unfinished US-003 and the exact resume point.
- `docs/reports/US-003-2026-09-29.md`: assembly report and acceptance limitations.
- `AGENTS.md`, `CHANGELOG.md`: persist the owner's requirement to track every PR's
  complete change set in this changelog.

### Verification

Supplementary GCC Debug and Release builds pass with `-Wall -Wextra -Werror`:
5 doctest cases, 17 assertions, 14 compiler probes and six validator rejection
checks in each configuration. Required MSVC Windows Debug/Release, AddressSanitizer,
Windows CTest and green CI on `main` are unverified. Completion is not accepted.
Determinism testing starts at US-010. No owner design decision is requested.


## Before this changelog existed — 2026-09-29 — `main`

| Commit | Change |
|---|---|
| `5cda584`, `b9a3794` | P-000: bootstrap the Mraw workspace; toolchain installed, D-12 decided |
| `ae799e9` | US-001: one CMake preset builds odysseus.exe, odysseus_headless.exe, odysseus_tests.exe with zero warnings |
| `e34e9c0` | US-002: GitHub Actions builds and tests every push |
| `1d172b0` | Codex sync from Google Drive at session start (`tools/sync-codex.ps1`) |
| `bb3e93a` | Dominus and Anima skills in `.claude/skills/` |
| `e7157d1` | Codex v1.2 from Anima: Luna engine first (requirements v1.4, ARC-09) |
| `2a31596` | P-001: adopt Codex v1.2 (Luna folders, new prompt order, D-04 and D-13) |
| `7393ca0`, `4488238` | Milestone.md progress snapshot AP-001; CI-004 |
| `98e45ca` | Project documents mirrored from Google Drive into `docs/project/` (`tools/sync-workspace.ps1`) |
