# M8d design: buildings (written at K-M8d, 2026-10-05)

Owner decisions: D-42, D-43, D-55 (docs/decision-requests/D-55.md). Brief: docs/plans/M8b-M8d-render-light-build-brief.md. A format change is a design question for the owner.

## K-M8d owner answers (2026-10-05)
| Question | Answer |
|---|---|
| Build menu | A side list panel (icon, name, cost; click to place) |
| Blueprints known at the start | Hut and windbreak; the rest are learned later (`known` flag in the kind file; the hero's save keeps the set) |
| Blueprint look | Translucent ghost until complete; then the finished building appears |
| Placement | Anywhere valid (flat, free, not solid or water), 1 m grid, 90 degree turns, red when invalid |
| Tests | Written with each story and compiled; the one full verify (Debug and Release) runs at X-M8d |

## 1. Layers
Simulation (`src/sim/building_*`, deterministic, whole numbers, saved): data (`building_data`), the placed buildings and their rules (`building_store`). Game (`src/game/buildings_view.*`, Editor tab, Build menu): drawing, ghost, input, the interactions. Nothing in the Simulation touches SDL or pixels; a cell is one tile = one metre.

## 2. Formats (fixed here)
- `assets/data/buildings/pieces.json`: `{"materials": {"wood": {"flammability": 60, "burnSeconds": 40}}, "pieces": [{"id", "label", "type": wall|floor|roof|door|window|post|fence, "material", "hp", "buildSeconds", "cost": {"wood": 2}, "colour": "#rrggbb", "height": metres}]}`. Layers: floor under everything, wall (wall, door, window, post, fence), roof over everything: one piece of each layer per cell.
- `assets/data/buildings/kinds.json`: `{"kinds": [{"id", "label", "footprint": [w, h], "layout": [{"piece", "x", "y"}], "cost" (optional: else the sum of the pieces), "buildSeconds" (optional: else the sum), "interior": "fade"|"map", "interiorLevel", "uses": [shelter, sleep, store, work], "light", "owner": none|clan|person, "wear": {"spring": 0, ...}, "buildable", "known", "colour"}]}`.
- `assets/data/buildings/prefabs/<id>.json`: one kind object in the same shape (the Editor's Prefab tab writes them); a prefab with the id of a kind in `kinds.json` is an error.
- Level version 6: `"buildings": [{"id", "kind" or "piece", "x", "y", "rot", "state", "owner", "interior", "condition"}]`, written only when there are some. Older files load without them.
- Save: `buildings.json` next to `things.json`: every building with its state, delivered materials, per-piece condition, fire, owner, contents, the drops on the ground and the fire stream state.

## 3. Rules
- **Placement**: every layout cell must be inside the map, not solid, not blocked by something the game names (plant, object, person), and its layer free. A whole building also needs its footprint free of other whole buildings. Rotation turns the layout in 90 degree steps about the footprint.
- **States**: Blueprint (materials missing or not all delivered), Building (all materials in, being worked), Finished, Rubble. Progress is limited by the share of materials delivered; pieces rise in layout order (floors, walls, doors, roofs). Cancel returns the delivered materials as a drop on the site; the hero picks a drop up by walking over it.
- **Piece by piece**: one piece is a one-piece building of kind `piece:<id>`; the same construction rules apply.
- **Walls block**: a finished wall, post or fence blocks its cell for walking and shots (`TileMap::setObstacle`); doors, windows, floors and roofs do not block (windows do not either: this is a game of people, not of glass). People walk into a room through a door only.
- **Rooms**: flood fill (4-neighbours) over the cells that hold no finished wall, door or window; a component that stays inside the map, is at most 64 cells, is fully roofed and touches a finished door is a room: warm and sheltered. Recomputed only when a piece is built, destroyed or removed.
- **No run, no bag**: with no run the hero has no bag, so the hero builds without materials (the cost is waived); with a run the cost comes out of the hero's bag.
- **Determinism**: every random draw is the stream `buildings` (PCG32), one draw per call; the store has a `hash()` that joins the game's determinism hash.

## 4. Interactions (data, M7)
`build-place` is not an interaction (it is the Build menu). The rest are in `assets/data/interactions/`: `deliver-materials` (target tag `blueprint`), `build-work` (tag `construction`, timed), `cancel-blueprint`, and later `repair`, `douse-fire` (M8e). Their effects are built-in actions: `do deliver-materials`, `do build-work`, `do cancel-blueprint`.

## 5. Order and tests
US-250 (data and own list), US-251 (Build menu, ghost, construction), US-252 (pieces, rooms), US-256 (Prefab tab, palette, buildable prefabs), X-M8d. Tests are written per story; the one full verify runs at X-M8d.
