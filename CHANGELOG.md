# Changelog

Record every pull request's full change set here before opening or updating it.
Entries describe the final changes and their verification; update an entry when
its PR changes rather than leaving an outdated description.

## US-241: generated normal maps (Mraw) - 2026-10-02

**State:** Merged into `qa`; verified in Debug (27 of 27, zero warnings); evidence in `docs/evidence/US-241/`.

- `odysseus_atlas --normals` (Tools) makes a normal atlas for every atlas picture (nine, committed in `assets/sprites/atlas/*_n.png`); height from the distance to the edge and the brightness, Sobel slopes; a hand-made `<frame>_n.png` next to `cuts.json` wins; a wrong-sized one is refused by name.
- The game gives the renderer the normal maps of the hero, characters, ground and the plant, tree and animal pages (mirrored animals get mirrored normals); sprites without a map, or with a map that does not fit, are lit flat with no error.
- `Luna` image ops `normalAtlas` and `mirroredNormals`; guide `docs/guides/lighting.md` extended. Tests: `tests/luna/normals_test.cpp`, `tests/game/normals_test.cpp` (generate, committed maps, missing, own map, the shaded hero on the GPU).
## US-240: the lighting pipeline (Mraw) - 2026-10-02

**State:** Merged into `qa`; verified in Debug (27 of 27, zero warnings); evidence in `docs/evidence/US-240/`.

- Lit sprite shader (ambient plus up to 64 point lights, normal-map facing), `setLighting` and `setNormalMap` through Window, backends and Renderer; the SDL fallback tints by the ambient colour only; additive draws are not lit.
- `assets/data/light/lights.json` (ambient, light kinds; D-49 values), `LightingData`, guide `docs/guides/lighting.md`; the world is drawn lit, the interface not; default is neutral, so the picture is unchanged.
- Tests: window tests for ambient, point light and a 64-light budget (0.28 ms on the card); game tests for the data, the round trip, errors and lit-world drawing.
## US-234: frame budget at the new size (Mraw) - 2026-10-01

**State:** Merged into `qa`; verified in Debug (27 of 27, zero warnings); 10-minute run in `docs/evidence/US-234/`.

- F3 overlay shows CPU (tick, draw) and GPU time; GPU time is measured on the card with a fence (SDL_GPU has no timestamp queries), only while the overlay or `--perf` is on. `--perf` logs frame figures each minute; `--people N` starts the clan with N people.
- Dev PC (RX 7900 XTX), Release, 1080p, 500 people, 10 minutes: 59.9 FPS average, draw 0.14 ms, GPU 0.20 ms, 32 of 35,920 frames over 20 ms (autosave at day end). Scaled to the D-06 minimum PC (4x GPU, 2x CPU): about 1.4 ms of 16.7 ms. Method in `docs/plans/US-234.md`.
- New test `US-234 The overlay shows CPU and GPU times`.
## US-233: every screen at the new size (Mraw) - 2026-10-01

**State:** Merged into `qa`; verified in Debug (27 of 27, zero warnings); contact sheet for the owner in `docs/evidence/US-233/`.

- `RunFlow` lays its panel out from the interface size (centred, as tall as its content, up to 600 x 420) and its shade covers the whole interface; paragraphs wrap to the panel width. All run screens, the menu, Settings and the dialogue panel use it.
- New test `US-233 Screens fit the interface at both UI scales`.
## US-232: camera zoom and UI scale (Mraw) - 2026-10-01

**State:** Merged into `qa`; verified in Debug (27 of 27, zero warnings), GPU screenshots in `docs/evidence/US-232/`.

- `ScaledRenderer` (Engine) draws a whole-number larger; the world is drawn at camera zoom 1x or 2x (default 2x) and the interface at UI scale 1x or 2x (default 1x), each laid out in its own pixels. `Camera::setViewSize`; pointer mapping goes through zoom and UI scale.
- Settings screen buttons; keys + and - and the mouse wheel zoom in play; `cameraZoom` and `uiScale` in `settings.json` (guide updated).
- Tests: `tests/luna/zoom_test.cpp`, two US-232 cases in `aiming_test.cpp`; older game tests run at zoom 1x.
## US-231: 960x540 virtual screen, window modes, hero facing and eight directions (Mraw) - 2026-10-01

**State:** Merged into `qa` locally; verified in Debug (27 of 27, zero warnings).

- Virtual screen 960x540 (`src/core/presentation.*`), windowed sizes, borderless and exclusive modes, Whole and Fill scaling, settings and pointer mapping (D-44, requirements v2.6).
- Tests and scripted window runs moved from the 480x270 coordinates to 960x540.
- Fix: a held left or right key always turns the hero that way (`odyssey_game.cpp`); the pointer keeps the aim.
- Hero art: eight facings use real frames from the owner's turn-around sheet (`ArtSet::frame`); West no longer shows a right-facing hero. New tests: "US-139 Left and right keys turn him", "US-231 Eight hero directions".

## Docs: merged parallel versions: Codex v2.6, requirements v2.8 (Dominus, Anima) - 2026-10-01

**State:** Documents only; merged into `qa`.

- While Anima published Codex v2.4 and v2.5 and requirements v2.6 and v2.7, Mraw aligned US-231 and US-232 with the owner's M8b answers (D-44, CI-010) on `story/US-231` and published its own "v2.4" Codex and "v2.6" requirements to Drive. Both lines are merged: Codex v2.6 and requirements v2.8 contain every change from both sides (Mraw's S-US-231 and S-US-232 prompts are kept word for word).
- `docs/project/requirements/`: requirements v2.8 and the backlog; `docs/Codex.md`, `CLAUDE.md`: Codex v2.6; `docs/codex-issues.md`: CI-010 resolved.
- For `story/US-231` when it merges `qa`: take `qa`'s version of `docs/Codex.md`, `CLAUDE.md`, `docs/codex-issues.md` and the two files in `docs/project/requirements/` (they already contain the branch's changes); keep the branch's own versions of `Handover.md`, `Limit.md`, `docs/status.md` and `docs/project/README.md`.
- From now on Codex changes go through Anima (A-002) so versions stay in one line.

## Docs: Codex v2.5, requirements v2.7, playtest plan (Dominus, Anima, D-48) - 2026-10-01

**State:** Documents only; merged into `qa`.

- Codex v2.5 synced from Anima: K-M13 reminds the owner to recruit the eight playtesters; X-M6 runs the playtest by the plan; D-14, D-48.
- Requirements v2.7 and the backlog mirrored from Drive: ARC-01..ARC-08 Decided (ARC-01 renamed Six-layer architecture), D-14 plan ready, D-48.
- `docs/plans/M6-playtest-plan.md`: who, recruiting, session script, interview, evidence and privacy for kill gate 2.
- `docs/decisions.md`: D-14 updated, D-48.

## Docs: completeness review, Codex v2.4 and requirements v2.6 (Dominus, Anima, D-47) - 2026-10-01

**State:** Documents and the workspace sync only; merged into `qa`.

- Codex v2.4 synced from Anima: CI-009 resolved (K-M8b checks that M8 is done, K-M9 that M8e is done); every remaining exit review (X-M8b..X-M14) runs `tools/verify.ps1 -Config Release` on the owner's PC for the strict 3-second first-frame check; D-07, D-09, D-10, D-11 closed or superseded; D-47.
- Requirements v2.6 and the backlog mirrored from Drive: stale decisions closed, STO-01 and SDC-01 absorbed, OPEN-08/10/15 answered, every MVP scope item Decided, ADR-018 in the ADR table, glossary extended, Figure 2 redrawn (`docs/project/diagrams/Odysseus - MVP Timeline.png`).
- `tools/sync-workspace.ps1`: the reading PDFs on Drive (Matt Ganzak guides) are no longer reported as unmirrored files.
- `docs/decisions.md`: D-07, D-09, D-10, D-11 closed, D-47; `docs/codex-issues.md`: CI-009 resolved.

