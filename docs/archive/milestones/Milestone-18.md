# Project Odyssey: assembly progress (18)

## AP-019 · 2026-09-30 · after US-012

| | |
|---|---|
| Codex / requirements | v1.5 / v1.5 |
| Repository | `qa` (this snapshot's commit); `main` tagged `m0-done`, `m1-done`, `m1b-done` |
| Milestone | **M2 Console clan simulator: 3 of 7** |
| Next | S-US-013 Remember events and spread gossip |

### Story just finished: US-012 Let people choose what to do (utility AI)
- The clan lives by itself: every game hour each person scores every action (gather, hunt, sleep, warm by the fire, talk, rest, wander) from needs, traits, skills, age and time of day, and does the best one. In the evening they eat together from the shared store.
- `odysseus_headless --inspect Garu` shows why: "Garu (13, Brave Talkative): Hunger 85, ... -> Talk 30 (chosen), Rest 10, Wander 5 ...".
- Balance: a first version stockpiled 24,514 meals in 10 years; now people work only when really hungry or when the store is low, so the store swings between about 200 meals in summer and 400 before winter.
- All tuning numbers are in `assets/data/sim/actions.json`.
- CI on `qa`: green ([run 36652631895](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36652631895)).

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 5 / 5 | Done, `m1b-done` |
| M2 | Console clan simulator (KILL GATE 1) | 3 / 7 | In progress |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **17 / 49** | |

### Decisions and codex issues
- No new decisions. Awaiting your review (delegated to Dominus): D-02, D-16, D-17.
- No new codex issues.
