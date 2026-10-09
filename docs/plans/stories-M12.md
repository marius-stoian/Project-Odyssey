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
