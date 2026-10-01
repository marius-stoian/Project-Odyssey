# Plans US-040, US-041, US-042, US-043, US-080, US-083 (M4: Region, tools and saves)

Assembly plan v1.9, prompts S-US-040..S-US-043, S-US-080, S-US-083 (M4). Decisions D-31 (Dominus, delegated; owner 2026-10-01: Dominus decides everything). One combined plan, because the six stories share their parts.

## What was built
| File | What |
|---|---|
| `src/sim/region.{h,cpp}` | (US-040, US-041) the region made from a seed with whole numbers only: integer value noise gives elevation and moisture; water below the lake level and along the contour of a river noise; mountains above 800 with cave mouths; forest where it is wet at middle heights; steppe elsewhere; the rim rises into mountains. Every tile and resource is a pure function of the seed and its place, cut into 32 x 32 chunks made on demand. Resources by biome: flint near water and caves, wood in forest, berries on forest edges, herds (5-12 animals) on steppe; berries regrow 14 days after picking in summer and autumn, the rest never. The start (water and food within 20 tiles, walkable to both) is found by a search from the middle; if the land offers none, a small fair start is carved (rare). Data: `assets/data/sim/region.json` |
| `src/sim/rivals.{h,cpp}` | (US-042) two rival clans of 10-20 people, each its own `sim::World` at least 60 tiles from the player's camp and 40 from each other; levels of detail (`Tier`: active within 30 tiles every tick, nearby to 120 tiles once a second, distant every 10 s); when a season changes a clan looks 14 tiles in eight directions and moves camp where food and water are clearly better |
| `src/sim/region_save.{h,cpp}`, `src/sim/save.{h,cpp}` | (US-043) the region is saved as the seed plus the chunks that changed (the harvested resources), with the safe write of the world save (temporary file, three backups) now shared as `writeSaveText`; loading falls back to the newest intact backup and keeps notes |
| `src/luna/engine/chunk_streamer.{h,cpp}` | (US-043) Luna: loads the chunks the view touches at once and the margin around it a few per tick, nearest first, and lets go of far ones |
| `src/game/region_level.{h,cpp}` | the region as a playable 256-tile level: biomes as tiles, wood as trees, berries as edible plants, flint as moss, each herd as one animal, a fire at the start |
| `src/game/odyssey_game.{h,cpp}` | (US-080, US-083) `--region <seed>`; rivals ticking by tier and their camps drawn with name and size; autosave of the clan world (and the region) at each day's end, timed and logged, kept with three backups, `--save-dir`, `--load` with a message when a backup had to be used; developer tools on F12 in Debug builds (clan speed 1x/2x/4x/16x, skip a day, click a person for needs, memories, relationships and AI scores); the Editor is off in a region |
| `src/luna`, `apps` | new key F12 and intent `DevTools` |

## Tests
| Story | Tests |
|---|---|
| US-040 | `tests/sim/region_test.cpp`: `US-040 Variety` (seeds 1 and 2 differ in layout and biome shares; all five biomes exist), `Repeatable` (the same seed twice is identical tile for tile; a lone chunk equals one made in a full pass), `Playable` (30 seeds: a walkable start with water and food within 20 tiles reachable on foot, generation under 10 s) |
| US-041 | `US-041 Biome rules` (every resource sits where its rule says), `US-041 Regrowth` (berries return after 14 days in summer or autumn, wood never) |
| US-042 | `US-042 Spawn` (8 seeds: two clans of 10-20, 60+ tiles from the camp), `LOD` (tiers; a rival simulated at 1 tick a second), `Autonomy` (a year for each rival: the date moves on, and they grow, shrink or move camp) |
| US-043 | `US-043 Delta save` (three changed chunks: only three stored, a file under 2 KB, loads back the same, four saves keep three backups, a damaged latest save falls back to a backup with a note), `tests/game/m4_test.cpp` `US-043 Streaming` (walking 2500 ticks across the region: no chunk in view ever missing, no tick over 50 ms) |
| US-080 | `US-080 Autosave and backups` (a save at each day's end under 200 ms, the latest and three backups, a damaged latest save loads the backup with a message) |
| US-083 | `US-083 Developer tools` (F12 opens the panel, a click selects a person, the speed buttons and the day skip work; in a Release build nothing opens), `US-040 The region as a level` |

## Manual checks
`docs/evidence/US-040/region-1.png`, `region-2.png` (two regions played as levels, the clan at the start fire); `docs/evidence/US-083/dev-tools.png` (the panel; autosave logged at 38 ms).

## Deviations (Dominus, D-31)
- Generation lives in `src/sim/region*.cpp` and `rivals.cpp`, not a `src/sim/worldgen/` folder: the layer include rules work per layer folder.
- Dear ImGui is not in the project's dependencies and cannot be checked without running the game, so the developer tools use Luna's own UI toolkit (same content: inspect, time control, Debug only).
- The game builds the whole 256-tile region as one level (it fits), so the chunk streamer is exercised by tests against the region rather than driving the level; it is ready for larger regions.
- A berry bush cut in the game regrows by the plant rule of US-136 (15 s), while the region's own regrowth rule (14 days) is the model for the clan's foraging; they are joined when the clan gathers from the region (M5).

## Verification
Builds only: Debug and Release compiled with zero warnings; the tests were written with the code and not run (owner, 2026-10-01: no testing until told otherwise). Full verification and CI are owed.
