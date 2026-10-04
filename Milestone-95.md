# Project Odyssey: assembly progress (95)

## AP-96 · 2026-10-04 · US-262 Placed NPCs are full persons

| | |
|---|---|
| Assembly plan / requirements | **v2.11** / **v2.10** (mirror) |
| Repository | `story/US-262` merged into `qa` |
| Milestone | M9a NPC foundation, in progress |

### State
- Debug build with zero warnings; tests written, not run (one full verify at X-M9a). `tests/sim/npc_population_test.cpp`, `tests/game/npc_people_test.cpp`.
- `sim::NpcPopulation` (compact store, daily rules, memories, hash, save), placed people become persons at play start, saved in `npcs.json` with the autosave. Manual checks: `docs/plans/US-262.md`.

### Decisions
- Technical, a design choice for the owner to review: placed persons live in their own `NpcPopulation`, not inside the clan `World`. The clan World gives every person a full opinion table and the clan's food store, which cannot scale to 100,000 persons (US-263) and would make every placed NPC a clan member. They follow the same kind of daily rules (needs, age, memories) but not the clan's courtship, feuds or store. Opinions between persons and the hero come in US-264.
- Technical: persons do not die in M9a (no food model); schedules and movement come in M9c.

### Next
- S-US-263 (the NPC store at scale, detail by distance, spatial grid, ADR-022, 100,000-person load test), then US-264..US-270, X-M9a.
