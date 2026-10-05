# Building data (US-250, US-256; D-42, D-55)

Buildings are data: pieces, kinds and prefabs live in `assets/data/buildings/`. Building data is read when the game starts (a prefab you save in the Editor is available at once; edits to `kinds.json` and `pieces.json` apply at the next start, and the game says so when you save). A mistake in a file is shown as `buildings/<file>:<line>: <message>`, that file is left out and the rest load.

A cell is one tile, which is one metre. A cell holds at most **one piece of each layer**: a *floor* under, a *wall* (wall, door, window, post or fence) standing, a *roof* over.

## `pieces.json`

```json
{
  "materials": { "wood": { "flammability": 60, "burnSeconds": 30 } },
  "maxRoomCells": 64,
  "pieces": [
    { "id": "wall-wood", "label": "Wooden wall", "type": "wall", "material": "wood", "hp": 100,
      "buildSeconds": 3, "cost": { "wood": 2 }, "colour": "#7a5a34", "height": 2 }
  ]
}
```

| Field | Meaning |
|---|---|
| `materials.<name>.flammability` | percent chance each second that a burning neighbour sets a piece of this material alight (US-255). 0: never |
| `materials.<name>.burnSeconds` | how long a piece burns before it falls to rubble. 0: the material does not burn |
| `maxRoomCells` | the biggest enclosed space that counts as a room (default 64) |
| `id`, `label` | the name used in files and the name shown |
| `type` | `wall`, `floor`, `roof`, `door`, `window`, `post` or `fence`. Walls, posts and fences block walking; doors and windows close a room's boundary, a door is a way through |
| `material` | a key of `materials` |
| `hp` | hit points; at 0 the piece falls to rubble |
| `buildSeconds` | work needed (seconds, up to three decimals) |
| `cost` | items brought to the site (item ids of `hero/items.json`) |
| `colour`, `height` | the placeholder art, and the height in metres (shots, shadows) |

## `kinds.json` and `prefabs/<id>.json`

A prefab is a kind in a file of its own; the Editor's **Prefab** tab writes it. A prefab may not use the id of a kind of `kinds.json`.

```json
{
  "id": "hut", "label": "Hut", "footprint": [3, 3],
  "layout": [ { "piece": "wall-wood", "x": 0, "y": 0 }, { "piece": "door-hide", "x": 1, "y": 2 } ],
  "cost": { "wood": 14 }, "buildSeconds": 40,
  "interior": "fade", "interiorLevel": "",
  "uses": ["shelter", "sleep"], "light": "torch", "owner": "person",
  "wear": { "winter": 1 }, "buildable": true, "known": true,
  "colour": "#a98a62", "note": "A one-room hut."
}
```

| Field | Meaning |
|---|---|
| `footprint` | `[width, height]` in cells, 1 to 32 |
| `layout` | pieces by cell offset from the top-left corner, before any turn; at most one piece of each layer per cell |
| `cost`, `buildSeconds` | optional: the sums of the pieces unless written |
| `interior` | `fade` (the roof fades when the hero is inside) or `map` (the door leads into the level named by `interiorLevel`). A placed building may override it (Editor) |
| `uses` | `shelter`, `sleep`, `store`, `work`: what the interactions and the clan see |
| `light` | a kind of `light/lights.json` that shines from a finished building at night |
| `owner` | `none`, `clan` or `person` |
| `wear` | hit points each piece loses when that season ends (`spring`, `summer`, `autumn`, `winter`); wear never takes a piece below 1 hp (D-55: very slow) |
| `buildable` | offered in the Build menu once known. `false`: only the owner places it |
| `known` | the hero has the blueprint at the start of a run (D-55: the hut and the windbreak) |
| `capacity` | storage kinds: how many meals of the clan's store the building keeps at half the spoilage (US-257) |

## In levels and saves

A level has a `buildings` list (level version 6), in its own list and not among the plants: `{"id", "kind", "x", "y", "turns", "finished", "owner", "interior", "interiorLevel"}`; `kind` may be `piece:<id>` for a lone piece. The save folder gets `buildings.json` (state, delivered materials, the hp of each piece, fire, owner, contents, drops, the knowledge of blueprints).

## Playing

- **B** opens the Build menu (a list on the left): *Buildings* (known and buildable) or *Pieces*. Pick one; the ghost follows the pointer, **R** turns it, a click places a blueprint (red: it cannot stand there), **Esc** or a right click stops.
- A blueprint is a see-through ghost until it is complete. Right-click it: *Bring materials*, *Build* (four seconds of work each time) and *Cancel* (the delivered materials are dropped on the site).
- Without a run the materials are free (the hero has no bag).
- Walls, posts and fences block walking; a room is a space enclosed by walls, doors and windows, fully roofed, with a door.

## Editor

- **Build** tool: choose a kind or prefab (`*`), **R** turns it, a click places it finished. Select a building to turn it, set its interior mode and level, set its owner, make it a blueprint, or delete it.
- **Prefab** tab: pick a piece, click the grid (right click erases the top piece of the cell), set the size, interior mode, uses, flags, cost and build time, then **Save**. The prefab appears in the Build tool and, when *buildable* and *known*, in the game's Build menu.

## Building life (M8e)

- The clan builds blueprints too; idle clan members bring up to 2 of each missing material and work (US-253).
- A building whose interior is `map` can be entered (*Go into ...*); the level named by `interiorLevel` is read from `assets/levels/<name>.json` and needs a place named `exit` with the tag `exit` (US-254).
- Rivals at war raid finished buildings at the start of a season; fire weapons ignite; repair and putting out fires are interactions (US-255).
- `owner: person` buildings are given to adults at dawn; housed people lose warmth more slowly; `uses: ["store"]` kinds with a `capacity` keep meals cool; interactions see the use as the tag `storage` (US-257).
