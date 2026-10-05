# Project Odyssey: assembly progress (38)

## AP-039 · 2026-09-30 · US-125 Done: place characters

| | |
|---|---|
| Codex / requirements | **v1.7** / **v1.7** |
| Repository | `qa`; `main` at `m2b-done` |
| Milestone | **M2c Level editor**: 6 of 7 stories |
| Next | **S-US-126** (level and character settings, the guide), then X-M2c, X-M2c |

### What happened
- US-125: the Editor places heroes and monsters (Place), selects, moves, turns and deletes them (Select, R, Delete), and edits their name, HP and sword damage; all undoable and saved. In Game mode your sword finds them. See `docs/evidence/US-125/editor-placing.png`.
- Your edited valley is kept as the game's level (committed as you saved it); the original demo is `assets/levels/demo.json` for the tests (delegated decision D-20). Your two new sprite sheets (weapons, nature) are untouched and not committed yet.
- Tests: 3 new cases and an end-to-end run; 0 warnings; ctest 24/24 Debug and Release.

### Decisions and codex issues
- D-05 and D-19 decided by you. Delegated: D-20 (which level the game and the tests use). Codex issue CI-007 open (low).
