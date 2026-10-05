# Project Odyssey: assembly progress (12)

## AP-013 · 2026-09-30 · after US-026

| | |
|---|---|
| Codex / requirements | v1.5 / v1.5 |
| Repository | `qa` (this snapshot's commit); `main` tagged `m0-done`, `m1-done` |
| Milestone | **M1b Luna Physics: 2 of 5** |
| Next | S-US-027 Fly projectiles with real ballistics |

### Story just finished: US-026 Detect hits between shapes
- Spheres, capsules and boxes: overlap tests report the contact point, the push-apart direction and the depth, for every pairing.
- **No tunnelling**: a spear tip moving 10 m per tick is swept along its whole path and hits a 0.2 m target at exactly 0.498 of the tick (0.0249 s), where checking only the tick positions would miss it.
- **Spatial grid** (2 m cells): with 1,000 bodies only 114 pairs need an exact test instead of 499,500, the result equals testing every pair, and a step takes 0.61 ms in Release (budget 2 ms).
- CI on `qa`: green ([run 36646262938](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36646262938)).

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 2 / 5 | In progress |
| M2 | Console clan simulator (KILL GATE 1) | 1 / 7 | Paused until M1b is done |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **12 / 49** | |

### Decisions and codex issues
- No new decisions. Awaiting your review (delegated to Dominus): D-02, D-16, D-17.
- No new codex issues.
