# Project Odyssey: assembly progress (48)

## AP-049 · 2026-10-01 · US-139: mouse aiming

| | |
|---|---|
| Assembly plan / requirements | **v1.9** / **v1.9** |
| Repository | `qa` pushed; hosted CI green (merge 00c0095) |
| Milestone | **M2d Content and combat**: 6 of 12 Done; US-139 Done |
| Next | Confirm CI on qa, then **S-US-140** (arc ballistics) |

### What happened
- Codex v1.9 adopted (P-008): your request for mouse aiming, ballistics and shootable ranged weapons became three stories (US-139, US-140, US-141) before plants; requirements v1.9 on Drive updated to match; D-25 records your answers.
- US-139: with a weapon in hand the hero now faces the mouse pointer, a dotted aim line and a crosshair follow it (red when out of the weapon's reach), and the left mouse button swings toward the pointer at the exact angle. The Interact key still attacks along the facing.
- Evidence: [docs/evidence/US-139/](docs/evidence/US-139/) (facing, aim line, a hit toward the pointer and a miss away from it).

### For your review
- **Decided by you (D-25):** free aim at the cursor, melee toward the cursor, arcs from Luna Physics landing at the cursor, bows/crossbows/thrown/staffs, no ammo, three stories before plants.
- Two small calls: only a held catalog weapon aims (the old demo sword and spear keep their own rules); holding the left button keeps attacking at the weapon's rate of fire.
- `Project Odyssey - MVP Backlog.xlsx` on Drive was not updated with the three new stories.
- Your uncommitted `valley.json` edits were not touched.

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0-M2c | Tooling to Level editor | 34 / 34 | Done |
| M2d | Content and combat | 6 / 12 | US-139 Done, CI green |
| M3-M6 | | 0 / 28 | Waits for your word after M2d |
| | **MVP total** | **40 / 74** | |

### Decisions and assembly issues
- D-25 (owner). No assembly issues.

### Verification and remaining work
- Debug and Release: zero warnings, 25/25 checks passed in each.
- Remaining in M2d: S-US-140, S-US-141, then S-US-136, S-US-137, S-US-138, X-M2d.
