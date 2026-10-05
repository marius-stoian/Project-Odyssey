# Build Brief: M2d Aiming and ballistics (addition to M2d)

Written by Mraw (the Dominus council) for Anima, 2026-10-01. Phase 1 of the Amek workflow: **Anima turns this brief into Codex v1.9** (three new story prompts inside M2d); Mraw then assembles them in order. Requirements v1.9 adds stories US-139..US-141 to epic E13 (the requirements document stays the source of truth for *what*).

## Goal
After US-135 (elements) the owner asked, before plants: "I want to be able to aim the weapons with the mouse and we need ballistics and multiple ranged weapons so that I can shoot enemies." Today ranged weapons fly flat in the hero's 8 facing directions, and the only arc is the old spear-throw demo. Now the hero aims at the mouse pointer, projectiles fly real arcs from Luna Physics, and bows, crossbows, thrown weapons and staffs are shootable from the hotbar.

## Owner decisions (2026-10-01, two rounds in chat) = D-25
| Topic | Decision |
|---|---|
| Aiming | **Free aim at the cursor**: the weapon points from the hero to the mouse pointer at any angle; left click attacks toward it; the hero faces that way (8 directions); a small aim line or crosshair shows where the attack goes. |
| Melee | **Swing toward the cursor**: the melee arc is centred on the cursor direction (exact angle), not the walking direction. The Interact key keeps working as before. |
| Ballistics | **Full arcs using Luna Physics**: arrows, thrown weapons and bolts that arc have height, gravity, launch angle and speed, using the existing fixed-point ballistics and aim solver from the spear throw. |
| Landing and hits | A shot **lands at the cursor** (the launch angle is solved for it; the distance is clamped to the weapon's range). It **hits anything in its path**: an enemy is hit when the shot passes through its body height (about 0 to 1.5 m); arcs that pass too high fly over; misses stick in the ground and vanish. **Rocks, trees and other solids block low shots; high arcs clear them.** |
| Ranged set | **Bows and crossbows** (arrow arcs, long range), **thrown weapons** (spear, chakram, axes: heavy arcs, short range, high damage), **staffs** (magic bolts: fast and **flat**, with their element effects). **Guns stay out of this scope.** |
| Limits | **No ammo, only cooldown**: each weapon keeps its rate of fire from weapons.json; ammo is a later story. |
| Order | **Three stories before plants, inside M2d.** US-136 Plants continues after them. |

## Stories
| ID | Title | Size | Needs | Outcome |
|---|---|---|---|---|
| US-139 | Mouse aiming | M | US-133, US-134 | The hero faces the pointer; left click attacks toward it; melee arcs centre on the cursor direction; an aim line and crosshair; Interact still works; scripted `--aim x:y:from:to` for tests |
| US-140 | Arc ballistics for shots | L | US-139 | Shots fly as Luna Physics arcs (height, gravity, angle solved to land at the cursor, clamped to range); hit tests along the path at body height; solids block low shots; a shadow on the ground shows where it is; misses stick in the ground |
| US-141 | Bows, crossbows, thrown weapons and staff bolts | M | US-140 | The ranged starters (bow, crossbow, thrown, staff) use the arcs (staff bolts flat and fast); speeds, ranges and damage from weapons.json; elements apply on hit; a shooting-range level (straw targets and goblins behind rocks) in the demo evidence; the spear-throw demo still works |

## Risks
- **Two aim systems**: the M1b spear-throw demo has its own aim solver path. Keep it working (its tests stay) and reuse its physics functions rather than duplicating them.
- **Top-down height**: height is drawn by lifting the sprite and drawing a ground shadow; hit tests use height, not the drawn position. Keep the conversion in one place.
- **Determinism**: arcs use Luna Physics fixed-point, so a replay with the same inputs flies the same shots (Charter rules 6 and 10).

## Next steps
1. Mraw updates **requirements v1.9**: stories US-139..US-141 in E13, D-25.
2. **Anima writes Codex v1.9**: P-008 Adopt v1.9, the three story prompts between S-US-135 and S-US-136, the execution order, the amendment log.
