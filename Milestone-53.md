# Project Odyssey: assembly progress (53)

## AP-054 · 2026-10-01 · M4 done: region, tools and saves

| | |
|---|---|
| Assembly plan / requirements | **v1.9** / **v1.9** |
| Repository | merged into local `qa`; nothing pushed since `5125364` |
| Milestone | **M4 Region, tools and saves**: 6 of 6 stories built |
| Next | **K-M5** (Vertical slice feature-complete) |

### What happened
- US-040/041: `--region <seed>` makes a 256 x 256 tile land from a seed: steppe, forest, rivers and lakes, mountains with caves; flint near water and caves, wood in forests, berries on forest edges, herds on the steppe; berries come back after 14 days in summer and autumn.
- US-042: two rival clans of 10-20 people, 60+ tiles away, each its own simulated world running once a second, moving camp with the seasons; their camps are drawn with name and size.
- US-043: the region saves as the seed plus the chunks that changed; a chunk streamer in Luna loads what the camera will show.
- US-080: the game autosaves at the end of each in-game day (38 ms measured), keeps three backups, and loads the newest good one with a message if the latest is damaged (`--save-dir`, `--load`).
- US-083: F12 (Debug builds) opens developer tools: speed 1x to 16x, skip a day, click a person for needs, memories, relationships and AI scores.

### Decided by Dominus (delegated)
- D-31 (docs/decisions.md): integer noise and pure chunks; the rivals as own worlds at the Nearby rate; developer tools on Luna's UI instead of Dear ImGui (not a dependency, cannot be verified here); the whole 256-tile region is one level.

### Skipped because of "no testing"
- 13 new test cases (`tests/sim/region_test.cpp`, `tests/game/m4_test.cpp`) written and compiled, never run; see `docs/gates/M4.md` and `docs/gates/M2d.md` for everything owed (full test run, CI, `main` merge and tags).

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0-M3 | up to the clan on screen | 49 / 49 | built |
| M4 | Region, tools and saves | 6 / 6 | built |
| M5-M6 | | 0 / 19 | M5 next |
| | **MVP total** | **55 / 74** | |
