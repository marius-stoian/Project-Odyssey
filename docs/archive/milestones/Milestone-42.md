# Project Odyssey: assembly progress (42)

## AP-043 · 2026-09-30 · US-130 Done: your new sheets are game content

| | |
|---|---|
| Codex / requirements | **v1.8** / **v1.8** |
| Repository | `qa` (CI green) |
| Milestone | **M2d Content and combat**: 1 of 9 stories |
| Next | **S-US-131** Hero HP, fighting back and death |

### What happened
- K-M2d: design in [docs/plans/M2d-content-design.md](../../plans/M2d-content-design.md).
- US-130: 653 items cut from your seven sheets (150 weapons, 153 plants, 50 animals, 200 effects, 100 weather) into a content atlas, with catalogs in `assets/data/`. Review them: [docs/evidence/US-130/](../../evidence/US-130) (a numbered picture and a name list per page).
- Your starter weapons, for review (swap any by setting `"starter"` in `assets/data/weapons.json`): iron sword, flame sword, steel battle axe, frost axe, iron spear, lightning spear, wooden longbow, venom recurve, throwing knives, void chakram, iron flail, fire whip, nature staff, void staff, flintlock pistol, venom pistol.
- Found on the sheets: the plant sheet has 153 plants (an extra "Lavendere", and an unlabelled red morel); one red mushroom is labelled "Blue". Catalog names follow the pictures.
- Which plants count as edible (fruit trees, crops, berries, brown and tall mushrooms, truffle, morel) and which block walking (trees, big bushes, bamboo, cactus) is set in `plants.json`; tell Dominus if you want it different.

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0-M2c | Tooling to Level editor | 34 / 34 | Done, `m2c-done` |
| M2d | Content and combat | 1 / 9 | In progress |
| M3-M6 | | 0 / 28 | Waits for your word after M2d |
| | **MVP total** | **35 / 71** | |

### Decisions and codex issues
- None new. No open codex issues.
