# Project Odyssey: assembly progress (102)

## AP-103 · 2026-10-04 · US-269 Editor kinds tab and map markers

| | |
|---|---|
| Assembly plan / requirements | **v2.11** / **v2.10** (mirror) |
| Repository | `story/US-269` merged into `qa` |
| Milestone | M9a NPC foundation, in progress |

### State
- Debug build with zero warnings; the four `US-269 ...` cases run alone and pass (full verify and Release at X-M9a). `tests/game/npc_kinds_tab_test.cpp`.
- Kinds tab in the Class panel (same rows as the NPC panel, writes `assets/data/npcs/<kind>.json`), and a ring-and-icon marker under every placed NPC with a class, Editor only (`src/game/npc_marker.cpp`). Manual checks: `docs/plans/stories-M9a.md#us-269` (screenshots are the owner's, GPU).

### Decisions
- Technical: the Kinds tab is a second tab of the Class panel, not a new toolbar button (the toolbar is full). The marker pictures are made in memory (24 icon bitmaps in code), no new art files.
- Technical: the marker sits just below the feet so it never hides the sprite.

### Next
- S-US-270 (the NPC test level `npc-test.json`), then X-M9a (the one full verify: Debug and Release, strict 3 s first frame on the owner's PC).
