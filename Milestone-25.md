# Project Odyssey: assembly progress (25)

## AP-026 · 2026-09-30 · US-110 Done: every death and feud has a reason

| | |
|---|---|
| Codex / requirements | **v1.6** / **v1.6** |
| Repository | `qa` (this snapshot's commit); `main` at `m1b-done` |
| Milestone | **M2b Story engine** (Kill Gate 1 retry): 1 of 6 stories |
| Next | **S-US-111** (quarrel, blame and revenge), then US-112..US-115, then the gate retry (you judge) |

### What happened
- K-M2b: the story design is written ([docs/plans/M2b-story-design.md](docs/plans/M2b-story-design.md)): an event log where every event has an id, people and causes; grudges; episodes derived from linked events; save version 3.
- US-110: the chronicle now says *why*. Example from the seed-7 run: "Winter, year 70: Awa the Second died of hunger in the hard winter, after Zuri the Second stole from the store." and "A feud broke out between Tok and Joro over stolen meat." `odysseus_headless --why <id>` lists the chain of earlier events.
- A failed harvest (a lean autumn, 20% of years) is now an event and the root of hard winters.
- Tests: 5 new cases plus a real save-version-2 upgrade; 0 warnings; all ctest green.

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 5 / 5 | Done, `m1b-done` |
| M2 | Console clan simulator (KILL GATE 1) | 7 / 7 | Built; gate failed, Pivot |
| M2b | Story engine (KILL GATE 1 retry) | 1 / 6 | In progress |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **22 / 55** | |

### Decisions and codex issues
- Decided by you: D-GATE-M2 (Pivot), D-18. Open: D-GATE-M2b (at the end of M2b).
- Please review when convenient (delegated to Dominus): D-02, D-16, D-17.
- No open codex issues.

