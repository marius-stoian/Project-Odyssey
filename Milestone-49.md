# Project Odyssey: assembly progress (49)

## AP-050 · 2026-10-01 · US-140: arc ballistics

| | |
|---|---|
| Assembly plan / requirements | **v1.9** / **v1.9** |
| Repository | `qa` pushed; hosted CI green (run 36782177188) |
| Milestone | **M2d Content and combat**: 7 of 12 Done; US-140 Done |
| Next | **S-US-141**, then S-US-136 |

### What happened
- US-139 (mouse aiming) is Done: CI on its merge was green.
- US-140: bows, crossbows and thrown weapons now fly real arcs from Luna Physics. They leave the hand 1.3 m up, and the launch angle is solved so the shot lands where the mouse points (clamped to the weapon's range). A shot stops at the first enemy it passes through (feet to 1.5 m), at a rock if it is still low, or at the ground, where a miss sticks for 2 s. The sprite is lifted by its height with a shadow on the ground.
- Evidence: [docs/evidence/US-140/](docs/evidence/US-140/) (an arrow in flight, a hit, a shot stopped by a rock, a shot over it).

### For your review
- Technical choices recorded in docs/plans/US-140.md: rocks are 1.0 m tall to shots (so a shot from a 1.3 m hand can clear one; trees get their own heights with plants); water does not stop a shot; keys (Interact) fire a shallow chest-height arrow out to the weapon's range because there is no pointer.
- Staff bolts and bullets stay flat (US-141 tunes the staff).
- Your uncommitted `valley.json` edits were not touched.

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0-M2c | Tooling to Level editor | 34 / 34 | Done |
| M2d | Content and combat | 7 / 12 | US-140 Done, CI green |
| M3-M6 | | 0 / 28 | Waits for your word after M2d |
| | **MVP total** | **41 / 74** | |

### Decisions and assembly issues
- D-25 (owner). No assembly issues.

### Verification and remaining work
- Debug and Release: zero warnings, 25/25 checks passed in each.
- Remaining in M2d: S-US-141, then S-US-136, S-US-137, S-US-138, X-M2d.
