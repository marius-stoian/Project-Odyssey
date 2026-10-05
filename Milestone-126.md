# Project Odyssey: assembly progress (126)

## AP-127 · 2026-10-05 · M10 Quests and story authoring (S-US-180)

| | |
|---|---|
| Assembly plan / requirements | **v2.12** (repository copy) / **v2.10** (mirror) |
| Repository | `story/US-180` merged into `qa` |
| Milestone | M10 Quests and story authoring |

### State
- S-US-180 DONE: quest files load with comments, line-numbered errors and a skip of bad files; `QuestBook` runs locked, available, active, done and failed with steps, branches, hints, fail rules and rewards; quest state is saved in `things.json` version 2.
- The quest-event matching that US-181 names (talk, goto, gather, give, craft, interact, defeat, wait, flag) is already in `QuestBook` and tested; US-181 adds the places where the Game reports events.
- Debug build zero warnings; the 15 `US-18*` cases pass (own cases only; the full verify runs at X-M10, D-41).

### Decisions and Codex issues
- No new decision. Deviation: files are flat in `src/sim/` (`quest_data`, `quest_book`), not `src/sim/quests/` as the Codex text says, to match every other Simulation file. Suggested for Anima: CI-017, change the path in the Codex M10 prompts.

### Files changed
`src/sim/quest_data.{h,cpp}`, `src/sim/quest_book.{h,cpp}` (new); `src/sim/rule_expr.cpp` (functions `quest`, `step`), `src/sim/rule_effect.cpp` (verb `quest`); `src/game/odyssey_game.{h,cpp}`, `src/game/builtin_actions.cpp`, `src/game/game_rules.h` (load, tick, save, reload); `tests/sim/quest_test.cpp` (new), `tests/sim/rules_test.cpp`; `CMakeLists.txt`; `docs/guides/quests.md`, `docs/guides/interaction-data.md`, `docs/plans/US-180.md`, `assets/data/schemas/quest.schema.json`, `docs/learning-journal.md`, `docs/status.md`, `CHANGELOG.md`, `Limit.md`, `Milestone-126.md`.

### Next
- S-US-181 Objectives from world events (the Game reports talk, goto, gather, give, craft, interact, defeat and flag events to the book).
