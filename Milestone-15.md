# Project Odyssey: assembly progress (15)

## AP-016 · 2026-09-30 · after US-029

| | |
|---|---|
| Codex / requirements | v1.5 / v1.5 |
| Repository | `qa` (this snapshot's commit); `main` tagged `m0-done`, `m1-done` |
| Milestone | **M1b Luna Physics: 5 of 5**, exit review next |
| Next | X-M1b Exit review M1b |

### Story just finished: US-029 Throw a spear in the demo
- **Play it**: run `odysseus.exe`, step left to face west, press E (or Space, or the gamepad's South button). The spear flies in an arc, its shadow glides along the path, and it sticks in the straw target 8 tiles away. Face north and throw: the spear hits the boulder in front of the second target instead.
- **Materials are data** (`assets/data/materials.json`): a flint-tipped spear deals 69.3 damage, a sharpened wooden one 12.0, exactly as the formula (kinetic energy x hardness / 10 x sharpness) predicts. A broken value is rejected with a message naming the file and field.
- Rock tiles are now 1.5 m boulders in 3D; the Engine draws height by lifting sprites up the screen and keeps shadows on the ground; the camera frames the hero and target during a throw.
- Screenshot: [the spear in flight](docs/evidence/US-029/spear-in-flight.png).
- CI on `qa`: green ([run 36649148955](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36649148955)).

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 5 / 5 | Exit review next |
| M2 | Console clan simulator (KILL GATE 1) | 1 / 7 | Resumes after X-M1b |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **15 / 49** | |

### Decisions and codex issues
- No new decisions. Awaiting your review (delegated to Dominus): D-02, D-16, D-17.
- No new codex issues.
