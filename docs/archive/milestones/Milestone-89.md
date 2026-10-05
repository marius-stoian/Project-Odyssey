# Project Odyssey: assembly progress (89)

## AP-090 · 2026-10-04 · US-245 Shadows from fires

| | |
|---|---|
| Assembly plan / requirements | **v2.9** / **v2.9** (mirror) |
| Repository | `story/US-245` merged into `qa` |
| Milestone | M8c Lighting and shadows, in progress (US-240..US-245, US-248 Done) |

### State
- Local `tools/verify.ps1 -Story US-245` in Debug. CI runs only on main (D-51).
- Fire shadows: nearest `fireShadows.maxPerObject` (default 2) shadow-casting lights per thing, away from the fire, faint, night only, off on Low. Tests in `tests/game/shadow_test.cpp`. GPU screenshots manual: `docs/plans/stories-M8c.md#us-245`.

### Decisions
- None requested. Technical: lights with `shadows: true` are the campfire and torch; length 0.4 to 1.5 times height by nearness; a light within 12 px of a thing is its own torch and is skipped.

### Next
- S-US-246 (weather and light), US-247, X-M8c. K-M9a stops on CI-012.