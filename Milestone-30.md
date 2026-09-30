# Project Odyssey: assembly progress (30)

## AP-031 · 2026-09-30 · US-115 Done: the clan's story in episodes

| | |
|---|---|
| Codex / requirements | **v1.6** / **v1.6** |
| Repository | `qa` (this snapshot's commit); `main` at `m1b-done` |
| Milestone | **M2b Story engine** (Kill Gate 1 retry): 6 of 6 stories built |
| Next | **X-M2b**: the exit review and the gate retry, which you judge alone |

### What happened
- US-115: `odysseus_headless --seed 7 --years 100 --story` prints at most 40 named episodes (beginning, turn, end, people), then the births, deaths, pairings and feuds with their reasons; `--chronicle` still prints every event. Details: `docs/plans/US-115.md`, `CHANGELOG.md`.
- Tests: 3 new cases and one command-line check; 0 warnings; ctest 21/21 Debug and Release.

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 5 / 5 | Done, `m1b-done` |
| M2 | Console clan simulator (KILL GATE 1) | 7 / 7 | Built; gate failed, Pivot |
| M2b | Story engine (KILL GATE 1 retry) | 6 / 6 | Built; gate waits for you |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **27 / 55** | |

### Decisions and codex issues
- Decided by you: D-GATE-M2 (Pivot), D-18. Open: D-GATE-M2b. No new delegated decisions; no codex issues.