## CI: guides keep LF too (Dominus) - 2026-10-01

**State:** Merged into `qa`.

- `.gitattributes`: `docs/guides/** text eol=lf`. The US-160 guide test looks for the text of `elder-fire.dlg` inside `docs/guides/dialogue-format.md`; Windows runners checked the guide out with CRLF, so it failed on CI (run 36890828492, the last test still red after the `.dlg` fix).

## US-230: Luna's SDL_GPU renderer (Avengers) - 2026-10-01

**State:** Built and verified (`tools/verify.ps1 -Story US-230`); merged into `qa`.

- Platform: `RenderBackend` with two implementations, `GpuBackend` (SDL_GPU device and swapchain, a virtual-screen texture, nearest-neighbour sampling, batched quads, Normal and Add pipelines, a whole-number blit into the window, screenshots) and `SdlRendererBackend` (the old drawing, now also drawing through a virtual-screen texture so both give the same picture); HLSL shaders compiled with the Windows SDK's `dxc.exe` at build time (`-DLUNA_GPU=OFF` or no `dxc.exe`: the GPU backend is left out); the Window falls back to SDL_Renderer with the reason logged.
- Engine and app: `--renderer auto|gpu|sdl`; the log says which renderer is used. The `Renderer` interface and `src/game/` did not change.
- Docs: `docs/adr/ADR-021-sdl-gpu-renderer.md`, `docs/plans/US-230.md`, teach-back, evidence `docs/evidence/US-230/` (demo level and camp on both renderers: byte-identical).
- Tests: `tests/game/renderer_test.cpp` (the demo level and the camp, GPU against SDL_Renderer, 0 different pixels; fallback), `tests/luna/pixels_window_test.cpp` (crisp pixels with each renderer; alpha, additive and scaled draws identical).


## CI: failures were hidden; fixed (Dominus) - 2026-10-01

**State:** Merged into `qa`.

- `.github/workflows/ci.yml`: each test step ran two `ctest` commands and PowerShell reported only the last exit code, so failing headless tests were hidden whenever the window tests passed. Both exit codes are now checked. Runs on `qa` since US-160 reported green while `odysseus_game_tests_c` and `odysseus_sim_tests` failed in Debug and Release on CI.
- `.gitattributes`: `*.dlg text eol=lf`. Windows runners checked the dialogue scripts out with CRLF, so the US-160 round-trip and reload tests failed on CI (they pass locally, where the files stay LF).
- `tests/luna/run_game_window.cmake`: on GitHub runners (no GPU) the Release first-frame limit is 10 s (3517 ms was measured); the 3-second player criterion is checked on the owner's PC with `tools/verify.ps1 -Config Release` at milestone exits.

## CI: faster verification (Dominus, D-46) - 2026-10-01

**State:** Merged into `qa`; this push is the first CI run with the new workflow.

- `.github/workflows/ci.yml`: runs on pushes to `qa` and `main` only (plus manual runs), skips docs-only pushes (`docs/**`, `*.md`), cancels a run when a newer push to the same branch arrives, caches the built vcpkg libraries (`actions/cache`, `VCPKG_BINARY_SOURCES`); builds and tests Release on `qa`, and also Debug on `main`.
- `tools/verify.ps1`: new `-Config Debug|Release|Both` (default Debug): the local check builds and tests Debug with AddressSanitizer.
- Codex v2.3 synced from Anima (`docs/Codex.md`, `CLAUDE.md`): Definition of Done, L-01 step 7 and every prompt still To do say which configuration is checked where.
- Requirements v2.5 and the backlog mirrored from Drive (ADR-014 trimmed, D-46); `docs/decisions.md`: D-46.

## Docs: Codex v2.2, requirements v2.4 and the M8b-M8e brief (Anima, Dominus) - 2026-10-01

**State:** Documents only, no code; merged into `qa`. The sync scripts ran (`tools/sync-codex.ps1`, `tools/sync-workspace.ps1`).

- Codex v2.2 synced from Anima (`docs/Codex.md`, `CLAUDE.md`): P-011; M8b Resolution and GPU renderer, M8c Lighting and shadows, M8d Buildings, M8e Building life (K, 21 story prompts, X each) between X-M8 and K-M9; Charter architecture rule 11 (rendering); D-06, D-42, D-43.
- Requirements v2.4 and the backlog mirrored from Drive: ENV-18..ENV-21, ARC-11, INT-07, INT-08, EDT-07, MVP-16, ADR-021; epics E22-E25, US-230..US-257; D-06 answered (mid-range target PC); M8d split into M8d and M8e.
- `docs/plans/M8b-M8d-render-light-build-brief.md`: the build brief, with the Round 17 answers in section 9.
- Left for P-011: D-06, D-42 and D-43 in `docs/decisions.md` and the new prompts in `docs/status.md`.

## K-M8b: kick off M8b Resolution and GPU renderer (Avengers) - 2026-10-01

**State:** Documents only; merged into `qa`.

