# Project Odyssey: assembly progress (47)

## AP-048 · 2026-09-30 · US-135: elements

| | |
|---|---|
| Assembly plan / requirements | **v1.8** / **v1.8** |
| Repository | US-135 merged into `qa` and pushed; hosted CI to confirm |
| Milestone | **M2d Content and combat**: 5 of 9 Done; US-135 verified locally |
| Next | Confirm CI on qa, then **S-US-136** (plants) |

### What happened
- US-134 closed first: CI on the pushed head was green, so it is marked Done everywhere.
- US-135: elemental weapons now do something. Fire burns 2 HP per second for 3 s, poison 1 HP per second for 5 s, ice slows the enemy to half speed for 2 s (its strike-back warning lasts twice as long), lightning jumps once to the nearest other enemy within 3 m for half the damage, and void heals you 25% of the damage you deal. Each shows its effect on the target while it lasts. All numbers live in `assets/data/weapons.json` under `elements`.
- Evidence: [docs/evidence/US-135/](docs/evidence/US-135/), one screenshot per element from the real game.

### For your review
- **Decided by you (D-24):** slowed enemies strike back slower; a repeat hit refreshes the timer and never stacks; the Codex example numbers stand.
- Two small calls I made: void and the lightning jump always do at least 1 HP; the jump never jumps again.
- The old "US-133 Starters fight" test now allows fire and poison weapons to hurt after the hit.
- Your uncommitted `valley.json` edits were not touched.

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0-M2c | Tooling to Level editor | 34 / 34 | Done |
| M2d | Content and combat | 5 / 9 | US-135 awaiting CI |
| M3-M6 | | 0 / 28 | Waits for your word after M2d |
| | **MVP total** | **39 / 71** | |

### Decisions and assembly issues
- D-24 (owner). No assembly issues.

### Verification and remaining work
- Debug and Release: zero warnings, 25/25 checks passed in each, including simulation determinism.
- Remaining in M2d: S-US-136 Plants, S-US-137, S-US-138, X-M2d.
