# Project Odyssey: assembly progress (160)

## AP-161 · 2026-10-10 · M12 World editing (X-M12 Done)

Codex v2.13, requirements v2.12.

### Done
- X-M12 exit review: the exit demonstration test `X-M12 Exit` (an edited region with a lake, a river, a person, a camp moved, a clan's store and a grudge is a small world file and a new game starts on it). Clean-worktree `verify.ps1 -Config Both` on the owner's PC: Debug and Release 22 of 22 and window group 8 of 8, zero warnings (docs/evidence/X-M12/). `qa` merged into `main`, CI on `main` green (run 37983773115), tag `m12-done`. Gate record: docs/gates/M12.md. M12 is complete.

### Decisions and Codex issues
- Delegated in M12: D-61 to D-68, listed in docs/gates/M12.md for the owner's review. No test was changed. Codex issues: none new; open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020, CI-021.

### Notes
- The owner's uncommitted edits (wanderer.json, level-7.json, dialogue test files, levels 5, 8, 9 in the `odysseus` checkout; goblin.json, wanderer.json, valley.json and sprites in the `odysseus-fix` checkout) were not committed. The clean worktree `odysseus-xm12` can be removed.
- The owner still has to do the manual walkthrough of the Region view (docs/plans/stories-M12.md) when convenient.

### Next
- K-M13 Politics kickoff (design docs/plans/M13-politics-design.md, then US-210..US-216). The owner recruits the eight playtesters for kill gate 2 (docs/plans/M6-playtest-plan.md section 3); agents contact nobody.
