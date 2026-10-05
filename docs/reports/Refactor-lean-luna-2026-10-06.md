# Refactor report: lean code, stronger Luna (2026-10-06)

Branch `refactor/lean-luna`, cut from `main` at `f0aa726` (M10b done). Not merged into `qa` or `main`: the owner decides.
Team: Mraw (Dominus hats: architect, programmer, tester).

## Goal

The owner asked for a full refactor that leaves the code efficient and light, with the accent on the game engine (Luna).
Rule used throughout: delete before adding; one shared copy instead of many; no behaviour change unless named below.

## Results in numbers

| | Before (`main`) | After (branch) |
|---|---|---|
| Production code (`src/`, `apps/`) | - | **289 lines fewer** (+368 / -657) in 71 files |
| Functions nobody calls | 48 | 2 (kept on purpose, see section 3); 46 removed |
| Places doing the same small job by hand | 52 (file read 15, safe write 7, lower case 11, id check 5, need lookup 4, words split/join/replace 10) | 1 shared function each |
| Debug build warnings | 0 | 0 |
| Debug tests (CTest entries) | 27 of 27 | 30 of 30 (three more game shards; four new check cases in `odysseus_tests` and `luna_tests`) |
| Debug test time (`verify.ps1`) | 964 s (serial) | **287 s** (headless `-j 6` 234 s + window tests 52 s) |
| Game test cases run | 436 | 436 (each exactly once, counted with `--count`) |

## 1. Luna engine

- **GPU renderer, zero heap allocations per batch** (`src/luna/platform/gpu_backend.cpp`). Every lit batch used to build a fresh 2 KB
  `std::vector<float>` for the light uniforms; it is now a fixed `std::array` on the stack. The six corners of a sprite are appended in one
  `insert` (one capacity check) instead of six `push_back`s. Same pixels, same shaders.
- **`TimeWindow<N>`** (`src/luna/engine/frame_stats.h`): the last N timings with `average()`, `worst()`, `last()`; never allocates. It replaces
  three hand-rolled ring buffers (draw, tick and frame times: 9 members and 4 loops) in `OdysseyGame`. Any game on Luna gets a perf overlay
  for free.
- **`intentFromName`** (`src/luna/engine/input.*`): one name table next to the `Intent` enum, with a `static_assert` that every intent has a
  name. The 30-line `if` chain in `apps/odysseus/main.cpp` is gone, and scripted runs (`--hold`) can now use every intent (before: 32 of 43;
  `Reload`, `Build`, `Journal`, `Confront`, `Actions`, `QuestDebug`, `PlayHere` and the four `List` intents were refused).
- **Dead engine API removed:** `Window::setFullscreen` (window modes go through `applyResolution`), `FixedStepClock::tickNanoseconds`,
  `stepProjectileTick` (physics), `RigidBody::inverseMass` and its write-only member, `RigidBody::onGround` accessor, `NodeGraph::setNextId`.
- The UI's search filter uses the shared `core::lowered` instead of its own copy.

## 2. One shared copy of the small helpers (`src/core/text.*`, new)

| Helper | Replaces |
|---|---|
| `readTextFile` | 15 private "read the whole file" places (dialogue, quests, interactions, buildings, smalltalk, editor help, saves) |
| `writeTextFileSafely(file, text, backups)` | 7 copies of "write to .tmp, rename, keep backups" (ADR-010): saves, levels, prefabs, NPC classes and kinds, graph and story-event editors |
| `lowered` | 11 lower-case copies (6 named functions, 5 inline loops) |
| `splitWords`, `joined`, `replaceAll` | 3 + 3 + 4 copies |

Simulation helpers in the same spirit:
- `sim::needFromName` (`src/sim/needs.*`): one case-insensitive lookup instead of four loops over `needName` (game rules, test play, graph check,
  built-in actions).
- `sim::validItemId(id, maxLength)`: the five "lower-case letters, digits and -" checks (items, NPC classes, prefabs, graph and story-event
  names) are one function.

