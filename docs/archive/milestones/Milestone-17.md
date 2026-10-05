# Project Odyssey: assembly progress (17)

## AP-018 · 2026-09-30 · after US-011

| | |
|---|---|
| Codex / requirements | v1.5 / v1.5 |
| Repository | `qa` (this snapshot's commit); `main` tagged `m0-done`, `m1-done`, `m1b-done` |
| Milestone | **M2 Console clan simulator: 2 of 7** |
| Next | S-US-012 Let people choose what to do (utility AI) |

### Story just finished: US-011 Give every person needs that change over time
- The console world has a clan: 20 founders with names and ages from `assets/data/sim/clan.json` and `names.json`.
- Every game hour each person gets hungrier, more tired, colder and lonelier (daily rates in `needs.json`; Warmth drops faster in winter). The 24 hourly drops add up exactly to the daily rate.
- A meal raises Hunger, never above 100. Someone whose Hunger stays at 0 for 3 days dies the next morning, and the chronicle records it: "Summer, year 1: Garu died of starvation."
- The world hash now covers every person, the food store and the chronicle; the determinism test still passes.
- CI on `qa`: green ([run 36651673745](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36651673745)).

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 5 / 5 | Done, `m1b-done` |
| M2 | Console clan simulator (KILL GATE 1) | 2 / 7 | In progress |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **16 / 49** | |

### Decisions and codex issues
- No new decisions. Awaiting your review (delegated to Dominus): D-02, D-16, D-17.
- No new codex issues.
