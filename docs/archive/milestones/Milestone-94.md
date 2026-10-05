# Project Odyssey: assembly progress (94)

## AP-95 · 2026-10-04 · US-261 Kind defaults and placed-NPC overrides

| | |
|---|---|
| Assembly plan / requirements | **v2.11** / **v2.10** (mirror) |
| Repository | `story/US-261` merged into `qa` |
| Milestone | M9a NPC foundation, in progress |

### State
- Debug build with zero warnings; tests written, not run (owner: one full verify at X-M9a). `tests/sim/npc_kind_test.cpp`, `tests/game/npc_kind_test.cpp`.
- Kind files (61 shipped), `resolveNpc` with the D-52 precedence, placed-NPC fields in the level (only when set), F5 reloads classes and kinds. Manual checks: `docs/plans/stories-M9a.md#us-261`.

### Decisions
- Technical: no level version bump in US-261 (the new fields are optional in version 4), so a level saved before this story is saved byte for byte the same (acceptance 'Old level'); this differs from the M9 design note, which expected a bump.
- Technical: a later layer overrides an earlier one for allow and deny (a placed allow can lift a class deny); inside one layer a deny beats an allow.

### Next
- S-US-262 (placed NPCs are full persons), US-263..US-270, X-M9a.