## 3. Dead code removed (46 functions, 2 write-only members)

Found by counting every function name declared in `src/**/*.h` across `src/`, `apps/` and `tests/`; removed only names with no caller anywhere.
Examples: `OdysseyGame::plantThing`, `animalArt`, `effectArt`, `aimTargetX/Y`, `overlayOn/showOverlay`; `BuildingData::buildableKinds`;
`BuildingStore::footprintCells`; `eventKindName`, `outcomeName`, `biomeName`, `resourceName`; `runnerIdOf`; `HeroLife::clearNews`,
`focusChosen`, `setRelation`; `RunFlow::tutorialOn/setTutorialOn`; `Sword::lastSlashFacing` and its write-only member (`slash()` lost its
unused parameter).

**Kept on purpose, owner to decide:** `HeroLife::gatherHerbs` and `HeroLife::hasToolFor` have no caller either, but they are gameplay
(an unwired action), not plumbing. Accessors used only by tests were kept.

## 4. Faster test loop

- The game tests ran as three shards, but shard C held 65 of 72 files (491 s alone) and the local `verify.ps1` ran everything one at a
  time. Each file was timed; shards C to E now hold balanced named lists (about 165 s of work each under load) and F catches every other
  file, so a new test file is never left out.
- Patterns are anchored at the folder (`*game?level_test.cpp`), so `level_test.cpp` no longer also matches `living_level_test.cpp`.
- `tools/verify.ps1` now runs like CI: headless tests with `-j 6`, window tests one at a time.

## Behaviour changes (all deliberate, all small)

1. Scripted runs accept every intent name (before: 32 of 43).
2. The graph and story-event editors' Save now creates a missing folder (shared writer) instead of failing.
3. Save and level writes: a failed backup rename no longer aborts the save; the new file is still written atomically and a failure of the
   final rename is reported (before: any rename failure threw).
4. Quest objectives split on any whitespace (before: spaces and tabs only).
5. `need(...)` in the graph checker, game rules, test play and built-in actions: unchanged (already any case). NPC schedules, NPC rules and
   NPC effects still expect lower-case need names, exactly as before.

## Verification

- `pwsh tools/verify.ps1 -Story REFACTOR-lean-luna` (Debug, AddressSanitizer): **VERIFIED**, 0 warning lines, 30 of 30; evidence in
  `docs/evidence/REFACTOR-lean-luna/`.
- Release build: every target compiled with 0 warnings and linked, except `odysseus.exe`, whose file was locked by a running copy of the
  game (started 02:02, left open on purpose). Release tests run in CI when the branch reaches `main` (D-51).
- Determinism hash tests are part of `odysseus_sim_tests` and pass unchanged (no simulation logic was altered).
- New checks: `Refactor: lowered, splitWords, joined and replaceAll`, `Refactor: a safe write replaces the file, keeps the backups...`
  (core), `Refactor: every intent has a name scripts can use`, `Refactor: a time window keeps the last N timings...` (Luna).

## Skipped, and when to do it

- **Splitting the large files** (`odyssey_game.cpp` 2.7k lines, `editor.cpp` 2.4k): moving code between files is a big diff for no runtime
  gain. Do it when a story needs to change one of them heavily.
- **Indexed quads in the GPU backend** (4 vertices + index buffer instead of 6 vertices, white colour dropped from the vertex): about a third
  less vertex data. Add when a perf run (`--perf`) shows the draw step near the 16.7 ms budget; today it is not.
- **Two JSON readers** (nlohmann for settings and saves, the own line-aware reader for content): both are needed today, the own one gives
  line numbers in error messages (ARC-08).
- **Milestone files at the repository root** (21 of them): a Codex rule puts them there; moving them is Anima's call.

## Next step for the owner

Review the branch. To take it: merge `refactor/lean-luna` into `qa` (Release is first tested at the next milestone exit, D-51).
