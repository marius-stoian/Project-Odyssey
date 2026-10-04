# Project Odyssey: assembly progress (90)

## AP-91 · 2026-10-04 · US-246 Weather and light

| | |
|---|---|
| Assembly plan / requirements | **v2.11** / **v2.10** (mirror) |
| Repository | `story/US-246` merged into `qa` |
| Milestone | M8c Lighting and shadows, in progress |

### State
- Tests written, not run: the owner asked (2026-10-04) to test M8c only once, at the exit X-M8c. `tests/game/weather_light_test.cpp` (dim, lightning, data cases) is in the test target.
- Weather light: `light.dim`, `light.tint` and `flash` in `weather.json`; the ambient light is multiplied by the weather light, blended over the 3 s fade; lightning is a seeded 0.2 s slot hash lifting the ambient 85% toward white. GPU screenshots manual: `docs/plans/US-246.md`.

### Decisions
- None requested. Technical: lightning is Game-side only (hash of the weather seed and a 0.2 s slot), never the simulation's random streams.

### Next
- S-US-247 (lighting in the Editor and quality settings), X-M8c (the one full verify, Debug and Release). K-M9a stops on CI-012.
