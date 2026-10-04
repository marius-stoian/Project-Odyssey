# Project Odyssey: assembly progress (103)

## AP-104 · 2026-10-04 · US-270 NPC test level

| | |
|---|---|
| Assembly plan / requirements | **v2.11** / **v2.10** (mirror) |
| Repository | `story/US-270` merged into `qa` |
| Milestone | M9a NPC foundation, all stories Done; X-M9a next |

### State
- Debug build with zero warnings; the three `US-270 ...` cases run alone and pass (full verify and Release at X-M9a). `tests/game/npc_test_level_test.cpp`.
- `assets/levels/npc-test.json` with the seven-NPC cast and five `npc-*.dlg` scripts; walk-through checklist in `docs/guides/npc-data.md`; manual checks in `docs/plans/US-270.md`.

### Decisions
- Technical: the level was written by `saveLevel` (so it round-trips byte for byte). Cast names: Tala, Ossa, Harn, Vell, Gur (the owner can rename in the Editor).

### Next
- X-M9a: the one full verify (Debug and Release, strict 3 s first frame on the owner's PC), merge qa into main, CI green, tag m9a-done.
