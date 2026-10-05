# Project Odyssey: assembly progress (27)

## AP-028 · 2026-09-30 · US-112 Done: care, sharing and adoption

| | |
|---|---|
| Codex / requirements | **v1.6** / **v1.6** |
| Repository | `qa` (this snapshot's commit); `main` at `m1b-done` |
| Milestone | **M2b Story engine** (Kill Gate 1 retry): 3 of 6 stories |
| Next | **S-US-113** (courtship, rivals, parting), then US-114..US-115, then the gate retry (you judge) |

### What happened
- US-111 is Done (CI on `qa` green, run 36678790336).
- US-112: people fall sick (more when hungry or cold) or get hurt hunting; the kind and the close nurse them and the patient remembers it for life ("Brak nursed Oren back to health."); in a famine the better fed feed the hungriest ("Iva shared food with hungry Kesh the Second."); orphans are taken in ("Ura, orphaned by the death of Ruk, was taken in by Arn."). Every line gives its reason.
- Balance on 100 seeds x 100 years: no crash or extinction (9 to 53 alive, median 38). The low end needs courtship (US-113) and a final tuning pass before the gate.
- Tests: 11 new cases; 0 warnings; ctest 20/20 Debug and Release.

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 5 / 5 | Done, `m1b-done` |
| M2 | Console clan simulator (KILL GATE 1) | 7 / 7 | Built; gate failed, Pivot |
| M2b | Story engine (KILL GATE 1 retry) | 3 / 6 | In progress |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **24 / 55** | |

### Decisions and codex issues
- Decided by you: D-GATE-M2 (Pivot), D-18. Open: D-GATE-M2b (at the end of M2b).
- Please review when convenient (delegated to Dominus): D-02, D-16, D-17.
- No open codex issues.

