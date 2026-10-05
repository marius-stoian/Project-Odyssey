# Project Odyssey: assembly progress (88)

## AP-089 · 2026-10-04 · US-244 Sun and moon shadows

| | |
|---|---|
| Assembly plan / requirements | **v2.9** / **v2.9** (mirror) |
| Repository | `story/US-244` merged into `qa` |
| Milestone | M8c Lighting and shadows, in progress (US-240..US-244, US-248 Done) |

### State
- `tools/verify.ps1 -Story US-244` in Debug: zero warnings; game tests 211 of 211 (one older draw-count test now ignores shadow draws). CI runs only on main (D-51).
- Hero, clan, placed characters, enemies, animals, plants and objects cast shadows along `celestialLight()`; length from catalog `height` (new, optional, with `shadow` and weather `shadowFade`); darkness from sky.json, eclipses and weather. Evidence: `docs/evidence/US-244/morning.png`.

### Decisions
- None requested. Technical: small plants cast nothing by default; shadows are cut from black copies of sprite textures and drawn row by row; sideways shadows get a minimum depth.

### Next
- S-US-245 (shadows from fires), US-246, US-247, X-M8c. K-M9a stops on CI-012.
