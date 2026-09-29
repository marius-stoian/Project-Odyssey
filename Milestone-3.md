# Project Odyssey: assembly progress (3)

## AP-004 · 2026-09-30 · after US-004

| | |
|---|---|
| Snapshot ID | **AP-004** |
| Codex | v1.3 (autonomous assembly) |
| Source of truth | Project Odyssey.docx v1.4 |
| Repository state | `qa` @ `43da11e`, CI green ([run 36636578405](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36636578405), 4 min 50 s) |
| Current milestone | **M0 Tooling ready: 4 of 4 stories Done** |
| Next prompt | **X-M0** exit review (merge `qa` into `main`, tag `m0-done`), then K-M1: the Luna engine |

### Story just finished: US-004 Log what happens and stop on broken assumptions
- Every run of the game writes `%APPDATA%\Project Odyssey\Odysseus\logs\session-*.log` with UTC-timestamped lines; only the last 5 runs are kept.
- `ODYSSEUS_ASSERT(condition, message)` logs the file and line and stops the debugger on the assert line in Debug; it vanishes in Release.
- The per-user folder comes from SDL3 in Luna's Platform layer (first real Luna code); SDL3 3.4.16 is now a dependency, visible to Platform only.
- Evidence: 3 doctest cases / 24 assertions; 7 game runs leave 5 logs; the cdb debugger stops at `assert_probe.cpp @ 13`. [Plan](docs/plans/US-004.md), [evidence](docs/evidence/US-004/).

### Milestones

| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Exit review next |
| M1 | Luna engine: walking skeleton | 0 / 5 | Ready |
| M2 | Console clan simulator (KILL GATE 1) | 0 / 7 | Ready (D-02 delegated to Dominus) |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **4 / 44** | |

### Decisions and issues
- Delegated decisions this story: none. Technical choices recorded in the plan: SDL3 now (needed for the per-user folder), UTC log times, `assertions.h` name.
- Codex issues: none open.
