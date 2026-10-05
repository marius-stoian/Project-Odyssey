# Project Odyssey: assembly progress (116)

## AP-117 · 2026-10-05 · M8d Buildings (K-M8d, US-250, US-251, US-252, US-256)

| | |
|---|---|
| Assembly plan / requirements | **v2.12** (repository copy, CI-013) / **v2.10** (mirror) |
| Repository | `qa` |
| Milestone | M8d Buildings |

### State
- K-M8d answered by the owner (D-55); design in `docs/plans/M8d-buildings-design.md`.
- US-250, US-251, US-252, US-256 are written: building data, the store, the Build menu with ghost and blueprints, pieces and rooms, the Editor's Build tool and Prefab tab. Their new test cases ran alone and pass; the one full verify is X-M8d.
- Owner-only: GPU screenshots of the Build menu, a blueprint and a built hut.

### Decisions
- D-55 (owner, 2026-10-05): Build menu as a side list, start blueprints hut and windbreak, ghost until complete, valid placement anywhere; M8e answers (slow wear, rivals at war only, roof fade by default, fire spreads piece to piece).

### Next
- X-M8d: the full verify (Debug and Release), gate `docs/gates/M8d.md`, merge into main, tag m8d-done; then K-M8e and the M8e stories.
