# Project Odyssey: assembly progress (133)

## AP-134 · 2026-10-05 · M10 Quests and story authoring (S-US-185)

| | |
|---|---|
| Repository | `story/US-185` merged into `qa` |
| Milestone | M10 Quests and story authoring |

### State
- S-US-185 DONE: the tutorial runs as the `first-day` quest; the twelve crossroads events are files in `assets/data/story/events/` with a `trigger` and an `order`; the Editor edits them under Events. All eight M10 stories are written. Debug build zero warnings; five `US-185` cases and the whole simulation test program (336 cases) pass; the full verify, Release and CI are X-M10.

### Decisions and Codex issues
- CI-019: the US-090 tests test the old Tutorial class, so the class and `tutorial.json` stay (unused by the game).

### Next
- X-M10: one full verify (Debug and Release), `docs/gates/M10.md`, merge `qa` into `main`, tag `m10-done`, CI on `main`.
