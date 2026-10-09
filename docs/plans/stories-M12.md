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

---

<a id="us-202"></a>

## US-202 Hand edits on top of the seed: plan and checks

Design: docs/plans/M12-world-editing-design.md sections 3, 6, 7 and 15. Decisions: D-63. Traces to EDT-04, ADR-010. Guide: docs/guides/world-editing.md.

## Built
- `src/sim/region.{h,cpp}`: tile edits over the seed (`setTileEdit`, `clearTileEdit`, `tileEdit`, `tileEditList`, `seedBiomeAt`, `onEdgeWall`). `biomeAt` reads the edits first; a painted tile makes the chunks within one tile be made again.
- `src/sim/world_file.{h,cpp}`: `WorldFile`, `loadWorld`, `saveWorld` (safe write, three backups), `makeWorldRegion`, `worldConfig`, `generatorDifferences`, `kMaxWorldEntries`. Format: `assets/worlds/<name>.json`, schema `assets/data/schemas/world.schema.json` (index entry `../worlds/*.json`).
- `src/game/world_history.{h,cpp}`: the world history (500 steps).
- `src/game/region_view.{h,cpp}`: the tool row (Pan, Brush, Rect, Fill, Reset, five biomes, size, Undo, Redo, Save), painting with the mouse, Ctrl+Z, Ctrl+Y, Ctrl+S, the world file read on open and written by Save, chunk pictures made again when painted (the old one drawn until room), the overview painted again.
- `Renderer::destroyTexture` through `Window` and both backends (the GPU backend lets go at the start of the next frame); `Minimap` gives back the picture it replaces and keeps the old one on screen while it is painted again.
- `GeneratorPanel`: the Preview and Now maps are kept and painted again; Preview lays the painted tiles over the new land; Apply keeps the edits.
- Tests: `tests/sim/world_file_test.cpp` (tile edits, tile order, round trip, chunk-local coordinates, small saves, refusals, cap) and `tests/game/world_edit_test.cpp` (Paint, mouse stroke and Undo, rectangle and fill steps, Reset, edge wall and refused fill, reopen with the file, settings keep the paint, middle-button pan).

## Manual checks (to run at X-M12)
1. F2, **Region**, **Brush**, **Water**, size 5: drag across a meadow; a lake appears at once and the overview follows within a second.
2. **Forest** with **Rect**: drag a rectangle; **Fill** with **Mountain** inside a lake; Ctrl+Z three times returns the seed's land, Ctrl+Y brings it back.
3. **Reset** over the painted tiles gives the seed's land back.
4. **Save**, **Back**, F2 **Region** again: the painted land is there. Open `assets/worlds/default.json`: only the painted tiles are listed.
5. Middle button drag pans while the Brush is chosen.

---

<a id="us-203"></a>

## US-203 Water and mountains: plan and checks

Design: docs/plans/M12-world-editing-design.md section 6. Decisions: D-64. Traces to EDT-04. Guide: docs/guides/world-editing.md.

## Built
- `src/sim/region_shapes.{h,cpp}`: `line4` (the four-connected line, the same from either end), `thicken` (the round brush stamp along tiles), `disc`.
- `src/sim/region_edits.{h,cpp}`: `landWarnings` (the start without water or food, a sealed cave mouth).
- `src/game/region_view.{h,cpp}`: tools River, Lake, Ridge, Cave, Ford, Dry and the Fords field; `paintRiver`, `paintLake`, `paintRidge`, `placeCave`, `paintFord`, `dryWater`; the shape preview while dragging; a warning line above the status line. Every tool is one step of Undo and only tile edits (no new format).
- `GeneratorPanel` moved under the tool row and made smaller.
- Tests: `tests/sim/region_shapes_test.cpp` (lines, thickening, discs, warnings) and `tests/game/water_tools_test.cpp` (River with a ring that blocks walking and a ford that lets people over, River in the game as solid water and a walkable ford, Lake and Ridge, Cave into a cliff that the game can enter, a sealed cave as a warning, Remove a generated lake as an override that undoes and saves, Ford, River with the mouse, water over the start).

## Manual checks (to run at X-M12)
1. F2, **Region**, **River**, size 3, **Fords** 0: drag a river across a meadow; it appears when you let go. Ctrl+Z removes it.
2. Play the same land (once US-207 lets the game read it): the river stops the hero; with Fords 60 there is a place to wade over.
3. **Lake**: press, drag out, release: a round lake. **Ridge**: a line of cliffs.
4. **Cave** on a cliff next to a meadow: a dark mouth; on a meadow: a message. On a cliff with only mountain around it: the red warning.
5. **Dry** on a lake: the whole lake turns to meadow; **Reset** on it brings the water back.

