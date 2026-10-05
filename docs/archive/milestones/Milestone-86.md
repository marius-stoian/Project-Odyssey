# Project Odyssey: assembly progress (86)

## AP-087 · 2026-10-04 · US-248 Celestial bodies

| | |
|---|---|
| Assembly plan / requirements | **v2.7** / **v2.9** |
| Repository | `story/US-248` merged into `qa` |
| Milestone | M8c Lighting and shadows, in progress (K-M8c, US-240..US-243, US-248 Done) |

### State
- `tools/verify.ps1 -Story US-248` in Debug: zero warnings; game tests 206 of 206 after fixing three US-155 count checks (objects now include the placeable sun and moon). CI runs only on main (D-51).
- Sun and moon are `celestial` objects of `objects.json`; default pair follows the clock; placed ones come from the Editor palette. `OdysseyGame::celestialLight()` is the one function US-244 uses. Sprites in the sky band; eclipses from `celestial-events.json` dim the world. Data mistakes show a message and the default pair stays.
- Not done here: GPU screenshots (manual checks in docs/plans/stories-M8c.md#us-248).

### Decisions
- None requested. Technical: placed fixed bodies take height from their catalog entry; clock bodies are hidden from the Editor palette; light direction is seen from the hero.

### Next
- S-US-244 (sun and moon shadows: draw from `celestialLight()`), then US-245, US-246, US-247, X-M8c.
