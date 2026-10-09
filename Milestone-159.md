# Project Odyssey: assembly progress (159)

## AP-160 · 2026-10-09 · M12 World editing (S-US-207 Done)

Codex v2.13, requirements v2.12.

### Done
- S-US-207 Play the edited region: the World row of the New Game screen (Generated and one button per file in `assets/worlds`, `--world <name>`), `OdysseyGame::loadWorld` and a run that begins on a world file (the file's seed, its setup applied: the clan's meals, opinions as written and grudges, the hero's items and debts, the rivals' stores and the camps the owner placed); **Play here** in the Region view (button, or P over the map) that saves the world first and starts a run with a grown hero on the chosen tile, Esc and F2 back to the Region view with the edits and the Editor's level as they were, nothing saved by a test run; `hero.json` version 3 (`world`, `worldHash`) with a copy of the file kept as `world.json`, a warning and the copy when the file changed, `loadRegion(..., makeBase)` that lays a run's changes onto the world's land; runs from before region editing (`hero.json` versions 1 and 2) load as they did.

### Decisions and Codex issues
- Delegated: D-68 Q1 to Q9 (docs/decision-requests/D-68.md). The rows of D-65, D-66 and D-67 were missing from docs/decisions.md and were added. Codex issues: none new. Open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020, CI-021.

### Verification
- Debug build of all targets, zero warnings (`docs/evidence/US-207/build.txt`). The 3 sim and 6 game `US-207` cases pass. Neighbours (`US-0*`, `US-1*`, `US-2*`, `US-3*`): sim 427 of 428 and game 497 of 522 pass; every failure is an NPC case (wanderer attitude and classes, hostile enemies, talk, schedules, trade stock) that follows the owner's uncommitted wanderer.json (and level-7) edit, none touches the world file, regions or run saves (build/us207-sim-neighbours.txt, build/us207-game-neighbours.txt). Not run: the full suites (X-M12).

### Notes
- The owner's uncommitted level and NPC edits (wanderer.json, level-7.json, dialogue test files, levels 5, 8, 9) are still in the checkout and were not committed; `wanderer.json` makes the sim case `US-261 Shipped kinds` fail on the wanderer's attitude (and the game case `US-291 Event`), as before.
- Left out (not in the acceptance criteria): the Quick check on a world (US-194 runs a seed from the data folder), a world picker inside the Editor's Region view (it edits `default`), the tutorial in a Play here.
- Found while writing: the hash constant had a digit missing (the published FNV-1a test value for "a" caught it).

### Next
- X-M12 Exit review: the exit demonstration (reshape the land, place a camp and a person, change a clan, press Play here, see it in the running game), clean-worktree verify Debug and Release, strict 3 s first-frame on the owner's PC; manual checks in docs/plans/stories-M12.md for US-200..US-207.
