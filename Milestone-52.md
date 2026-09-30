# Project Odyssey: assembly progress (52)

## AP-053 · 2026-10-01 · M3 done: the clan on screen

| | |
|---|---|
| Assembly plan / requirements | **v1.9** / **v1.9** |
| Repository | merged into local `qa`; nothing pushed since `5125364` (owner: no tests or CI until M4) |
| Milestone | **M3 Living clan on screen**: 3 of 3 stories built |
| Next | **K-M4** (Region, tools and saves) |

### What happened
- US-030: people are drawn from layers (body, hair, outfit, spear) that are recoloured by palettes, so a few small code-drawn pictures make over two thousand different people.
- US-032: the M2 simulation now runs inside the game on levels marked `"clan": true` (or with `--clan`): 20 people appear at a fire, walk between the gathering ground, the hunting ground, the store and their beds as the simulation says, smoothly at 60 frames a second. New level `assets/levels/camp.json`. `--clan-speed 40` fast-forwards for demos.
- US-031: a small bubble over a person who is cold (shivering), hungry, unwell, tired or lonely; hover a person for a panel with exact needs and what they are doing.

### Decided by Dominus (owner: "Dominus decides everything until M7 or M8")
- D-30 in docs/decisions.md: how the clan appears, placeholder layered art, emotes, hover panel. Override any of it.

### Skipped because of "no testing until M4"
- The clan tests in `tests/game/clan_test.cpp` (9 cases) were written and compiled, never run; no full test run, CI, merge to `main` or tag for M2d or M3 yet. All owed at the M4 gate (see docs/gates/M2d.md).

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0-M2d | up to Content and combat | 46 / 46 | built; M2d checks owed at M4 |
| M3 | Living clan on screen | 3 / 3 | built |
| M4-M6 | | 0 / 25 | M4 next |
| | **MVP total** | **49 / 74** | |
