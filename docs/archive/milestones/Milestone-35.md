# Project Odyssey: assembly progress (35)

## AP-036 · 2026-09-30 · US-122 Done: levels are files

| | |
|---|---|
| Codex / requirements | **v1.7** / **v1.7** |
| Repository | `qa`; `main` at `m2b-done` |
| Milestone | **M2c Level editor**: 3 of 7 stories |
| Next | **S-US-123** (Game mode and Editor mode), then US-124..US-126, X-M2c |

### What happened
- US-122: the demo world is now a file, `assets/levels/valley.json` (ground, hero start, straw targets, goblin); tiles and characters are defined in `assets/data/tiles.json` and `characters.json`; `odysseus.exe --level <file>` plays another level. Saves are safe (3 backups) and damaged files fall back to a backup.
- Tests: 3 new cases; 0 warnings; ctest 21/21 Debug and Release.

### Decisions and codex issues
- D-05 and D-19 decided by you. No delegated decisions; no codex issues.
