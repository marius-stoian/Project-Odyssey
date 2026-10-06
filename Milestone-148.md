# Project Odyssey: assembly progress (148)

## AP-149 · 2026-10-06 · M11 Data editors (S-US-195 Done)

Codex v2.13, requirements v2.12.

### State
- S-US-195 Done on `qa` (branch `story/US-195`): game rules in `assets/data/rules/<name>.json` (`standard`, `peaceful`): the New Game presets and comforts (moved from `hero/hero.json`), the thresholds of victory, and switches for weather, combat, rivals, tutorial, markers, chronicle and politics. The New Game screen has a Rules row; a level may name `rules` (level version 7); a run keeps its rules in its save (hero save version 2); a rules file in play is live for weather, markers and chronicle. The Data tab shows the Game Rules heading and summary on a rules file.
- Debug build of every program and test executable: zero warnings. Own cases: sim 6 and game 5 pass; the whole Simulation run is 386 of 387 (the one failure is the owner's uncommitted `wanderer.json`); full verification and CI at X-M11 (D-41).
- Guide `docs/guides/game-rules.md`, plan `docs/plans/stories-M11.md#us-195`, teach-back in `docs/learning-journal.md`, evidence `docs/evidence/US-195/`.

### Decisions and Codex issues
- Decided by Dominus (delegated, D-60 Q10): no separate `comfort` section (the comforts are `newGame.comforts`); the switches are read where each system starts, not threaded through constructors; hero.json is still read for the moved keys, with a warning, for one version.
- Codex issues: none new.

### Next
- S-US-196 Routines (profession `day` and `night` blocks with `prefer` weights, the timeline view).
