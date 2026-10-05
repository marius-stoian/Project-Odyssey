# Project Odyssey: assembly progress (77)

## AP-078 · 2026-10-01 · US-231 960x540 virtual screen and window modes

| | |
|---|---|
| Assembly plan / requirements | **v2.4** / **v2.6** (local copy; the Drive copy of the requirements is still v2.5) |
| Repository | `story/US-231` merged into `qa` locally, not pushed |
| Milestone | M8b Resolution and GPU renderer, in progress (K-M8b, US-230, US-231 Done) |

### State
- US-231 passed `tools/verify.ps1 -Story US-231` in Debug: zero warnings, 27 of 27 test groups (`docs/evidence/US-231/`). Release runs in CI on `qa`.
- The game draws a 960x540 virtual screen; window modes and Whole/Fill scaling follow D-44 and requirements v2.6. Pointer mapping uses the presentation rectangle.
- Test coordinates written for the old 480x270 screen were moved to 960x540 (aiming, weapons, animals, plants, arc, m4, pickups, place, settings, paint, weather and the scripted window tests).
- Owner bug fix: a held left or right key always turns the hero that way; the pointer still decides the aim and, without such a key, the facing.
- Hero sprites: the owner's sheet is a turn-around (row S front to three-quarter, row E side views, row N back). The old mapping showed West as a right-facing hero. Each of the eight facings now has its own frames (table in `ArtSet::frame`, `src/game/art.cpp`); the owner approved the picks on 2026-10-01 after trying the build.

### Decisions
- Delegated: none. Owner: use the local requirements v2.6 as the source (the Drive copy is unsynced); the hero faces the key, not the pointer, while left or right is held; diagonals use the sheet's three-quarter frames.

### Next
- Push `qa` and confirm green CI (needs the owner's go, an earlier push was refused by the approval review), sync the Drive requirements to v2.6, then S-US-232.
