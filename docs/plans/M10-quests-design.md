# M10 design: quests and story authoring (written at K-M10, 2026-10-05)

Decisions: D-57 (docs/decision-requests/D-57.md, delegated to Dominus under D-41). Contract: docs/plans/M10-M13-authoring-brief.md sections 4.1 and 4.2. Stories: US-180, 181, 182, 183, 186, 184, 187, 185. Traces: STO-04, STO-05, EDT-06, ADR-019, ADR-020.

## 1. Layers and files
| Layer | New files | Existing files touched (additive) |
|---|---|---|
| Simulation | `src/sim/quest_data.{h,cpp}` (types, loader, writer, objective parser), `quest_book.{h,cpp}` (state machine, event queue, saved state), `quest_check.{h,cpp}` (validation, US-187); flat in src/sim like every other file there | `rule_expr.cpp` (functions `quest(id)`, `step(id)`), `rule_effect.cpp` (verb `quest`), `region_save.*` (quest section, version bump + migration), `CMakeLists.txt` |
| Game | `src/game/quest_host.{h,cpp}` (glue: emits events, runs the book), `quest_ui.{h,cpp}` (journal, tracker, markers), `quest_graph.{h,cpp}` (graph tab, US-184), `quest_debugger.{h,cpp}` (US-186), `story_events.{h,cpp}` (US-185) | `odyssey_game.cpp` (hooks), `editor.cpp/.h` (Quests tab), `game_rules.*` (systems.markers), `CMakeLists.txt` |
| Data | `assets/data/quests/*.json`, `assets/data/story/events/*.json`, `docs/guides/quests.md` | nothing removed until US-185 |

## 2. Quest data (US-180)
One JSON file per quest, `//` comments allowed (parsed with `ignore_comments`); the id equals the file name without `.json`. Fields as in brief 4.1: `id, title, giver, requires[], start, steps{}, fail[], rewards[], journal`. A step has `text, objective, marker, hint{after,text}, next, branches[{if,to}]`. `next` is a step id or `END`. Errors read `quests/<file>.json:<line>: <message>`; a bad quest is skipped and the rest loads (acceptance "Error"). The line comes from searching the source text for the offending key or value (the DOM has no positions).

## 3. State machine (US-180)
`QuestBook` holds one `QuestState` per loaded quest: `status` (locked, available, active, done, failed), `step` id, `stepStartTick`, `progress` (count for the current objective). `QuestBook::update(context, now)` runs once per game second, quests in id order (Charter rule 6):
- locked -> available when every `requires` expression is true (empty list = true).
- available -> active by `quest start <id>` (dialogue effect, giver hand-out) or at once when `giver` is `none`.
- active: when the step's objective is met, the step moves to the first true `branches[].if` target, else `next`. `END` sets done and runs `rewards` through `ActionRunner::runEffects` (actor 0, the hero).
- active -> failed when any `fail` expression is true. Failed is final; the debugger can reset it.

Whole numbers and ids only; no pointers. `QuestBook::hash()` joins the determinism check.

## 4. Events and objectives (US-181)
`QuestEvents` is a small queue in the Simulation: `QuestEvent{kind, subject, amount}`; kinds talk, goto, gather, give, craft, interact, defeat, flag. The Game emits one at each place the world already knows the fact. `update` drains the queue in order and counts matching events against the active step's objective since the step started (earlier events never count). `wait <time>` counts ticks since the step started; `flag <name>` is true while the flag is set. The objective text is parsed once at load and checked against the catalogs by US-187.

## 5. Getting and handing in (US-182)
`giver` resolves at run time: a person id, `role:<role>` = any person with that role, kind or tag (the quest can be taken once), or `none`. Dialogue and interactions get the effect verb `quest start|complete|fail <id>` and the conditions `quest(id)` (status as text) and `step(id)` (current step id, empty when not active). A giver with an available quest gets an extra choice "Do you have work for me?" built from quest data (no .dlg edit needed). Handing in is an objective (`talk <who>` on the last step). The M9c action schema gets the source `quest` that D-54 Q11 deferred.

## 6. Journal, tracker, markers (US-183)
- **Journal:** a tab in the Esc menu with Active, Done, Failed; each entry shows the title, the current step text and, for done quests, the `journal` line.
- **Tracker:** top-right of the HUD, one pinned quest (the newest started; click a journal entry to pin), step text and progress `2/3`; the hint shows there after `hint.after` without progress.
- **Markers:** a pin or edge arrow to the nearest target resolved from `marker` (`tag:`, `object:`, `npc:`, `place:`); `systems.markers` in Game Rules turns them off.

## 7. Play-in-editor debugger (US-186)
An Editor panel on a throwaway copy of the quest book and flags (same idea as `sim::TestPlay`): every quest with status and step; buttons Start, Jump to step, Complete, Fail, Reset; set a flag; skip time. Nothing is written unless the owner saves.

## 8. Quest graph (US-184) and validation (US-187)
A view over the quest file, like the M9 dialogue graph (`NodeGraph` widget, `GraphEditCommand` undo): cards Start, Step, Branch (condition on a wire), Reward, Fail rule, End. Saved by `writeQuest` (canonical order, comments dropped, as `writeDialogue` does), positions in `<id>.quest.layout.json`. `checkQuest` finds: unknown or unreachable step, loop without exit, unknown giver, unknown item/kind/place/interaction in objectives and rewards, unparsable expressions, duplicate ids. It runs live in the Editor and in `tools/verify`.

## 9. Tutorial and story events (US-185)
`assets/data/hero/tutorial.json` becomes `assets/data/quests/first-day.json` (same three steps: gather, interact eat, interact tend). The `Tutorial` class leaves the HUD path; tracker and the elder's bubble show the steps. `crossroads.json` becomes one file per event in `assets/data/story/events/` with a `trigger` condition; the old file stays readable for one version with a warning. Saves from before the migration map tutorial state to quest state (save version bump).

## 10. Order, tests, kill rule
US-180, 181, 182, 183, 186, 184, 187, 185, then X-M10. Per D-41 each story writes its tests (doctest, `US-18x ...` names), builds Debug with zero warnings and merges into qa; X-M10 runs the one full verify (Debug and Release) and CI. If US-184 passes 4 weeks it falls back to a form editor (as D-56 Q20).
