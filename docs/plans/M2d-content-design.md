# M2d design: content and combat

Architect's design for K-M2d (Codex v1.8), written before US-130. It follows the M2d design notes in the Codex and the owner's decisions D-21 and D-22. Everything here is a **technical** choice; any design question found while building goes to the owner in chat (D-22).

## 1. Art pipeline v2 (US-130)

### Atlas pages
Today an `Atlas` has two fixed pictures (characters 32x48, tiles 32x32). M2d needs more cell sizes, so the atlas becomes a set of **pages**, each a picture of equal cells:

| Page | Cell | Holds | Fit |
|---|---|---|---|
| `characters` | 32x48 | as today | feet bottom centre |
| `tiles` | 32x32 | as today | fill |
| `icons` | 32x32 | 150 weapons | centred |
| `plants-small` | 32x32 | grass, flowers, crops, mushrooms | bottom centre |
| `plants-tall` | 32x64 | bushes, reeds, sunflower, corn | bottom centre |
| `trees` | 64x96 | trees | bottom centre |
| `animals` | 64x48 | 50 animals (and mirrored copies) | bottom centre |
| `effects` | 48x48 | 100 animated x 4 frames, 100 elemental | centred |
| `weather` | 160x90 | 100 weather x 4 frames | fill |

As built (US-130): the M2d pages live in a second index, `content.json` with `content-<page>.png`, beside the M2c `atlas.json` (characters and tiles untouched); `ContentAtlas` answers page and rectangle by frame name. The cut list is `assets/sprites/content-cuts.json`. Animals have their own `animals.json` until US-137 merges them into the character palette.

### Cut list additions
- `kind` gains `icon`, `plant-small`, `plant-tall`, `tree`, `animal`, `effect`, `weather` (the page it goes to).
- Per-cut `background` (`[r, g, b]`) and `tolerance`, overriding the sheet sample.
- `frames` (a count, default 1) splits the rectangle into N equal columns: effect and weather cells with 4 frames side by side.
- A `grid` entry generates many cuts: sheet, origin, cell size, gap, columns, rows, an inner rectangle inside each cell (to skip titles and frame numbers), kind, and the list of names in reading order. The two 10x10 grid sheets (weapons, elemental effects) and the weather grid use it.
- The uneven sheets (sci-fi weapons, plants, animals, the labelled effect atlas) get plain rectangles, found with `odysseus_atlas --find` (blob search) and checked on the contact sheets.

### Backgrounds
The M2c flood fill compares every pixel with one background colour, which fails on painted gradients. M2d adds a **smooth flood fill**: start at the rectangle's border pixels close to the sample, and spread to a neighbour when it is close *to the neighbour it came from* (local step under the tolerance) and not far from the sample (a looser global limit). Gradients go; outlines stop the fill because the step into a dark outline is large. Then `opaqueBounds` trims, and `fitInto` scales with the box filter.

Effects and weather are light on dark: for them the cutter keeps the pixels and sets **alpha from brightness** over the local background (a luminance key), so glows fade out softly instead of leaving hard dark edges.

### Catalogs (assets/data)
- `weapons.json`: `name, frame, class (sword|axe|spear|bow|thrown|whip|staff|gun), element (none|fire|ice|lightning|poison|void), era (fantasy|future), starter, damage, speed, range`. Numbers come from the class defaults until US-133 tunes the starters.
- `plants.json`: `name, frame, size (small|tall|tree), blocks, edible, inspect`.
- `characters.json`: the 50 animals added with `frames`, `directions: 2` (side view east and its mirror west, a new value), `hp`, `enemy`, `strikeDamage`, `reach`. The M2c monster "wolf" keeps its name; the animal is "grey wolf".
- `effects.json`: `name, frames (base name), count, ticksPerFrame, loop, anchor (centre|feet)`.
- `weather.json`: `name, frames, count, ticksPerFrame, weight`; the entry "clear" has no frames and a third of the weight.

Validation follows `level.cpp`: every problem is a `DataError` naming the file and the field; unknown frames are errors.

