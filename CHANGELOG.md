# Changelog

Record every pull request's full change set here before opening or updating it.
Entries describe the final changes and their verification; update an entry when
its PR changes rather than leaving an outdated description.

## US-015 / S-US-015: Soak-test the simulation from the command line (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`.

- Simulation: `report.{h,cpp}` (population, deaths by cause, average needs, food, couples, feuds, mammoths, chronicle size); the mammoth herd passes once a year.
- `odysseus_headless`: `--years`, `--help`, strict number parsing (`std::from_chars`), usage message and exit code 2 on bad input, report and tick time.
- Tests: `US-015 Run` and `US-015 Bad input` (ctest, the real program), `US-015 The report adds up`.
- Evidence: 100-year soak for seed 7 in Release and Debug (same world hash), bad-input output.
- Docs: plan `docs/plans/US-015.md`, teach-back entry.
- Verification: 0 warnings; ctest 19/19 Debug and Release.

## US-014 / S-US-014: Write a readable chronicle (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`.

- Simulation: chronicle importance levels, `select(year, threshold)`, `formatEntry`; life events (pairing with courting, conception, pregnancy, births with inherited traits, childbirth, old age, grief, feuds and peace, first mammoth, empty-store evenings); names never repeat without an ordinal; carrying capacity (daily forage and game budgets) and cumulative hunger damage.
- Data: `assets/data/sim/life.json` (new); `actions.json` (forage and game budgets, rarer mammoths).
- `odysseus_headless --chronicle [year] --threshold <n>`.
- Tests: `US-014 Record`, `US-014 Filter`, generations.
- Docs: plan `docs/plans/US-014.md` (with the balance notes), D-02 tuning note, teach-back entry; evidence: a century's chronicle for seed 42.
- Verification: 0 warnings; ctest 17/17 Debug and Release.

## US-013 / S-US-013: Remember events and spread gossip (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`.

- Data: `assets/data/sim/social.json` (new).
- Simulation: `memory.{h,cpp}` (memories, social config, forgetting, memory limit); people keep memories, opinions and last gift and theft days; new actions GiveGift and Steal; `World::giveGift`, `recordTheft`, `talk` (gossip at half strength), favourite partners by opinion, daily forgetting; memories and opinions in the world hash.
- Tests: `US-013 Memory`, `US-013 Gossip`, `US-013 Forgetting`, memory limit, a living clan's year.
- Docs: plan `docs/plans/US-013.md`, teach-back entry.
- Verification: 0 warnings; ctest 17/17 Debug and Release.

## US-012 / S-US-012: Let people choose what to do (utility AI) (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`.

- Data: `assets/data/sim/actions.json` (new); `needs.json` keeps decay, meal and death rules.
- Simulation: `actions.{h,cpp}`, `ai.{h,cpp}` (availability, scores, decision with seeded tie-break, printable decisions); traits, skills, current action and last decision on `Person`; founders get traits and skills; the World runs hourly decisions and action effects (food store, hunting with rare mammoths, sleep, fire, talk, rest, practice), the evening meal and daily spoilage; `setDailyLife(false)` for needs-only tests.
- `odysseus_headless --inspect <name or id>`; population and food printed.
- Tests: `US-012 Pick best action`, `US-012 No option`, `US-012 Inspectable`, first-year survival; US-011 tests run with daily life off.
- Docs: plan `docs/plans/US-012.md`, teach-back entry.
- Verification: 0 warnings; ctest 17/17 Debug and Release.

## US-011 / S-US-011: Give every person needs that change over time (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. Second story of M2 (paused during M1b, resumed on its branch).

- Data: `assets/data/sim/needs.json`, `clan.json`, `names.json`.
- Simulation: `needs.{h,cpp}` (hourly decay adding up exactly to the daily rates, winter Warmth, capped satisfaction), `person.{h,cpp}`, `clan.{h,cpp}` (founders from data, names), `chronicle.{h,cpp}`; `World` holds the clan, the food store and the chronicle, decays needs every game hour, ages people and applies starvation and winter-cold deaths each morning; the world hash covers them; `calendar`: `kHoursPerDay`, `ticksPerHour()`, day length must split into hours.
- Tests: `US-011 Decay`, `US-011 Satisfaction`, `US-011 Consequence`, starting clan from data.
- Docs: plan `docs/plans/US-011.md`, teach-back entry.
- Verification: 0 warnings; ctest 17/17 Debug and Release.

