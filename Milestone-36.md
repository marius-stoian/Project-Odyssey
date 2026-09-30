# Project Odyssey: assembly progress (36)

## AP-037 · 2026-09-30 · US-123 Done: F1 plays, F2 edits

| | |
|---|---|
| Codex / requirements | **v1.7** / **v1.7** |
| Repository | `qa`; `main` at `m2b-done` |
| Milestone | **M2c Level editor**: 4 of 7 stories |
| Next | **S-US-124** (paint ground tiles), then US-125..US-126, X-M2c |

### What happened
- US-123: F2 opens the Editor (the world pauses, the camera pans with WASD or the right mouse button, the hero start is shown); F1 plays the level again. `odysseus.exe --editor` starts there. See `docs/evidence/US-123/editor.png`.
- Codex issue CI-007 (low, worked around): the editor code sits in `src/game/editor.*`, not in a sub-folder, because of the layer rule ADR-016.
- Tests: 3 new cases and an end-to-end run; 0 warnings; ctest 22/22 Debug and Release.

### Decisions and codex issues
- D-05 and D-19 decided by you. No delegated decisions. Codex issue CI-007 open (low).
