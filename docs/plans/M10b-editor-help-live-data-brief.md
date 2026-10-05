# Build brief: M10b Editor help and live data

Status: **Handed to Anima (Codex v2.13).** Owner decisions: D-58 (docs/decision-requests/D-58.md, Decided 2026-10-05). Written by Dominus (Mraw) at the owner's request, 2026-10-05. Builders follow the Codex prompts, not this brief.

## 1. Goal

Make the Editor easier to use and quicker to test. Every field in every editor explains itself (a tooltip) and offers the values it accepts (a suggestion list). Anything the owner saves (in the Editor or in a text editor) shows in the running game without a restart. A level edit is never hidden by an older run save.

Source of truth: Project Odyssey.docx v2.12 (Round 25) holds epic E29 (M10b), stories US-300..US-305 and requirements EDT-09, EDT-10, EDT-11, NFR-09, MVP-18.

## 2. What exists today (2026-10-05)

| Thing | Today | Where |
|---|---|---|
| Hover hints | Buttons and palette entries only (`Button::hint`, about 36). Drawn next to the pointer, no delay | `src/luna/engine/ui.h`, `src/game/editor.cpp` |
| Fields | About 41 `TextField` / `NumberField` in three editors: Level Editor panels (level, NPC, class, kind, plant, light...), Building editor, Graph editor. **No hint, no suggestions**; Script has a **Pick** button that cycles files | `editor.cpp` (28), `building_editor.cpp` (9), `graph_editor.cpp` (4) |
| Level save | Ctrl+S writes the level; F1 restarts play from the in-memory level, so level edits already show in play | `OdysseyGame::switchMode` |
| Data reload | Graph editor Save and F5 reload interactions and dialogues; F5 also reloads NPC Classes and partner defaults; building kind saves reload buildings | `OdysseyGame::reloadInteractions`, `odyssey_game.cpp:1846` |
| Not reloaded | `objects.json`, `lights.json` (guides say "restart the game"); anything edited outside the Editor until F5 | `docs/guides/editor.md`, `interaction-data.md` |
| Run save on launch | `things.json`, `buildings.json` and the people are restored over the level, so a level edit (moved plant, changed NPC) can be hidden by the older run state | `OdysseyGame` load at `odyssey_game.cpp:1244-1253` |

## 3. Owner decisions (D-58, summary)

| Q | Decision |
|---|---|
| Placement | New milestone **M10b**, queued **after X-M10**, before M11. Full Amek workflow |
| Tooltip | **Purpose + range + example**, one or two short lines, same look and hover behaviour as today's button hints |
| Suggestions | **Dropdown list** under the field while typing, filtered by what is typed; Up/Down choose, Tab/Enter accept, Esc closes |
| Field scope | **All three editors** (Level, Building, Graph); a test fails when any field lacks a tooltip, so new fields get one too |
| Numbers | Suggest the **default** (kind/class), the **min** and **max**, and the **last 5 values typed** in that field this session |
| Help source | **Data file** `assets/data/editor/help.json`: one entry per field; edited without a rebuild, hot-reloaded, ready for translation |
| Live data | **Hot-reload every data file** the Editor saves: level, NPC classes and kinds, prefabs and building kinds, dialogues, interactions, plus `objects.json`, `lights.json` and the rest of `assets/data/` |
| Reload trigger | **On every Editor save, plus a file watch**: files changed outside are noticed within 1 s. **All or nothing** like F5: a broken file keeps the last good data and shows its mistakes |
| Things in play | **Follow the new data at once**: placed lights, objects and NPCs without own values take the new values; a deleted kind leaves a red "missing kind" marker and a warning, never a crash |
| Run save vs level | **Level edits win**: when the level was saved after the run's save, every thing changed, added or removed in the level comes from the level; untouched run state (clan, hero, ripening berries) stays |
| Exit gate | **Automated tests + a 10-minute owner walkthrough** (`docs/gates/M10b-walkthrough.md`) |

## 4. Design proposal (for the architect; K-M10b design doc fixes it)

Technical choices below are Dominus's (implementation, not design); the architect may change them in `docs/plans/M10b-editor-help-design.md` with a reason.

### 4.1 help.json

