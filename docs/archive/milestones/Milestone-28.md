# Project Odyssey: assembly progress (28)

## AP-029 · 2026-09-30 · US-113 Done: courtship, rivals and parting

| | |
|---|---|
| Codex / requirements | **v1.6** / **v1.6** |
| Repository | `qa` (this snapshot's commit); `main` at `m1b-done` |
| Milestone | **M2b Story engine** (Kill Gate 1 retry): 4 of 6 stories |
| Next | **S-US-114** (teaching and hunting parties), then US-115 (episodes), then the gate retry (you judge) |

### What happened
- US-113: pairing became a courtship (suitors, gifts and time together, both must agree), turned-down suitors, jealous rivals who may quarrel, and partings with a reason. The Pairing, Rejection, Jealousy and Parting events link to their causes. Details: `docs/plans/stories-M2b.md#us-113`, `CHANGELOG.md`.
- Also merged to `qa` at your request: the hero sword (Shift), a standing enemy with HP and a red hit flash (branch `chore/sword-enemy-demo`). Bow stays the default weapon so the accepted spear tests keep passing.
- Balance (your new limit: 10 seeds x 100 years): 27 to 45 alive. Seed 7: 109 courtships begun, 46 pairings, 4 turned down, 6 jealousies, 0 partings. Partings are rare; the final tuning pass before the gate may raise them.
- Tests: 9 new cases; 0 warnings; ctest 20/20 Debug and Release.

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 5 / 5 | Done, `m1b-done` |
| M2 | Console clan simulator (KILL GATE 1) | 7 / 7 | Built; gate failed, Pivot |
| M2b | Story engine (KILL GATE 1 retry) | 4 / 6 | In progress |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **25 / 55** | |

### Decisions and codex issues
- Decided by you: D-GATE-M2 (Pivot), D-18. Open: D-GATE-M2b (at the end of M2b). No new delegated decisions; no codex issues.
