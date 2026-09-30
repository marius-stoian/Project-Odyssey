# Project Odyssey: assembly progress (14)

## AP-015 · 2026-09-30 · after US-028

| | |
|---|---|
| Codex / requirements | v1.5 / v1.5 |
| Repository | `qa` (this snapshot's commit); `main` tagged `m0-done`, `m1-done` |
| Milestone | **M1b Luna Physics: 4 of 5** |
| Next | S-US-029 Throw a spear in the demo |

### Story just finished: US-028 Push and bounce bodies
- Bodies have mass: a 140 N s shove gives a 70 kg person exactly 2 m/s; a steady 140 N push for one second does the same.
- **Bounces**: a ball with restitution 0.5 reaches 0.25, 0.0625, 0.0156, 0.0039 m (a quarter each time, e^2) and then lies still and "sleeps" (no more computing until something pushes it).
- **Friction**: a crate sliding at 3 m/s on grass stops after 1.22597 m; the textbook v^2/(2 mu g) says 1.22597 m.
- `RigidBody` is a class with invariants (positive mass, sleeping bodies never move), a C++ lesson in the learning journal.
- CI on `qa`: green ([run 36647621150](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36647621150)).

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 4 / 5 | In progress |
| M2 | Console clan simulator (KILL GATE 1) | 1 / 7 | Paused until M1b is done |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **14 / 49** | |

### Decisions and codex issues
- No new decisions. Awaiting your review (delegated to Dominus): D-02, D-16, D-17.
- No new codex issues.
