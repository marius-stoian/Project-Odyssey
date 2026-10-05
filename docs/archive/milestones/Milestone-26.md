# Project Odyssey: assembly progress (26)

## AP-027 · 2026-09-30 · US-111 Done: quarrels, blame and revenge

| | |
|---|---|
| Codex / requirements | **v1.6** / **v1.6** |
| Repository | `qa` (this snapshot's commit); `main` at `m1b-done` |
| Milestone | **M2b Story engine** (Kill Gate 1 retry): 2 of 6 stories |
| Next | **S-US-112** (sharing, nursing, sickness), then US-113..US-115, then the gate retry (you judge) |

### What happened
- US-110 is Done (CI on `qa` green, run 36677668628).
- US-111: people quarrel when short-tempered and mutually unfriendly, grieving kin blame the thief they know of or the killer, and a feud that keeps worsening ends in an attack: a fight (someone hurt, sometimes killed) or the clan drives the aggressor out. Example (seed 7): "Lin attacked Tok over stolen meat: Lin lost the fight and was killed." then "Ilka blamed Tok for Lin's death, because Tok struck the blow." and years later "The clan drove Tok out for attacking Joro after the fight."
- Wounds are a first health system: the hurt rest, mend in 5 to 12 days, and can die. US-112 adds sickness and nursing on top.
- Balance checked on 30 seeds x 100 years: no crash, no extinction (17 to 50 alive), 1 to 14 exiles per century.
- Tests: 8 new cases; 0 warnings; ctest 20/20 Debug and Release.

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 5 / 5 | Done, `m1b-done` |
| M2 | Console clan simulator (KILL GATE 1) | 7 / 7 | Built; gate failed, Pivot |
| M2b | Story engine (KILL GATE 1 retry) | 2 / 6 | In progress |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **23 / 55** | |

### Decisions and codex issues
- Decided by you: D-GATE-M2 (Pivot), D-18. Open: D-GATE-M2b (at the end of M2b).
- Please review when convenient (delegated to Dominus): D-02, D-16, D-17.
- No open codex issues.

