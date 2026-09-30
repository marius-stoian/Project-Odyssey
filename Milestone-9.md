# Project Odyssey: assembly progress (9)

## AP-010 · 2026-09-30 · after US-010

| | |
|---|---|
| Codex / requirements | v1.3 / v1.4 |
| Repository | `qa` @ `68b348f`; `main` tagged **`m1-done`** (Luna complete, CI green on main) |
| Milestone | **M2 Console clan simulator: 1 of 7** |
| Next | S-US-011 needs (Hunger, Energy, Warmth, Social) |

### Story just finished: US-010 Advance a seeded world clock
- The simulation now has its own clock: 20 ticks per game second, 2-minute days, 7-day seasons, 28-day years, daily weather, and speed control (pause, 1x, 2x, 4x; D-01).
- **Determinism is now tested on every build**: seed 42 always gives the same world hash after 10,000 ticks.
- Content is data: `assets/data/sim/calendar.json`, validated with errors that name the file and field.
- `odysseus_headless.exe --seed 42 --days 28` → "Spring, year 2, day 1".

### Delegated decision (Dominus) — please review
| ID | Decision |
|---|---|
| **D-02** | Side characters for Age 1: 4 needs, 6 traits (Brave, Timid, Kind, Greedy, Talkative, Diligent), one opinion per pair plus kinship, memories with feelings, gossip at half strength ([why](docs/decision-requests/D-02.md)) |

### Progress: **11 / 44 stories** (M0 4/4, M1 5/5, M2 1/7)
