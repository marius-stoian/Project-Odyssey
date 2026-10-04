# Project Odyssey: assembly progress (85)

## AP-086 · 2026-10-04 · US-243 Fires, torches and glowing effects

| | |
|---|---|
| Assembly plan / requirements | **v2.6** / **v2.8** |
| Repository | `story/US-243` merged into `qa` |
| Milestone | M8c Lighting and shadows, in progress (K-M8c, US-240..US-243 Done) |

### State
- `tools/verify.ps1 -Story US-243` in Debug: zero warnings; CI (Release) on the push to `qa`.
- Fires, burning fire pits, held weapons and clan torches light the dark, with an optional seeded flicker. Evidence: `docs/evidence/US-243/night-camp.png`.
- A failing older test (US-240 ambient) wrote its own lights.json without the new kinds; fixed in the test.

### Decisions
- None requested. Technical: lights fade in with darkness and add nothing by day; flicker is off by default (D-49).

### Next
- S-US-244 (sun and moon shadows), then US-245, US-246, US-247, X-M8c.
