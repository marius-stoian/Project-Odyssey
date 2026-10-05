# Story plans: M2d

Per-story plans for milestone M2d, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-130](#us-130)
- [US-131](#us-131)
- [US-132](#us-132)
- [US-133](#us-133)
- [US-134](#us-134)
- [US-135](#us-135)
- [US-136](#us-136)
- [US-137](#us-137)
- [US-138](#us-138)
- [US-139](#us-139)
- [US-140](#us-140)
- [US-141](#us-141)

---

<a id="us-130"></a>

## Plan US-130: Content catalogs from the new sheets

Codex v1.8, prompt S-US-130. Design: [M2d content design](M2d-content-design.md) section 1. Traces to D-05, D-21, ARC-08.

### What was built
| File | What |
|---|---|
| `src/luna/engine/image_ops.{h,cpp}` | `keyAlpha` (sheets already transparent), `keyBrightness` (light on dark: brighter = more opaque), `removeColour` (a background removed everywhere, holes too), `keepMainFigure` (the largest group centred in a focus rectangle and its loose parts; a neighbour reaching in is dropped) |
| `src/game/content_art.{h,cpp}` | the content atlas: pages of equal cells (icons 32x32, plants 32x32 / 32x64, trees 64x96, animals 64x48, effects 48x48, weather 32x64), `loadContentCuts`, `cutContent`, `saveContent`, `loadContent`, `numberedSheet` |
| `src/game/catalogs.{h,cpp}` | weapons, plants, animals, effects and weather catalogs, validated (file and field in every error; frames checked against the atlas) |
| `apps/atlas/main.cpp` | also cuts `assets/sprites/content-cuts.json`; `--content-preview FOLDER` writes a numbered sheet and a name list per page |
| `assets/sprites/content-cuts.json` | 653 cuts (1,191 frames) from the seven sheets |
| `assets/sprites/atlas/content-*.png`, `content.json` | the content atlas |
| `assets/data/weapons.json` (150), `plants.json` (153), `animals.json` (50), `effects.json` (200), `weather.json` (101) | the catalogs; first numbers, tunable by hand |
| `tools/art/` | `plan.ps1` (+ `Sheet.cs`, `load.ps1`) measured the sheets and wrote the cut list; `catalogs.ps1` wrote the first catalogs |

Technical choices (Dominus): the content atlas is a second index (`content.json`) beside the M2c `atlas.json`, so M2c code and tests are untouched; animals have their own `animals.json` (merged into the Editor's character palette in US-137).

### What the sheets turned out to be
- Plants, animals and the labelled effect atlas are **already transparent** (items alpha about 250, background near 0): the key is an alpha threshold; the labels are opaque dark boxes and are used to find each item (it stands above its label).
- The weapon grids have a plain dark background; the grid lines were found as thin light ridges. The weather and elemental grids are not evenly spaced; their lines were measured.
- The plant sheet has 152 labels for 153 plants: one red morel has no label, one red mushroom is labelled "Mushroom (Blue)", and an extra "Lavendere" (named golden lavender here). Names come from the catalog, not the printed numbers.

### Tests
| Scenario | Test |
|---|---|
| Cut | `US-130 Cut` (653 items in the atlas; catalogs 150/153/50/200/101 with every frame in the atlas; 16 starters: one plain and one elemental per class; 20 enemy animals; clear sky a third of the weather weight) |
| Review | `US-130 Review` (a numbered sheet and a name list per page in docs/evidence/US-130/, numbers matching the cut list) |
| Valid | `US-130 Valid` (a wrong class names `weapons.json` and `weapons[0].class`; a frame missing from the atlas names `weapons[0].frame`) |
| Keys | `US-130 Keys` (alpha, brightness, colour and main-figure keys on small pictures) |

### Manual checks (done 2026-09-30)
- Every numbered sheet in docs/evidence/US-130/ reviewed: weapons (150), animals (50), trees (56), tall plants (36), small plants (61) clean, no label text, no halo, no neighbour's parts (fixed on the way: outlines eaten by a flood fill, background left inside bows, label frame lines, a rhino's horn beside the hippo).
- Effects and weather: placeholder quality. Effects show their first frame on the sheet (the first frame of a burst is small by nature); weather frames are small (about 28x60 on the sheet) and will be tiled over the screen in US-138.
- The starter set for the owner's review: [docs/evidence/US-130/icons.md](../evidence/US-130/icons.md), items marked `starter` in weapons.json: iron sword, flame sword, steel battle axe, frost axe, iron spear, lightning spear, wooden longbow, venom recurve, throwing knives, void chakram, iron flail, fire whip, nature staff, void staff, flintlock pistol, venom pistol.

---

<a id="us-131"></a>

## Plan US-131: Hero HP, fighting back and death

Codex v1.8, prompt S-US-131. Design: [M2d content design](M2d-content-design.md) section 3. Traces to D-21.

### What was built
| File | What |
|---|---|
| `src/game/enemy.{h,cpp}` | a strike-back state machine (`Idle` -> `WindUp`, 10 ticks = 0.5 s): `provoke()` when hit, `update()` answers true on the tick the strike lands; hits during a wind-up do not restart it; the dead do not strike; `reachMetres` |
| `src/game/level.{h,cpp}` | character kinds may carry `reach` (metres, 0.5-10; 1.5 when not given) |
| `src/game/odyssey_game.{h,cpp}` | the hero's 100 HP; a hit enemy winds up and strikes with its `swordDamage` (the M2c "sword damage" property is its own strike) if the hero is within reach when the wind-up ends; at 0 HP the screen fades for 1 s, the hero ignores input, then starts at the hero start with 100 HP; enemies keep their HP; HUD: "HP n/100" top left (red at a quarter or less), a red "!" over an enemy winding up |
| `assets/data/characters.json` | strike numbers (D-21): goblin, skeleton, wolf 8; spider 6; slime, bat 5; plant monster, ghost 10; orc 15; troll 20 |

Enemies at 0 HP were already gone from play (not drawn, not hit) until the level restarts; the smoke puff comes with US-132.

### Tests
| Scenario | Test |
|---|---|
| Strike back | `US-131 Strike back` (a sword hit starts the wind-up; nothing for 9 ticks; on the 10th the hero loses the goblin's strike; the next swing, another strike) and `US-131 One strike per wind-up` |
| Out of reach | `US-131 Out of reach` (stepping away during the wind-up: no damage) |
| Death and respawn | `US-131 Death and respawn` (a 150-damage goblin fells the hero: no walking while fallen, back at the start with 100 HP after 20 ticks, the goblin keeps its HP; a defeated goblin never strikes) |

### Manual checks (done 2026-09-30)
- Scripted fight on demo.json: `docs/evidence/US-131/wind-up.png` (the red "!" over the goblin, 95/100, hero HP 100/100) and `struck.png` (hero HP 96/100 after the strike); log: "Goblin struck the hero for 4, HP 96 / 100".

---

<a id="us-132"></a>

## Plan US-132: Effect player

Codex v1.8, prompt S-US-132. Design: [M2d content design](M2d-content-design.md) section 2. Traces to D-21, ARC-09.

### What was built
| File | What |
|---|---|
| `src/luna/engine/renderer.{h,cpp}`, `src/luna/platform/window.{h,cpp}`, `src/luna/engine/ui.{h,cpp}` | `drawStyled`: a source stretched onto a destination, see-through by alpha, normal or additive blend; in the SDL window, the test recorder and the in-memory `ImageRenderer` |
| `src/luna/engine/effects.{h,cpp}` | `EffectPlayer` (game-agnostic): effects from texture frames, ticks per frame, once or looping, centred on a world or screen point, optional size; `start`, `stop`, `update`, `draw` |
| `src/game/odyssey_game.{h,cpp}` | loads the catalogs and the content atlas; `playEffect(name, x, y, size)` from effects.json; a hit spark on every sword hit, a smoke puff when an enemy falls, dust puffs every third tick behind a flying spear; effects cleared when play restarts |

### Tests
| Scenario | Test |
|---|---|
| One-shot | `US-132 One-shot` (frames 0,0,1,1,2,2 then gone) |
| Looping | `US-132 Looping` (still running after 100 ticks, right frame; stopped by handle) |
| Combat | `US-132 Combat effects` (a hit spark on the hit, gone when played; dust trails the spear), `US-132 Death smoke`, `US-132 Placement and style`, `US-132 Additive light` |

### Manual checks (done 2026-09-30)
- `docs/evidence/US-132/spark.png`: the spark on the goblin as the blade lands; `trail.png`: a spear thrown north with its dust trail.

---

<a id="us-133"></a>

## Plan US-133: Weapon classes and the starter set

Codex v1.8, prompt S-US-133. Design: [M2d content design](M2d-content-design.md) section 3. Traces to D-21.

### What was built
| File | What |
|---|---|
| `src/game/weapons.{h,cpp}` | `WeaponBehaviour` (virtual `melee`, `swing`, `launch`), `MeleeBehaviour` (sword, axe, spear, whip/flail arcs and reach) and `RangedBehaviour` (bow, thrown, staff, gun projectiles); damage, speed and range come from weapons.json |
| `src/game/odyssey_game.{h,cpp}` | the 16 starters cycled with Shift; attacks and projectiles go through the class behaviour; held icon drawn in the hero's hand, mirrored with the facing |
| `apps/atlas` (`--starters`) | writes the numbered starter contact sheet |
| `assets/data/weapons.json` | `starter` flags (one plain and one elemental per class) |

### Tests
| Scenario | Test |
|---|---|
| Classes | `US-133 Classes`, `US-133 Starters fight` |
| Starter set | `US-133 Starter set` (swap by editing `starter`, no code change) |
| In hand | `US-133 In hand` |
| Kept | US-029 sword and spear tests (`combat_test.cpp`) |

### Manual checks (done 2026-09-30)
- `docs/evidence/US-133/starters.png` and `starters.md`: numbered contact sheet of the 16 starters (owner reviews; swap by editing `starter` in weapons.json).
- `docs/evidence/US-133/staff.png`: a staff bolt against a goblin; every class's attack behaviour is covered by `Starters fight`.
- `tools/verify.ps1 -Story US-133`: zero warnings, ctest 25/25 in Debug and Release (`windows-debug.txt`, `windows-release.txt`).

### Known gap
Held icons are mirrored for west, not rotated: the renderer has no rotation yet.

---

<a id="us-134"></a>

## Plan US-134: Pickups and the hotbar

Assembly plan v1.8, prompt S-US-134. Design: [M2d content design](M2d-content-design.md). Traces to D-21, D-23 (owner answers of this story), ADR-010.

### What was built
| File | What |
|---|---|
| `src/game/level.{h,cpp}` | level format **version 2**: `pickups` (id, weapon, x, y); version 1 files load unchanged with none; unknown weapon names are refused naming `pickups[i].weapon`; `Definitions::weapons` (names from weapons.json plus the two demo weapons `Spear throw`, `Sword`); `resized` drops pickups outside |
| `src/game/pickups.{h,cpp}` | `WeaponArt` and `drawWeaponIcon` (icon, or a gold badge for the demo weapons), `kPickupReach` 16 px, `kPickupSize` 20 px |
| `src/game/editor*.{h,cpp}` | **Weapon** tool and palette (16 starters + 2 demo weapons); Select moves pickups; Delete removes; `PickupsCommand` for Undo/Redo; pickups drawn with a shadow and a gold frame when selected |
| `src/luna/platform`, `src/luna/engine/input.*`, `apps/odysseus/main.cpp` | keys 1-9, intents `Slot1..Slot9` (and `--hold Slot3` for scripts) |
| `src/game/odyssey_game.{h,cpp}` | 9-slot hotbar (`hotbar()`, `pickUp`, `selectSlot`, Shift cycles filled slots), pickups in the world, "Hotbar full", hotbar drawn at bottom centre; starters no longer cycle with Shift |
| `assets/levels/demo.json` | version 2, with the spear throw and sword as pickups by the hero start |
| `docs/guides/editor.md` | pickups and the hotbar explained |

### Tests
| Scenario | Test |
|---|---|
| Place | `US-134 Place` (place, save as version 2, reload, Undo/Redo, id never reused), `US-134 Move and delete`, `US-134 Level versions` (version 1 file loads, upgrades on save, bad weapon named) |
| Pick up | `US-134 Pick up` (first free slot, gone until restart), `US-134 Full hotbar` |
| Select | `US-134 Select` (1-9, empty hands, Shift cycles and wraps), `US-134 Hotbar drawn`; `input` test: number keys become Slot1-Slot9 |
| Kept | every earlier test; `US-133` tests now add weapons with `pickUp` instead of cycling |

### Manual checks (done 2026-09-30)
- `docs/evidence/US-134/hotbar.png`: three weapons (iron sword, frost axe, wooden longbow) in slots 1-3, slot 1 held and in the hero's hand, two more pickups on the ground.
- `docs/evidence/US-134/editor-weapons.png`: the Weapon tool with its palette and a just-placed iron spear.
- `tools/verify.ps1 -Story US-134`: see `windows-debug.txt` and `windows-release.txt`.

### Notes
- `valley.json` is version 1 with no pickups, so the hero starts empty-handed there until you place some with the Weapon tool.
- Restarting the level (F2, then F1) empties the hotbar and puts every pickup back.

### Resume review (2026-09-30)
- Restored the pickup spark chosen in D-23. The combat-effect test waits for pickup effects to finish before checking a separate hit spark.
- The editor end-to-end test now reads the next available ID from the demo and verifies the saved character identity, since pickups also use IDs.
- Reviewed both screenshots: three filled hotbar slots, nine numbered boxes, held-slot frame, world pickups and the editor weapon palette are visible.
- The held-icon test also waits for pickup sparks to finish, keeping its one-icon and mirrored-texture assertions intact.

Final local verification: zero warnings in both builds; Debug 25/25 (276.34 s), Release 25/25 (67.93 s). All acceptance scenarios pass; hosted CI green (run 36766225054).

Integration: merged into qa and pushed; hosted CI green.

---

<a id="us-135"></a>

## Plan US-135: Elements

Assembly plan v1.8, prompt S-US-135. Design: [M2d content design](M2d-content-design.md) section 3. Traces to D-21 and D-24 (owner answers of this story).

### What was built
| File | What |
|---|---|
| `assets/data/weapons.json` | new `elements` section: numbers and effect names for fire, ice, lightning, poison and void. Edit the JSON to tune; no code change |
| `src/game/catalogs.{h,cpp}` | `ElementDef` and `Catalogs::element()`; each number is range-checked and every effect name must exist in effects.json (`elements.fire.effect` is named in the error) |
| `src/game/status.{h,cpp}` | `StatusEffects`: burn and poison (HP per second, ticks left, the part of an HP not yet lost) and slow (factor, ticks left). `apply` restarts the timer, `tick` returns the whole HP lost |
| `src/game/enemy.{h,cpp}` | every enemy carries `StatusEffects`; a slowed enemy winds up at the slow factor (half speed: 20 ticks, not 10); `takeDamage(damage, flash)` so burning does not flash red every tick |
| `src/game/odyssey_game.{h,cpp}` | `strike(enemy, damage, weapon)` applies the weapon's element at the hit (`applyElement`); `tickStatus` runs each tick before enemies strike; status effects from effects.json restart each time the last one ends |

### The numbers (D-24, all in weapons.json)
| Element | Effect |
|---|---|
| fire | burns 2 HP per second for 3 s (6 HP) |
| poison | 1 HP per second for 5 s (5 HP) |
| ice | slows to 50% for 2 s; a slowed enemy strikes back after 1 s instead of 0.5 s |
| lightning | jumps once to the nearest other living enemy within 3 m for half the weapon's damage (at least 1); the jump does not jump again |
| void | 25% of the damage dealt heals the hero (at least 1 HP, never above 100) |

A new hit of the same element restarts the timer; it never stacks (D-24).

### Tests
| Scenario | Test |
|---|---|
| Numbers | `US-135 Numbers`, `US-135 Bad numbers` (a wrong effect name is refused, naming the field) |
| Over time | `US-135 Status effects` (burn 6 HP in 3 s, poison 5 HP in 5 s, re-apply restarts), `US-135 Fire`, `US-135 Fire shows`, `US-135 Poison` |
| Ice and void | `US-135 Ice` (the strike comes after 20 ticks), `US-135 Void` (hero heals 1 HP after being struck) |
| Lightning | `US-135 Lightning` (jumps to the goblin 2 m away for half the damage; the goblin 10 m away is untouched; with nobody near nothing jumps) |

### Manual checks (real game window, Debug)
Levels in `docs/evidence/US-135/levels/`; run with `odysseus.exe --level <file> --hold Interact:0.3:0.35 --screenshot out.bmp`. Each screenshot was looked at.
- `fire.png`: flame sword; the goblin is burning (flame glow), HP 94 after a hit and one tick of burning.
- `poison.png`: venom sword; green drip on the goblin.
- `ice.png`: frost sword; frost shards on the goblin and the hero's red "!" warning while it winds up slowly.
- `lightning.png`: lightning spear; a bolt between two goblins 0.2 s after the hit, the second goblin at 97/100 (half of 6).
- `void.png`: void katana, second swing; green healing glow on the hero, HP 97/100 after being struck for 4.

### Verification
`tools/verify.ps1 -Story US-135`: see `windows-debug.txt` and `windows-release.txt`.

Final local verification: zero warnings in both builds; Debug 25/25, Release 25/25. All acceptance scenarios pass. The older test "US-133 Starters fight" now lets fire and poison weapons hurt after the hit. Integration: merged into qa and pushed; hosted CI green.

---

<a id="us-136"></a>

## Plan US-136: Plants

Assembly plan v1.9, prompt S-US-136. Design: [M2d content design](M2d-content-design.md) section 5. Traces to D-21 and D-27 (technical and small design choices Dominus took under the owner's full authority of 2026-10-01).

### What was built
| File | What |
|---|---|
| `src/game/level.{h,cpp}` | level format version 2 gains `plants` (id, kind, x, y); `PlacedPlant`; `Definitions::plants` and `hasPlant`; an unknown kind or shared id is refused naming `plants[i].kind` / `plants[i].id`; `resized` drops plants outside; version 1 files still load |
| `src/luna/engine/tile_map.{h,cpp}` | obstacle layer: `setObstacle(x, y, heightMetres)` and `obstacleHeight`; a cell with an obstacle is solid for walking and flat shots (Luna stays game-agnostic: it only knows "something stands here, this tall") |
| `src/game/plants.{h,cpp}` | `PlantArt` (pictures from the content atlas pages `plants-small`, `plants-tall`, `trees`), `drawPlant`, `drawPlantIcon`, `plantExtent`, `plantObstacleHeight` (bushes and tall plants 1.5 m, trees 3 m), `WorldPlant`, constants (reach 1.5 m, text 3 s, regrow 15 s, heal 10) |
| `src/game/odyssey_game.{h,cpp}` | level plants in the play state; big ones set the obstacle on their foot cell; `destroyPlant` (leaf burst, edible: +10 HP with a healing glow, obstacle cleared, 300-tick timer); `tickPlants` regrows the same plant at a random free cell inside the camera view from the seeded PCG32 stream `plants`, with the growth effect; `inspectNearestPlant`; plants drawn behind or in front of the hero by their feet; melee swings also cut plants in their arc; flat shots and arcs hit big plants |
| `src/luna/engine/input.{h,cpp}`, `apps/odysseus/main.cpp` | new intent `Inspect` on the right mouse button (and `--hold Inspect`) |
| `src/game/editor.{h,cpp}`, `src/game/editor_history.{h,cpp}` | **Plant** tool and palette (153 plants by picture, 36 per page, arrows turn pages); place (one plant per cell, feet at the middle of the cell's bottom edge), select, drag (keeps to the grid), Delete, Undo/Redo (`PlantsCommand`); plants drawn in the Editor; toolbar label **Weapon** became **Arms** and gaps narrowed so all 13 buttons fit in 480 pixels |
| `docs/guides/editor.md` | plants explained |

### Rules
- Only **big** plants (`blocks: true` in plants.json: bushes, tall plants, trees) block walking and shots. Shots and arrows fly through small plants, but any melee swing cuts the nearest one (sword, spear) or all (axe, whip) in its arc.
- A flat bolt that stops against a big plant's cell cuts the plant; an arc that passes through a big plant's body (0.6 m wide, up to 1.5 m) cuts it.
- Regrow: a random cell of the camera view, at most 64 tries (walkable, no plant growing there, no character there, at least 40 px from the hero); if none, it tries again a second later. The stream is seeded the same at every start (`Pcg32(1, 5)`), so a replay grows the same plants back in the same places. Play never changes the level file.
- Inspect (D-27): Interact with **empty hands** or the **right mouse button** (any time) looks at the nearest plant within 1.5 m. With a weapon in hand Interact attacks as before, so keyboard play can chop and look.

### Tests
| Scenario | Test |
|---|---|
| Place and block | `US-136 Level format` (version 2 round trip, old levels, unknown kind, shared id), `US-136 Editor` (place, one per cell, save, undo/redo, palette pages, select/move/delete), `US-136 Place and block` (a tree stops the hero, a flower does not, plants are drawn) |
| Inspect, chop and eat | `US-136 Inspect` (name and text for 3 s with empty hands, the Inspect intent with a weapon, nothing far away), `US-136 Chop` (a sword swing with leaves, a destroyed tree no longer blocks, an arrow cuts a tree), `US-136 Eat` (a goblin hurts the hero to 80, the edible plant heals to 90; at full health nothing to heal) |
| Regrow | `US-136 Regrow` (15 s later the same kind grows back inside the view, free and away from the hero, with the growth effect; same spot every run; a regrown tree blocks its new cell) |

### Manual checks (real game window, Debug)
Level `docs/evidence/US-136/levels/garden.json` (trees, bushes, flowers and edible plants around the hero).
- `garden.png`: the planted level, plants behind and in front of the hero by their feet.
- `inspect.png`: the name and its line in a gold-framed box above the carrot top.
- `chop.png`: a sword swing east; the plant is gone and the leaves fly.
- `regrow.png`: 15 s later the carrot top has grown back at another tile inside the view, with the growth effect.
- `editor-plants.png`: the Plant tool and its first palette page (1/5).

### Verification
`tools/verify.ps1 -Story US-136`: see `windows-debug.txt` and `windows-release.txt`.

Checks: Debug and Release builds with zero warnings (tools/verify.ps1 was stopped during the test run by the owner's instruction "Proceed without testing until reaching M4"); the 7 US-136 cases, all 77 game tests and all 17 engine tests had passed in Debug before the final docs. The full verification and CI are owed at the M4 gate.

---

<a id="us-137"></a>

## Plan US-137: Animals in the Editor

Assembly plan v1.9, prompt S-US-137. Design: [M2d content design](M2d-content-design.md). Traces to D-21.

### What was built
| File | What |
|---|---|
| `src/game/level.{h,cpp}` | the 50 animals of animals.json become character kinds after the twelve of characters.json (`CharacterKindDef::animal`; name, HP and strike damage from the file; a name shared with a character is refused, naming animals.json) |
| `src/game/animals.{h,cpp}` | `AnimalArt` (content page `animals` and its mirrored copy), `drawAnimal` (feet at the middle of the bottom edge; west side uses the mirrored page; a hit brightens it), `drawAnimalIcon` |
| `src/game/odyssey_game.{h,cpp}` | enemy animals are `Enemy` objects like goblins (`animal`, `kindName`): hit, flash, strike back after the 0.5 s wind-up if the hero is within reach, die at 0 HP; bystanders are drawn but never targeted, so weapons, arrows and bolts pass them |
| `src/game/editor.{h,cpp}` | the character palette is paged, 12 kinds per page with arrows (page 1 is the twelve characters where they were, pages 2-6 the animals, scaled, with names and (enemy) or (harmless) in the hint); placed animals are drawn, picked and moved like characters; the properties panel is unchanged (name, HP, facing, strike damage in the Sword field) |
| `docs/guides/editor.md` | which animals are enemies |

### Rules
- Enemies (20): grey wolf, fox, bear, boar, wild pig, cougar, lynx, leopard, jaguar, cheetah, lion, tiger, snow leopard, hyena, jackal, rhino, hippopotamus, buffalo, bull, water buffalo. All others are harmless: they cannot be hit and take no damage.
- An animal's picture is one side view; its **facing** picks the side: W, NW and SW show the mirrored picture, the other five the picture as cut. The sheet's animals do not all face the same way, so the owner sets the facing in the properties panel until the art is normalised.

### Tests
| Scenario | Test |
|---|---|
| Place | `US-137 The fifty animals are character kinds` (50 kinds, 20 enemies, HP and strike damage from the file), `US-137 Place` (all 50 placed in the Editor with name and HP, saved, read back, standing in the game as 20 enemies and 30 bystanders, palette pages) |
| Enemies | `US-137 Enemies` (a grey wolf is hit, strikes back after half a second, dies at 0 HP, is drawn) |
| Bystanders | `US-137 Bystanders` (a deer, a cow and a rabbit next to the hero are unharmed by swings over all of them, and are drawn) |

### Manual checks (real game window, Debug)
Level `docs/evidence/US-137/levels/animals.json`.
- `animals.png`: wolf, bear, boar and tiger with health bars; deer, cow, rabbit and elephant without (bystanders).
- `editor-animals.png`: the character palette, page 3 of 6, animals by picture.

### Verification
Builds only: Debug and Release compiled earlier in this session with zero warnings; the US-137 tests were written with the code and not run (owner's instruction of 2026-10-01 "Proceed without testing until reaching M4"). Full verification and CI are owed at the M4 gate.

---

<a id="us-138"></a>

## Plan US-138: Placed effects and random weather

Assembly plan v1.9, prompt S-US-138. Design: [M2d content design](M2d-content-design.md) section 6. Traces to D-21, ADR-011.

### What was built
| File | What |
|---|---|
| `src/game/level.{h,cpp}` | level format version 2 gains `effects` (id, name, x, y); `PlacedEffect`; `Definitions::loopingEffects` (the 17 looping effects of effects.json) and `hasLoopingEffect`; an unknown or one-shot effect and a shared id are refused naming `effects[i].name` / `.id`; `resized` drops effects outside; older levels load |
| `src/game/editor.{h,cpp}`, `src/game/editor_history.{h,cpp}`, `src/game/effect_art.h` | **Fx** tool and palette (17 looping effects by first picture); place at the click, select, drag, Delete, Undo/Redo (`EffectsCommand`); placed effects shown by their first picture; toolbar button **Grid** is now **#** so 14 buttons fit in 480 pixels |
| `src/game/weather.{h,cpp}` | `WeatherCycle`: a seeded PCG32 stream (`weather`) picks the next weather by the weights of weather.json every 1200-2400 ticks (60-120 s); the game starts clear; the new weather fades in over 60 ticks (3 s) while the old fades out; `seedFromText` (FNV-1a of the level name, the default seed); `force` |
| `src/game/odyssey_game.{h,cpp}` | placed effects start as looping Luna effects when the level starts (`startPlacedEffects`); weather is drawn above the world and below the interface (`drawWeather`): rain, snow and sparks tiled over the screen and added to the picture, fog and clouds stretched and laid on top, each layer with its own alpha for the cross-fade; the Editor draws no weather |
| `apps/odysseus/main.cpp` | `--seed <n>` fixes the weather sequence; `--weather <name>` starts under a named weather (for screenshots) |

### Rules
- Clear sky has weight 50 of 150 in weather.json, so it comes about one time in three. Weather is only for the eyes: nothing in play depends on it.
- The default seed is the hash of the level's name, so a level plays under the same weathers every time; restarting the level restarts the cycle.
- Technical note: the weather sheet is cut in cells of 32 x 64 (not the 160 x 90 the design notes hoped for), so light weathers are tiled and fog and clouds stretched; fog shows as a soft bank with clear edges. A better look needs new art.

### Tests
| Scenario | Test |
|---|---|
| Placed effects | `US-138 Placed effects` (version 2 round trip, unknown and one-shot effects refused, a campfire loops 400 ticks after a reload, the Editor places, moves, deletes, undoes and saves them) |
| Weather | `US-138 Weather cycle` (starts clear; changes every 60-120 s; fades in over 3 s; clear about a third of 3000 changes), `US-138 Weather in the game` (two games with one seed have the same weather; it is drawn over the world) |
| Repeatable | `US-138 Weather cycle`: the same seed gives the same 30 weathers in the same order, another seed another order; the default seed is a hash of the level name |

### Manual checks (real game window, Debug)
Level `docs/evidence/US-138/levels/ambient.json`.
- `placed-effects.png`: a campfire, a portal, a magic circle and fireflies looping in the world.
- `weather-steady-rain.png`, `weather-light-snow.png`, `weather-dense-fog.png`: three weathers (started with `--weather`).
- `editor-effects.png`: the Fx tool and its palette.

### Verification
Builds only: Debug and Release compiled with zero warnings; the US-138 tests were written with the code and not run (owner's instruction of 2026-10-01 "Proceed without testing until reaching M4"). Full verification and CI are owed at the M4 gate.

---

<a id="us-139"></a>

## Plan US-139: Mouse aiming

Assembly plan v1.9, prompt S-US-139. Brief: [M2d aiming brief](M2d-aiming-brief.md). Traces to D-25 (owner, 2026-10-01).

### What was built
| File | What |
|---|---|
| `src/luna/engine/input.{h,cpp}` | new intent `Attack`: the left mouse button (or a scripted one); Game code still reads intents only (Charter rule 4) |
| `apps/odysseus/main.cpp` | `--aim x:y:from:to` (same as `--point`) and `--hold Attack:from:to` for scripted play |
| `src/game/weapons.{h,cpp}` | `swingToward` and `launchToward` take a unit direction; the old `swing` and `launch` (a facing) forward to them; `facingToward(dx, dy)` gives the nearest of the 8 facings |
| `src/game/hero.h` | `Hero::face(Facing)` turns the hero without walking |
| `src/game/odyssey_game.{h,cpp}` | `updateAim` (pointer plus camera gives the world point; direction from the hero's feet; the hero faces it while a catalog weapon is held and the pointer is over the picture); Attack goes toward the pointer, Interact along the facing; `drawAim` draws a dotted aim line (up to the weapon's range) and a crosshair, red when the pointer is beyond range |

### Rules
- Only a held catalog weapon aims; the demo sword slash and spear throw keep their own facing rules.
- Attack repeats while held (limited by the weapon's cooldown); Interact is one attack per press, along the facing, as before.
- With the pointer off the picture, nothing aims and the hero keeps the facing from walking.

### Tests
| Scenario | Test |
|---|---|
| Face the cursor | `US-139 Facing from a direction` (8 directions and the 22.5-degree edges), `US-139 Face the cursor` (facing, aim vector, line and crosshair drawn, gone off the picture) |
| Swing toward the cursor | `US-139 Swing toward the cursor` (north-east hits, south-west misses, exact angle rather than nearest facing) |
| Keys still work | `US-139 Keys still work`, `US-139 Interact goes along the facing even with the pointer elsewhere`; `luna_tests` "the left button is the Attack intent"; every earlier test |

### Manual checks (real game window, Debug)
Level `docs/evidence/US-139/levels/aim.json` (iron sword at the start, a goblin north-east).
- `face-east.png`, `face-northwest.png`: the hero faces the pointer; the dotted aim line and crosshair follow it (red: beyond the sword's reach).
- `swing-northeast.png`: Attack with the pointer north-east: the goblin takes 5 damage (95/100).
- `swing-southwest.png`: the same swing with the pointer south-west misses (log: "swung toward 133 degrees, facing SouthWest: 0 hit").

### Verification
`tools/verify.ps1 -Story US-139`: see `windows-debug.txt` and `windows-release.txt`.

Final local verification: zero warnings in both builds; Debug 25/25, Release 25/25. All acceptance scenarios pass. Integration: merged into qa and pushed; hosted CI green.

### Follow-up (2026-10-01, owner request): the hero always faces the pointer, and no more flicker
Owner: "fix the hero character orientation so it is oriented by mouse pointer, and fix random sprite switches from side to side when continuously walking sideways" (D-26).

**What was wrong**
1. Only a held catalog weapon turned the hero to the pointer; with empty hands or the demo weapons he faced the way he walked.
2. When he did follow the pointer, the facing was recomputed from scratch every tick: walking first set the facing to the walking direction and the pointer then overwrote it. With the pointer close to him or near the edge between two facings (22.5 degrees), tiny changes (the camera lags, he walks past the pointer) flipped the sprite from side to side.

**What changed** (`OdysseyGame::updateAim`, `facingToward` with hysteresis in `weapons.cpp`)
- While the pointer is over the picture the hero faces it, whatever he holds and however he walks (walking backwards is possible). Off the picture he faces the way he walks, as before. Only a held catalog weapon still aims its attacks and shows the aim line.
- The facing is measured from his chest (20 px above the feet), so a pointer level with the sprite means "sideways".
- Dead zone: a pointer within 16 px of his chest does not turn him.
- Hysteresis: he keeps his current facing until the pointer is 10 degrees past the edge of its 45-degree sector. The facing before the tick's walking is what is compared, so walking cannot fight the pointer.

**Tests:** `US-139 The hero always faces the pointer` (empty hands, walking east, pointer west: he faces west; pointer off the picture: east), `US-139 No flicker walking past the pointer` (both directions, empty and armed, 9 pointer offsets x 3 heights, 60 ticks each: at most 2 facing changes; it fails 32 times without the dead zone and hysteresis), `US-139 Facing with hysteresis`. Screenshot `docs/evidence/US-139/walk-east-facing-pointer.png` (level `levels/orient.json`): walking east with the pointer to the west.

---

<a id="us-140"></a>

## Plan US-140: Arc ballistics for shots

Assembly plan v1.9, prompt S-US-140. Brief: [M2d aiming brief](M2d-aiming-brief.md). Traces to D-25 (owner, 2026-10-01), ARC-10, ADR-017.

### What was built
| File | What |
|---|---|
| `src/game/arc_shots.{h,cpp}` | `launchArcShot` (the launch angle is solved with Luna Physics `aimLaunchAngle` so the shot lands at the pointer, with distance clamped to the weapon's range and to what its launch speed can reach) and `stepArcShots` (one physics tick per shot with `flyTick`: the first of rocks, the ground or an enemy stops it; a shot that came down sticks for 40 ticks, then is gone; one that leaves the map or flies 10 s is lost) |
| `src/game/odyssey_game.{h,cpp}` | bow and thrown classes arc (staff bolts and bullets stay flat); `attackWith` takes the distance to the pointer; events strike the enemy (damage, element, provoke as before) or start a dust puff; `drawHeld` draws the sprite lifted by its height with a shadow on the ground; log lines say how each shot ended |
| `tests/game/arc_test.cpp`, `tests/game/weapons_test.cpp` | new tests; the earlier "US-133 Starters fight" aims thrown weapons with the pointer, since they now land where aimed |

### Rules (D-25 and technical choices)
- Launch from the hand: 1.3 m up, 0.3 m ahead of the feet. The low arc is always used (Luna Physics' `launchAngleWithoutDrag`). Speeds: bows 16 m/s, thrown 10 m/s. No air drag, no wind.
- Mouse aim lands on the ground at the pointer. A key aim (Interact) has no pointer: it is a shallow arrow aimed at chest height (1 m) out to the weapon's range.
- An enemy is hit inside a box 0.6 m wide from its feet to 1.5 m up. Only rocks stop a shot (1.0 m tall: a low arc stops at them, a higher one clears them); water and other walking-only tiles do not.
- Technical note: rocks are 1.0 m here (1.5 m for the spear demo's boulders) so an arrow leaving a 1.3 m hand can clear one; taller things (trees) get their own heights with the plants (US-136).

### Tests
| Scenario | Test |
|---|---|
| Lands at the cursor | `US-140 Lands at the cursor`: bows at 3, 6 and 9 m and a thrown weapon at an angle all land within half a metre of the aim point; two runs land on the exact same point (fixed-point) |
| Hits in its path | `US-140 Hits in its path`: a chest-height shot hits the enemy; a dead enemy or one off to the side is missed; a rock blocks a shot that is still low (aimed just behind it), a shot aimed 9 m out clears it; water does not stop a shot; `US-140 In the game`: a goblin 4 m south takes the bow's damage and the shot is spent |
| Range and misses | `US-140 Range and misses`: a pointer 30 m away lands at the bow's 10 m; a miss sticks in the ground for 40 ticks and is removed |

### Manual checks (real game window, Debug)
Levels in `docs/evidence/US-140/levels/` (`arc.json`: a goblin 4 m south; `arc-rock.json`: a rock 1 m ahead of the hero).
- `arc-in-flight.png`: an arrow in the air, lifted above its path, with the aim line.
- `arc-hits-goblin.png`: the arrow hit the goblin (95/100), a spark on it.
- `arc-blocked-by-rock.png`: aimed just behind the rock, the arrow stops at the rock (dust).
- `arc-clears-rock.png`: aimed 7 m out, the arrow passes over the rock and comes down at (41.9, 32.8) m.

### Verification
`tools/verify.ps1 -Story US-140`: see `windows-debug.txt` and `windows-release.txt`.

Final local verification: zero warnings in both builds; Debug 25/25, Release 25/25. All acceptance scenarios pass. Integration: merged into qa and pushed; hosted CI green.

---

<a id="us-141"></a>

## Plan US-141: Bows, crossbows, thrown weapons and staff bolts

Assembly plan v1.9, prompt S-US-141. Brief: [M2d aiming brief](M2d-aiming-brief.md). Traces to D-25 (owner, 2026-10-01).

### What was built
| File | What |
|---|---|
| `assets/data/weapons.json` | new `classes` section: launch speed in m/s for bow (16, also crossbows), thrown (10), staff (12), gun (24); edit the JSON to tune, no code change |
| `src/game/catalogs.{h,cpp}` | `ClassDef`, `Catalogs::weaponClass()`; `classes.<name>.launchSpeed` is range-checked and a missing class is named in the error |
| `src/game/arc_shots.{h,cpp}` | `launchArcShot` takes the launch speed (no hard-coded speeds left) |
| `src/game/odyssey_game.{h,cpp}` | bows, crossbows and thrown weapons launch the US-140 arcs at the speed from the file; staff bolts fly flat toward the pointer at the staff's speed from the file; `cameraView()` for tests |
| `assets/levels/range.json` | the shooting range: 24 x 24 tiles, a row of pickups at the start (longbow, crossbow, venom recurve, throwing knives, void chakram, nature staff, void staff), four goblins (two in the open, one behind a rock, one by another), rocks as cover |
| `tests/game/ranged_test.cpp` | see below |

### What each class does
| Class | Path | Speed | Range (weapons.json) |
|---|---|---|---|
| bow, crossbow | arc | 16 m/s | 10 m |
| thrown | arc, heavier and slower | 10 m/s | 7 m |
| staff | flat bolt, aimed at the pointer | 12 m/s | per weapon |
| gun | flat, unchanged, not part of this story | 24 m/s | per weapon |

Damage and rate of fire come from each weapon; elements apply on hit as for melee (poison, slow, chain, drain). No ammo, only the rate-of-fire cooldown (D-25).

### Tests
| Scenario | Test |
|---|---|
| Each ranged class shoots | `US-141 Each ranged class shoots` (longbow, crossbow, throwing knives: physics arcs; nature staff: a flat projectile; the goblin loses the weapon's damage), `US-141 Every ranged starter is shootable from the hotbar` (all 6 ranged starters), `US-141 Speeds come from weapons.json` (changing the number changes what loads; a missing class is named) |
| Elements on shots | `US-141 Elements on shots` (poison arrow poisons, frost arrow slows, a staff's lightning chains to a second goblin) |
| Shooting range | `US-141 Shooting range` (walking along the pickups fills the hotbar with 7 weapons; at least 3 goblins; a longbow arrow hurts the goblin in the open) |

### Manual checks (real game window, Debug)
Level `assets/levels/range.json` (copy in `docs/evidence/US-141/levels/`); the hero walks east along the pickups, chooses a slot, aims with the pointer and fires (`--hold`, `--aim`, `--hold Attack`).
- `longbow.png`, `crossbow.png`: the arrow in the air with its shadow and the dotted aim line toward the goblin.
- `throwing-knives.png`: the thrown knife has hit (HP 96/100 after 4 damage).
- `nature-staff.png`: the flat bolt has hit; the goblin flashes red with its strike-back warning (94/100 after 6).

### Verification
`tools/verify.ps1 -Story US-141`: see `windows-debug.txt` and `windows-release.txt`.

Final local verification: zero warnings in both builds; Debug 25/25, Release 25/25. All acceptance scenarios pass. Integration: merged into qa and pushed; hosted CI green.
