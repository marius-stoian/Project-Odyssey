# Project Odyssey: assembly progress (51)

## AP-052 · 2026-10-01 · M2d finished: orientation fix, plants, animals, effects and weather

| | |
|---|---|
| Assembly plan / requirements | **v1.9** / **v1.9** |
| Repository | everything is merged into local `qa`, nothing pushed since `5125364` (owner: no tests or CI until M4) |
| Milestone | **M2d Content and combat**: 12 of 12 stories built; exit review written, merge to `main` waits for the M4 gate |
| Next | **K-M3** (Living clan on screen): needs your design answers first |

### What happened
- US-139 follow-up: the hero now always faces the mouse pointer (empty hands too) and no longer flickers from side to side when walking past it.
- US-136 plants: a Plant tool (153 plants), trees and bushes block walking and shots, the right mouse button (or Interact with empty hands) shows a plant's name and line, any weapon hit destroys a plant with leaves, edible ones heal 10 HP, and they regrow after 15 s inside the view.
- US-137 animals: the 50 animals are placeable (character palette, 6 pages); the 20 predators and boars fight, the rest are harmless.
- US-138: a Fx tool for 17 looping effects; random weather every 60-120 s with a 3 s fade, clear about one time in three (`--weather`, `--seed`).
- X-M2d: `docs/gates/M2d.md` lists each exit criterion and what is still owed.

### For your review
- **Small choices recorded as D-27, D-28, D-29** (you may override): big plants only block; Inspect on the right mouse button; animals are character kinds with facing picking east or west; the placed-effect and weather choices.
- **Toolbar names changed** to fit 480 pixels: **Weapon** is now **Arms**, **Grid** is now **#**; new buttons **Plant** and **Fx**.
- The animal sheet's animals face different ways, so you choose east or west per animal in the properties panel.
- The weather art is cut in 32 x 64 cells, so fog looks like a soft bank; better weather needs new art.
- Skipped because of "no testing until M4": every full test run after US-141, CI, and the merge of `qa` into `main` with the tag `m2d-done`. All are listed in `docs/gates/M2d.md`.

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0-M2c | Tooling to Level editor | 34 / 34 | Done |
| M2d | Content and combat | 12 / 12 | built, exit checks owed at M4 |
| M3-M6 | | 0 / 28 | M3 next |
| | **MVP total** | **46 / 74** | |

### Decisions and assembly issues
- D-25, D-26 (owner); D-27, D-28, D-29 (Dominus, small). Codex issue for Anima: CI is to run at milestone gates only (CI-007).
