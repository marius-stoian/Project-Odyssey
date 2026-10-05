# Project Odyssey: assembly progress (21)

## AP-022 · 2026-09-30 · after US-015

| | |
|---|---|
| Codex / requirements | v1.5 / v1.5 |
| Repository | `qa` (this snapshot's commit); `main` tagged `m0-done`, `m1-done`, `m1b-done` |
| Milestone | **M2 Console clan simulator: 6 of 7** |
| Next | S-US-016 Save and load the simulation |

### Story just finished: US-015 Soak-test the simulation from the command line
- `odysseus_headless --seed 7 --years 100` simulates a century in about half a second and reports: "Population: 47 alive (20 founders, 93 born, 66 died)", deaths by cause (old age 53, a mammoth's tusks 8, childbirth 5), average needs of the living, the store, couples, feuds, mammoths, and the tick time (0.08 microseconds per tick).
- The same 100 years give the same world hash in Debug and Release: 10149270126131427195.
- Wrong input (`--years -5`, `--years abc`, unknown options, missing values) prints the usage message and exits with code 2.
- Balance found by the soak: the mammoth herd now passes once a year (before: 214 mammoths and 29 hunters killed in a century).
- CI on `qa`: green ([run 36655954557](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36655954557)).

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 5 / 5 | Done, `m1b-done` |
| M2 | Console clan simulator (KILL GATE 1) | 6 / 7 | In progress |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **20 / 49** | |

### Decisions and codex issues
- No new decisions. Awaiting your review (delegated to Dominus): D-02, D-16, D-17.
- Open for Anima: CI-006 (US-020's first-frame timing on GitHub's Debug runner).
