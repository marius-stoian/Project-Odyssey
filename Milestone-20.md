# Project Odyssey: assembly progress (20)

## AP-021 · 2026-09-30 · after US-014

| | |
|---|---|
| Codex / requirements | v1.5 / v1.5 |
| Repository | `qa` (this snapshot's commit); `main` tagged `m0-done`, `m1-done`, `m1b-done` |
| Milestone | **M2 Console clan simulator: 5 of 7** |
| Next | S-US-015 Soak-test the simulation from the command line |

### Story just finished: US-014 Write a readable chronicle
- The clan's story writes itself: couples form (unpaired adults court), children are born and named, sometimes after a parent ("Joro the Second was born to Joro and Tala."), the old die, mothers can die in childbirth, mammoths are brought down, feuds break out and end in peace, and hard winters empty the food store.
- `odysseus_headless --days 2800 --chronicle` prints a century of what is worth telling (importance 50 and above); `--chronicle 1 --threshold 0` shows everything, even the gifts. Read [a century of seed 42](docs/evidence/US-014/chronicle-seed42-100-years.txt).
- Balance across 12 simulated centuries: the valley now has a carrying capacity (a daily forage and game budget), so clans settle at about 30-75 people with lean winters instead of growing to 150+.
- Tuning note on D-02: couples now pair at a mutual opinion of +50 (D-02 said +60) because at +60 too few couples formed and clans died out. The numbers are in `assets/data/sim/life.json`.
- CI on `qa`: green ([run 36654559425](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36654559425)).

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 5 / 5 | Done, `m1b-done` |
| M2 | Console clan simulator (KILL GATE 1) | 5 / 7 | In progress |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **19 / 49** | |

### Decisions and codex issues
- No new decisions. Awaiting your review (delegated to Dominus): D-02, D-16, D-17.
- New codex issue for Anima: CI-006 (US-020's 3-second first-frame check fails intermittently on GitHub's Debug runner: 3150 ms and 8054 ms seen; the re-runs passed).
