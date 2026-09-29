# Project Odyssey: assembly progress (11)

## AP-012 · 2026-09-30 · after US-025

| | |
|---|---|
| Codex / requirements | v1.5 / v1.5 |
| Repository | `qa` (this snapshot's commit); `main` tagged `m0-done`, `m1-done` |
| Milestone | **M1b Luna Physics: 1 of 5** |
| Next | S-US-026 Detect hits between shapes |

### Story just finished: US-025 Build deterministic 3D math
- **Luna has its sixth layer, Physics** (`src/luna/physics/`). It uses Core only; Engine, Simulation and Game may use it. Eleven compiler probes prove the new rules, and four more rule-breaking files are rejected by the include check.
- **`Fixed` numbers** (32.32): whole-number arithmetic only, with our own 128-bit multiply and long division, so every PC computes the same bits. `sqrt`, `sin`, `cos`; `Vec3`; quaternion rotations.
- Proof: exact values checked bit for bit (and 100,000 random cases against the processor's own 128-bit instructions); four quarter turns return to the start; **one million mixed operations give the same hash, 17224312723153614174, in Debug, Release and on GitHub's CI machine**.
- An automated review keeps `float` and `double` out of Luna Physics (Charter rule 10).
- CI on `qa`: green ([run 36645422757](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36645422757)).

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 1 / 5 | In progress |
| M2 | Console clan simulator (KILL GATE 1) | 1 / 7 | Paused until M1b is done |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **11 / 49** | |

### Decisions and codex issues
- No new decisions. Awaiting your review (delegated to Dominus): D-02, D-16, D-17.
- No new codex issues.
