# Project Odyssey: assembly progress (101)

## AP-102 · 2026-10-04 · US-268 Editor NPC panel

| | |
|---|---|
| Assembly plan / requirements | **v2.11** / **v2.10** (mirror) |
| Repository | `story/US-268` merged into `qa` |
| Milestone | M9a NPC foundation, in progress |

### State
- Debug build with zero warnings; tests written, not run (one full verify at X-M9a). `tests/game/npc_editor_test.cpp`.
- NPC panel in the Editor (classes, attitude, family, dialogue per partner type, action ticks, reset), model methods that keep only differences, one Undo step per change. Manual checks and screenshots: `docs/plans/US-268.md`.

### Decisions
- Technical: an NPC cannot be given zero classes (an empty list means 'inherit the kind'); documented in the guide. The Codex wording 'Give gift' action does not exist yet; the test uses `talk`.
- Technical: the panel is made of existing widgets (list boxes, buttons, text and number fields); the partner type and the attitude are cycled by clicking.

### Next
- S-US-269 (Editor kinds tab and map markers: colour ring and icon under each placed NPC), then US-270 (the test level), X-M9a.