```jsonc
// assets/data/editor/help.json
{ "version": 1,
  "fields": {
    "npc.sword":      { "purpose": "Damage of one strike", "range": "0 to 999", "example": "wolf 8", "suggest": "number" },
    "npc.script":     { "purpose": ".dlg file used with this partner", "example": "elder-fire.dlg", "suggest": "files:dialogue/*.dlg" },
    "npc.classes":    { "purpose": "NPC Classes, comma separated", "example": "trader, elder", "suggest": "catalog:npc-classes", "list": true },
    "plant.own":      { "purpose": "Own interaction values for this plant", "example": "gather.delay=60", "suggest": "catalog:interaction-fields" }
  } }
```

- Field ids are `<panel>.<field>`, stable, given in code where the field is made (`field.helpId = "npc.sword"`).
- `suggest` sources: `number` (default, min, max, last 5), `files:<folder>/<glob>`, `catalog:<name>` (npc-classes, npc-kinds, partner-types, interactions, interaction-fields, light-kinds, objects, plants, characters, items, building-kinds, prefabs, quests, levels), `values:<a>|<b>|...` (a fixed list), `none`. `list: true` completes the item after the last comma.
- A missing or broken `help.json` never stops the Editor: fields show no tooltip and the status line names the mistake.

### 4.2 Luna widgets (game-agnostic, Charter rule 9)

- `TextField` and `NumberField` gain `hint` (drawn like `Button::hint`) and `suggest` (a function from typed text to a list of strings). `NumberField` suggestions are numbers shown as text.
- A new `SuggestList` overlay under the focused field: up to 8 rows, scrolls; prefix matches first, then substring, case-insensitive; Up/Down, Tab/Enter accept, Esc closes, a click on a row accepts. It never steals keys when closed.
- The game side (`src/game/editor_help.*`) loads help.json, resolves `suggest` sources against the current catalogs, and fills `hint` and `suggest` for every field when a panel is built.

### 4.3 Live data (`src/game/data_reload.*`)

- One registry of reloadable data sets: each with its files (path or glob), an all-or-nothing reload function (read into a side registry; only a clean result replaces the data in use, like `reloadInteractions`) and what must follow it (placed things, Editor palettes, help suggestions).
- Editor saves call `reload.changed(path)` after the write. A watcher polls `last_write_time` of the watched folders every 250 ms (no new dependency) with a 300 ms debounce, so a change is live within 1 s; writes made by the game itself are ignored once.
- F5 stays and reloads everything.
- Placed things keep their ids and own values; anything without an own value follows the new kind or class. A placed thing whose kind is gone draws a red "?" marker, is skipped by play, and the warning names the level entry.
- The open level changed outside the Editor: reloaded when the Editor has no unsaved changes; otherwise the status line says so and nothing is overwritten.
- Budget (NFR-09): a reload of one data set under 100 ms on the D-06 PC; the frame never stalls more than one frame.

### 4.4 Level edits win over the run save

- Every run save (`things.json`, `buildings.json`, people) also stores the level's **baseline**: per placed thing (plants and objects, characters/NPCs, pickups, effects, lights, level buildings) its id and a hash of its level entry. Save version bump with a migration: an old save without a baseline keeps today's behaviour once, then writes one.
- On load, each id is compared with the level now: same hash keeps the run state; changed or new comes fresh from the level; gone from the level is removed from the run. The status line says how many things the level updated.
- The same rule runs when the Editor saves while a run is loaded, so F1 after a save never resurrects old run state.

## 5. Stories (epic E29, milestone M10b)

| Story | Title | Size | Depends on |
|---|---|---|---|
| US-300 | Field tooltips from help.json | M | US-123, US-173 |
| US-301 | Suggestion list widget | M | US-300 |
| US-302 | Suggestions on every field | L | US-301, US-260, US-170 |
| US-303 | Live reload of every data file | L | US-156, US-260 |
| US-304 | File watch for outside edits | M | US-303 |
| US-305 | Level edits win over the run save | L | US-303, US-153 |

Order: US-300, US-301, US-302, US-303, US-304, US-305. Estimate 3 / 4 / 6 weeks.

### Acceptance criteria (Given / When / Then)

**US-300** As the owner, I want every Editor field to say what it does, its range and an example, so that I never need the guide to fill a form.
- Hover: Given the NPC panel, When the pointer rests on Sword, Then a tooltip shows purpose, range and example from help.json.
- Coverage: Given all panels of the Level, Building and Graph editors, When the coverage test builds them, Then every field has a help id with an entry, and a missing one fails the test by name.
- Broken file: Given a help.json with a syntax error, When the Editor opens, Then fields show no tooltip, the status line names the line, and nothing crashes.

