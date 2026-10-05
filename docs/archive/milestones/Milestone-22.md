# Project Odyssey: assembly progress (22)

## AP-023 · 2026-09-30 · after US-016

| | |
|---|---|
| Codex / requirements | v1.5 / v1.5 |
| Repository | `qa` (this snapshot's commit); `main` tagged `m0-done`, `m1-done`, `m1b-done` |
| Milestone | **M2 Console clan simulator: 7 of 7** |
| Next | X-M2 Exit review M2 = Kill Gate 1 |

### Story just finished: US-016 Save and load the simulation
- The world saves and loads exactly: after 50 years, save and load give the same world hash; `--save`, `--load` and 50 more years give the very world that 100 years in one go make (hash 10149270126131427195 both ways).
- Crash-safe (ADR-010): saves go to a `.tmp` file first and are renamed in one step; three backups are kept; a damaged save is skipped for the newest intact backup; with nothing usable you get a clear message, never a crash.
- Old saves (version 1, the needs-only world) are upgraded; a save from a newer version, or one that does not add up, is refused with a message saying why.
- CI on `qa`: green ([run 36656695591](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36656695591)).

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 5 / 5 | Done, `m1b-done` |
| M2 | Console clan simulator (KILL GATE 1) | 7 / 7 | In progress |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **21 / 49** | |

### Decisions and codex issues
- No new decisions. Awaiting your review (delegated to Dominus): D-02, D-16, D-17.
- Open for Anima: CI-006 (US-020's first-frame timing on GitHub's Debug runner).