## X-M1b: Exit review M1b, Luna Physics (Mraw) — 2026-09-30

**State:** On `qa`; merged into `main` and tagged `m1b-done` once CI on `main` is green.

- `docs/gates/M1b.md`: every exit criterion met, with the textbook comparisons (range, flight time, drag, drift, bounce heights, friction distance) and the in-flight screenshot.
- New gate test `M1b Whole physics is identical on every build` (`tests/physics/determinism_test.cpp`): 12 throws with drag and wind, a bouncing and sliding ball for 400 ticks and a 1,000-body contact step, hashed and pinned (17309765312882650619) so Debug, Release and CI must agree.
- Evidence: `docs/evidence/M1b/` (physics test output in both builds, ctest logs, screenshot).
- Verification: 0 warnings; ctest 17/17 Debug and Release; luna_physics_tests 19 cases, 404,518 assertions.

## US-029 / S-US-029: Throw a spear in the demo (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. Completes the M1b stories (Luna Physics).

- Luna Physics: `Material`, `massOf`, `kineticEnergy`; `flyTick` skips obstacles outside the path box.
- Luna Engine: `physics_view.{h,cpp}` (metres to pixels, top-down position lifted by height, ground shadow, on-screen direction).
- Data: `assets/data/materials.json` (flint, wood, straw, stone; damage scale; flint and wooden spears).
- Game: `materials.{h,cpp}` (validated loading, `impactDamage`), `spear_range.{h,cpp}` (boulders from rock tiles, ground, straw targets, auto-aimed throws, swept flight, sticking spears), prop art (spears in 8 directions, target, shadow), `OdysseyGame` (Interact throws alternating flint and wooden spears, two targets, a camera that frames the throw, hit logging), a boulder on the north path; `odysseus.exe` reads data from `ODYSSEUS_DATA_DIR` and logs target totals.
- Tests: `US-029 Throw`, `US-029 Material`, `US-029 Materials are validated`, `US-029 Blocked` (game tests); `US-029 Throw in the game` (real window, label `window`).
- Evidence: `docs/evidence/US-029/spear-in-flight.png`, `spear-hit.png`, game log.
- Verification: 0 warnings; ctest 17/17 Debug and Release.

## US-028 / S-US-028: Push and bounce bodies (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`.

- Luna Physics: `rigid_body.{h,cpp}`: `SurfaceMaterial` and combining rules, `Ground`, `RigidBody` (invariants checked in the constructor; impulses, forces, exact constant-acceleration flight, impact times inside a step, restitution and friction impulses, Coulomb sliding, rest and sleep, wake on push).
- Tests: `US-028 Impulse` (70 kg, 140 N s -> 2 m/s), `US-028 Bounce and rest` (height ratios 0.25 = e^2, then asleep), `US-028 Friction` (stops at v^2/(2 mu g) = 1.226 m).
- Docs: plan `docs/plans/US-028.md`, teach-back entry.
- Verification: 0 warnings; ctest 16/16 Debug and Release.

## US-027 / S-US-027: Fly projectiles with real ballistics (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`.

- Luna Physics: `atan2`; `ballistics.{h,cpp}`: `Air` (density, wind, gravity), `Projectile` (mass, Cd*A), quadratic drag against the air's motion, semi-implicit Euler at 10 sub-steps per tick, `flyTick` (swept collisions per sub-step), `flyUntilLanding`, `launchAngleWithoutDrag` (textbook low arc), `aimLaunchAngle` (secant refinement with drag), `launchVelocity`.
- Tests: `US-027 Arc` (40.704 m vs v^2/g = 40.775 m), `US-027 Drag and wind` (within 1% of an independent Runge-Kutta solution of the drag equation; 1.20 m drift in a 5 m/s crosswind), `US-027 Aim` (25 m target hit after 33 ticks), atan2 accuracy.
- Docs: plan `docs/plans/US-027.md`, teach-back entry.
- Verification: 0 warnings; ctest 16/16 Debug and Release.

## US-026 / S-US-026: Detect hits between shapes (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`.

