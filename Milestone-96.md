# Project Odyssey: assembly progress (96)

## AP-97 · 2026-10-04 · US-263 The NPC store and detail by distance

| | |
|---|---|
| Assembly plan / requirements | **v2.11** / **v2.10** (mirror) |
| Repository | `story/US-263` merged into `qa` |
| Milestone | M9a NPC foundation, in progress |

### State
- Debug and Release build with zero warnings. Tests written; the four `US-263` sim cases were run once in Release for the ADR numbers and passed (100,000 persons: a day 1.5 ms, worst tick 1.0 ms, save 16 ms and 4.7 MB). Everything else is verified once at X-M9a.
- ADR-022, spatial grid, detail by distance, compact save v2. The `US-263 Frame` case (game with 100,000 far persons) is written, not yet run.

### Decisions
- Technical: the near radius is 800 px (25 tiles) and the grid cell 256 px; budgets are a day under 50 ms, the worst tick under 8 ms, a save under 100 ms. The measurements beat them by 30x or more, so no decision request for the owner is needed.
- Technical: a person is identical at the end of a day whether near or far (the fall through hour h is rate x h / 24), which is how 'no jump, no loss' holds.

### Next
- S-US-264 (attitudes and opinions: nine words, per pair of persons who met, changed by the D-52 events, saved), then US-265..US-270, X-M9a.
