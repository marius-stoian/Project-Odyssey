# World editing (M12)

The Region view (F2, **Region** on the toolbar) shows the whole generated land of the game on a zoomable map. Later stories of M12 add editing on top of it (generator settings, hand edits, camps, clans and people); this page grows with them.

## The Region view (US-200)
- **Back** or Esc returns to the level editor.
- **Zoom:** the mouse wheel, the + and - keys or the **+** and **-** buttons. The widest zoom (2 pixels a tile) shows the whole 256 x 256 map; the closest (64 pixels a tile) shows single tiles with cell lines.
- **Pan:** drag with the left or middle mouse button, or use the movement keys. Click in the overview (bottom-right) to jump there. **Start** centres on where the clan begins (outlined in gold).
- **Layers:** Terrain, Water, Plants (trees and berries), Things (flint), People (animals and herds for now), Places and Camps (empty until later stories). Each button shows or hides one layer; it only changes what is drawn.
- The view is made from the same seed and the same `assets/data/sim/region.json` as the game, so it is the same world. It changes nothing: nothing is saved.

## Generator settings (US-201)
Press **Settings** in the Region view. The panel on the right has one field for each setting of `assets/data/sim/region.json` (the size of the region and of a chunk are not offered: changing them would move every chunk). Hover a field for what it does, its range and an example.
- **Preview** makes a small map of the land your numbers would make, beside the map of the land now, and says how much of it is water, mountain and forest. It writes nothing. It takes a fraction of a second.
- **Apply** writes the changed settings to `region.json` (nothing else in the file moves) and opens the new land in the Region view. A game in play keeps its land; the next new game uses the new settings.
- **Revert** puts the fields back to the settings of the land on screen. **Close** hides the panel.
- A number that breaks a rule is refused with the reason: the mountain level must be at least 100 above the lake level, the herd maximum at least the herd minimum.
- **Your hand edits** (from the next stories) are never moved or dropped. When the new land does not suit one (a thing now stands in water, a camp has lost its water), the panel lists it so you can decide.

Example: lake level 140 to 300 and mountain level 800 to 900 makes a much wetter region; Preview shows the share of water rising.

## Painting the land (US-202)
Under the bar of the Region view is a row of tools. **Pan** (the default) drags the map with the left button. **Brush**, **Rect**, **Fill** and **Reset** paint with the left button; then the middle or right button pans. Choose a biome (Steppe, Forest, Water, Mountain, Cave) and the brush size (1 to 9, a round brush).
- **Brush:** click or drag; a fast drag leaves no gaps. **Rect:** drag from corner to corner; the frame shows where. **Fill:** click; the connected tiles of the same biome change (refused above 20,000 tiles). **Reset:** give tiles back to the land the seed makes.
- Each stroke, rectangle or fill is one step of Undo: Ctrl+Z, Ctrl+Y (500 steps). The outer ring of mountains (the edge wall) is never painted.
- What grows follows the land: trees stop at a forest you painted over, and a meadow you painted into a forest has none.
- **Save** (or Ctrl+S) writes `assets/worlds/default.json`. The status line shows "(unsaved)" until you do. The file holds the seed and only the tiles that differ from it, so 100 painted tiles are 100 short entries, not the map. A world file belongs to one seed: when the region has another seed the file is left alone.
- Painting a tile the biome the seed already gives is not an edit. If you change the generator settings later, such a tile follows the new land.

Example file (one painted tile in chunk 2,1; coordinates are inside the chunk):
```json
{"worldVersion": 1, "seed": 1, "generator": {}, "overrides": {"chunks": {"2,1": [{"x": 6, "y": 8, "biome": "Water"}]}, "things": [], "people": [], "places": [], "camps": [], "resources": []}}
```
`generator` lists only the settings that differ from `assets/data/sim/region.json`. The other lists (things, people, places, camps, resources) are for the next stories. The schema is `assets/data/schemas/world.schema.json`.

## Water and mountains (US-203)
More tools in the row under the bar. They are made of the same painted tiles as the Brush, so they save, undo (one step each) and keep through generator changes like any painting.
- **River:** press at the source, drag, release at the mouth. The river is as wide as the brush size. **Fords** (the number field next to the size) breaks the river with a crossing of land every that many tiles; 0 means no fords, so nobody can cross. A river that runs edge to edge with no ford cuts the land in two.
- **Ford:** click a river or lake: the water under the brush becomes land, a place to wade over.
- **Lake:** press at the middle, drag out to the shore, release.
- **Ridge:** drag a line of cliffs, as wide as the brush.
- **Cave:** click a cliff (a mountain tile) to put a cave mouth into it. In the game a cave mouth is a path tile, so people can walk in. A mouth with only mountain around it is sealed: the view warns.
- **Dry:** click a lake or a river: all the water connected to it becomes meadow. The seed's lake is still in the seed; the meadow is a change on top of it (**Reset** brings the lake back).
- A warning line appears above the status line when the start has become water or has lost its water and food, or when a cave mouth has no way in. It is only a warning: you may want an island.

Example: to cut the land with a river that can be crossed in one place, choose **River**, size 3, set **Fords** to 60, and drag from the north edge to the south edge. There is a crossing of meadow every 60 tiles, the first one 30 tiles from the source.
