# Story plans: M11

Per-story plans for milestone M11 (Data editors), merged from the former `docs/plans/US-<id>.md` files. Each story has its own section; the design is in `docs/plans/M11-data-editors-design.md`.

- [US-190](#us-190)
- [US-191](#us-191)
- [US-192](#us-192)
- [US-193](#us-193)
- [US-194](#us-194)
- [US-195](#us-195)
- [US-196](#us-196)

---

<a id="us-190"></a>

## US-190 Schemas for every data file: plan and checks

Design: docs/plans/M11-data-editors-design.md sections 3 and 4. Decisions: D-60 (Q1 to Q4). Traces to EDT-02, ADR-020. Guide: docs/guides/schemas.md.

## Built
- `src/sim/json_text.{h,cpp}`: `JsonLines`, a second reader of a JSON text (comments allowed) that remembers the line of every member and element under its path (`weapons[2].damage`). The parsed document has no positions, so this is what lets an error say "line 23".
- `src/sim/schema.{h,cpp}`: the schema subset of the design (type, properties, additionalProperties, items, required, minimum, maximum, minItems, maxItems, maxLength, enum, default, description, example, ref, format, and at the root provides, loaders, externalFields). `check(root, data, lines)` walks document and schema together and returns issues (an error stops a load, a warning does not) and the links it met (`RefUse`). `collectProvided` walks a `provides` path. `SchemaSet::load` reads `index.json` and the schemas it names; a schema that breaks its own rules is a `DataError` naming the schema file and key. `install`, `installFromFolder`, `checkFile`, `checkLoaded` (throws the first error as "file:line: path: problem"), `issuesForText`.
- `src/sim/schema_index.{h,cpp}`: `buildIndex` reads the whole data folder with the set (files without a schema, files that do not parse, every schema issue, the catalogs the files provide, every use of a catalog entry, broken links) and `checkDrift` compares the member names each loader reads with the fields its schema describes (`keysReadBy`, `quotedWords`, `functionBody`).
- `src/sim/schema_install.h`: the one function a program calls, with no JSON types in it (the programs do not link the JSON library).
- Hooks: `readJsonFile` (every loader that uses it), and the loaders of the rule files through `rules::schemaDiagnostics` in `interaction.cpp`, `npc_class.cpp`, `npc_kind.cpp`, `quest_data.cpp`, `smalltalk.cpp`, `partner_types.cpp`, `building_data.cpp` (kinds, pieces, prefabs); the tutorial. `apps/odysseus/main.cpp` and `apps/headless/main.cpp` install the schemas before anything is read.
- `assets/data/schemas/`: `index.json` and 43 schemas covering all 183 JSON files (the 10 `.dlg` text files are listed as text). Drafted from the shipped data, the loaders' range checks and the comments of the config structs, then reviewed by hand: every field has a description. `quest` and `story-event` (from M10) were rewritten to the final subset.
- `docs/guides/schemas.md`.

## Technical choices (recorded in D-60)
- One schema per file shape, with a glob index (Q1), so the 66 interaction files share one schema and an NPC kind file is described once.
- Errors that make a file unusable are fatal for that file at load; unknown fields warn (Q3). A rule file with a schema mistake is left out and reported like one with any other mistake of its own.
- Optional fields that no shipped file uses yet (a plant's `height` and `shadow`, a light's `flicker`, an NPC kind's `trade` and `schedule`, an interaction's `chronicle`) were found by the drift test and added to the schemas: the test did its job on the first run.
- The numbers of a range come from the loaders. Where a loader's bound is not a number (`config.maximum`), the schema leaves that side open and the loader's own check still applies: the schema is never stricter than the loader.
- The drift test reads names two ways. A name the loader reads in a call (`.at("x")`, `.contains("x")`, `["x"]`, `value("x", ...)`, the `require...` and `read...` helpers) must be in the schema. A name the schema describes must be written somewhere as a string in the loader. Names read from a table (the seven affinities, the opinion events, the attitudes, the four seasons, the five elements) are listed in `externalFields`.

## Tests (tests/sim/schema_test.cpp, tests/game/schema_game_test.cpp, "US-190 ...")
Lines (the scanner with comments and nested paths), Subset (every key of the subset, with the exact messages), Provided, Globs, Coverage (every file of `assets/data/` has a schema and passes it, every link names an entry), Help text (every field has a description), Errors (a value out of range in a copy of needs.json is refused with file, line, field and range), Links (a recipe naming an item no catalog has is listed with file and line), Loaders (every sim loader loads the shipped files with the schemas installed), Rule files (a schema mistake in an interaction file is reported with its line and the file is left out), Drift, Function bodies, Names read by a loader; and in the Game: Game loaders (catalogs, definitions, lights, sky, celestial events, materials, tutorial) and Game errors (a weapon's damage of -4 is refused: `weapons.json:... damage: must be between 0 and 1000 (is -4)`).

## Manual checks (to run at X-M11)
1. `odysseus_sim_tests "-tc=US-190*"` and `odysseus_game_tests "-tc=US-190*"`: all green.
2. Open `assets/data/sim/needs.json`, change `"maximum": 100` to `"maximum": 5`, start `odysseus.exe`: the game stops with `needs.json:N: maximum: must be between 10 and 1000 (is 5)`. Put it back.
3. Open `assets/data/interactions/gather.json`, write `"range": "far"`, press F5 in the game: the mistakes panel names `interactions/gather.json`, the line and `range: must be a number (is text)`, and the last good data stays in use. Put it back.
4. Add a field `"colur": 1` to a plant in `plants.json`: the log says once `is not a field this file knows`; the game still starts.

---

<a id="us-191"></a>

## US-191 Schema-driven form editor: plan and checks

Design: docs/plans/M11-data-editors-design.md sections 5 and 7. Decisions: D-60 (Q5 revised here, Q6, Q7). Traces to EDT-02, CI-007, CI-021. Guide: docs/guides/data-editor.md.

## Built
- **Patch writer** `src/sim/json_patch.*`: `patchJsonText(original, before, after)` saves an edited document by patching the text it came from (changed values where they stand; a container that gained or lost entries rebuilt in the style of the original; everything else byte for byte), `writeJsonText` for a new file, `sameJson`, `leadingComments`. This replaces the "canonical writer" of the design (D-60 Q5 revised): it also keeps the comments inside a file and needs no one-time reformat of the shipped files.
- **Document** `src/sim/data_document.*`: `DataDocument` (a file open for editing: set, insert, remove, move, add, remove and rename a member, undo and redo up to 200 steps, `dirty`, `save` through a temporary file with three backups, `reloadFromDisk`) and `DocPath` (`weapons[2].damage`, `fields["npc.sword"]`). `JsonLines::childPath` writes the same path form, so the validator's issue paths and the form's paths agree.
- **Form model** `src/sim/data_form.*`: `entriesOf` (the entries of a file: an element of each list the schema provides a catalog from, plus `(file)`), `buildRows` (the rows of an entry from the document and its schema: numbers, texts, choices, yes/no, links, lists of words, groups, lists of objects, maps, fields the file leaves out, members the schema does not know, each with its error and its help id), `applyText` (what was typed becomes an edit, or a refusal with the reason), `defaultValue` (a new entry from the required fields), `uniqueName`.
- **The tab** `src/game/data_editor.*`: `DataEditor` (folder, file and entry lists with a search box; the form as Luna widgets, wheel scrolling; Save, Undo, Redo, New entry, Close; refuses to leave unsaved edits; reads a file again when it changes outside and nothing is unsaved). `EditorHelp::setGenerated` takes the entries made from the schemas (`data.<schema>.<path>`): one help source. The Editor has a **Data** button (`src/game/editor.*`); a save calls `OdysseyGame::dataFileSaved`, which reads the sets that watch the file again; the watcher tells the tab about files changed outside.
- **Luna** `src/luna/engine/ui.*`: `Label`, `Toggle` (yes/no that lines up with a text field) and `TextField::invalid` (a red frame).
- **Live catalogs (CI-007, CI-021)** `src/game/odyssey_reload.cpp`: weapons, animals, effects and weather join the `catalog` set. What the play state holds is pointed at the new definition by its name: the shots in the air (`ArcShot`, `Projectile`; a shot whose weapon is gone is dropped), the starter weapons and the Editor's weapon palette (`rebuildWeaponLists`), the weather cycle (it goes on under the same weather), the atlas places of weapons, animals, effects and weather (`rebuildCatalogArt`), `Definitions` (the weapon and looping-effect names). The hotbar, the lights and the effects already held names. `tiles.json` and `materials.json` stay at the next start (the map holds tile numbers).
- Schemas: `required` lists for the 17 catalog and entry shapes, so a new entry is made of what its loader needs; `graph-layout.schema.json` and four index lines for the graph editor's `.layout.json` sidecars (found by the coverage test when the owner made one).
- The JSON library is a public dependency of the game target now (the Editor's header holds a document).

## Technical choices
- Undo is a stack of whole copies of the document (up to 200). The files are small (the largest, `weapons.json`, is 23 KB), so a copy is cheap and the code is simple.
- An edit that does not change the document is no step of undo (typing the same number again).
- A value that does not fit its field is refused and the field shows what the file holds again; the status line names the reason. The file can only hold a value outside its range if it was edited outside the game, and then the row shows the validator's message.
- A list of words with no comma in any word is one comma-separated box; a list whose entries hold commas (small-talk sentences) or mix texts with objects is one row for each entry, so nothing is cut at a comma.
- The keys of a map keep the order of the file (no arrows); list entries have Up and Down.
- Mechanics data (`sim/`, `hero/`) are not swapped live yet: US-194 does it with its Quick check, so that the exit demonstration (one changed mechanic seen in the running game) is possible.

## Tests ("US-191 ...")
Sim: Patch keeps the text (every shipped file, byte for byte), Patch one value, Patch keeps comments and notes, Patch adds and removes entries, Patch a whole document, Patch every value of every file, Patch lists that grow and shrink, Patch objects that gain and lose members; Paths, Edit and save, Undo, Lists and members, Refused edits, Changed outside (the document); Entries, Rows of an entry, Edit from the form, Groups lists and maps, Words, Mistakes in the file show in their row, New entries, Every file has rows (every row of every file has a help id, and typing a row's own text back is no edit). Game: Data tab opens a file, edit and save (one line of the file changes, the game is told the file), keeps notes and comments, undo (five edits, five undos), does not drop unsaved edits, fields groups and lists, new entry, by hand (a click, typing and Enter; Ctrl+S; Esc asks, a second Esc leaves), help (every row of every file finds its help entry), draws; Live catalogs: a weapon's damage changes while the game runs, a shot in the air follows the new definition, a weapon that is gone takes its shots with it, a new weapon is offered at once, a mistake keeps the old catalog, the weather goes on under the same name.

## Manual checks (to run at X-M11)
1. Start `odysseus.exe`, press F2, click **Data**. Folder `(top)`, file `weapons.json`, entry `iron sword`: the form shows its fields; rest the pointer on `damage`: a tooltip with purpose, range and example.
2. Click `damage`, type `5000`, Enter: refused, red message at the bottom, the field is back to `5`. Type `9`, Enter: the bottom line says `*unsaved`.
3. Ctrl+S. Open `assets/data/weapons.json` in a text editor (or `git diff`): exactly one line changed. In the game press F1, pick up the iron sword and strike an enemy: it takes 9.
4. Press F2, **Data**, `plants.json`, **New entry**: a plant named `new-entry`. Give it the `frame` of an existing plant (type its name, for example the one in the first entry), leave `blocks` off, set `inspect`, Ctrl+S; the Editor's plant palette now has it (page of plants).
5. Make five edits in `sim/needs.json`, press Ctrl+Z five times: the file is as it was.

---

<a id="us-192"></a>

## US-192 Picture pickers and cutting frames: plan and checks

Design: docs/plans/M11-data-editors-design.md section 11. Decisions: D-60 (Q12, decided here). Traces to EDT-02, D-05. Guide: docs/guides/data-editor.md ("Pictures").

## Built
- **The cut, without a screen** (`src/game/atlas_cuts.*`): `addCut` checks a request (a sheet of the sprites folder, a rectangle inside it of at least 4 by 4, a name that is new and a name, a target that exists), patches the line into `cuts.json` (a character or a tile) or `content-cuts.json` (a content page; written with `"key": "flood"`) through `sim::DataDocument`, so the lines that were there stay and the new one takes their style, then cuts the atlases again with `rebuildAtlases`; a cut that makes the atlas fail is taken out again. `rebuildAtlases` is the library call of the existing cutter (`loadCuts`, `cutAtlas`, `saveAtlas`, `loadContentCuts`, `cutContent`, `saveContent`, and `writeNormalAtlases` when normal atlases are there already). `cutSheets` and `cutTargets` list what the tool offers.
- **The screens** (`src/game/picture_tool.*`): `PictureGrid` (the frames of a page as pictures, wheel to scroll, name on hover, click to pick), `SheetView` (a sheet at its own size, wheel and right button to move it, left drag for the rectangle), and `PictureTool` with the **picker** and the **Cut tool**; whole-screen, Esc leaves. The pictures are made into textures by the game (`MakeTexture`), so the tool draws with the renderer in use.
- **The Data tab** (`src/game/data_editor.*`): a frame field (`format: frame`) gets a **...** button (`openPicker`); the top bar gets **Cut tool** (`openCutTool`); the chosen entry's frames play in a box at the right of the form (`refreshPreview`, `previewFrames`): the picture its `frame` names, or for `effects.json` and `weather.json` the animation of its own name, at its `ticksPerFrame`. The game gives the tab the sprites folder and the renderer's texture maker (`setPictures`).

## Technical choices (Decided by Dominus, delegated, D-60 Q12)
- The atlas is cut again **by a library call** (`rebuildAtlases` in `src/game/atlas_cuts.cpp`, the functions `apps/atlas` uses): no child process. `apps/atlas/main.cpp` keeps its own loop (it also writes previews and the starters sheet).
- The Cut tool serves **both** cut files: `cuts.json` (characters and tiles, as the Codex says) and `content-cuts.json` (weapons, plants, animals, effects, weather: the frames the data fields name). The frame fields of the data files name content frames, so a Cut tool that only wrote `cuts.json` could not give a weapon or an animal its picture.
- The running game reads the new atlas at its **next start** (its pictures are made into textures once, at start); the preview and the picker read it at once.
- A new content cut is written with `"key": "flood"` and the default tolerance; the owner edits the line for a sheet that needs another key.

## Tests ("US-192 ...")
A cut is added to cuts.json and to content-cuts.json in the style of the file, the atlases are cut again (atlas.json, content.json), everything that cannot be cut is said and changes nothing, a cut that breaks the atlas is taken out again; the picker shows the frames of the atlas as pictures (a page chosen from the list, a click writes the name, Esc leaves); the preview plays the two frames of an effect (the colour in the box changes), shows nothing for an entry without a picture and the frame a weapon names; the Cut tool: the rectangle dragged on the sheet, a name, a target, Cut, the file and the atlas changed, the picker offers the new picture.

## Manual checks (to run at X-M11)
1. F2, **Data**, `animals.json`, an animal: a box at the right plays its picture. Press **...** at the end of `frame`: the pictures of the atlas, the animals page first; click one, Ctrl+S.
2. **Cut tool**: the sheet `Pixel-Art Animal Sprite Sheet.png`, target `animals`, drag round an animal, name it `snow hare`, **Cut**: the message says the atlas was cut again; `git diff assets/sprites/content-cuts.json` shows one added line.
3. Start the game again and give an animal the frame `snow hare`: it is drawn with the new picture.

---

<a id="us-193"></a>

## US-193 Entity editor: plan and checks

Design: docs/plans/M11-data-editors-design.md section 6. Decisions: D-60 (Q8). Traces to EDT-02, INT-02. Guide: docs/guides/data-editor.md ("Copy, rename, delete").

## Built
- **Reference engine** `src/sim/data_refs.*`: `usesOf` lists every place an entry of a catalog is used; `planRename` makes the whole rename without writing (the new text of every file that changes and the list of places); `applyPlan` writes every file (each through a temporary file, one backup) and puts the written ones back when one fails; `replaceWord` (a name as a whole word, outside quoted prose); `validEntryName`. The places come from: the schemas (`ref` fields, `keyRef` maps, texts whose `format` is rule, effect or objective), the entry's own file (the schema's `provides`), the levels (a fixed table: characters, pickups, plants, effects, lights, buildings and the economy tables) and the dialogue files (the `[if ...]` conditions, the `{...}` effect lists, `@when` and `@who`). Files are patched with the Data tab's own patch writer, so a rename changes only the lines that name the entry.
- **Schema**: `keyRef` (the keys of a map name catalog entries) on the item tables of recipes, building kinds and pieces, and the trade tables of NPC kinds and classes; `target.kinds` of an interaction links to the kinds catalog (animals provide it too); quest objectives have `format: objective`.
- **The tab** `src/game/data_editor.*`: Copy, Rename, Delete and Interactions in the top bar; `Question` (the dialog: title, places, the new name, Rename or Delete anyway, Cancel, Esc); `copyEntry`, `beginRename`, `beginDelete`, `confirm`, `cancel`, `interactionsOfEntry`. After a rename the game is told every changed data file and the form shows the new name. The game tells the tab each kind's tags (`setTagsOf`), derived tags included, so a copy offers the interactions of the original. The Editor opens the interaction graph on the first matching interaction (`setOpenInteraction`).

## Technical choices
- A rename is refused while the open file has unsaved changes, because it writes files (including that one): the owner saves or undoes first. A delete is an edit of the open file and so needs no such rule.
- A name that is a catalog entry in two catalogs (a plant is in "plants" and "kinds") is renamed in both: `describeEntry` takes every catalog the schema provides that name to from the same field.
- Words inside rules are matched as whole words, so renaming `berries` leaves `eat-berries` and the quoted sentence `"{hero} shared berries"` alone. The dialog lists every place first; names that exist in two catalogs (an item and a plant called the same) would both change, which the list shows.
- Levels are patched through the same document and patch code, in the level file's own format.
- Saved runs are not rewritten: a run save is the player's, not data.

## Tests ("US-193 ...")
Sim: Words (whole words, hyphens, quotes, names with spaces), Uses (an item's uses across a quest, a dialogue, an interaction and a level's economy), Rename an item (the plan, nothing written by planning, the written files, the dialogue's prose and `eat-berries` untouched, the whole data folder still passes every schema and link, the loaders read the result), Rename a plant that levels and rules name (the plants and kinds catalogs), A rename that cannot be done says why, A plan that fails halfway is undone. Game: Copy an entry, Rename an entry everywhere it is used (the list first, Esc leaves everything, Confirm writes all, the game is told, the folder is consistent), A rename that cannot be done, Delete an entry that is still used (listed, waits, undone by Ctrl+Z), Delete an entry nothing uses, The question has a screen; and in the running game: a copied plant is a plant of the game at once (catalog, level definitions, same interactions).

## Manual checks (to run at X-M11)
1. F2, **Data**, `plants.json`, the `wheat` entry, **Copy**: `wheat-copy` is chosen. Change `inspect`, Ctrl+S. In the Editor's plant palette the copy is there; place it, press F1: right-click it: the same actions as the wheat.
2. `hero/items.json`, the `berries` entry, **Rename**: the list names the quest, the dialogue, the interactions and the level; type `red-berries`, **Rename**. Open `assets/data/dialogue/elder-fire.dlg`: `has(hero, red-berries, 1)` and `take hero red-berries 1`, while the sentence `"{hero} shared berries"` is as it was.
3. **Delete** the `berries` entry: the places are listed and nothing happens until **Delete anyway**; Esc leaves it. After the delete Ctrl+Z brings it back.

---

<a id="us-194"></a>

## US-194 Mechanics and story tuning with a quick check: plan and checks

Design: docs/plans/M11-data-editors-design.md sections 7 and 8. Decisions: D-60 (Q9). Traces to EDT-02, STO-02. Guide: docs/guides/data-editor.md ("Mechanics and the Quick check").

## Built
- **Forms**: every `sim/*.json` and `hero/*.json` already had a schema (US-190) and so opens as forms (US-191); a test opens each and checks it has rows.
- **Quick check** `src/sim/quick_check.*`: `QuickCheck` (loads the data folder, runs the clan a slice of days at a time, `summary()` gives the headless report plus the number of episodes and the world hash), `runQuickCheck` (the whole run) and `formatQuickSummary` (lines "label: now (last run before, change)"). Nothing in it reads a clock, so the same data and seed give the same summary (ADR-011). The Data tab's **Quick check** button (`DataEditor::startQuickCheck`) runs it 30 days at every update on the seed of the level in play (`setQuickSeed`), shows the progress and then the summary in the question screen with the last run of this session beside it; Esc stops it.
- **Mechanics live** (design section 7, left to this story): `OdysseyGame::reloadMechanics` is the new data set `mechanics` (`sim/`, `hero/`, `story/`), taken out of `next-start`. It reads the clan's rules and the hero data whole on the side, checks them against the run in play, and swaps them in one piece: `World::replaceConfig` and the hero data held by the game (`HeroLife` points at it). It runs between frames, so a swap is always between two ticks. `World::configProblem` refuses a change of ticks per day, days per season or the top of the needs scale; a hero file that drops the run's preset or comfort level is refused too. A refusal keeps the old data and says why.

## Technical choices
- The check reads the folder on disk, not the open document: the Data tab saves before the owner expects the numbers, and a check on edits that are not in the files would not be the data the game uses. Unsaved edits are named in the status line.
- No new thread and no child process (D-60 Q9): the 20 years of the shipped data are 560 days, so 30 days per update is about 19 updates.
- The headless program keeps its own loop: it prints more than the check does, and the check shares the same `World` and report, which is what matters for the numbers to agree.
- Rivals' clans and the levels' own clan seeds keep the rules they started with: they are rebuilt with a level or a new run.

## Tests ("US-194 ...")
Sim: the same data and seed give the same summary and another seed another one; slices of days give the same summary as one run; a changed hunger rate shows next to the last run (hungrier clan, `last run` and the change in the lines); a broken file is reported. Game: every mechanics and story file shows as forms; the Quick check runs by updates, shows the summary, the second run shows `last run` and `+0`, it has a screen and Esc closes it; Esc stops a run, a broken file says it cannot be read; in the running game a changed `mealValue` and an item's value are live, a changed days per season and a broken file are refused with the old rules kept; reloadAll reloads six sets.

## Manual checks (to run at X-M11)
1. F2, **Data**, `sim/needs.json`, `dailyDecay` hunger 60, Ctrl+S, **Quick check**: a summary with no `last run` column. Hunger back to 30, Ctrl+S, **Quick check**: the second summary names the first as `last run`.
2. In a game in play, change `mealValue` in `sim/needs.json` to 70 and save: the toast says it was reloaded, and the hero's meals fill more hunger from then on.
3. Change `sim/calendar.json` `daysPerSeason` to 8 and save: the toast says it was not reloaded and why; the game goes on with 7.

---

<a id="us-195"></a>

## US-195 Game Rules page: plan and checks

Design: docs/plans/M11-data-editors-design.md section 9. Decisions: D-60 (Q10). Traces to EDT-03, MVP-09. Guide: docs/guides/game-rules.md.

## Built
- **Rules files** `assets/data/rules/<name>.json` (`standard.json`, `peaceful.json`), schema `rules.schema.json` (index entry `rules/*.json`), loader `sim::loadPlayRules` in `src/sim/play_rules.*` (`PlayRules`, `PlaySystems`, `playRuleNames`, `applyRules`, `describeRules`). Sections: `newGame` (presets, comforts), `victory` (win percent, combined win percent, lose below people, rival follower percent), `systems` (weather, combat, rivals, tutorial, markers, chronicle, politics: each true or false, left out is true).
- **Moved**: the presets, comforts and thresholds of victory leave `hero/hero.json` (the shipped file no longer has them). `loadHeroData(dataDirectory, rulesName = "standard")` reads standard's, then the named file's on top of it, so a set changes what it names and the rest is standard's. A hero.json that still carries them is read for what the rules file leaves out, with a warning in the log, for this version (the migration of the design).
- **Game**: `OdysseyGame::chooseRules` decides the rules (the level's `rules`, else the New Game pick, else standard; a run in play keeps its own) and reads the hero data again under them. The switches: weather (the cycle does not run, the sky is clear), combat (placed characters that would fight become bystanders, `startFight` refuses), rivals (none created), tutorial (the first-day flag is not set), markers (the quest arrow is not drawn), chronicle (`chronicleLine` adds nothing), politics (held for M13). The New Game screen has a **Rules** row (only when more than one file) and shows the presets and comforts of the picked set. A saved change to a rules file is the data set `mechanics` (`rules/` watched): weather, markers and chronicle follow at once, the rest at the next start.
- **Formats with versions**: level version 7 adds `rules` (written only when set; older levels load without it); the hero's save version 2 adds `rules` (`HeroLife::savedRules` reads it before the run is loaded; a version 1 save is standard).
- **The page**: the Data tab on a file of `rules/` shows a heading and a one-line summary of what the saved file switches and the thresholds (`DataEditor::gameRulesSummary`).

## Technical choices (Decided by Dominus, delegated, D-60)
- No separate `comfort` section: the comforts are the `newGame.comforts` list (the design listed both; one list is the same data).
- The switches are read where each system starts or is drawn (a flag test), not threaded through every system's constructor: the simulation's own systems have no switch in them, so the headless run needs none.
- The hero data is read again under the rules at the start of a run or level, not held twice: one `HeroData`, replaced in place.

## Tests ("US-195 ...")
Sim: the shipped rules (values, the two files, hero.json without the moved keys), the picked file's values are used and the rest is standard's, the old place still read, mistakes name the file and field, a threshold changed makes a game won at the new value, the rules are in the hero's save (and a version 1 save has none), the data folder still passes every schema. Game: weather off and no weather happens (level rules and pick), a level that names rules is played under them (combat off: no enemies, nobody provoked) and comes before the pick, the pick changes presets, thresholds and the tutorial, markers and chronicle switches, a saved change to the rules in play is live, the Game Rules page (forms, summary, refusal, drawn).

## Manual checks (to run at X-M11)
1. F2, **Data**, `rules/standard.json`: the heading and the summary line; set `systems.weather` to no, Ctrl+S: the line says `weather off` and the sky clears. Put it back.
2. New Game: a **Rules** row with Standard and Peaceful; pick Peaceful and Start: no enemies and no rival clans.
3. `victory.winPercent` 40 in `peaceful.json`: a game under it ends in victory at 40% of Trade or Religion.

---

<a id="us-196"></a>

## US-196 Daily routines as data: plan and checks

Design: docs/plans/M11-data-editors-design.md section 10. Decisions: D-60 (Q11). Traces to SDC-07, SDC-12, INT-05. Guide: docs/guides/npc-data.md ("Routines").

## Built
- **Weights on a block** (`src/sim/npc_schedule.*`): `ScheduleBlock::prefer` (tag to whole percent, 0 to 500), `weighted()`, the text grammar `... prefer tag=weight,tag=weight` in `parseScheduleText` / `scheduleText`, the JSON key `prefer` in the one reader (`npc_extras.cpp`: class, kind, NPC and level files; `blocksText` writes it), the director's own save (a fourth element of a block, only when set) and its hash (nothing is mixed for a block without weights, so older worlds keep their hash). Schemas: `npc-class` and `npc-kind` (`prefer` on the four block shapes).
- **The director** (`NpcDirector::guided`): the score of every choice, for places, animals, events and partners, is multiplied by the weights of the active block for the choice's tags (the interaction id, the tags its file asks of the target, the tags of the target). The interruption rule is untouched: `stepNear` sends a hungry person to eat and a frightened one home before any choice is made.
- **Professions** (`hero/professions.json`): `day` and `night` in the same block format. One reader: `rules::scheduleFromJson` (a public wrapper of the reader of the class files) loads them in `loadHeroData`; a mistake names `professions[i].day`. Schema `hero-professions` (the block shape with `prefer`); `hunter` and `gatherer` ship with a routine.
- **Clan members** (`SimConfig::routines`, filled by `configForComfort`): a grown-up takes the routine of their profession (`World::professionOf`: `hunter` when the hunting skill is above the gathering skill, else `gatherer`); the active block's weights multiply the scores of the clan's own actions (`Situation::routinePercent`, `actionTags`: work, gather, hunt, sleep, warm, talk, gift, steal, rest, wander). While a need is below `routineDangerBelow` (new in `sim/actions.json`, schema `sim-actions`) the weights are left aside: the needs win. A saved run is restored with the same routines (`replaceConfig` after the hero is loaded, which also brings back the comfort level).
- **The timeline** (`src/game/timeline_view.*`): `TimelineView`, a Luna widget of 24 hours with a block for each entry; dragging a block's left edge moves its start in steps of 15 minutes, never past a neighbour; releasing reports once. In the Editor's schedule form (`Editor::addScheduleRows`) it writes the text of the moved blocks through the setter of the text lines; in the Data tab (`DataEditor::routinesShown`, `moveRoutineBlock`) it is the same edit as typing the time into the `from` field of the form.

## Technical choices (Decided by Dominus, delegated, D-60)
- The tags of an interaction are its id, the tags it asks of its target and the tags of the target: no new field in the shipped interaction files (the Codex asks for tags to weigh; this gives them with nothing to rewrite).
- A clan member's profession is worked out from their skills, not stored: the clan has no profession field, and a stored one would change every save. It is the two the clan's own AI already tells apart (hunt and gather).
- The weights of a clan routine are keyed by the action words; they multiply the AI's score, so a routine with no `prefer` changes nothing and no world hash moves.
- The timeline lives in the Editor's schedule form and in the Data tab; the bar never writes by itself.

## Tests ("US-196 ...")
Sim: weights (product, no weight, zero, nothing), the routine (hunters choose the post in the weighted block every time and at chance outside it), needs win (a starving hunter eats and is at home), one format (text, class file, the director's save and hash, mistakes), professions carry a routine (the same reader as a class, mistakes name the file and field), the clan takes the routine of its profession (nobody sleeps while the routine forbids it, the same world without routines), needs win in the clan (an exhausted person sleeps). Game: the bar's steps and limits, dragging a block of the hunters' routine in the Data tab (the field, the form row, one changed line, the weights kept, `loadHeroData` reads it, Undo), the same bar for a class, the weights typed in the Editor's form and saved with the level.

## Manual checks (to run at X-M11)
1. F2, **Data**, `hero/professions.json`, the `hunter` entry: a bar of 24 hours above the form. Drag the left edge of the second block from 14:00 to 12:00 and let go: the `from` of that block in the form says `12:00`. Ctrl+S, then `git diff assets/data/hero/professions.json`: one line.
2. Editor, select an NPC, type `06:00 work market prefer animal=300; 21:00 sleep home` in **Day**: a bar appears under the line; drag the second block: the text line changes.
3. Quick check (Data tab) before and after changing the hunter's `prefer.hunt` from 200 to 500: the summary shows the effect on the clan.
