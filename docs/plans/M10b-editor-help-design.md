# M10b design: Editor help and live data

Status: written at K-M10b (2026-10-05) by Mraw (Architect, Full Stack, Tester). Owner decisions: D-58 (docs/decision-requests/D-58.md) and D-59 (docs/decision-requests/D-59.md). Brief: docs/plans/M10b-editor-help-live-data-brief.md. Stories US-300..US-305, exit X-M10b. Technical choices below are the architect's; a change to anything the owner decided is a design question for the owner.

## 1. Facts found in the code (2026-10-05)

| Thing | Where | Consequence |
|---|---|---|
| `TextField` and `NumberField` have no hint and no suggestion; `Button` has `hint` and draws it in `drawOverlay` next to the pointer, no delay | `src/luna/engine/ui.h` | Add `hint`, `helpId` and `suggest` to both fields; a 0.4 s rest timer (D-59 Q1) for fields only; buttons keep today's instant hint |
| Fields are made inline with `panel.add<TextField>(...)` in five places: Level Editor (`editor.cpp`, 28 call sites, two of them in loops over schedule and trade fields), Building editor (9), Graph editor (`graph_editor.cpp`: a `field` lambda used about 40 times plus the name box, a number and the Test-play field), Story event editor (`story_event_editor.cpp`: a `text`/`number` lambda, about 14 fields, new in M10) | the four editor files | The brief counted about 41 and three editors. The real number is larger (about 100 field ids) and there is a fourth editor (Story events). D-58 Q5 says "every field in every editor", so the coverage test covers all four; no design change, only a bigger help.json |
| The Graph editor builds one panel per selected card type and its field ids are positional (`at(0)`, `at(1)`) | `graph_editor.cpp:660-745` | Help ids are `graph.<cardtype>.<label>` (for example `graph.line.says`, `graph.quest.giver`, `graph.header.when`) |
| F5 calls `reloadInteractions()` (interactions, dialogues, small talk, quests, all or nothing, report in `interactionReport_`) and `npcClasses_.reload()` plus `refreshTraders/refreshLife/reloadPartnerDefaults` | `odyssey_game.cpp:823`, `:1882` | These become the first entries of the reload registry; F5 keeps calling the registry's "reload everything" |
| `objects.json`, `plants.json`, `lights.json`, `weather.json`, `tiles.json` and the other catalogs are read once at start (lights at `odyssey_game.cpp:123`) | `odyssey_game.cpp` constructor | US-303 adds all-or-nothing reloaders for the catalogs and re-points everything that holds a `PlantDef*` (CI-007 history: `WorldPlant::def`, hotbar weapons, `starters_`, enemy kinds hold raw pointers) |
| A run is restored from `clan.json`, `region.json` (the level of the run), `things.json` (version 2: plant states, timers, flags, quests), `buildings.json`, hero and NPC population | `odyssey_game.cpp:1236-1300`, `:975-1000` | The baseline of US-305 goes into `things.json` as version 3 |

## 2. help.json (US-300)

File: `assets/data/editor/help.json`, comments allowed (the data-file parser already tolerates them), errors reported as `file:line: message` like every other data file.

```jsonc
{ "version": 1,
  "fields": {
    "npc.sword": { "purpose": "Damage of one strike", "range": "0 to 999", "example": "wolf 8", "suggest": "number" },
    "npc.script": { "purpose": ".dlg file used with this partner", "example": "elder-fire.dlg", "suggest": "files:dialogue/*.dlg" },
    "class.tags": { "purpose": "Tags the class gives its people", "example": "trader, elder", "suggest": "catalog:tags", "list": true } } }
```