<a id="us-204"></a>

## US-204 Things, people and places in the region: plan and checks

Design: docs/plans/M12-world-editing-design.md section 8. Decisions: D-65. Traces to EDT-04, STO-04. Guide: docs/guides/world-editing.md.

## Built
- `src/sim/world_places.{h,cpp}`: the rules for entries put on the land. `addPlaced`, `movePlaced`, `removePlaced`, `hideSeedThing` (a tombstone for the seed's own wood, berries, flint or herd), `setPlacedProperty`, each returning a `PlacedChange` (the entry before and after) that `applyPlacedChange` does and undoes; `placementProblem` (outside, on water or mountain, no kind, a place without a unique name, something of the group on the tile, a bad property); `placedPropertyProblem` (what each group may set); `nextPlacedId`; `placedNames`; `resolveConflicts` (Keep, Move to the nearest tile that can hold the entry, Remove).
- `src/sim/region_edits.h`: `PlacedEdit` gains `name`, `npcClass` and `properties`.
- `src/sim/world_file.cpp`: the three new fields are written and read; a property the group cannot set, or an id used twice, is a DataError naming file and field. `assets/data/schemas/world.schema.json`: `things`, `people` and `places` entries described with examples.
- `src/game/region_level.{h,cpp}`: `levelFromRegion(region, definitions, catalogs, edits, &problems)` leaves out the seed things the edits took away and adds the entries: a thing is a plant or object (with the interaction overrides of its properties) or an animal, a person is a character with its class and properties, a place is a named spot whose first tag is its kind. An entry that cannot be made is named in `problems`.
- `src/game/region_view.{h,cpp}`: tools Place, Move and Take away; a row under the tools (Thing / Person / Place, Kind with the catalog list, Name, Set key=value, Fix: move, Fix: drop); squares on the map in the Things, People and Places layers, the picked one ringed, names at close zoom; every action is one step of the world history (`WorldCommand::placed`).
- `src/game/odyssey_game.cpp`: the palette from the catalogs; the catalogs `people` and `goals`; `places` and `markers` also list the region's names; the quest check catalog includes them. `assets/data/editor/help.json`: `graph.quest.giver`, `graph.header.who` and `graph.step.goal` offer them.
- Tests: `tests/sim/world_places_test.cpp` (Place, Places, Move and Undo, Tombstone, Stable ids, Conflicts, Properties, Cap) and `tests/game/place_tool_test.cpp` (Place with the game's level, Place by a click, Move and take away, Properties in the game, Fix: move, Pickers and a place marker, Palette).

## Technical choices
- Entries are drawn straight from the list, not from the chunk pictures, so no picture is made again when one is placed, moved or undone.
- A place's name is free text in the world file and the Editor; the level names it with the quest word of it (`red-cliff`), because level files only hold words and quests say words.
- The game reads the world file from US-207 (D-63). Until then the level is made from the file by `levelFromRegion` with the edits, which the tests do.

## Manual checks (to run at X-M12)
1. F2, **Region**, zoom in on a meadow, **Place**, **Thing**, Kind: a fire pit (the list shows the kinds), click a tile: a white square. **Ctrl+Z** removes it, **Ctrl+Y** brings it back.
2. **Person**, Kind: goblin, Name: Old Mara, click: a red square with its name at close zoom. **Place**, Kind: shrine, Name: Red Cliff, click: a gold square. A second place called "red cliff" is refused.
3. **Move**: click Old Mara (ringed), click an empty tile: she moves, her id (hover) does not change. **Set** `hp=300`, then place nothing else: the entry keeps it; `hp=lots` is refused with the reason.
4. **Take away** on a seed tree: a grey outline; **Save**; open `assets/worlds/default.json`: a `things` entry with `remove` true. Regenerate (Settings, change the seed's settings, Apply): the tree stays away.
5. Paint a lake over a placed thing: the Settings panel names it; **Fix: move** puts it on the nearest meadow; Ctrl+Z puts it back.
6. In the Graph editor, a quest step: the Goal field offers "goto red-cliff" and "talk old-mara", the Marker field offers `place:Red Cliff`, the Giver field offers Old Mara. Once US-207 lets the game read the world file, the marker points at the place in the running game.

<a id="us-205"></a>

## US-205 Camps and resources: plan and checks

Design: docs/plans/M12-world-editing-design.md section 9. Decisions: D-66. Traces to EDT-04, MVP-10. Guide: docs/guides/world-editing.md.

## Built
- `src/sim/region.{h,cpp}`: hand edits of what grows: `hideResource`, `setResourceAmount`, `addResource`, `setStart`, `clearPlacedEdits`, `seedResource`, `restoreAmount`; `harvest` takes one unit at a time from a flint or wood spot of several; `goodSite` counts a food spot that was put there. `src/sim/region_save.cpp`: what is left of a spot is kept in `amounts` (optional, version unchanged).
- `src/sim/world_places.{h,cpp}`: the camp rules (kind, one player camp, open ground, 8 tiles apart, the site check unless forced) and the resource rules (kind, `amount`), the property `people` of a camp, `campsOf`, `applyPlacedToRegion`. `makeWorldRegion` applies the resource and camp entries.
- `src/sim/rivals.{h,cpp}`: `Rivals(..., placed)` starts the first clans at the placed rival camps, with their name and size; the generator fills up to two.
- `src/game/region_view.{h,cpp}`: groups Camp and Resource in the placing row, the Anyway toggle, the camps and resources drawn in the Camps and Things layers, the region follows every placement, move, removal and Undo (`refreshPlaced`).
- `assets/data/schemas/world.schema.json`: `camps` and `resources` described; guide section with an example.
- Tests: sim `US-205 Rules ...`, `US-205 Camps ...`, `US-205 Resources ...`, `US-205 Saves ...` (in `tests/sim/world_places_test.cpp`); game `US-205 Camps ...`, `US-205 Resources ...` (in `tests/game/place_tool_test.cpp`).

## Technical choices
- A camp is checked with the same function the generator uses, so the Editor cannot accept a site the generator would refuse unless the owner says "anyway".
- A herd's `amount` is its animals; a flint or wood spot of several gives one unit each time.
- The game reads the world file from US-207 (D-63); the running level still draws one plant a spot.

## Manual checks (to run at X-M12)
1. F2, **Region**, **Place**, **Camp**, Kind `rival`, Name `the Crow Clan`, click on a meadow near water far from your start: a square. Click on a lake: refused, the message says water.
2. Click on a dry plain with no water and food near: refused with the reason; press **Anyway**, click again: placed.
3. **Move** the camp: pick it, click an empty tile with water and food near. Ctrl+Z puts it back.
4. **Camp**, Kind `player`: the start marker (gold outline) jumps there. Take the camp away: the start goes back.
5. **Resource**, Kind `flint`, click a flint spot of the seed, **Set** `amount=50`: the entry is saved; in a test run (US-207) 50 flint can be taken. **Take away** on a tree hides it; Ctrl+Z brings it back.

<a id="us-206"></a>

## US-206 Clans and people inspector: plan and checks

Design: docs/plans/M12-world-editing-design.md section 10. Decisions: D-67. Traces to EDT-05, SDC-07, PIL-08. Guide: docs/guides/world-editing.md.

## Built
- `src/sim/world_setup.{h,cpp}`: the `clans` and `people` sections (`ClanSetup`, `PersonSetup`, `WorldSetup`), their file reading and writing, the text of every field and back (`storeText`, `debtsText`, `opinionsText`, `grudgesText`, `kinText`...), `setupProblems` (the consistency checks), and what a game starts with: `applyClanSetup` (the meals, kin, opinions as written, grudges told by the chronicle), `applyHeroSetup` (store items and debts to rivals), `applyRivalSetup` (a rival's meals).
- `src/sim/world.*` (`setOpinion`, `addSetupGrudge`), `src/sim/hero_life.*` (`stockItem`, `addDebt`): the small doors the setup comes in by. `src/sim/world_file.*`: `WorldFile::setup`, written and read with the other sections. `assets/data/schemas/world.schema.json`: `clans` and `people`.
- `src/sim/world_places.cpp` and `src/game/region_level.cpp`: a placed person's `allow`, `deny`, `day`, `night`, `stock` and `does` properties, checked by the Editor's own field rules and applied to the character in the level.
- `src/game/region_view.{h,cpp}`: the **Inspect** button and panel (Clan and Member targets with their fields, the list of inconsistencies), `setClanField`, `setPersonField`; every change is one step of Undo (`WorldCommand::setup`).
- Tests: `tests/sim/world_setup_test.cpp` (Social, Kin, Economy for the hero and for a rival, Text fields, Checks, World file) and `US-206 Overrides`, `US-206 Inspector` in `tests/game/place_tool_test.cpp`.

## Technical choices
- Opinions are set as written, not added, so the owner's -50 is what the game starts with.
- A grudge is a chronicle entry first and a grudge second, so "the chronicle can tell it" is true by construction.
- Clan members are addressed by number: they exist only when a game starts.

## Manual checks (to run at X-M12)
1. F2, **Region**, **Inspect**. In **Clan** `player`: Store `food=80 flint=20`, Debts `the Crow Clan: fur=3 value=12 days=10`. The red list under the panel says the Crow Clan is not in the world; place a rival camp named `the Crow Clan` (Place, Camp): the line goes.
2. **Member** `0`: Opinions `1=-50`, Grudges `1:30:stole the last flint`, Kin `mother=2`. Ctrl+Z undoes the last field; Ctrl+Y does it again. Save and read `assets/worlds/default.json`: the `clans` and `people` sections.
3. Place two goblins; pick one (Move tool), `Set` `hp=250`, `deny=barter`, `day=06:00 work market; 21:00 sleep home`: only that one is changed; a bad routine text is refused with the reason.
4. Once US-207 starts a game from the file: the store shows 80 meals, the debt is on the books, member 0 has opinion -50 of member 1, and the chronicle holds the grudge with its reason.

<a id="us-207"></a>

## US-207 Play the edited region: plan and checks

Design: docs/plans/M12-world-editing-design.md sections 11 and 12. Decisions: D-68. Traces to EDT-04, EDT-06. Guide: docs/guides/world-editing.md (Playing the world).

## Built
- `src/game/play_world.{h,cpp}`: where the world files are (`worldsFolder`, `worldFilePath`, `worldNames`).
- `src/sim/world_file.*`: `worldTextHash`, `worldFileHash` (FNV-1a, 16 hex digits). `src/sim/region_save.*`: `loadRegion(..., makeBase)` lays a run's region changes onto the world's land instead of the seed's. `src/sim/hero_life.*`: `NewGame::world` and `worldHash`, `hero.json` version 3 (`world`, `worldHash`), `HeroLife::savedWorld`.
- `src/game/odyssey_game.*`: `loadWorld`, `startNewRun` with a world (the file's seed, the setup applied by `applyWorldSetup`, a copy of the file kept by `keepWorldCopy`), the rivals made from the placed camps with their stores (`resetPlay`), `playWorldHere` and `leaveWorldPlay` (Esc and F2), no autosave during a test run, `loadAutosave` that rebuilds the land from the run's copy and warns about a changed file.
- `src/game/region_view.*`: the **Play here** button, the P key, `takePlayRequest`, `saveBeforePlay`. `src/game/run_flow.*`: the World row of the New Game screen, `pickWorld`. `apps/odysseus/main.cpp`: `--world <name>`.
- Tests: `tests/sim/world_run_test.cpp` (Fingerprint, Run save, Region save) and `tests/game/play_world_test.cpp` (New game, a world that cannot be played, Play here, the Region view's request, World saves, Old saves).

## Technical choices
- A run is made, not an Editor session: the setup of US-206 needs a clan and a hero life, so Play here starts a new run with a grown hero.
- The run keeps a copy of the world file: a name and a fingerprint can warn, only a copy can keep the world the run began in.
- The rivals are made again from the seed on every load, so their stores and camps are applied in `resetPlay`, not once.

## Manual checks (to run at X-M12)
1. F2, **Region**, paint a lake near the start, place a person and a rival camp named `the Crow Clan`, **Inspect**: player Store `food=80`. Click **Play here**: the game starts, the clan's store shows 80 meals, the lake is water, the person stands where placed. Esc: back in the Region view, the edits are there.
2. Over the map press **P** on a tile of water: refused with the reason. On a meadow tile: the hero stands on that tile.
3. Quit, start the game, **New Game**: the World row offers `default`; pick it, Start: every edit is there. Play a day (the autosave is written), change the world file in the Region view and save it, then `--load`: the message says the file was changed after the run began, and the lake is still there.
4. Load a run saved by an older build (no `world` in `hero.json`): it loads and plays as before.