- Luna Physics: `Sphere`, `Capsule`, `Box`, `Shape` (variant), `bounds()`, `overlap()` for all six pairings (contact point, normal, depth), `raycast()`, `sweep()` of a moving sphere (Minkowski sum; exact rounded box corners), closest-point helpers; `SpatialGrid` (2 m cells, sorted unique candidate pairs) and `findContacts()`.
- Tests: `US-026 Overlap`, `US-026 No tunnelling` (10 m per tick, 0.2 m target, time of impact 0.498 of a tick), `US-026 Many bodies` (1,000 bodies, 114 pairs tested, same contacts as all pairs, 0.61 ms in Release), every shape pairing, rays and rounded corners.
- Docs: plan `docs/plans/US-026.md`, teach-back entry.
- Verification: 0 warnings; ctest 16/16 Debug and Release.

## US-025 / S-US-025: Build deterministic 3D math (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. First story of M1b (Luna Physics).

- New layer Luna Physics (`src/luna/physics/`, target `luna_physics`, ARC-10): `Fixed` 32.32 numbers (own 128-bit multiply and long division, rounded to nearest, overflow asserted in Debug), `sqrt`, `sin`, `cos`, `degrees`; `Vec3` (dot, cross, length, normalise); `Quat` (axis-angle, product, conjugate, rotate, normalise). No floating point inside the layer.
- Layer enforcement (ADR-016 update): all six `boundary.h` know the Physics identity; the include validator's table; Engine and Simulation link Physics.
- Tests: `luna_physics_tests` (new, Physics identity): `US-025 Exact arithmetic` (+ 100,000 pairs against the CPU's 128-bit instructions), `US-025 Rotations`, `US-025 Determinism` (1,000,000 operations, pinned hash), sine/cosine accuracy, vectors; `US-025 Physics layer rules` (11 compiler probes); 4 more validator fixtures; `US-025 Physics uses no floating point` (source review).
- Docs: plan `docs/plans/US-025.md`, ADR-016 update, README, teach-back entry.
- Verification: 0 warnings; ctest 16/16 Debug and Release.

## Codex v1.5 and K-M1b (Anima, Mraw) — 2026-09-30

**State:** On `qa`.

- Codex v1.5 (Anima): Limit.md and `tools/verify.ps1` are state files; L-01 continues paused story branches, verifies with `verify.ps1`, updates Limit.md; the Charter says how to stop safely at usage limits; section 0: Dominus designs, implements and tests, Anima alone writes the Codex; continuous assembly; P-004.
- K-M1b: `docs/plans/M1b-physics-design.md` (fixed-point 32.32 with portable 128-bit arithmetic, Vec3 and quaternions, shapes and swept tests, spatial grid, integrator, ballistics and aim solver, rigid bodies, materials, top-down drawing of 3D).
- Session end: Milestone-10.md (AP-011), Limit.md points the next chat at S-US-025.

## Luna Physics added to the requirements and the Codex (Dominus, Anima) — 2026-09-30

**State:** On `qa`. Owner decisions of 2026-09-30: Luna gets its own physics, core in the MVP, full 3D math, built right after M1.

- Requirements v1.5 (Drive, mirrored to `docs/project/requirements/`): PHY-01..PHY-06 (Luna Physics, hit detection, ballistics, rigid-body dynamics, element physics and chemistry, later-Age physics), ARC-10 (Physics layer), ARC-01 now six layers, ADR-016 and ADR-017 recorded, MVP-12 decided and MVP-13 added, architecture risk and cut-list rows, milestone M1b (9 weeks likely) with epic E10 and stories US-025..US-029; later milestones shifted 9 weeks (MVP likely 72 weeks, 49 stories).
- Backlog workbook: M1b in the timeline and Gantt, E10, US-025..US-029, decision statuses, MVP-13, kill-gate rows corrected to M2.
- Codex v1.4 (Anima): Charter rules 1, 3, 9 and new rule 10 (deterministic fixed-point physics, SI units, 1 tile = 1 m); K-M1b, S-US-025..S-US-029, X-M1b; P-003.
- Repo: `docs/adr/ADR-017-luna-physics.md`, ADR index, README layer tables, status (M1b next; US-011 paused on its branch), decisions (D-15), design-doc note.

## US-010 / S-US-010: Advance a seeded world clock (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. First story of M2 (console clan simulator).

- Core: `Pcg32` random streams, `Hasher` (FNV-1a 64).
- Simulation: `DataError`, JSON content loading (nlohmann-json 3.12, D-13), `Calendar` (2400 ticks/day, 7 days/season), `GameClock` (pause/1x/2x/4x), `World` (ticks, daily weather, `hash()`).
- `assets/data/sim/calendar.json`; `odysseus_headless --seed --days --data`.
- Tests: `odysseus_sim_tests` (new, Simulation identity): Calendar, Determinism, Speed control, data validation. `tools/verify.ps1`: the tester's standard build-and-test run with evidence.
- Verification: 0 warnings; ctest 13/13 Debug and Release.

## US-024 / S-US-024: Walk the character around the map (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. Completes M1 (Luna walking skeleton).

- Luna Engine: `moveAndCollide()` tile collision (flush stops, wall sliding); scripted input (`InputMap::setScripted`, `RunOptions::holds`).
- Game: `Hero` (8-way movement, 96 px/s, feet collision box, walking animation, idle facing the last direction, interpolated drawing); boulder on the east path; camera follows the hero.
- `odysseus.exe --hold <Intent>:<from>:<to>` scripted play; the final hero position is logged.
- Tests: `odysseus_game_tests` (new, Game identity: `US-024 Walk right`, `Stop at a rock`, `Stop and face the last direction`, diagonal speed), Luna collision tests, end to end `US-024 Walk to the rock` (label `window`).
- Verification: 0 warnings; ctest 12/12 Debug and Release; real window: hero stops at x 1142.0 facing East.

## US-023 / S-US-023: Show a tile map with a following camera (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`.

- Luna Engine: `TileMap` (2D grid in one vector, solid tiles, `visibleTiles`, draws only what the camera sees), `Camera` (smooth follow, interpolation, whole pixels, clamped to the world).
- Game: the 64x64 test valley (`test_map.cpp`); the map drawn through the camera with the hero on top.
- Tests: `US-023 Only visible tiles are drawn`, `US-023 Camera follows and stops at the map edges`, TileMap grid test.
- Verification: 0 warnings; ctest 10/10 Debug and Release; screenshot `docs/evidence/US-023/game-map.png`.

## US-022 / S-US-022: Draw sprites with crisp pixels (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`.

- Core: `Point`, `Rect`. Luna Platform: textures (nearest-neighbour, alpha), `drawTexture`, `presentationRect`, `outputRect`, `setSize`, `readPixels` (whole window), `saveScreenshot`, `WindowResized` event.
- Luna Engine: `Image`/`Color`, `Renderer` interface with `WindowRenderer` and `RecordingRenderer`, `integerScale()`; `Game::start(Renderer&)` and `render(Renderer&, alpha)`; pixel scale logged at start and on resize.
- Game: code-drawn placeholder art (hero 32x48 in 8 directions x 4 frames; grass, path, rock, water tiles 32x32); the hero drawn mid-screen.
- `odysseus.exe --screenshot <file.bmp>` saves the last frame.
- Tests: `US-022 Whole-number scale`, `luna_window_tests` (`US-022 Crisp pixels`, label `window`: real hidden window, pixel readback).
- Verification: 0 warnings; ctest 10/10 Debug and Release; 1920x1080 x4 with 0 wrong pixels; 1366x768 x2 letterboxed at (203, 114).

## US-021 / S-US-021: Control the game through intents (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`.

- Luna Platform: Luna `Key` (physical positions), `GamepadButton`, `GamepadAxis` events; `translateEvent()` from SDL; gamepads opened/closed on plug/unplug.
- Luna Engine: `Intent`, `Intents` (held, pressed once, moveX/moveY), `InputMap` with default bindings (WASD/arrows, E/Space/Enter, Esc; stick with 0.3 dead zone, D-pad, South, Start); `Game::update(const Intents&)`.
- Tests: `luna_platform_tests` (new, Platform identity), `US-021 Default bindings`, `US-021 Gamepad`, `US-021 Game reads only intents` (automated review).
- Verification: 0 warnings; ctest 9/9 Debug and Release. No physical gamepad available: proven with synthetic SDL events.

## US-020 / S-US-020: Open a window with a steady game loop (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. First story of M1 (Luna engine).

- Luna Platform: `System` (SDL start/stop), `Window` (SDL_Window + SDL_Renderer, VSync, 480x270 integer-scaled virtual screen), clock, `requestQuit`, Luna events.
- Luna Engine: `FixedStepClock` (20 ticks/s, capped catch-up, interpolation alpha), `FrameStats`, `Game` interface, `run()` loop with logging.
- Game: `OdysseyGame` and its window settings. `odysseus.exe` opens the window; `--quit-after <s>`, `--log-dir <folder>`.
- Tests: `luna_tests` (Engine identity, `US-020 Steady`), end-to-end `US-020 Open and close` (label `window`).
- Docs: `docs/plans/M1-luna-design.md`, `docs/plans/US-020.md`, evidence, teach-back; delegated decisions D-16 (32x32 tiles), D-17 (8-way movement).
- Verification: 0 warnings; ctest 7/7 Debug and Release; 60-second run: 60.0 FPS, 1200 ticks, first frame 279 ms.

## US-004 / S-US-004: Log what happens and stop on broken assumptions (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`.

- `src/core/log.{h,cpp}`: `LogSession` (RAII) writes `session-YYYYMMDD-HHMMSS-mmm-NN.log` with UTC-timestamped lines, keeps the last 5 session logs, never touches other files; `logInfo` / `logWarning` / `logError`.
- `src/core/assertions.{h,cpp}`: `ODYSSEUS_ASSERT(condition, message)` logs `Assertion failed: ... at file:line` and breaks into the debugger in Debug; compiles away in Release without unused-variable warnings.
- `src/luna/platform/user_paths.{h,cpp}`: `luna::platform::userDataDirectory()` via SDL3 `SDL_GetPrefPath` (Charter rule 2), SDL3 linked PRIVATE to Platform only.
- `vcpkg.json`: add `sdl3` (3.4.16, D-13). `CMakeLists.txt`: new sources, SDL3, `us004_assert_probe`.
- `apps/odysseus/main.cpp`: one log session per run in `%APPDATA%\Project Odyssey\Odysseus\logs`.
- Tests: `tests/core/log_test.cpp` (US-004 Log file, Rotation, Assert), `tests/core/assert_probe.cpp`.
- Docs: plan, evidence (`docs/evidence/US-004/`), teach-back, README "Logs".
- Verification: Debug and Release 0 warnings; ctest 5/5 both; US-004 doctest 3 cases / 24 assertions; end to end: 7 runs leave 5 logs; cdb stops at `assert_probe.cpp @ 13`.

## QA integration of ChatGPT's work (Mraw) — 2026-09-30 — branch `qa`

**State:** Merged into `qa`; GitHub CI green on `qa` ([run 36635345962](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36635345962)). `qa` merges into `main` at the M0 exit review.

### Integration
- New branch `qa` from `main` @ `76ee34e`; ChatGPT's work recreated as `story/US-003` @ `2170dfb` on its base `fb21b48` (all 90 uploaded files verified identical) and merged with `--no-ff`.
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
Imported unchanged as `2170dfb` and merged into `qa` on 2026-09-30; Windows
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
| `3d96f58`, `c8c6301` | P-000: bootstrap the Mraw workspace; toolchain installed, D-12 decided |
| `d231374` | US-001: one CMake preset builds odysseus.exe, odysseus_headless.exe, odysseus_tests.exe with zero warnings |
| `38290bb` | US-002: GitHub Actions builds and tests every push |
| `383cbd9` | Codex sync from Google Drive at session start (`tools/sync-codex.ps1`) |
| `d54ce65` | Dominus and Anima skills in `.claude/skills/` |
| `5770852` | Codex v1.2 from Anima: Luna engine first (requirements v1.4, ARC-09) |
| `2d7dc23` | P-001: adopt Codex v1.2 (Luna folders, new prompt order, D-04 and D-13) |
| `3438f4b`, `fb21b48` | Milestone.md progress snapshot AP-001; CI-004 |
| `76ee34e` | Project documents mirrored from Google Drive into `docs/project/` (`tools/sync-workspace.ps1`) |
