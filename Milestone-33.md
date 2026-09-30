# Project Odyssey: assembly progress (33)

## AP-034 · 2026-09-30 · US-120 Done: your art in the game

| | |
|---|---|
| Codex / requirements | **v1.7** / **v1.7** |
| Repository | `qa`; `main` at `m2b-done` |
| Milestone | **M2c Level editor**: 1 of 7 stories |
| Next | **S-US-121** (pointer, font and widgets), then US-122..US-126, X-M2c |

### What happened
- Requirements v1.7 (Drive; E12, US-120..US-126, D-05, D-19, Round 10) and the backlog updated; Codex v1.7 written by Anima and synced; P-006 and K-M2c done (design: `docs/plans/M2c-editor-design.md`).
- US-120: the game draws your hero, ground tiles and a goblin from your sheets, cut into atlases by `odysseus_atlas` (74 character frames, 16 tiles). See `docs/evidence/US-120/contact-sheet.png` and `game-own-art.png`.
- Finding: your "8 directions" hero sheet holds three views (front, side, back) with eight animation frames each; west is the side view mirrored.
- Tests: 5 new cases; 0 warnings; ctest 21/21 Debug and Release.

### Decisions and codex issues
- D-05 and D-19 decided by you. No delegated decisions; no codex issues.
