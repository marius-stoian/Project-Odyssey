# Project Odyssey: assembly progress (4)

## AP-005 · 2026-09-30 · after US-020

| | |
|---|---|
| Codex / requirements | v1.3 / v1.4 |
| Repository | `qa` @ `3f84f51`, CI green on GitHub (the cloud Windows machine opened the real window too); `main` @ `b98e293` tagged **`m0-done`** |
| Milestone | **M0 complete** (exit review met, tagged). **M1 Luna engine: 1 of 5** |
| Next | S-US-021 input intents |

### Story just finished: US-020 Open a window with a steady game loop
- **Luna's first real code**: Platform `System` and `Window` (SDL3, VSync, 480x270 virtual screen scaled by whole numbers); Engine `FixedStepClock` (20 ticks/s), `FrameStats`, `Game` interface and the `run()` loop.
- `odysseus.exe` now opens a **1280 x 720 window** titled "Project Odyssey 0.1.0" (a dusk-coloured screen for now).
- Evidence: 60-second run **60.0 FPS, 1200 ticks, first frame after 279 ms**; tick count identical at 30/60/144 Hz; clean close. [Plan](../../plans/stories-M1.md#us-020), [design](../../plans/M1-luna-design.md).

### Delegated decisions (Dominus) — please review
| ID | Decision |
|---|---|
| D-16 | Tiles are 32x32 px ([why](../../decision-requests/D-16.md)) |
| D-17 | 8-way movement, although US-024 says four directions, because you chose 8 facing directions (D-04) ([why](../../decision-requests/D-17.md)) |
| D-05 (M1 only) | Placeholder art is drawn by code until M3 decides real art |

### Progress: **6 / 44 stories** (M0 4/4, M1 1/5)
