# Project Odyssey: assembly progress (122)

## AP-123 · 2026-10-05 · M9 Interaction and dialogue editor (US-173)

| | |
|---|---|
| Assembly plan / requirements | **v2.12** (repository copy) / **v2.10** (mirror) |
| Repository | `story/US-173`, merged into `qa` |
| Milestone | M9 Interaction and dialogue editor |

### State
- US-173 Attach to placed things written. Overlap check first: a placed NPC already had a per-partner dialogue from M9a, so that part only gained the Pick list and the Graph button; nothing was rewritten. New: own interaction values for a placed plant (`overrides`), applied by the action runner through an adjuster, edited in the Editor, saved in the level. 6 US-173 cases ran alone and pass in Debug; the full verify is X-M9.

### Decisions
- Technical: overrides are `delay` (the wait of every `after` effect, the regrow time) and `duration`, in seconds; only placed plants carry them (the Codex example); the range is left alone because the menu checks it before an action starts. A level without overrides is written byte for byte as before.
- Level format: no version bump; `overrides` is optional on a plant and a bad value names `plants[n].overrides[m].field`.

### Files changed in shared code (overlap guard)
`src/game/level.{h,cpp}` (optional field, reader, writer), `src/game/plants.h` (one field), `src/game/editor.{h,cpp}` (plant panel, overrides, Pick and Graph buttons), `src/game/odyssey_game.cpp` (copy of overrides, the adjuster), `src/sim/interaction.{h,cpp}` (`applyPatch`), `src/sim/action_runner.{h,cpp}` (`setAdjuster`), `tests/game/camp.h` (one optional field), `docs/guides/editor.md`.

### Next
- S-US-174 Test-play, then X-M9 (the one full verify, Debug and Release; gate file; merge into main; tag m9-done).