**US-301** As the owner, I want a list of matching values under the field I type in, so that I pick instead of remembering names.
- Filter: Given a field with suggestions trader, trapper, elder, When I type "tr", Then the list shows trader and trapper.
- Keys: Given the list open, When I press Down then Tab, Then the second row is the field's text; Esc closes the list and keeps what I typed.
- Quiet: Given the list closed, When I press Tab, Enter or the arrows, Then they do what they did before M10b.

**US-302** As the owner, I want every field to suggest the values it accepts, so that I never type an id that does not exist.
- Text: Given the NPC Script field, When I type "el", Then the .dlg files of the dialogue folder starting with "el" are listed.
- Lists: Given the Classes field holding "trader, ", When I type "e", Then the classes starting with "e" are listed and accepting one keeps "trader, ".
- Numbers: Given the HP field of a wolf, When it gets focus, Then the list shows the kind default, the min, the max and the last 5 values typed there.
- Fresh: Given a new NPC Class saved, When I open the Classes field, Then the new class is in the list without a restart.

**US-303** As the owner, I want everything I save to show in the running game at once, so that I test changes without a restart.
- Catalog: Given a running game, When I change a light kind's colour in lights.json and save, Then placed lights of that kind shine in the new colour within 1 s.
- Object: Given objects.json, When I add an object and save, Then it is on the Plant palette's object page without a restart.
- All or nothing: Given a broken interaction file saved, When the reload runs, Then the last good data stays in use and the mistakes panel names the file and line.
- Missing kind: Given a placed thing whose kind I deleted, When the data reloads, Then it shows a red "?" marker and a warning naming the level entry, and the game runs on.

**US-304** As the owner, I want files I edit in a text editor to reload by themselves, so that I can work outside the Editor too.
- Outside: Given the game running, When I save an interaction file in a text editor, Then the change is live within 1 s, without F5.
- Own writes: Given the Editor saves a file, When the watcher sees it, Then it does not reload it a second time.
- Open level: Given the Editor open with unsaved changes, When the level file changes on disk, Then nothing is overwritten and the status line says the file changed.

**US-305** As the owner, I want my level edits to win over an older run save, so that what I saved is what I play.
- Edited: Given a run saved, When I move a berry bush in the Editor and save, Then the next start (and F1) shows it in its new place, fresh.
- Untouched: Given the same run, When I start again, Then the berries I picked on another bush are still ripening and the clan and hero are as saved.
- Removed: Given an NPC I deleted from the level, When the run loads, Then it is not in the run and the status line counts the update.
- Old save: Given a save from before M10b, When it loads, Then it behaves as before once and is written with a baseline.

## 6. Requirements (new, docx v2.12)

| ID | Area | Requirement |
|---|---|---|
| EDT-09 | Editor | Field help: every field of every editor has a tooltip (purpose, range, example) from `assets/data/editor/help.json`; a test enforces it |
| EDT-10 | Editor | Suggestions: every field offers a filtered dropdown of accepted values (catalogs, files, fixed lists; numbers: default, min, max, last 5) |
| EDT-11 | Editor | Live data: any saved data file (Editor or outside) applies in the running game within 1 s, all or nothing; level edits win over an older run save by id |
| NFR-09 | Live reload | One data set reloads in under 100 ms on the D-06 PC; a change is live within 1 s; a reload never stalls more than one frame |
| MVP-18 | Editor usability | Tooltips and suggestions on every field; saved changes live in the game without a restart |

## 7. Definition of Done for M10b

Per story: tests written and passing locally in Debug with zero warnings, merged into qa, CI Release green on qa (D-46), guide updated (`docs/guides/editor.md`: tooltips, suggestions, live data; remove every "restart the game"), teach-back entry, Milestone file. X-M10b: `pwsh tools/verify.ps1` Debug and Release, CI on qa and main, the owner walkthrough passed, tag `m10b-done`.

## 8. Risks and cross-discipline notes

- **M11 overlap (Solution Architect):** M11 builds schema forms (ADR-020). help.json is keyed by field id; M11's schema `description`/`examples` should feed the same help entries rather than a second help source. Flag for K-M11.
- **Parallel session (PM):** M10 is being built in the main checkout. M10b starts only after X-M10, so the two never touch `editor.cpp` at the same time.
- **Live reload vs determinism (Tester):** reload changes data mid-run; replays and the determinism tests run with the watcher off (`--no-watch`; the tests and headless runs turn it off, the game has it on).
- **UX:** the suggestion list must not cover the field being typed; open below, or above near the screen bottom.
- **Tech Writer:** the editor, interaction-data and lighting guides drop their "restart the game" lines in US-303.
