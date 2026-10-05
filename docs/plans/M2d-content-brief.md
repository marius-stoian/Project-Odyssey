# Build Brief: M2d Weapons, Nature, Effects and Weather

Written by Mraw (the Dominus council) for Anima, 2026-09-30. Phase 1 of the Amek workflow: **Anima turns this brief into Codex v1.8** (prompts for the stories below); Mraw then assembles them in order. Requirements v1.8 must add epic E13, milestone M2d and these stories first (the requirements document stays the source of truth for *what*).

## Goal
The seven sprite sheets the owner added after M2c become real game content: **weapons** the hero picks up and fights with, **plants** that stand in the world and can be chopped, **animals** placed like characters, **visual effects** on hits, plant actions and as placeable decoration, and **weather** that changes by itself. The Editor places all of it; Game mode plays it.

## How decisions are taken from now on
The owner takes **every design decision** personally, in interactive question rounds in chat (owner, 2026-09-30). Dominus no longer delegates design or scope questions: when one comes up during assembly, Mraw stops and asks in chat. Technical choices (how to cut sheets, code structure) stay with Dominus and are recorded in ADRs. The Charter's human-gates section must say this (Codex v1.8).

## Owner decisions (2026-09-30, six rounds in chat) = D-21
| Topic | Decision (owner's words in quotes where given) |
|---|---|
| Scope | **Full gameplay**, in a **new milestone M2d before M3**. |
| Weapon set | All 150 weapons are cut and catalogued; a **starter set is playable**: **one plain + one elemental weapon per class** (about 16), picked by Dominus from a numbered contact sheet the owner reviews and may swap. |
| Getting weapons | **Pickups + hotbar**: weapons are placed in levels with the Editor; walking over one adds it to a 9-slot hotbar; keys 1-9 select, Shift cycles. |
| Weapon behaviour | **By class**, each class its own attack: sword arc, axe heavy and slow, spear long thrust, bow/crossbow arrow, thrown (spinning), whip/flail reach, staff magic bolt, gun fast projectile. Damage, speed, range per weapon in JSON. |
| Elements | **Hit VFX + status effect**: fire burns over time, ice slows, lightning chains to one nearby enemy, poison damages over time, void/dark drains HP to the hero. |
| Plants | Trees and big plants **block walking** like rocks; flowers, grass, mushrooms are walk-over decoration; **all are interactable**: Interact shows **inspect text** (name + a flavour line); weapons **chop/destroy** them; destroying an **edible** plant **heals the hero a flat 10 HP** ("heal when destroying plants as current scope"). |
| Regrow | "**Regrow randomly in camera view after 15 seconds** for M2d scope": 15 s after a plant is destroyed, the same kind regrows at a random free spot inside the current camera view, with a tree-growth effect. |
| Animals | Placed like characters, **stand still with properties**. **Enemies = predators + boars**: wolf, fox, bear, cougar, lynx, leopard, jaguar, cheetah, lion, tiger, snow leopard, hyena, jackal, boar, wild pig, rhino, hippopotamus, bull, buffalo, water buffalo. All others are bystanders. |
| Fighting back | "**Hit back when attacked** as initial behaviour": when hit, an enemy (animals and goblins) strikes back once after a **0.5 s wind-up with a telegraph flash**, only if the hero is within its **reach (1.5 m)**; a hero out of reach (a bow shot) is safe. Damage per kind in JSON: goblins and small predators 5-10, big ones (bear, lion, tiger, rhino...) 15-20. |
| Hero HP | The hero gets **100 HP** (shown on screen). At 0: short fade, **respawn at the hero start with 100 HP**; enemies keep their HP. |
| Death | Characters and animals at 0 HP **die, nothing drops**: a death effect (smoke puff), removed until the level restarts. |
| VFX | Play on **combat hits and elements** (hit sparks, element effects, death smoke, projectile trails), on **plant actions** (leaf burst on chop, healing glow on eat, tree growth on regrow), and as **Editor-placeable looping effects** (fireflies, campfire, portal, magic circle...). |
| Weather | **Random weather** ("random weather in M2d scope"), visual only: a random weather every **60-120 s of play**, **3 s fade**; clear sky is picked about 1 in 3. Seeded, so tests are repeatable. |

## The new art (survey)
| File | Holds | Layout |
|---|---|---|
| `100-Icon Fantasy Weapon Sprite Sheet.png` (1254x1254) | 100 weapons: swords, axes/hammers/maces, spears/scythes, bows/crossbows, throwing stars/chakrams, whips/flails/chains, staves, pistols, rifles, heavy guns | 10x10 grid on dark cells, no labels |
| `Fantasy Sci-Fi Weapon Sprite Sheet.png` (1774x887) | 50 weapons, fantasy to sci-fi | 5 rows, **uneven cells**, dark |
| `Botanical Sprite Atlas_ 150 Nature Assets.png` (1536x1024) | 150 plants: grasses, flowers, crops, bushes, trees (normal, fantasy, glowing), mushrooms | rows, **name labels under each**, painted gradient background |
| `Pixel-Art Animal Sprite Sheet.png` (1536x1024) | 50 animals, side view | rows, **labels**, painted background |
| `100-Effect Pixel Art VFX Atlas.png` (1536x1024) | 100 effects x 4 frames (fire, earth, water, ice, wind, lightning, poison, holy, arcane, dark, blood, nature, space) | labelled cells, painted background |
| `Pixel Art Elemental VFX Grid.png` (1254x1254) | 100 single-frame elemental effects (sparks, projectiles, bursts, slashes, orbs, pillars) | 10x10 grid, dark cells |
| `Pixel Weather Sprite Atlas.png` (1254x1254) | 100 weather types x 4 frames (rain, snow, fog, storm, sand, leaves, ash, aurora...) | 10x10 labelled cells, dark |

Facts that shape the work:
- The grid sheets can be cut automatically. The labelled sheets need the **label strip excluded** and the **painted background removed** (gradients: flood fill with tolerance is not enough everywhere; a per-sheet background sample plus opaque-bounds trimming is needed).
- Some label numbers on the sheets are wrong or repeated (for example two "7." animals); the catalogs use **names**, not the printed numbers.
- Animals face one way (side view): the other side is mirrored, as for the hero's west view.
- Weather frames become **screen overlays** (scaled and tiled), not world sprites.

## Architecture rules that apply
- Rule 7: weapons, plants, animals, effects and weather are **data** (`assets/data/weapons.json`, `plants.json`, `characters.json` extended, `effects.json`, `weather.json`), validated with errors naming file and field.
- Rule 9: a generic **sprite-animation / effect player** (one-shot and looping) belongs in Luna Engine; which effect plays when is Game code.
- Rule 6: combat, HP, regrow timing and weather choice use **seeded PCG32 streams**; no wall-clock time. Weather is visual, but still seeded so screenshots in tests are repeatable.
- Rule 8: the level file gains pickups, plants and placed effects: **level format version 2**, reading version 1 files unchanged (US-122 rule: never lose a level).
- Rule 4: new intents: `Slot1..Slot9`, `Interact` (exists: reuse), `CycleWeapon` (Shift, already switching weapons).

## Stories (proposed IDs, epic E13 Content and Combat, milestone M2d)
| ID | Story | Size | Depends on | Acceptance in one line |
|---|---|---|---|---|
| US-130 | Content catalogs from the new sheets | L | none | `cuts.json` gains all seven sheets; atlases for weapons (32x32 icons), plants (by size: 32x32, 32x64, 64x96 trees), animals (scaled to fit 64x48, mirrored for the other side), effects (4-frame strips) and weather (4-frame overlays); JSON catalogs with name, class/kind, element, edible, blocking, enemy; numbered contact sheets in `docs/evidence/US-130/` for the owner |
| US-131 | Hero HP, fighting back and death | M | none | Hero 100 HP on screen; enemies strike back after a 0.5 s telegraphed wind-up if the hero is within 1.5 m; damage per kind from JSON; hero at 0 respawns at the start; enemies at 0 die (removed until restart) |
| US-132 | Effect player | M | US-130 | Luna plays one-shot and looping sprite animations in the world and on screen; hit sparks, death smoke and projectile trails in Game mode |
| US-133 | Weapon classes and the starter set | L | US-130, US-132 | 8 weapon classes with their own attack; the ~16 starter weapons playable with stats from JSON; the held weapon drawn in the hero's hand from its icon; owner reviews the starter contact sheet |
| US-134 | Pickups and the hotbar | M | US-133 | Editor places weapon pickups; walking over one adds it to a 9-slot hotbar on screen; 1-9 select, Shift cycles; level format v2 (v1 still loads) |
| US-135 | Elements | S | US-133 | Fire burns, ice slows, lightning chains, poison over time, void drains, each with its effect; numbers in JSON; tests per element |
| US-136 | Plants | L | US-130, US-132, US-134 | Plant palette in the Editor; big plants block walking; Interact shows inspect text; weapons chop/destroy with a leaf burst; edible ones heal 10 with a healing glow; regrow after 15 s at a random free spot in camera view with a growth effect |
| US-137 | Animals in the Editor | S | US-130, US-131 | All 50 animals in the character palette; predators and boars are enemies with strike-back numbers; others bystanders; properties panel as for characters |
| US-138 | Placed effects and random weather | M | US-132 | Editor places looping effects (saved in the level); random weather every 60-120 s with a 3 s fade, clear about 1 in 3, drawn as a screen overlay; seeded |
| X-M2d | M2d exit review | S | all above | Owner guide updated; evidence screenshots per story; qa into main, tag `m2d-done`; do not start M3 without the owner |

## Risks
- **Size**: M2d is bigger than M2c (9 stories, three L). Order the stories so each ends with something the owner can play.
- **Background removal on painted sheets** (plants, animals, effects) may leave halos; the fallback is a manual `rect` plus a hand-set background colour per cut in `cuts.json`.
- **Balance**: strike-back damage and weapon numbers are first guesses; all are JSON so the owner can tune them without code.
- **Licence**: D-05 still applies: generated art, placeholder quality; the owner checks the generator's licence before any public release.

## Next steps
1. Mraw updates **requirements v1.8** on Drive: epic E13, milestone M2d before M3, stories US-130..US-138, D-21 (all decisions above), the new decision rule (owner takes design decisions), resolution-log round 11; syncs to the repo.
2. **Anima writes Codex v1.8**: P-007 Adopt v1.8, the M2d section (design notes, K-M2d, S-US-130..S-US-138, X-M2d), the Charter's human-gates change, the D-21 row, execution order.
3. **Mraw assembles M2d** story by story, asking the owner in chat whenever a design question appears.