- `docs/decisions.md`: D-44, the owner's answers to the M8b design questions (window sizes, whole steps with bars by default, zoom and UI scale in Settings and zoom on wheel and keys, first start at windowed 1280x720, zoom 2x, UI scale 1x, lighting Medium).
- `docs/plans/M8b-renderer-design.md`: the GPU path (device and swapchain, a virtual screen texture, batching, HLSL shaders compiled with the Windows SDK's `dxc.exe` at build time, fallback to SDL_Renderer), presentation and window modes, camera zoom and UI scale, layout rules, tests without a GPU, performance method.
- `docs/codex-issues.md`: CI-009 (K-M8b step 1 asks to confirm M8e, which cannot be done before M8b).
- `docs/status.md`: K-M8b Done; `Milestone-75.md`, `Limit.md`.
## X-M8: exit review of Speak to NPCs (Avengers) - 2026-10-01

**State:** Documents only; all five exit criteria met; `qa` merged into `main`, tag `m8-done`.

- `docs/gates/M8.md`: one section per exit criterion with its evidence, the stories, the questions for the owner (the greeting rule, the 50 small-talk lines) and the delegated technical choices.
- `docs/gates/M8-smalltalk.md`: 50 generated small-talk lines from one seed for the owner to read, with the count of repeated lines (42 different, none more than twice).
- `Milestone-74.md` (AP-075), `Limit.md`, `docs/status.md`.
## US-165: NPCs talk to each other (Avengers) - 2026-10-01

**State:** Built and verified (`tools/verify.ps1 -Story US-165`); merged into `qa`.

- Simulation: `World::takeTalks()` (who has just talked, for the screen only: not saved, not hashed), `selectPair` (the `@pair` script for two people and a kind of event).
- Game: `Exchanges` (talk, quarrel, courtship, pairing, sharing and gift events of two clan members within 12 m of the hero become an exchange of speech bubbles: 3 s a line, in turn, one exchange at a time, three may wait) and `Bubbles::remove`.
- Data and docs: `social.*` topics in `smalltalk.json`, `pair-elder-child.dlg`, the guide section "Clan members talking to each other", `docs/plans/US-165.md`, teach-back, evidence `docs/evidence/US-165/`.
- Tests: 5 game cases in `tests/game/exchange_test.cpp`, 3 simulation cases in `tests/sim/selection_test.cpp`.
## US-164: Conversations are remembered (Avengers) - 2026-10-01

**State:** Built and verified (`tools/verify.ps1 -Story US-164`); merged into `qa`.

- Simulation: `World::rememberConversation` (an ordinary memory, Gift or Quarrel by the feeling, major from 60, plus a free-text note), `MemoryNote::clause`, `FlagStore` (story notes, ordered, saved, hashed).
- Game: the effects `remember`, `flag` and `chronicle` are carried out; `flag(name)` reads the store; flags are saved in `things.json` and start empty in a new run; the rude answer of generated small talk leaves a bad memory (-40); the elder remembers the berries (20).
- Docs: the guide section "Being remembered", `docs/plans/US-164.md`, teach-back.
- Tests: `tests/sim/memory_talk_test.cpp` (7 cases: memory, gossip at half strength, two days, flags, saved), 4 new game cases in `tests/game/conversation_test.cpp`.
## US-163: Generated small talk (Avengers) - 2026-10-01

**State:** Built and verified (`tools/verify.ps1 -Story US-163`); merged into `qa`.

- Data: `assets/data/dialogue/smalltalk.json` (topics memory, people, needs, season, hero, hunt; templates for any mood and for a mood).
- Simulation: `smalltalk` (the checked file reader and the generator: topic by weights, one of the three newest facts, a template for the mood that is not tired out), `MemoryNote` and `Person::notes` (free-text memories: saved, hashed, never read by the simulation), `{smalltalk.topic}` in scripts, the generated talk with Thank you and Be quiet (opinion -10).
- Game: Talk with no script opens small talk (and still warms the two); the file loads and reloads with the dialogue files.
- Tests: `tests/sim/smalltalk_test.cpp` (11 cases), 4 new game cases in `tests/game/conversation_test.cpp`; the older Talk checks of US-152 and US-161 follow the new behaviour. Docs: the small-talk section of the dialogue guide, `docs/plans/US-163.md`, teach-back, evidence `docs/evidence/US-163/` (50 sample lines, a screenshot).
## P-011: adopt Codex v2.2 and record D-40..D-43 (Avengers) - 2026-10-01

**State:** Documents only; merged into `qa`.

- `docs/decisions.md`: D-06 Decided (mid-range target PC); D-40 and D-41 (missing since P-010) and D-42 and D-43 added.
- `docs/status.md`: P-011 Done; K-M8b..X-M8e (29 rows, To do) between X-M8 and K-M9; Codex v2.2 in the header.
- `Limit.md` updated. CLAUDE.md, the Codex and the requirements were already synced.

## US-162: Who says what (Avengers) - 2026-10-01

**State:** Built and verified (`tools/verify.ps1 -Story US-162`); merged into `qa`.

- Simulation: `selectScript` breaks exact ties with the seeded stream (one draw per call), `selectBark` and `barkText` for greetings, roles `hunter` and `gatherer`, the condition function `mood(who)`.
- Game: `Bubbles` and `updateGreetings` (friendly people within 3 m greet in a bubble, at most once a minute each, at most two bubbles at once, nearest first); the greeting cooldowns and the stream "dialogue" belong to the game and reset with a run.
- Data and docs: `greet-elder.dlg`, `greet-friend.dlg`, `greet-friend-warm.dlg`, the guide sections on ties, roles, mood and greetings, `docs/plans/US-162.md`, teach-back, evidence `docs/evidence/US-162/`.
- Tests: `tests/sim/selection_test.cpp` (7 cases), `tests/game/greeting_test.cpp` (4 cases).
## US-161: Conversations and the dialogue panel (Avengers) - 2026-10-01

**State:** Built and verified (`tools/verify.ps1 -Story US-161`: zero warnings, 27 of 27 tests in Debug and Release); merged into `qa`.

- Simulation: `conversation` (the runtime: lines and choices by condition, effects in order through the action runner, END, `leave`; the mood word; `{hero}` and `{npc}` tokens), `dialogue_select` (roles `elder` and `child`; the script that speaks for someone: name over role over kind, then priority), `ActionRunner::runEffects`.
- Game: `Screen::Talk` in `RunFlow` (the world pauses; name and mood, words, up to five numbered choices by mouse or keys 1 to 5, greyed choices with their reason, Esc leaves); Talk opens it when a script fits, else the plain talk; the effect `opinion npc hero n` and the condition `opinion(a, b)` are real.
- Tests: `tests/sim/conversation_test.cpp` (8 cases), `tests/game/conversation_test.cpp` (5 cases). Docs: `docs/plans/US-161.md`, the new section of `docs/guides/dialogue-format.md`, teach-back, evidence `docs/evidence/US-161/`.
## P-010: adopt Codex v2.1 (Avengers) - 2026-10-01

**State:** Documents only; merged into `qa`.

- `docs/status.md`: P-010 Done; K-M10..X-M14 (47 rows, To do) in Codex order; X-M6 now waits for X-M14.
- `docs/codex-issues.md`: CI-007 and CI-008 marked resolved in Codex v2.1. D-40, D-41 and the v2.1 Charter were already synced.
- `Milestone-68.md` (AP-069) and `Limit.md` updated.

## US-160: The .dlg format (Avengers) - 2026-10-01

**State:** Built and verified (`tools/verify.ps1 -Story US-160`); merged into `qa`.

- Simulation: `dialogue_script` (the `.dlg` parser with `file:line: message` errors, the script model, the canonical writer, `DialogueLibrary`).
- Data and docs: `assets/data/dialogue/elder-fire.dlg`, `docs/guides/dialogue-format.md`, `docs/plans/US-160.md`, teach-back.
- Game: the conversations load at start and reload with F5 together with the interaction files (same panel).
- Tests: 10 simulation cases (`tests/sim/dialogue_test.cpp`, incl. the line-9 error, the exact round trip of every shipped file and 300 mutations) and 1 game case.

## Docs: Codex v2.1, requirements v2.2 and the M10-M14 brief (Anima, Dominus) - 2026-10-01

**State:** Documents only, no code; merged into `qa`. The sync scripts ran (`tools/sync-codex.ps1`, `tools/sync-workspace.ps1`).

- Codex v2.1 synced from Anima (`docs/Codex.md`, `CLAUDE.md`): P-010; M10 Quests and story authoring, M11 Data editors, M12 World editing, M13 Politics, M14 Technology (K, 37 story prompts, X each); kill gate 2 after M14; M10-M14 exceptions in the Charter and L-01 (D-41: Dominus decides design questions as delegated, tests run at exit reviews); CI-007 and CI-008 resolved in the Codex.
- Requirements v2.2 and the backlog mirrored from Drive: STO-04, STO-05, EDT-01..EDT-06, SDC-07, PIL-08 (Politics), PIL-09 (Technology), MVP-08 (four pillars), MVP-09, MVP-15, ADR-020; epics E17-E21, US-180..US-226; D-40, D-41.
- `docs/plans/M10-M13-authoring-brief.md`: owner answers of Round 15 (section 9) and M14 Technology (section 10).
- Left for P-010: D-40 and D-41 in `docs/decisions.md`, the new prompts in `docs/status.md`, CI-007 and CI-008 marked resolved in `docs/codex-issues.md`.

## US-154: NPCs and animals use interactions (Avengers) - 2026-10-01

**State:** Built and verified (`tools/verify.ps1 -Story US-154`); merged into `qa`.

- Simulation: `pickBest` and `CooldownTable` (`npc_chooser`), `World::drainPersonNeed`.
- Game: `NpcLife` (clan members and animals score the interactions near them with the files' `npc.score`, walk there, and do them with the same runner as the hero; danger drops what they were doing), actors and new subjects (animals, the hero with `armed` and `moving` tags) in the rule context, `need(...)` now means how much is missing, built-in `graze` and `flee`, errands in the clan view, placed harmless animals now walk, graze and flee.
- Data and docs: `graze`, `flee-predator`, `flee-armed-hero`, `flee-moving-hero`; the grasses tagged `grass`; guide section "Clan members and animals act on their own"; `docs/plans/US-154.md`; teach-back.
- Tests: 3 sim cases, 6 game cases (`tests/game/npc_test.cpp`); the menu test helper moved to `tests/game/camp.h`.

## US-155: Age 1 world objects (Avengers) - 2026-10-01

**State:** Built and verified (`tools/verify.ps1 -Story US-155`); merged into `qa`.

- Data: `assets/data/objects.json` (fire pit, knapping stone, food store, shelter, flint nodule, water source, sleeping furs) and 9 interaction files.
- Game: objects are loaded into the plant catalog (flagged `object`) and placed, saved and edited like plants (codex issue CI-008: no level format bump); programmer art by code (`object_art`); the Editor's plant tool gets the objects as a last page; built-in actions `restore` and `warm-nearby`, effect verb `fx`.
- Simulation: `World::satisfyPersonNeed`.
- Docs and tests: guide section "World objects", `docs/plans/US-155.md`, teach-back, 6 new game cases and 1 sim case; `US-130 Cut`, `US-136 Editor` and `US-151 Catalog` adapted (reasons in the plan).

## US-153: Timed actions and world state (Avengers) - 2026-10-01

**State:** Built and verified (`tools/verify.ps1 -Story US-153`); merged into `qa`.

- Simulation: `ActionRunner` (durations, progress, interruption, waiting effects in a deterministic order, saved timers), `EffectHost`, `ThingRef`.
- Game: the hero's timed actions with a ring of dots over the target; moving or attacking stops the action and gives nothing; plants have states and a plant out of its starting state is hidden (D-37); `things.json` saved with the autosave (plant states and waiting effects); `gather.json` is a 3 s job that picks the plant and ripens it again after 15 s; built-in `gather` became `gather-berries`.
- Docs and tests: guide section "Timed actions and things that change", `docs/plans/US-153.md`, teach-back, `tests/sim/runner_test.cpp` (9 cases), 5 new cases in `tests/game/menu_test.cpp`; D-37 recorded.

## US-152: The context menu from data (Avengers) - 2026-10-01

**State:** Built and verified (`tools/verify.ps1 -Story US-152`); merged into `qa`.

- Simulation: the effect verb `do` (a built-in action of the game); `LoadOptions::knownBuiltins` makes an unknown built-in a load error.
- Game: `Subject` and `subjectAt` (what the hero can act on), `GameRuleContext` for any subject, 14 built-in actions moved unchanged out of the old menu code (`builtin_actions.cpp`), `RunFlow::openContext` now builds the menu from the registry.
- Data: 20 interaction files (every action the old menu had); `gather.json` now does what the game did (instant, 2 m, "Gather").
- Docs and tests: guide sections "Built-in actions" and the tags of the game's things, `docs/plans/US-152.md`, teach-back, `tests/game/menu_test.cpp` (6 cases); US-150 and US-156 tests adapted (see the plan).

## US-156: Hot reload (F5) and the validation panel (Avengers) - 2026-10-01

**State:** Built and verified (`tools/verify.ps1 -Story US-156`); merged into `qa`.

- Luna: key F5 and the intent `Reload` (platform, engine).
- Game: `OdysseyGame::reloadInteractions()` (all or nothing: only a clean registry replaces the data in use), the error panel (`file:line: message`, Game and Editor modes, also shown at start when a file was left out).
- Docs and tests: guide section "Editing while the game runs: F5", `docs/plans/US-156.md`, codex issue CI-007 (catalog reload not part of this story), teach-back, 4 cases in `tests/game/tags_test.cpp`.

## US-151: Tags and smart objects (Avengers) - 2026-10-01

**State:** Built and verified (`tools/verify.ps1 -Story US-151`); merged into `qa`.

- Tags and states: optional `tags` and `states` in plants, animals, weapons and characters (derived from the old fields when not written); `Catalogs::knownTags()`; unknown tag in an interaction file = warning naming file and tag.
- Game: `GameRuleContext` (the real world for the rule language), `plantOffers` / `plantThing` / `setPlantState`, `WorldPlant::state`; the plant context menu lists the interaction files' offers (Gather, Inspect); `assets/data/interactions/inspect.json`.
- Docs and tests: guide section "Tags and states", `docs/plans/US-151.md`, teach-back, `tests/game/tags_test.cpp` (5 cases).

## US-150: Interaction data and the rule language (Avengers) - 2026-10-01

**State:** Built and verified (`tools/verify.ps1 -Story US-150`: 0 warnings, 27 of 27 tests in Debug and Release); merged into `qa`.

- Simulation: `rule_json` (line-aware JSON reader with comments), `rule_expr` (condition and score language: lexer, recursive-descent parser, evaluator, 8 functions), `rule_effect` (13 effect verbs, `after` delays), `interaction` (registry that loads a folder, matching with reasons, `file:line: message` errors, canonical writer).
- Data and docs: `assets/data/interactions/gather.json`, `docs/guides/interaction-data.md`, `docs/plans/US-150.md`, teach-back in `docs/learning-journal.md`.
- Game: loads the interactions at start and logs mistakes (`odyssey_game.*`).
- Tests: `tests/sim/rules_test.cpp` (20 cases, includes the line-12 error, the guide check, 300 damaged-file mutations), `tests/game/interactions_test.cpp`. Evidence `docs/evidence/US-150/`.
 (Avengers) - 2026-10-01

**State:** Docs only. D-36 records the owner's four design answers; the design is `docs/plans/M7-interactions-design.md`; Milestone-57.md (AP-058).

## P-009: Adopt Codex v2.0 and pay the test debt (Avengers) - 2026-10-01

**State:** Docs only; no code changed. Verified: Debug and Release build with 0 warnings, 25 of 25 tests pass in each (`docs/evidence/P-009/`).

- `docs/status.md` lists P-009 and the M7, M8, M9 prompts; `docs/decisions.md` records D-08 (answered by D-34) and D-35; `CLAUDE.md` and `docs/Codex.md` are at v2.0; the requirements and backlog are synced from Drive.
- CI speed: the tests that open no window run in parallel (`ctest -LE window -j 4`), window tests one at a time after them; `odysseus_game_tests` is split into three CTest runs by source file (A, B, and C = everything else); Debug-only trims (Release keeps the full sizes) of `US-139 No flicker walking past the pointer` (108 to 16 combinations), `US-040 Playable` (30 to 8 seeds) and `US-014 No two living people share a name` (2 to 1 seed). Locally the non-window Debug tests went from about 450 s to 94 s.
- `US-029` flight run is retried up to 3 times because one slow frame on a CI runner let the spear land before the quit (CI run 36828924780); the check itself is unchanged.
- `docs/gates/test-debt.md` records the run; the owed test checks of M2d, M4, M5, M6 are closed there.


## M6: Playtest readiness - US-090, US-091, US-092 (Avengers) - 2026-10-01

**State:** Built and compiled; tests written, not run; merged into local `qa`. X-M6 waits for the owner (D-GATE-M6).

- Tutorial (`tutorial.*`, `tutorial.json`), crash report (`luna::platform::installCrashHandler`), packaging (install rules and CPack zip), opt-in session statistics (`session_stats.*`), the first-launch Privacy screen, `HeroLife::eatBerries` and `tendCampFire`.
- D-33 records the choices. Tests: `tests/game/m6_test.cpp`. Exit review `docs/gates/M6.md`.

## M5: The vertical slice - US-050..US-055, US-060..US-063, US-070..US-073, US-081, US-082 (Avengers) - 2026-10-01

**State:** Built and compiled (Debug); simulation tests written, not run (owner: no testing); merged into local `qa`.

- Simulation: `HeroData` (assets/data/hero/*.json, validated with file and field), `HeroLife` (imprint, presets, comforts, focus and crossroads, mantle, professions, crafting quality, apprenticeship, dominion, barter and debts, sacred fire, aging, win and lose, save and load).
- Game and Luna: `RunFlow` screens and context menu, `GameSettings`, F3 overlay, fullscreen switching, `--new-game`.
- D-32 records the choices. Tests: `tests/sim/hero_test.cpp`. Evidence `docs/evidence/US-050`; exit review `docs/gates/M5.md`.
## M4: Region, tools and saves — US-040, US-041, US-042, US-043, US-080, US-083 (Avengers) — 2026-10-01

**State:** Built and compiled (Debug and Release, zero warnings); tests written but not run (owner: no testing); merged into local `qa`.

- `sim::Region` (seeded, integer-only, chunked), resources by biome with berry regrowth, `Rivals` (two clans, levels of detail, moving camps), region delta saves, `ChunkStreamer` (Luna), `levelFromRegion`; `--region`, `--save-dir`, `--load`; autosave at each day's end with backups; F12 developer tools (Debug only); `writeSaveText` shared by the world and region saves.
- D-31 records the choices. Tests: `tests/sim/region_test.cpp`, `tests/game/m4_test.cpp`. Evidence `docs/evidence/US-040`, `US-083`; plan `docs/plans/M4-region-tools-saves.md`; exit review `docs/gates/M4.md`.
## M3: The clan on screen — US-030, US-032, US-031 (Avengers) — 2026-10-01

**State:** Built and compiled (Debug and Release, zero warnings); tests written but not run (owner: no testing until M4); merged into local `qa`. Full verification and CI at the M4 gate.

- Luna: `recoloured` and `composed` (`sprite_layers`). Game: layered code-drawn people (`clan_art`), `ClanView` (the simulation's actions given places, people walking at 60 px/s with interpolation), `Level::clan` and `assets/levels/camp.json`, the clan's simulation running inside the game (`--clan`, `--clan-speed`), emote bubbles and shiver, the hover panel, the date line.
- D-30 records the choices. Tests in `tests/game/clan_test.cpp`. Evidence `docs/evidence/US-030..US-032`; plans `docs/plans/US-030.md`, `US-031.md`, `US-032.md`.
## US-138: Placed effects and random weather (Avengers) — 2026-10-01

**State:** Built and compiled (Debug and Release, zero warnings); tests written but not run by the owner's instruction "Proceed without testing until reaching M4"; merged into local `qa`. Full verification and CI at the M4 gate.

- Level format version 2 gains `effects`; the Editor has an **Fx** tool and palette of the 17 looping effects with place, select, move, delete, Undo and Redo; the toolbar button **Grid** is now **#**.
- `WeatherCycle` (seeded PCG32 stream `weather`): a random weather from weather.json every 60-120 s, 3 s cross-fade, clear about a third of the time; drawn over the world and under the interface; `--seed` and `--weather` flags.
- D-29 records the small choices. Tests: US-138 Weather cycle, Weather in the game, Placed effects. Evidence `docs/evidence/US-138/`; plan `docs/plans/US-138.md`; guide `docs/guides/editor.md`.
## US-137: Animals in the Editor (Avengers) — 2026-10-01

**State:** Built and compiled (Debug and Release, zero warnings); tests written but not run by the owner's instruction "Proceed without testing until reaching M4"; merged into local `qa`. Full verification and CI at the M4 gate.

- The 50 animals are character kinds (`CharacterKindDef::animal`); the Editor's character palette has pages (12 to a page) with arrows; animals are drawn from the content atlas side views (`animals.{h,cpp}`), mirrored for west; the 20 predators and boars are enemies that are hit, strike back and die; the others are harmless bystanders.
- D-28 records the small choices. Tests: US-137 The fifty animals are character kinds, Place, Enemies, Bystanders. Evidence `docs/evidence/US-137/`; plan `docs/plans/US-137.md`; guide `docs/guides/editor.md`.
## US-136: Plants (Avengers) — 2026-10-01

**State:** Done on local verification (Debug and Release); merged into local `qa`, pushed with the M2d milestone gate.

- Level format version 2 gains `plants`; the Editor has a **Plant** tool and a palette of all 153 plants (36 to a page); the toolbar button **Weapon** is now **Arms**.
- Game: plants stand in the world; big ones (bushes, trees) block walking and flat shots through a new obstacle layer in Luna's `TileMap`; Interact with empty hands or the right mouse button (new intent `Inspect`) shows a plant's name and text for 3 s; any weapon hit destroys a plant with a leaf burst, an edible one heals 10 HP; 15 s later the same plant grows back at a random free cell in the camera view (seeded PCG32), with a growth effect.
- D-27 records the small choices. Tests: US-136 Level format, Editor, Place and block, Inspect, Chop, Eat, Regrow. Evidence `docs/evidence/US-136/`; plan `docs/plans/US-136.md`; guide `docs/guides/editor.md`.
## Hero orientation by the pointer, no sprite flicker (Avengers) — 2026-10-01 (follow-up to US-139, D-26)

**State:** Done on local verification (Debug and Release, 25/25); merged into local `qa`, pushed with the M2d milestone gate.

- The hero always faces the mouse pointer while it is over the picture (also with empty hands and the demo weapons); off the picture he faces the way he walks. Only a held catalog weapon still aims attacks and shows the aim line.
- Flicker fix: the facing is measured from the chest, ignores a pointer within 16 px, and changes only when the pointer is 10 degrees past the edge of the current facing's sector (`facingToward` with hysteresis); the facing before the tick's walking is what counts.
- Tests: US-139 The hero always faces the pointer, No flicker walking past the pointer (fails without the fix), Facing with hysteresis. Evidence `docs/evidence/US-139/walk-east-facing-pointer.png`; plan `docs/plans/US-139.md` (follow-up section).

## US-141: Bows, crossbows, thrown weapons and staff bolts (Avengers) — 2026-10-01

**State:** Done. Merged into `qa`, pushed; hosted CI green (run 36783802996).

- `assets/data/weapons.json`: new `classes` section (launch speeds for bow, thrown, staff, gun), validated naming file and field; no speeds are hard-coded any more (`ClassDef`, `Catalogs::weaponClass`, `launchArcShot(weapon, launchSpeed, ...)`).
- Game: bows and crossbows (class bow) and thrown weapons shoot the US-140 arcs, staffs fire flat bolts aimed at the pointer, all at the speed from the file; elements apply on hit; `OdysseyGame::cameraView()`.
- `assets/levels/range.json`: the shooting range (seven ranged weapons to pick up, goblins in the open and behind rocks).
- Tests: US-141 Each ranged class shoots, Every ranged starter is shootable from the hotbar, Elements on shots, Speeds come from weapons.json, Shooting range. Evidence `docs/evidence/US-141/`; plan `docs/plans/US-141.md`.

## US-140: Arc ballistics for shots (Avengers) — 2026-10-01

**State:** Done. Merged into `qa`, pushed; hosted CI green (run 36782177188).

- `src/game/arc_shots.{h,cpp}`: bow and thrown shots are Luna Physics projectiles (fixed-point, gravity, height); the launch angle lands them at the pointer, clamped to the weapon's range and its speed's reach; each tick they stop at the first enemy (feet to 1.5 m), rock (1.0 m tall) or the ground; a miss sticks 2 s, then is gone.
- Game: sprites lifted by height with a ground shadow; log line per shot and end; staff bolts and bullets keep the flat path. Key aim (Interact) fires a shallow chest-height arrow to the weapon's range.
- Earlier test "US-133 Starters fight" aims thrown weapons with the pointer. D-25 covers the design.
- Tests: US-140 Lands at the cursor, Range and misses, Hits in its path, In the game. Evidence `docs/evidence/US-140/`; plan `docs/plans/US-140.md`.

## US-139: Mouse aiming (Avengers) — 2026-10-01

**State:** Done. Merged into `qa`, pushed; hosted CI green (merge 00c0095).

- Luna Engine: new intent `Attack` (left mouse button, or scripted with `--hold Attack`); `--aim` flag (same as `--point`).
- Game: while a catalog weapon is held and the pointer is over the picture, the hero faces the pointer (nearest of 8) and Attack swings or shoots toward it at the exact angle; Interact still attacks along the facing. Dotted aim line (to the weapon's range) and a crosshair (red beyond range).
- `WeaponBehaviour::swingToward` / `launchToward` (unit direction), `facingToward`, `Hero::face`; the old facing versions still work.
- D-25 recorded. Tests: US-139 Facing from a direction, Face the cursor, Swing toward the cursor, Keys still work, Interact goes along the facing...; luna_tests: the left button is the Attack intent. Evidence `docs/evidence/US-139/`; plan `docs/plans/US-139.md`.

## US-135: Elements (Avengers) — 2026-09-30

**State:** Done. Merged into `qa`, pushed; hosted CI run 36776897333 green.

- `assets/data/weapons.json`: new `elements` section with the numbers and effect names per element; `loadCatalogs` range-checks them and requires every effect name to exist in effects.json (`ElementDef`, `Catalogs::element`).
- `src/game/status.{h,cpp}`: `StatusEffects` (burn, poison, slow) held by every `Enemy`; a new hit restarts the timer, it never stacks.
- Combat: fire burns 2 HP/s for 3 s, poison 1 HP/s for 5 s, ice slows to 50% for 2 s (a slowed enemy winds up at half speed), lightning jumps once to the nearest other enemy within 3 m for half damage, void heals the hero 25% of the damage. Hit and status effects come from effects.json. `Enemy::takeDamage(damage, flash)`.
- D-24 (owner answers of this story) recorded in docs/decisions.md.
- Tests: US-135 Numbers, Status effects, Fire, Fire shows, Poison, Ice, Lightning, Void, Bad numbers (odysseus_game_tests). Evidence `docs/evidence/US-135/` (a screenshot per element, the levels used); plan `docs/plans/US-135.md`.

## US-134: Pickups and the hotbar (Avengers) — 2026-09-30

**State:** Done. Merged into `qa`, pushed; hosted CI run 36766225054 green.

- Level format version 2 (`pickups`: weapon name and position); version 1 files load unchanged; unknown weapon names are refused with the field named. `Definitions::weapons`.
- Editor: Weapon tool and palette (16 starters + the two demo weapons), Select moves, Delete removes, Undo and Redo through `PickupsCommand`.
- Platform and Engine: keys 1-9, intents `Slot1..Slot9` (scriptable with `--hold Slot3`).
- Game: 9-slot hotbar at the bottom centre, pickups lying in the world, first free slot, "Hotbar full", 1-9 and Shift; the starters no longer cycle with Shift. `pickups.{h,cpp}`.
- `assets/levels/demo.json` is version 2 with the spear throw and sword as pickups. D-23 recorded in docs/decisions.md. docs/guides/editor.md explains pickups.
- Tests: US-134 Place, Move and delete, Level versions, Pick up, Full hotbar, Select, Hotbar drawn (odysseus_game_tests); number keys (luna_tests). Evidence `docs/evidence/US-134/`; plan `docs/plans/US-134.md`.
- Verification: zero warnings; 25/25 checks passed in Debug and Release, including simulation determinism and the earlier end-to-end tests. The first run exposed stale ID/effect assumptions and one missed scripted paint click; the full rerun passed.

## US-133: Weapon classes and the starter set (Mraw) — 2026-09-30

**State:** Done; merged into `qa`.

- Game: `WeaponBehaviour` with `MeleeBehaviour` and `RangedBehaviour` for the 8 classes; projectiles for bow, thrown, staff, gun; 16 starters cycled with Shift; held icon in the hero's hand; `atlas --starters` contact sheet.
- Tests: US-133 Classes, Starters fight, Starter set, In hand; US-029 tests kept. Evidence `docs/evidence/US-133/`; plan `docs/plans/US-133.md`.
- Verification: `tools/verify.ps1 -Story US-133`: zero warnings, ctest 25/25 in Debug and Release.
- Known gap: held icons are mirrored for west, not rotated (no renderer rotation).

## US-132: Effect player (Mraw) — 2026-09-30

**State:** Done; merged into `qa`.

- Luna: `Renderer::drawStyled` (stretch, alpha, additive) in the window, the recorder and `ImageRenderer`; `EffectPlayer` (`effects.{h,cpp}`).
- Game: catalogs and content atlas loaded; `playEffect`; hit spark, death smoke, spear dust trail.
- Tests: US-132 One-shot, Looping, Placement and style, Additive light (luna_tests); Combat effects, Death smoke (odysseus_game_tests). Evidence `docs/evidence/US-132/`; plan `docs/plans/US-132.md`.
- Verification: `tools/verify.ps1 -Story US-132`: zero warnings, ctest 25/25 in Debug and Release.

## US-131: Hero HP, fighting back and death (Mraw) — 2026-09-30

**State:** Done; merged into `qa`.

- `Enemy`: strike-back state machine (0.5 s wind-up, one strike per wind-up, no strikes from the dead), `reachMetres`; character kinds may set `reach`.
- `OdysseyGame`: hero 100 HP; hit enemies strike back with their sword damage within reach; fall, 1 s fade, respawn at the hero start; HUD (hero HP, red "!" over a wind-up).
- `characters.json`: strike numbers per D-21.
- Tests: US-131 Strike back, One strike per wind-up, Out of reach, Death and respawn; evidence `docs/evidence/US-131/`; plan `docs/plans/US-131.md`.
- Verification: `tools/verify.ps1 -Story US-131`: zero warnings, ctest 25/25 in Debug and Release.

## US-130: Content catalogs from the new sheets (Mraw) — 2026-09-30

**State:** Done; merged into `qa`.

- Luna `image_ops`: `keyAlpha`, `keyBrightness`, `removeColour`, `keepMainFigure`.
- Game: content atlas (`content_art.{h,cpp}`: pages, cut list, cutting, save/load, numbered review sheets) and catalogs (`catalogs.{h,cpp}`: weapons, plants, animals, effects, weather; validated, frames checked against the atlas).
- `odysseus_atlas` cuts `assets/sprites/content-cuts.json` (653 items, 1,191 frames) into `assets/sprites/atlas/content-*.png` + `content.json`; `--content-preview`.
- Data: `weapons.json` (150, 16 starters), `plants.json` (153), `animals.json` (50, 20 enemies), `effects.json` (200), `weather.json` (101); `tools/art/` scripts that measured the sheets and wrote the first catalogs; the seven sheets are in `assets/sprites/`.
- Evidence: `docs/evidence/US-130/` (numbered sheet and name list per page); plan `docs/plans/US-130.md`.
- Verification: `tools/verify.ps1 -Story US-130`: Debug and Release zero warnings, ctest 25/25 in both (new cases US-130 Cut, Keys, Valid, Review).

## K-M2d: Kick off M2d (Mraw) — 2026-09-30

**State:** Done; merged into `qa`.

- `docs/plans/M2d-content-design.md`: atlas pages, cut-list grids and smooth background removal, catalogs, Luna renderer additions and effect player, combat state machine, level format version 2, plants, weather.
- Verification: docs only.

## P-007: Adopt Codex v1.8 (Mraw) — 2026-09-30

**State:** Done; merged into `qa`.

- `docs/status.md`: P-007 Done; K-M2d, S-US-130..S-US-138, X-M2d added (To do).
- `docs/decisions.md`: D-21 (M2d content and combat) and D-22 (design decisions are the owner's).
- `assets/sprites/`: the seven new sheets committed unchanged.
- `Limit.md`, `Milestone-41.md` (AP-042): next prompt K-M2d.
- Verification: docs and assets only; CI on `qa`.

## M2d planning: brief, requirements v1.8, Codex v1.8 (Mraw, Anima) — 2026-09-30

**State:** Done; merged into `qa`.

- `docs/plans/M2d-content-brief.md`: the owner's seven new sprite sheets and every design decision from six chat rounds (D-21), and the new rule that design decisions are the owner's (D-22).
- Requirements v1.8 synced from Drive (`docs/project/requirements/`): epic E13, milestone M2d before M3, stories US-130..US-138, D-21, D-22, resolution-log round 11.
- Codex v1.8 by Anima (`docs/Codex.md`, `CLAUDE.md` regenerated): P-007, the M2d section (K-M2d, S-US-130..S-US-138, X-M2d), Charter human gate 3, D-15 chain, execution order, amendment log.
- Verification: docs only; no code changed.


## X-M2c: Exit review M2c (Mraw) — 2026-09-30

**State:** Done; `qa` merged into `main`, tag `m2c-done`.

- Evidence: `docs/gates/M2c.md`, `docs/evidence/X-M2c/` (a scripted session: paint, place, rename, save; then load, play and strike).
- Fix: scripted input with two holds of the same intent no longer presses it on every tick (`src/luna/engine/application.cpp`); regression ctest `X-M2c Two scripted presses`; ctest 25/25 in Debug and Release, zero warnings.


## US-126 / S-US-126: Level and character settings (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. Last story of M2c.

- Game: the Editor's Level panel sets the level's name, width and height (a resize keeps what was painted and drops characters that fall outside) and the default ground; the hero start marker is dragged with Select; New and Open switch levels and ask first about unsaved changes (Save, Discard, Cancel); every setting is one step of Undo. The game plays the level the Editor has open.
- Engine: hover hints stay on screen (`UiPainter::setScreen`, `keepOnScreen`).
- Docs: `docs/guides/editor.md`, the owner's guide to every control.
- Tests: `tests/game/settings_test.cpp` (4 cases) and a UI case; ctest 24/24 in Debug and Release, zero warnings.


## US-125 / S-US-125: Place characters (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. Sixth story of M2c.

- Game: the Editor places characters from a palette of every kind (hero, wanderer, 10 monsters); Select picks one by clicking it, drags it to move it, R turns it, Delete removes it; a properties panel edits its name, HP and sword damage; every change is one step of Undo; saved with the level. In Game mode placed enemies take sword hits, flash red and are defeated; other placed characters stand where they were put.
- Levels: `assets/levels/demo.json` (the original demo) is what every test plays; the owner's edited `valley.json` stays the game's level (delegated decision D-20); level backups are not committed.
- Tests: `tests/game/place_test.cpp` (3 cases), end-to-end `US-125 Place in the game`; the end-to-end tests of US-024, US-029, US-123 and US-124 now play demo.json; ctest 24/24 in Debug and Release, zero warnings.


## US-124 / S-US-124: Paint ground tiles (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. Fifth story of M2c.

- Game: the Editor paints ground: brush (click or drag), rectangle, flood fill and eraser; a toolbar and a tile palette with hints; a grid (G); undo and redo (Ctrl+Z, Ctrl+Y, 100 steps, one step per stroke or fill); Ctrl+S saves the level safely; a status line. Solid ground blocks walking in Game mode.
- Code: `src/game/editor_history.{h,cpp}` (commands, history, line, rectangle and flood fill); `src/game/editor.{h,cpp}` extended.
- Tests: `tests/game/paint_test.cpp` (4 cases, including random undo and redo sequences), end-to-end `US-124 Paint in the game`; ctest 23/23 in Debug and Release, zero warnings.


## US-123 / S-US-123: Game mode and Editor mode (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. Fourth story of M2c.

- Game: F2 opens the Editor (the world pauses; the camera pans with the move keys or a right-button drag; the level, its characters, targets and hero start are shown); F1 plays the level again from the hero start (the play state is rebuilt from the level). The mode is shown in the top-right corner. `odysseus.exe --editor` starts in the Editor.
- Code: `src/game/editor.{h,cpp}` (codex issue CI-007: not in a sub-folder, because of ADR-016).
- Tests: `tests/game/modes_test.cpp` (3 cases), end-to-end `US-123 Modes in the game`; ctest 22/22 in Debug and Release, zero warnings.


## US-122 / S-US-122: Levels as data (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. Third story of M2c.

- Data: `assets/data/tiles.json` (16 ground kinds), `assets/data/characters.json` (12 character kinds), `assets/levels/valley.json` (the demo that used to be code).
- Game: `level.{h,cpp}` (definitions; level model; validated reading with file and field in every error; safe saving with 3 backups; falling back to a backup when damaged); the game starts from a level (`--level <file>`); enemies are the level's placed characters, each drawn with its own art; the ground strip follows tiles.json.
- Tests: `tests/game/level_test.cpp` (3 cases), US-120 tests adapted; ctest 21/21 in Debug and Release, zero warnings.


## US-121 / S-US-121: Point, click and read on screen (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. Second story of M2c.

- Platform: mouse movement, buttons and wheel, typed text, and the editor's keys (F1, F2, Delete, Backspace, Ctrl, Z, Y, G, R).
- Engine: new intents (ModeGame, ModeEditor, Undo, Redo, Save, Delete, ToggleGrid, Rotate, Erase, Confirm) and the `Pointer` in virtual pixels; scripted pointer and typing; the UI toolkit (`ui.{h,cpp}`: 5x7 font, 12 colours, button, list, number and text fields, panel) and `ImageRenderer` for pixel tests.
- Program: `odysseus.exe --click / --drag / --point / --type`; every intent usable with `--hold`.
- Tests: `tests/luna/ui_test.cpp` (4 cases), SDL mouse and text translation; ctest 21/21 in Debug and Release, zero warnings.


## US-120 / S-US-120: Real art in the game (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. First story of M2c.

- Engine: PNG reading and writing (stb from vcpkg, ADR-018) and image operations (crop, box-filter fit, mirror, background removal, blob finding).
- Game: `art.{h,cpp}` cuts the owner's sheets by `assets/sprites/cuts.json` into atlases, loads them, and gives the game its pictures (programmer art when the atlas is missing or damaged, with the file and reason logged). The hero, the ground and the demo enemy now come from the owner's art.
- Program: `odysseus_atlas` (writes `assets/sprites/atlas/`, `--preview` contact sheet, `--find` to measure a sheet).
- Data: the owner's sheets committed unchanged; `cuts.json` (74 character frames, 16 tiles); the atlas.
- Tests: `tests/game/art_test.cpp` (5 cases); ctest 21/21 in Debug and Release, zero warnings.


## US-115 / S-US-115: Tell the clan's story in episodes (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. Last story of M2b.

- Simulation: episodes derived from the chronicle (no new state, so the hash and the save format are unchanged). Events and their causes are linked; the hardship events of a hard season are joined; the best groups (at least 3 events, one important) are told, at most 40 a century, each as one named paragraph: "The Hard Winter of year 2. It began in Summer, year 2: ... The turn came in Winter, year 2: ... It ended in Spring, year 3: ... Those who lived it: ...". Names by theme: hardship, feud and vengeance, hunting, sickness, love, apprenticeship.
- Headless runner: `--story` prints the episodes, then the births, deaths, pairings, partings and feuds with their reasons; `--chronicle` still prints every event.
- Data: `story.json` section `episodes`.
- Code: `src/sim/episodes.{h,cpp}` (new).
- Tests: `tests/sim/story_episode_test.cpp` (3 cases) and the command-line check `US-115 Both views`; ctest 21/21 in Debug and Release, zero warnings.
- Balance (10 seeds x 100 years): 29 to 51 alive; every seed tells the maximum 40 episodes.


## US-114 / S-US-114: Teach the young and hunt together (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. Fifth story of M2b.

- Simulation: apprenticeship (a skilled well adult takes a youth of 12 to 15 in the master's better skill; the apprentice learns a point a day, master and youth grow close, graduation at 16 or near the master's skill; Apprentice and Graduation events). Hunting parties: a mammoth sighting gathers 3 to 5 well hunters (else the sighter hunts alone as before), with a leader, a hero and a coward; the party's strength decides success; danger and rescue (Rescue event, the rescued keeps a debt of gratitude for life); every member remembers what the others did (new memory kinds Heroism and Cowardice). `bringDownMammoth` now returns its event and can cite the party.
- Data: `story.json` sections `teaching` and `hunt`.
- Save: new person fields `master`, `apprentice`, `teachHunt`, `teachEvent` (version 3; upgrade from 2 fills defaults; validated on load). All in the world hash.
- Code: `src/sim/world_hunt.cpp` (new); `joinNames` moved to `chronicle.cpp`.
- Tests: `tests/sim/story_hunt_test.cpp` (6 cases); ctest 20/20 in Debug and Release, zero warnings.
- Balance (10 seeds x 100 years): 29 to 51 alive. Seed 7: 205 apprenticeships, 73 hunting parties, 61 rescues, 2 cowards.


## US-113 / S-US-113: Court and compete for a partner (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. Fourth story of M2b.

- Simulation: pairing is now a courtship. An unpaired adult courts the one they like best (at least 20); gifts and time together warm the loved one by 2 a day; a loved one who thinks ill of the suitor turns them down (Rejection); two people pair only when both think enough of each other and the loved one chooses (Pairing cites the courtship). Rivals grow jealous (Jealousy event with causes, a lifelong Rejection memory, a grudge, maybe a quarrel that names the reason). Partners of whom either thinks less than -20 of the other part (Parting says why: "parted after a bitter quarrel", "over stolen meat", "as their love faded"). `World::pair` returns the event id; `World::part` is new; the dead and the exiled are no longer courted.
- Data: `story.json` sections `courtship`, `rivals`, `parting`.
- Save: new person fields `courting`, `courtDays`, `courtEvent`, `courtPauseDay` (version 3; upgrade from 2 fills defaults; validated on load). All in the world hash.
- Code: `src/sim/world_love.cpp` (new); `heaviestGrudge` moved to `person.cpp`.
- Demo (outside the Codex, owner request): hero sword on Shift, standing enemy with HP, red hit flash (branch `chore/sword-enemy-demo`); Bow stays the default weapon so the US-029 tests hold.
- Tests: `tests/sim/story_love_test.cpp` (9 cases); ctest 20/20 in Debug and Release, zero warnings.
- Balance (owner limit: 10 seeds x 100 years): 27 to 45 alive (before: down to 9). Seed 7 story: 109 courtships begun, 46 pairings, 4 turned down, 6 jealousies, 0 partings; partings are rare because partners rarely fall out (tuning pass before the gate).


## US-112 / S-US-112: Share food and nurse the sick (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. Third story of M2b.

- Simulation: sickness (likelier when hungry or cold; the sick cannot work; it can kill, "died of the sickness, weakened by hunger"), hunting wounds, nursing by the kind and close (faster recovery, fewer deaths, gratitude kept for life), sharing food in a famine, orphans taken in (guardians count as family). New file `src/sim/world_care.cpp`; `Person` gains carer, nursing, guardian.
- Data: `story.json` sections `sickness`, `nursing`, `sharing`, `adoption`.
- Tuning: wound and sickness numbers chosen on 100 seeds x 100 years (9 to 53 alive, median 38); the low end is lifted by courtship (US-113) and a final pass.
- Tests: `tests/sim/story_care_test.cpp` (11 cases).

## US-111 / S-US-111: Quarrel, blame and take revenge (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. Second story of M2b.

- Simulation: every morning each person meets someone (often a person they hold a grudge against); people who dislike each other quarrel (more when hungry or tired); grieving kin blame the thief they know of or the one who struck the blow; a feud that keeps worsening ends in an attack: a fight (injury, sometimes death) or, when the clan is against the aggressor, an exile. New: `Health` (wounds heal or kill), exile (`Person::exiled`, counted apart from the dead), death causes "a fight" and "wounds", feud records (start event, since, last revenge).
- Data: `story.json` sections `quarrel`, `blame`, `revenge`, `health`.
- Code: `src/sim/world_story.cpp`; `world.{h,cpp}`, `person.h`, `memory.h`, `ai.{h,cpp}`, `save.cpp` (feud records, health, exile), `report.{h,cpp}`.
- Tests: `tests/sim/story_quarrel_test.cpp` (8 cases), shared helpers `tests/sim/story_helpers.h`, the version-2 upgrade test now also covers feuds and health.
- Balance: 30 seeds x 100 years, no crash or extinction (17 to 50 alive).

## US-110 / S-US-110: Give every death and feud a reason (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. First story of M2b (Kill Gate 1 retry).

- Simulation: chronicle entries carry an id, an `EventKind`, up to three people and the ids of their causes (`Chronicle::record`, `find`, `explainEvent`); grudges (`Grudge`) and memories know their event; thefts are events (seen or not); the empty store names the thieves and the failed harvest; deaths by hunger name the store and the thief ("Hano died of hunger in the hard winter, after Brak stole from the store."); feuds name their strongest grudge ("over stolen meat"); a lean autumn (20% a year) is a new event and the root of hard winters.
- Data: `assets/data/sim/story.json` (season, causes), validated with errors like `season.leanAutumnPercent`.
- Saves: version 3 (event links, grudges, story stream, lean flag); versions 1 and 2 upgrade.
- Runner: `[#id]` in chronicle lines, `--why <id>`.
- Tests: `US-110 ...` cases in `tests/sim/story_test.cpp`, a real version-2 upgrade in `save_test.cpp`, ctest `US-110 Traceable`; existing texts updated ("died of hunger", "over stolen meat").

## Kill Gate 1 pivot: requirements v1.6, Codex v1.6, P-005 (Mraw, Anima) — 2026-09-30

**State:** On `qa`.

- Owner verdict on Kill Gate 1 (D-GATE-M2): Pivot. Owner's redesign choices recorded as D-18; retry gate D-GATE-M2b open.
- Requirements v1.6 and backlog (Drive, mirrored): STO-02, STO-03, SDC-02; milestone M2b Story engine; epic E11; US-110..US-115; timeline shifted 15 weeks; 12.6 statuses updated.
- Codex v1.6 (Anima): P-005, K-M2b, S-US-110..S-US-115, X-M2b; decision table; execution order; CI-006 resolved; header without AI-vendor names.
- P-005: docs/status.md (X-M2 failed, M2b prompts), docs/decisions.md, CI-006 fix in `tests/luna/run_game_window.cmake` and `CMakeLists.txt` (first frame within 3 s in Release, 15 s in Debug).

## X-M2: Exit review M2 = Kill Gate 1 (Mraw) — 2026-09-30

**State:** On `qa`. **Waiting for the owner** (human gate D-GATE-M2); `qa` is not merged into `main` and M3 does not start until the answer.

- `docs/gates/M2.md`: 100 years without crashing (13 seeds, all finished) met; determinism met (same 100-year hash in Debug and Release: 6887756077218264421); "2 of 3 readers find a story" needs people: `docs/gates/M2-reader-packet.md` (instructions, three questions, the seed-7 chronicle, 244 entries) and `docs/decision-requests/D-GATE-M2.md` (options Go / Go with a "why" layer / Pivot / Stop; recommendation: Go if 2 of 3 find a story).
- Fixes found while preparing the gate: names are now unique among the living (`US-014 No two living people share a name`); an empty food store is a major chronicle event (importance 70) so famines show.
- `docs/decisions.md`: D-GATE-M2 (open); `docs/status.md`: X-M2 blocked on the human gate.
- Verification: 0 warnings; ctest 19/19 Debug and Release.

## US-016 / S-US-016: Save and load the simulation (Mraw) — 2026-09-30

**State:** Accepted; merged into `qa`. Last story of M2.

- Simulation: `save.{h,cpp}` (versioned JSON saves of the whole state, temp file then rename, three backups, fallback to the newest intact backup, upgrade from version 1, clear refusals of newer or inconsistent saves); `World` befriends `WorldArchive`.
- `odysseus_headless --save <file>`, `--load <file>`.
- Tests: `US-016 Round trip`, `US-016 Crash-safe`, three backups, `US-016 Old version`, inconsistent saves.
- Evidence: 50 years + save + load + 50 years = 100 years straight (same hash).
- Docs: plan `docs/plans/US-016.md`, teach-back entry.
- Verification: 0 warnings; ctest 19/19 Debug and Release.

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
