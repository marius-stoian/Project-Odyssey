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