- Field id = `<panel>.<field>`, set in code where the field is made: `field.helpId = "npc.sword"`. Panels: `level`, `npc`, `class`, `kind`, `plant`, `economy`, `schedule`, `trade`, `building`, `prefab`, `graph.<cardtype>`, `graph.header`, `graph.test`, `event`. Loop-made fields (schedule, trade) get `<panel>.<field name>`.
- A tooltip shows `purpose`, then `range`, then `example`, one short line each (at most three lines, wrapped to the screen). It appears after the pointer has rested on the field 0.4 s (D-59 Q1), disappears when the pointer moves out or the field is clicked, and is drawn by `drawOverlay` like `Button::hint`.
- `range` for a number field is generated from its `minimum` and `maximum` when the entry has no `range`, so the numbers cannot drift from the code. A text entry keeps its own `range` text.
- Validator (`editor_help.cpp`, used by the loader and the test): `version` is 1; every entry has `purpose` and `example`; `suggest` is `number`, `none`, `values:a|b|c`, `files:<folder>/<glob>` or `catalog:<name>` with a known name; unknown keys are warnings, not errors. A missing or broken file never stops the Editor: fields show no tooltip and the Editor status line names the file and line.
- Coverage test (`tests/game/editor_help_test.cpp`): builds every panel of the four editors headless, walks the widget tree, and fails with one line per field that has no `helpId`, a `helpId` without an entry, or an entry no field uses. Fields are found by `dynamic_cast` on `TextField`/`NumberField`, so a field added in M11 or later fails the test until it has an entry. The test also writes the field list per editor to the test log (the counts for docs/gates/M10b.md).
- The entries themselves are written in US-300 from that generated list, not guessed. One help source: M11 schema forms feed their entries into this file (K-M11 step 1).

## 3. SuggestList widget (US-301)

Luna Engine, `src/luna/engine/ui.h` and `ui.cpp`, game-agnostic (Charter rule 9).

- `TextField` and `NumberField` gain `std::function<std::vector<std::string>(const std::string& typed)> suggest`. When it is empty they behave exactly as today.
- Opening: **on focus** (D-59 Q2). The list shows at once with every value (numbers: default, min, max, last 5), typing filters it. A field with no suggestions shows no list.
- Filter: case-insensitive; prefix matches first, then substring matches; duplicates dropped; up to 8 rows, scrolling with Up/Down beyond that. Order inside each group is the source's order.
- Keys, only while the list is open: Up/Down move the highlight; Tab or Enter accept the highlighted row (with no highlight, Enter keeps the typed text as today and Tab does nothing); Esc closes the list and keeps the typed text; a click on a row accepts it; a click elsewhere commits the field as today and closes the list. A closed list takes no key (US-301 "Quiet").
- Accepting writes the row into the field and commits it through `onChange`, as pressing Enter does. For `list: true` fields (comma separated), the filter applies to the text after the last comma and accepting replaces only that part.
- Position: under the field, or above it when fewer than 8 rows fit before the screen bottom. The list is drawn in `drawOverlay` so it sits over the other widgets, and it uses `Panel::drawOverlay` order so only one list is open at a time (focusing another field closes it).
- `NumberField` suggestions are numbers shown as text; accepting parses and clamps to `minimum`..`maximum`.
- Tests (headless, `ImageRenderer` pixels plus key injection): filter ("tr" gives trader and trapper), keys (Down, Tab; Esc keeps typed text), quiet when closed, 8-row limit and scrolling, above/below placement, list-item completion.

## 4. Suggestion sources (US-302)

`src/game/editor_help.*` resolves a field's `suggest` string to a function when a panel is built, reading the live catalogs each time the field gets focus, so a class saved a moment ago is in the list without a restart ("Fresh").