## 2. Luna additions (US-132, US-138)
The `Renderer` today draws a texture region at a point. M2d adds, in Luna Engine and the SDL renderer (and `RecordingRenderer` for tests):
- `drawScaled(texture, source, destination)` for weather overlays;
- per-draw **alpha** (0-255) for fades, and a **blend mode** (normal or additive) for glows;
- `EffectPlayer`: a list of running effects (frames, ticks per frame, loop, world or screen position, depth); `start`, `stop`, `update` per tick, `draw` with the camera. It knows frames only as rectangles on a texture: nothing about Odysseus.

## 3. Combat (US-131, US-133, US-135)
- `Health` (current, max) on the hero and on every placed character.
- Enemy strike-back is a state machine per enemy: `Idle -> WindUp (10 ticks = 0.5 s, the telegraph flash) -> Strike (one tick) -> Idle`. A hit while idle starts the wind-up; hits during the wind-up do not restart it. At the strike, the hero takes `strikeDamage` if the distance feet to feet is at most `reach`.
- Hero at 0 HP: 20 ticks of fade, then respawn at the hero start with full HP; the level otherwise continues. Enemies at 0 HP: removed from the play copy (the level file is unchanged), with the death effect.
- Weapons: an abstract `WeaponClass` with `attack(context)` returning hits or a projectile; one class per kind. Melee classes test an arc or a line in front of the hero (range in metres, angle per class); projectile classes spawn a `Projectile` (speed, range, sprite, trail effect) moved per tick with the existing tile collision. The M1b spear throw becomes the `thrown`/`spear` path and keeps its physics.
- Elements: `StatusEffects` on each character (burn and poison: damage per second and time left; slow: factor and time left). Chain and drain are applied at the hit. All numbers per element are in `weapons.json` under `elements`.
- Randomness: none needed for combat in M2d.

## 4. Pickups and hotbar (US-134)
- Level format **version 2**: optional arrays `pickups` (weapon, x, y), `plants` (id, kind, x, y), `effects` (id, name, x, y). Reading version 1 gives empty arrays; saving always writes version 2. Ids are shared with characters (`nextId`).
- `Hotbar`: 9 slots of weapon names and the selected slot. Pickups are collected when the hero's feet are within 0.5 m.
- Intents `Slot1..Slot9` from keys 1-9 in Platform events and the Engine input map.
- Editor: a palette tab switch (Tiles, Characters, Weapons, Plants, Effects) instead of more side panels, so the 480x270 screen stays readable.

## 5. Plants (US-136)
- Big plants (`blocks: true`) mark their foot cell solid in the play copy's collision, like a rock; destroyed plants clear it.
- Interact: the nearest plant within 1.5 m shows its name and inspect text for 3 s (a UI label above it).
- Any weapon hit on a plant destroys it: leaf burst; if edible, the hero heals 10 (capped at max) with a healing glow.
- Regrow: a timer of 300 ticks (15 s); then a PCG32 stream (`plants`) picks cells inside the camera view until it finds a free one (walkable, no plant, not under the hero, not a character's cell; at most 64 tries, else retry next second).

## 6. Weather (US-138)
- A PCG32 stream (`weather`) picks the next weather by weight and the next change after 1200-2400 ticks (60-120 s). A change cross-fades the old and the new overlay over 60 ticks (3 s) with alpha.
- Overlays are drawn scaled to the virtual screen, additive for light weathers (rain, snow, sparkles) and normal alpha for fog and clouds (a flag in weather.json), above the world and below the UI.
- Seed: `--seed` or the level's name hash when none is given, so tests are repeatable.

## 7. Tests
Each story adds "US-13x ..." doctest cases headless. The end-to-end scripts reuse `demo.json` (D-20), with new fixtures where a level needs pickups, plants or animals (`assets/levels/combat-demo.json`, a copy that tests never write).

## 8. Order and risks
US-130 comes first (all later stories need the catalogs); US-131 in parallel is possible, but stories go one at a time. The riskiest step is background removal on the painted sheets: the contact sheets are the check, and a cut may always fall back to a hand-set background and tolerance.
