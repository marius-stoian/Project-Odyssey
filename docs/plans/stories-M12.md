# Story plans: M12

Per-story plans for milestone M12 (World editing), one section per story. The design is in `docs/plans/M12-world-editing-design.md`; decisions in D-61.

- [US-200](#us-200)

---

<a id="us-200"></a>

## US-200 The region in the Editor: plan and checks

Design: docs/plans/M12-world-editing-design.md sections 2 and 4. Decisions: D-61 (Q4, Q9). Traces to EDT-04. Guide: docs/guides/world-editing.md.

## Built
- `src/luna/engine/minimap.{h,cpp}`: `Minimap`, a game-agnostic cached picture of a big map. It is painted a few rows at a time (`build`), uploaded once when complete, and turns a click into a map cell (`cellAt`).
- `src/game/region_view.{h,cpp}`: `RegionView`. It makes its own `sim::Region` from the same seed and `assets/data/sim/region.json` as the game, so it shows the same world. Seven layers (Terrain, Water, Plants, Things, People, Places, Camps); six zoom steps from 2 px a tile (the whole 256 x 256 map on screen) to 64 px (single tiles, with cell lines); a drag pans, the wheel and the + and - keys zoom around the pointer; an overview in the corner (a cached `Minimap` with the visible part outlined) moves the view when clicked; the start is outlined in gold; the status line names the tile under the pointer.
- Each 32-tile chunk is drawn from one small picture per layer (one pixel a tile, stretched), made the first time the chunk is seen and kept. At most `kChunkBuildsPerFrame` (4) are made in a frame; the rest wait for the next frame. Only the part of a chunk that is on screen is drawn. Hiding a layer only skips its pictures.
- People and animals share the People layer until people can be placed (US-204). Places and Camps are switches with nothing to show until US-204 and US-205.
- `src/game/editor.{h,cpp}`: the **Region** button on the toolbar; the view takes the whole screen like the Data tab and Esc comes back. `src/game/odyssey_game.cpp` tells it which seed is in play (seed 1 when no region is loaded).
- Tests: `tests/game/region_view_test.cpp` (Open, Open budget, Layers, Same world, overview, wheel and drag) and `tests/luna/minimap_test.cpp`.

## Technical choices
- The renderer cannot release a texture yet, so a chunk picture is never rebuilt in this story (the region cannot change here). US-202 (hand edits) adds `Renderer::destroyTexture` before it needs to rebuild one.
- The view calls the region's own pure functions (`biomeAt`, the chunk's resource list), never a copy of the generator, so a change in the generator reaches the view with no second edit.

## Manual checks (to run at X-M12)
1. F2, **Region**: the whole map shows at once; the overview in the bottom-right corner fills in within a second.
2. Scroll the wheel up over a river: the map zooms around the pointer; at the closest zoom single tiles and cell lines show.
3. Drag with the left mouse button: the map pans smoothly. Click in the overview: the view jumps there.
4. Switch **Plants**, **People** and **Things** off: only terrain and water remain. Switch them on: they come back.
5. Compare with the game: **Back**, F1, play. The shape of the lakes and the places of trees match the Region view (the seed in the status line is the run's seed).
6. Frame rate: pan across the whole map at the closest zoom; it stays smooth (F3 shows the frame rate).

---

<a id="us-201"></a>

## US-201 Generator settings with live preview: plan and checks

Design: docs/plans/M12-world-editing-design.md section 5 (amended in section 15). Decisions: D-62. Traces to EDT-04. Guide: docs/guides/world-editing.md.

## Built
- `src/game/generator_panel.{h,cpp}`: `GeneratorPanel`, opened by the **Settings** button of the Region view. One number field for each of 14 settings of `assets/data/sim/region.json` (size and chunk size are not offered), with tooltips from the schema's help (`data.sim-region.<key>`). **Preview** makes a `sim::Region` from the draft and paints a 128 x 128 sample of it (32 rows a tick) beside the same sample of the land on screen, with the share of water, mountain and forest. **Apply** patches only the changed lines of `region.json` through `DataDocument` (the same writer as the Data tab, three backups), tells the game the file was written (the sets that watch it read it again), and reopens the Region view over the new land with the same zoom and position. **Revert** and **Close**. A draft that breaks a rule is refused with the reason.
- `src/sim/region.{h,cpp}`: `regionConfigProblems`, every rule of `loadRegionConfig` for a draft; a test keeps them equal.
- `src/sim/region_edits.{h,cpp}`: `RegionEdits` (painted tiles, placed things, people, places, camps, resources, tombstones), `effectiveBiome`, `findConflicts`. No file format yet (US-202).
- `RegionView` holds the edits, shows the panel, gives it the pointer and the keys while the pointer is over it or a field is being typed in.
- Tests: `tests/game/generator_panel_test.cpp` (Settings, Preview, refusals, Apply, Keep edits, typing in a field) and `tests/sim/region_edits_test.cpp` (the check equals the loader, conflicts, tombstones).

## Technical choices (D-62)
- Apply writes region.json (not the world file, which does not exist yet); the seed is not a field; the run in play keeps its land.
- Each Preview uploads one 128 x 128 texture (the renderer cannot release textures; US-202 adds that).

## Manual checks (to run at X-M12)
1. F2, **Region**, **Settings**: 14 fields on the right; hover one: purpose, range, example.
2. Change the lake level to 300 and the mountain level to 900, **Preview**: a map appears beside "Now" in under a second and the water share rises.
3. Type a mountain level of 150: Preview says the mountain level must be at least 240; nothing changes.
4. **Apply**: the big map changes; `assets/data/sim/region.json` differs from before in exactly those two lines (open it in an editor).
5. **Back**, start a new game: the land follows the new settings. Set the numbers back and Apply to restore the shipped land.