| `suggest` | Source | Fields (examples) |
|---|---|---|
| `number` | `{default, min, max}` plus the last 5 values typed in that field id this session (kept in an `EditorHistory`-like map in `editor_help`, not saved) | HP, Sword, Family, Width, Height, Owner, W, H, Seconds, priority, min age, max age, order |
| `files:<folder>/<glob>` | file names under `assets/data/<folder>` (re-listed on focus) | Script, Talk, Level (building interior), the Graph `goto`/sub-call names, quest files |
| `catalog:npc-classes`, `npc-kinds`, `partner-types`, `interactions`, `interaction-fields`, `light-kinds`, `objects`, `plants`, `characters`, `items`, `building-kinds`, `prefabs`, `quests`, `levels`, `tags`, `places`, `markers` | the loaded catalogs and registries (`npcClasses_`, `interactions_`, `catalogs_`, `lighting_`, `buildings_`, `quests_`, the level's places) | Classes, Tags, Allow, Deny, Does, Colour (light kinds), Own, giver, kinds, marker, Cost items |
| `values:a|b|c` | a fixed list in help.json | role (elder, friend), time words, comparison words |
| `none` | nothing | free text such as `title`, `says`, `note` |

- The number default comes from the kind or class being edited (for example the wolf's HP); the owner of each number field passes a `defaultValue` getter, so `editor_help` never knows the data types (the layer rule: Game code, not Engine).
- Fields that already have a dedicated picker (Script's Pick button) keep it; the button and the list agree.
- Tests: each source returns the expected names on a fixture catalog; the Script, Classes and HP examples of the acceptance criteria; "Fresh" saves a class and reads it again without rebuilding the panel.

## 5. Reload registry (US-303)

`src/game/data_reload.*`. One registry of reloadable data sets.

```cpp
struct DataSet {
    std::string name;                         // "interactions", "lights", "objects", "npc-classes"
    std::vector<std::filesystem::path> watch; // files or folders (recursive) it reads
    std::function<ReloadResult()> reload;     // reads on the side; commits only a clean result; reports file:line mistakes
    std::function<void()> follow;             // what must follow a commit (placed things, palettes, help suggestions)
};
```

- `DataReload::changed(path)` finds the data sets whose `watch` contains the path and reloads each once (several files of one set in one call reload the set once). `reloadAll()` is F5 and reloads every set.
- All or nothing, per data set: the files are read into a side copy; only a result with no errors replaces the data in use and runs `follow`. Errors keep the last good data, go to `interactionReport_` (the existing mistakes panel, now fed by every set with the file and line) and to the status line in red until the next clean reload of that set (D-59 Q3). Warnings are logged.
- Data sets and what follows each:

| Set | Files | Follows |
|---|---|---|
| interactions, dialogue, small talk, quests | `interactions/`, `dialogue/`, `quests/` (what `reloadInteractions` reads today) | `syncEditorActions`, `syncGraphCatalog`, quest book swap (unchanged code, moved behind the registry) |
| npc-classes, partner defaults | `npc-classes/`, `interactions/defaults-*.json` | `editor_.classesChanged`, `refreshTraders`, `reloadPartnerDefaults`, `refreshLife` |
| lights | `light/lights.json` | placed lights and light objects re-point to the new kind; the lighting pass reads the new colour next frame |
| objects, plants | `objects.json`, `plants.json` | the plant catalog is replaced by name; every `WorldPlant::def` is re-pointed by name (a thing holds the name, not the pointer, across the swap); palettes rebuilt; a kind that is gone: see below |
| characters, weapons, animals, materials, tiles, weather, effects, buildings kinds, story events, sim data | the matching `assets/data/` files | each gets a reloader that re-points holders by name; sets that cannot be swapped safely while a run is loaded (the tile atlas, sim region data) are reported by the registry as "applies at the next start" in the status line and never half-applied |
| help | `editor/help.json` | the Editor re-reads help and suggestions |
| level | the open level file | see section 6 |

- Placed things follow the new data unless they have their own value (a placed light with its own colour keeps it). A placed thing whose kind is gone keeps its level entry, draws a red "?" marker (D-58 Q9), is skipped by play and by the simulation, and one warning names the level entry (`level "valley": plant #12 "oak" has no kind in plants.json`). It comes back by itself when the kind returns.
- Toast (D-59 Q3): `Reloaded <file or set>` for 2 seconds on success; failure keeps the mistakes-panel entry and a red status line.
- Budget (NFR-09): one data set under 100 ms on the D-06 PC, and the frame never stalls more than one frame. The reload time is measured per set (as `lastInteractionReloadMs_` is today) and written to the gate file.
- Tests: light colour change reaches placed lights; a new object appears on the object page; a broken file keeps the old data and the report names file and line; a deleted kind shows the marker and the warning and the game keeps running; timing under the budget on the repository's own data.
- Never edit `assets/levels/valley.json`; tests copy the data folder they change (recursively, like the fixed `US-133 Starter set` test).

## 6. File watcher (US-304)

- A polling watcher in `src/game/data_reload.*`: every 250 ms it compares `last_write_time` and size of each watched file (folders: recursive listing, cheap at about 300 files); a changed file is held for a 300 ms debounce (a text editor often writes twice) and then passed to `DataReload::changed`. Worst case from save to live is about 250 + 300 + the reload, under the 1 s of D-58 Q8.
- No new library. It runs in the game's update on the main thread (a poll is a few hundred `stat` calls); nothing reloads from a second thread, so determinism and the swap stay simple.
- Own writes: every save the game makes through `sim::writeSaveText`/the Editor save path registers the path and the new write time (`DataReload::noteOwnWrite`); the watcher skips a change whose time matches, so the Editor's own save reloads once (by `changed`) and not twice.
- The open level: a change on disk is applied when the Editor has no unsaved changes (the level is re-read and the Editor reloads it); with unsaved changes nothing is overwritten and the status line says "valley.json changed on disk" (US-304 "Open level"). The dirty flag already exists for the quit prompt.
- `--no-watch` turns the watcher off. Tests and the headless runs (`--screenshot`, `--quit-after`, ctest groups) pass it, so no determinism test ever sees a reload. A manual `DataReload::changed` in tests does not need the watcher.
- Hidden or temporary files (names starting `.` or `~`, ending `.tmp`, `.swp`, `.bak1`) are ignored.
- Tests (fake clock for the poll): a touched file reloads once after the debounce; two writes inside the debounce reload once; an own write reloads once; `--no-watch` never reloads; an unsaved Editor level is never overwritten.

## 7. Level edits win over the run save (US-305)

- Baseline: a run save stores, per placed thing of the level (plants and objects, people, pickups, effects, lights, level buildings), its id and a 64-bit hash of its level entry (FNV-1a over the canonical JSON of the entry, so it is stable across runs and platforms). It is written into `things.json`, version 3, under `"baseline": { "<id>": "<hash hex>" }`, plus the level file's name and the save time.
- Load: the level is read from the level file; for each id in the level: same hash as the baseline means the run state stays; a different hash or an id not in the baseline means the thing comes fresh from the level and its run state is dropped; an id in the baseline but gone from the level means the thing and its run state are removed. Untouched run state (the clan, the hero, ripening berries of untouched things, quests, flags) stays. The status line reports "the level updated N things".
- Migration (Charter rule 8): `things.json` version 2 has no baseline; loading it keeps today's behaviour once (no merge) and the next save writes version 3 with a baseline. Version 3 loaded by an older build is not a goal. The loader error "saved by another version" keeps working for versions above 3.
- The same merge runs when the Editor saves while a run is loaded, so F1 after a save never brings old run state back.
- Open point to check in the US-305 plan before coding: how `region.json` (the run's own copy of the level) and the level file relate when a run is loaded (`levelFromRegion`), because the merge must compare against the level file, not against the run's copy. If the two cannot be reconciled without a save-format change beyond the baseline, that is a design question for the owner (the S-US-305 prompt says so).
- Tests: a bush moved in the level appears in its new place in a loaded run while a ripening bush elsewhere keeps its state; an added thing appears; a removed thing is gone; a version 2 save loads once without a merge and rewrites as version 3; the clan and hero are unchanged.

## 8. Test plan and exit

| Story | Tests (new files under `tests/game/` and `tests/luna/`) | Group |
|---|---|---|
| US-300 | `editor_help_test.cpp`: loader, validator, broken file, hover timer, coverage over four editors | game |
| US-301 | `suggest_list_test.cpp` (Luna): filter, keys, quiet, limit, placement, list items | luna |
| US-302 | `editor_suggest_test.cpp`: every source, Script/Classes/HP/Fresh examples | game |
| US-303 | `data_reload_test.cpp`: light, object, all-or-nothing, missing kind, timing | game |
| US-304 | `data_watch_test.cpp`: debounce, own write, `--no-watch`, unsaved level | game |
| US-305 | `run_save_baseline_test.cpp`: merge, migration, clan and hero untouched | game |

- Each story runs `pwsh tools/verify.ps1 -Story US-30x` (Debug, zero warnings, every ctest group green); M10b is under the owner-decides rules (D-22), so every story runs the full verification, not the D-41 deferral. CI (D-51/CI-011): CI runs on main at the exit; Mraw reads the Codex "green CI on qa" as "green local verify, CI at X-M10b" until Anima resolves CI-011.
- X-M10b: verify Debug and Release, `docs/gates/M10b.md` with the coverage output (fields per editor) and the NFR-09 timings, `docs/gates/M10b-walkthrough.md` (10 minutes: hover five fields in each editor; pick a class, a dialogue file and a number from lists; change a light kind's colour and add an object in a text editor and watch the game; save a broken interaction and see the mistakes panel; move a bush, save, start the run again), then the owner's Pass or Fail.

## 9. Codex remarks (for Anima, not blockers)

- The Codex and the brief count about 41 fields in three editors; the code has about 100 in four (the Story event editor from M10 is the fourth). Built as D-58 Q5 says: every field in every editor. The final count is in docs/gates/M10b.md.
- The S-US-305 prompt names things "matched by id"; plants, objects and people have ids in the level, other kinds are matched by their index in the level list unless US-305's plan finds a stable id (recorded there).
