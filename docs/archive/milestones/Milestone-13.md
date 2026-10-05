# Project Odyssey: assembly progress (13)

## AP-014 · 2026-09-30 · after US-027

| | |
|---|---|
| Codex / requirements | v1.5 / v1.5 |
| Repository | `qa` (this snapshot's commit); `main` tagged `m0-done`, `m1-done` |
| Milestone | **M1b Luna Physics: 3 of 5** |
| Next | S-US-028 Push and bounce bodies |

### Story just finished: US-027 Fly projectiles with real ballistics
- Spears fly under gravity, quadratic air drag (F = 1/2 rho Cd A v^2, against the motion through the air) and wind, in 10 sub-steps per tick.
- **Textbook check**: 20 m/s at 45 degrees lands at 40.70 m; v^2/g = 40.77 m (0.17% off, 1% allowed).
- **Drag and wind**: a 1.5 kg spear falls 4.7 m short and a 5 m/s crosswind carries it 1.20 m downwind, both within 1% of an independent high-precision solution of the drag equation.
- **Aim solver**: the textbook launch-angle formula, then refined for drag; a straw target 25 m away is hit after 33 ticks. Too-far targets are reported out of reach.
- CI on `qa`: green ([run 36646913608](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36646913608)).

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 3 / 5 | In progress |
| M2 | Console clan simulator (KILL GATE 1) | 1 / 7 | Paused until M1b is done |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **13 / 49** | |

### Decisions and codex issues
- No new decisions. Awaiting your review (delegated to Dominus): D-02, D-16, D-17.
- No new codex issues.
