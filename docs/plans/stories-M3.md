# Story plans: M3

Per-story plans for milestone M3, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-030](#us-030)
- [US-031](#us-031)
- [US-032](#us-032)

---

<a id="us-030"></a>

## Plan US-030: Compose characters from layers

Assembly plan v1.9, prompt S-US-030 (M3). Decision D-30 (Dominus, delegated): programmer art layers, drawn by code (D-05: own art, placeholder quality).

### What was built
| File | What |
|---|---|
| `src/luna/engine/sprite_layers.{h,cpp}` | Luna (game-agnostic): `recoloured(layer, swaps)` swaps exact colours and keeps alpha; `composed(layers)` stacks same-size layers, see-through pixels show what is below |
| `src/game/clan_art.{h,cpp}` | the layers as code-drawn sheets in the 4 x 8 grid of 32 x 48 cells of the character sheet (walking frames x 8 facings): a body, 3 hair styles (short, long, top knot), 3 outfits (tunic, fur wrap, robe) and a spear, all in marker colours; palettes: 4 skin tones, 6 hair colours, 6 outfit colours; `composeLook` recolours and stacks; `lookOf(person)` gives each person a fixed look from the id, sex and age |

### Tests (`tests/game/clan_test.cpp`)
| Scenario | Test |
|---|---|
| Compose | `US-030 Compose` (every layer has the same size; in all 32 cells the composed look draws more than the body alone) |
| Swap | `US-030 Swap` (two looks that differ only in the outfit differ only where an outfit layer draws) |
| Palette | `US-030 Palette` (one hair picture, three palettes: the same shape, three distinct colours), `US-030 Looks of people` |

### Manual checks
The layered people are visible in `docs/evidence/US-032/camp-start.png` (each person a different skin, hair, outfit and spear).

### Verification
Builds only: Debug and Release compiled with zero warnings; the tests were written with the code and not run (owner, 2026-10-01: no testing until M4). Full verification and CI are owed at the M4 gate.

---

<a id="us-031"></a>

## Plan US-031: Read people's state at a glance

Assembly plan v1.9, prompt S-US-031 (M3). Decision D-30 (Dominus, delegated).

### What was built (`src/game/clan_view.{h,cpp}`, `src/game/odyssey_game.cpp`)
- `emoteOf(person)`: the most urgent need wins: cold (Warmth 20 or less), hungry (Hunger 20 or less), unwell, tired (Energy 15 or less), lonely (Social 15 or less).
- On screen: a small bubble over the head (a snowflake for cold, food for hunger, a red + when unwell, z when tired, ? when lonely) and a **shiver** (the figure shakes a pixel every other tick) when cold.
- Hover a person: a gold-framed panel with their name, sex and age, what they are doing now, the four needs as exact numbers and how they feel.

### Tests
| Scenario | Test |
|---|---|
| Emote | `US-031 Emotes` (each need at its threshold gives its emote, in priority order; the dead show none) |
| Details on demand | `US-031 Details on demand` (the pointer over a person adds the panel to what is drawn) |

### Manual checks
`docs/evidence/US-031/camp-emotes.png` (run with `--clan-speed 40`).

### Verification
Builds only: Debug and Release compiled with zero warnings; the tests were written with the code and not run (owner, 2026-10-01: no testing until M4). Full verification and CI are owed at the M4 gate.

---

<a id="us-032"></a>

## Plan US-032: See the simulated clan on screen

Assembly plan v1.9, prompt S-US-032 (M3). Decisions D-30 (Dominus, delegated).

### What was built
| File | What |
|---|---|
| `src/game/clan_view.{h,cpp}` | `ClanView`: the simulation has no places, only what people do each hour, so the view gives each action a place around the camp (fire, gathering ground west, hunting ground east, food store, talking place, beds in a ring) and each person a spot of their own there (golden angle), and walks them there at 60 px/s on the 20 ticks a second (`previous` and current positions, so drawing is interpolated); the nearest free ground is used, a wedged person steps to the goal after 3 s; the camp is where the first flame of the level burns, else the hero start |
| `src/game/odyssey_game.{h,cpp}` | a level marked `"clan": true` (or `--clan`) runs the M2 `sim::World` inside the game, one simulation tick per game tick (`--clan-speed n` fast forwards for demos); figures drawn behind or in front of the hero by their feet; a line with season, day, hour, temperature, population and food; `--clan` and `--clan-speed` flags |
| `src/game/level.{h,cpp}` | `Level::clan` (written only when true) |
| `assets/levels/camp.json` | the camp: a fire in a ring of stones, paths west-east and north, trees, a pond, and the clan |

### Tests
| Scenario | Test |
|---|---|
| Mirror | `US-032 Mirror and smooth` (20 people, 20 figures), `US-032 The clan in the game` (camp.json runs the clan: 20 figures, one simulation tick per game tick, 20 more draws than without the clan) |
| Smooth | `US-032 Mirror and smooth` (two game days: nobody moves faster than walking pace, and the picture at alpha 0.5 is between the two ticks) |
| Separation | the US-003 architecture checks: `sim` holds no drawing code; the view lives in `src/game` and only reads the world |

### Manual checks
`docs/evidence/US-032/camp-start.png` (people at the fire at night) and `camp-later.png`.

### Verification
Builds only: Debug and Release compiled with zero warnings; the tests were written with the code and not run (owner, 2026-10-01: no testing until M4). Full verification and CI are owed at the M4 gate.
