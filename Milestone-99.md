# Project Odyssey: assembly progress (99)

## AP-100 · 2026-10-04 · US-266 Confront

| | |
|---|---|
| Assembly plan / requirements | **v2.11** / **v2.10** (mirror) |
| Repository | `story/US-266` merged into `qa` |
| Milestone | M9a NPC foundation, in progress |

### State
- Debug build with zero warnings; tests written, not run (one full verify at X-M9a). `tests/game/npc_confront_test.cpp`.
- Confront menu (key C and a right-click entry), five data-driven confront files, opinions spread to friends in hearing range, calm and provoke outcomes, creatures with a kind file are NPCs. Manual checks: `docs/plans/US-266.md`.

### Decisions
- Technical: the amounts live in the interaction files (not in opinions.json); `hearingTiles` (12) is the one number in opinions.json. 'Knows the target' means the holder has met it or shares its family.
- Technical: enemies only wind up to strike when hit, so 'about to attack' means winding up; de-escalate cancels the wind-up (70 in 100 by default, a roll of the dialogue stream).
- Technical: the Actions key X and its intent are registered now for US-267.

### Next
- S-US-267 (actions: allow/deny applied to offers, hidden when denied or unmet, the Actions pop-up on key X with requirements in plain words), then US-268..US-270, X-M9a.
