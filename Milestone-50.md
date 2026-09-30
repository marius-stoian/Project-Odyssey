# Project Odyssey: assembly progress (50)

## AP-051 · 2026-10-01 · US-141: bows, crossbows, thrown weapons and staff bolts

| | |
|---|---|
| Assembly plan / requirements | **v1.9** / **v1.9** |
| Repository | US-141 merged into `qa` and pushed; hosted CI to confirm (also for US-140) |
| Milestone | **M2d Content and combat**: 8 of 12 Done if CI is green; US-141 verified locally |
| Next | **Stopped by the owner before S-US-136 (plants).** Confirm CI on qa first |

### What happened
- US-141: launch speeds for bows, thrown weapons, staffs and guns now live in `assets/data/weapons.json` under `classes`. Bows and crossbows and thrown weapons shoot the US-140 arcs; staffs fire flat bolts toward the mouse; elements apply on hit. There is no ammo, only each weapon's rate of fire.
- New level `assets/levels/range.json`: a shooting range with seven ranged weapons to pick up and four goblins, in the open and near rocks.
- Evidence: [docs/evidence/US-141/](docs/evidence/US-141/).

### For your review
- The hero's start on the range is right next to a rock on the path; it is cover, but move it in the editor if it annoys you.
- The owner's instruction of 2026-10-01: stop before US-136. M2d still has plants, animals, placed effects and weather (US-136..US-138) and the exit review.
- `Project Odyssey - MVP Backlog.xlsx` on Drive was not updated with US-139..US-141.
- Your uncommitted `valley.json` edits were not touched.

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0-M2c | Tooling to Level editor | 34 / 34 | Done |
| M2d | Content and combat | 8 / 12 | US-140 and US-141 awaiting CI |
| M3-M6 | | 0 / 28 | Waits for your word after M2d |
| | **MVP total** | **42 / 74** | |

### Decisions and assembly issues
- D-25 (owner). No assembly issues.

### Verification and remaining work
- Debug and Release: zero warnings, 25/25 checks passed in each.
- Remaining in M2d: S-US-136, S-US-137, S-US-138, X-M2d.
