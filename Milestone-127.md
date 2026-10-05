# Project Odyssey: assembly progress (127)

## AP-128 · 2026-10-05 · M10 Quests and story authoring (S-US-181)

| | |
|---|---|
| Assembly plan / requirements | **v2.12** (repository copy) / **v2.10** (mirror) |
| Repository | `story/US-181` merged into `qa` |
| Milestone | M10 Quests and story authoring |

### State
- S-US-181 DONE: the Game reports the hero's talk, goto, gather, give, craft, interact and defeat events to the quest book; wait and flag are looked at once a second. Other people are never counted (the observers check the actor).
- Debug build zero warnings; the five `US-181` cases and the earlier `US-180` ones pass (own cases only; full verify at X-M10, D-41).

### Decisions and Codex issues
- None new. Choice (technical): gather = any item entering the hero bag except crafted ones; give = a `take hero ...` effect; goto = within three tiles of a named level place.

### Files changed
`src/sim/action_runner.{h,cpp}` (finish observer), `src/sim/hero_life.{h,cpp}` (item observer), `src/game/odyssey_game.{h,cpp}`, `src/game/builtin_actions.cpp`, `src/game/game_rules.h`; tests `tests/sim/runner_test.cpp`, `tests/sim/hero_test.cpp`; `docs/guides/quests.md`, `docs/plans/US-181.md`, `docs/learning-journal.md`, `docs/status.md`, `CHANGELOG.md`, `Limit.md`, `Milestone-127.md`.

### Next
- S-US-182 Getting and handing in quests (giver resolution, extra dialogue choice, `quest` effect verb run by the game, `quest(id)` and `step(id)` in the game rule context, the NPC action source `quest`).
