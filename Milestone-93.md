# Project Odyssey: assembly progress (93)

## AP-94 · 2026-10-04 · US-260 NPC Classes

| | |
|---|---|
| Assembly plan / requirements | **v2.11** / **v2.10** (mirror) |
| Repository | `story/US-260` merged into `qa` |
| Milestone | M9a NPC foundation, in progress |

### State
- Debug build with zero warnings; tests written, not run (owner: one full verify at X-M9a). `tests/sim/npc_class_test.cpp`, `tests/game/npc_class_editor_test.cpp`.
- NPC Class catalog (`src/sim/npc_class.*`), `NpcClassBook`, Editor Class panel, 7 shipped classes, level version 4 (`classes` on placed characters). Manual checks: `docs/plans/US-260.md`.

### Decisions
- Technical: the 24 class icons are a built-in name list in the Simulation (drawn as pixel art in US-269); class names missing a file in a level only warn; picking classes for a placed NPC is the NPC panel (US-268).

### Next
- S-US-261 (kind defaults and placed-NPC overrides), then US-262..US-270, X-M9a.
