# Project Odyssey: assembly progress (10)

## AP-011 · 2026-09-30 · session end: Luna Physics planned, M1b started

| | |
|---|---|
| Codex / requirements | **v1.5** / **v1.5** |
| Repository | `qa` (this snapshot's commit); `main` tagged `m0-done`, `m1-done` |
| Milestone | **M1b Luna Physics: kicked off (0 of 5 stories)**; M2 paused (US-010 Done, US-011 half-done on `story/US-011`) |
| Next | **S-US-025** Build deterministic 3D math (creates the `luna_physics` layer) |

### What happened since AP-010
1. **You added physics to the engine.** Requirements v1.5: PHY-01..PHY-06 (Luna Physics, hit detection, ballistics, rigid bodies, element physics and chemistry, later-Age physics), ARC-10 (a sixth layer, Physics, between Core and Engine/Simulation), ADR-017 (our own deterministic fixed-point 3D physics, SI units, 1 tile = 1 m). New milestone **M1b** right after M1 with stories US-025..US-029; the MVP grows to 49 stories, likely 72 weeks. The backlog workbook matches (checked by Excel).
2. **Anima issued Codex v1.4** (the M1b prompts, Charter rule 10) and **v1.5** (assembly instructions: Limit.md is the resume point, `tools/verify.ps1` is the standard verification, paused branches are continued, safe stops at usage limits, Dominus builds while Anima alone writes the Codex).
3. **K-M1b done**: the physics design is written ([docs/plans/M1b-physics-design.md](../../plans/M1b-physics-design.md)): 32.32 fixed-point numbers with our own 128-bit multiply and divide, Vec3 and quaternions, spheres/capsules/boxes with swept tests and a spatial grid, semi-implicit Euler at 20 Hz, textbook-checked ballistics and bounces, materials as data, 3D drawn top-down with shadows.

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 0 / 5 | Kicked off, design written |
| M2 | Console clan simulator (KILL GATE 1) | 1 / 7 | Paused until M1b is done |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **10 / 49** | |

### Awaiting your review (delegated to Dominus)
D-02 (side characters), D-16 (32x32 tiles), D-17 (8-way movement): see docs/decisions.md.
