# M12 design: World editing (written at K-M12, 2026-10-09)

Status: written at K-M12 by Mraw (Architect, Full Stack, Tester, Tech Writer). Decisions: D-61 (docs/decision-requests/D-61.md, delegated to Dominus under D-41). Contract: docs/plans/M10-M13-authoring-brief.md sections 4.6, 5 and 6. Stories US-200, 201, 202, 203, 204, 205, 206, 207, exit X-M12. Traces: EDT-04, EDT-05, ADR-010, ADR-020, SDC-12.

## 1. What the code looks like today (found 2026-10-09)

| Thing | Where | Consequence |
|---|---|---|
| The region is a pure function of seed and `RegionConfig` (`lakeLevel`, `mountainLevel`, `riverBand`, `forestMoisture`, `caveNoise`, `edgeWall`, resource per-mille values, `startNeedWithin`); 256 x 256 tiles, 32-tile chunks, whole numbers only | `src/sim/region.{h,cpp}`, `assets/data/sim/region.json` | The generator settings of US-201 are exactly these fields; a preview must call the same code, never a copy |
| `biomeAt` and `resourceAt` are pure and never load a chunk; `chunk(cx, cy)` makes and keeps a chunk; `overrides_` already exists as a private map for the carved start area | `region.cpp` | A tile override layer extends an existing idea; the pure functions must stay pure for the unedited seed |
| A save keeps the seed and the changed chunks only (`kRegionSaveVersion = 1`); loading refuses a size or chunk-size mismatch | `src/sim/region_save.*` | World edits are a separate file (the world file), not part of the run save; the run save records which world file and its version |
| `levelFromRegion` turns a region into a playable `Level` (grass, water, stone, trees, berries, moss, herds) | `src/game/region_level.*` | The Editor's Region view and the game must go through this one function ("Same world", US-200) |
| Levels are versioned (`kLevelVersion = 7`) and the Editor already edits Levels with a command `History` | `src/game/level.*`, `editor_history.*` | A world edit is a command in a world history of its own (D-61 Q5), like the Data tab's own undo |
| NPC population saves at `kSaveVersion = 3`, NPC schedules at 1 | `npc_population.*`, `npc_director.*` | People overrides name kinds and ids that these files already understand; no second people format |
| Schemas, `provides` catalogs, rename across files, Quick check, routines and the timeline view exist (M11) | `src/sim/schema.*`, `data_editor.*`, `timeline_view.*` | The world file gets a schema, a guide and the validator for free; the inspector reuses the form widgets and the timeline |

## 2. Layers and files

| Layer | New files | Existing files touched (additive) |
|---|---|---|
| Luna Engine | `luna/ui`: `Minimap` widget (a cached texture of a grid of colours, click and drag to move, game-agnostic) | `ui.{h,cpp}` |
| Simulation | `src/sim/world_file.{h,cpp}` (the world file: load, validate, write, version and migration), `world_overrides.{h,cpp}` (tile, thing, person, place, camp, resource overrides applied over the seed; conflict list), `world_regen.{h,cpp}` (regeneration with conflicts), `clan_setup.{h,cpp}` (per clan and person setup read at game start) | `region.{h,cpp}` (an override view that the generator consults; pure functions stay pure when there are no overrides), `region_save.*` (records the world file name and version) |
| Game | `region_view.{h,cpp}` (US-200 view, chunk streaming, layers), `generator_panel.{h,cpp}` (US-201), `world_brush.{h,cpp}` (US-202, 203 tools), `place_tool.{h,cpp}` (US-204), `camp_tool.{h,cpp}` (US-205), `world_inspector.{h,cpp}` (US-206), `play_world.{h,cpp}` (US-207) | `editor.{h,cpp}` (World button and Esc/Ctrl+S wiring), `region_level.*` (applies overrides), `odyssey_game.*` (opens a world file) |
| Data | `assets/data/schemas/world.schema.json` and the `index.json` entry, `assets/worlds/default.json` (empty overrides, the shipped generator settings), `docs/guides/world-editing.md` | none |

## 3. The world file (US-200, US-202)
Contract: brief section 4.6. `assets/worlds/<name>.json`, `{ "worldVersion": 1, seed, generator, overrides, clans, people }`; **only differences from the seed are stored**. Changes to the brief's shape, all decided in D-61:

- `generator` holds only the keys that differ from `assets/data/sim/region.json`; an empty object means "the shipped settings". The region size and chunk size are not editable (a different size moves every chunk key and breaks the saves).
- Tile overrides are stored per chunk as a run list: `"cx,cy": [ { "x": 3, "y": 5, "biome": "Water" }, ... ]`, tile coordinates local to the chunk, sorted by tile index so the file diffs cleanly. A chunk with no override is absent.
- Things, people, places, camps and resources are lists of entries with a stable `id` (`"t-0001"`). Each entry has `kind` (a catalog id), `at` (tile x, y), and `state` (`add`, `move` with `from`, `remove` of a seed thing at `at`). A removed seed thing is a tombstone entry, so regeneration cannot bring it back.
- The file is written with the M11 text-patching writer, so owner comments and layout survive.
- Every list is capped (default 20,000 entries across the file, a Game Rules value); over the cap the Editor refuses the edit and says so.

## 4. Region view, streaming and budget (US-200)
- The view draws from a **chunk cache** keyed by chunk and layer-set. A chunk texture is 32 x 32 tiles at the current zoom level's tile size; at most 64 chunk textures live (LRU), so the memory budget is below 24 MB at 8 px a tile (RGBA, 256 x 256 x 64 / ... computed in the story and written into the test).
- **Zoom** has seven levels from one tile on screen at 64 px (single-tile) up to the whole 256 x 256 map in one **minimap** texture (1 pixel a tile, 256 x 256, rebuilt only when an edit touches it, one chunk strip at a time).
- **Layers** (switches): terrain, water, plants, things, people, places, camps. The view asks the region and the overrides for each layer's marks; hiding a layer only skips its draw pass, so "only terrain shows" is a draw-pass test.
- **Budget** (written in the story's test as a limit, measured on the owner's PC class, D-06): a pan across the whole map at the closest zoom builds at most 4 chunk textures a frame and stays above 30 frames a second in Debug and 60 in Release; opening the view allocates no more than 1 chunk strip per frame. A frame-time test with an injected clock proves the build limit; the frame rate is checked in the X-M12 manual run.
- **Same world** is proven by a test: for the seed set used by `region_level_test`, the Editor view's tile sample (`biomeAt` plus overrides) equals the game's `levelFromRegion` ground, tile for tile, and a fingerprint of both is compared.

## 5. Generator settings with live preview (US-201)
- A form per `RegionConfig` field, built from the M11 schema (`region.schema.json` gives range and help), plus sliders for the five land levels. `size` and `chunkSize` are shown read-only.
- **Live preview** recomputes only the minimap at 1 pixel a tile through `Region::biomeAt` (pure, no chunk loaded), in slices of 32 rows a frame so a drag never stalls; the full view updates on release. The preview and the saved world use the same `Region` code (no copy).
- The **start is recomputed** with the new settings (`findStart`), shown as a marker, and the panel lists a **start check**: the start is walkable, has water and food within `startNeedWithin`, and all of it is reachable. A setting that breaks the start check is refused at save with the reason (as the shipped region's carved start does at load).
- **Seed** is a field with a Roll button (random from the Editor's clock, never from the sim stream) and a lock; changing it asks about the overrides (Q3).

## 6. Hand edits on top of the seed (US-202, US-203)
- **Brush tools:** paint a biome (steppe, forest, water, mountain, cave mouth) with a round brush of 1 to 9 tiles, a fill, and an eyedropper. Every stroke is one command in the **world history** (D-61 Q5): Ctrl+Z, Ctrl+Y, up to 500 commands, shared by all M12 tools.
- **US-203 Water and mountains** are the same override layer with extra tools: draw a river as a line with a width (carved as water along a Bresenham line using whole numbers), a lake as a filled disc, a ridge as mountain line with a width, and a "smooth shore" pass. These tools are macro commands made of tile overrides, so there is one file format and one regeneration rule.
- **Edge wall:** the outer `edgeWall` ring stays mountain; the brush refuses it (the world stays closed).
- **Walkability check:** after any water or mountain edit, the Editor checks that the start can still reach water, food and every placed camp on foot, and lists what became unreachable (a warning, not a block: the owner may want an island).

## 7. Regeneration and conflicts (ADR-010, ADR-020)
Changing the seed or the generator settings regenerates the land under the overrides. Rules (D-61 Q3):
1. Overrides stay where they are (tile coordinates), never follow the changed land.
2. After regeneration the Editor lists **conflicts**, each with the entry id and the reason: a placed thing on water or mountain now; a camp that fails `goodSite`; a tombstone for a seed thing that no longer exists; a person placed where nothing walkable remains.
3. The owner chooses per conflict (keep, move to the nearest valid tile, remove) or for all; nothing changes silently. A saved world with unresolved conflicts still loads (the game shows the entry, but skips placement of an invalid thing and logs it once).
4. A test regenerates the default seed with 3 different `mountainLevel` values and checks that the conflict list is exact and stable.

## 8. Things, people, places (US-204)
- Tools place a **thing** (object, plant, animal, item pile) or a **person** (an NPC kind, US-155 spawns) or a **place** (named spot: shrine, meeting ground, grave, camp site) from the catalogs, with the M11 reference pickers.
- Each placed entry is an **override per placed thing** (brief: overrides per kind and per placed thing): a placed thing may carry `properties` that win over its kind's data (hp, tags, the interaction it offers); the schema marks which are allowed.
- People are not copied: a placed person is `{ kind, at, name?, class?, properties }` and is created by the NPC population when the region is loaded, with the same ids on every load.
- Selection, move (drag), duplicate, delete, multi-select with a box, snap to tile. Everything is a world command.

## 9. Camps and resources (US-205)
- A **camp** is a clan seat: name, clan id, position, a radius, and its start store; the tool runs the shared `goodSite` check and shows the verdict and the reason; an invalid site can be placed with an explicit "place anyway" that is recorded in the entry (`forced: true`) and listed in the Quick check.
- **Resources** (flint, wood, berries, herds) are placed, moved or removed as entries; a harvest state (taken day) is a run state and is never stored in the world file.
- Seed resources can be hidden by tombstone (remove) so the owner can clear a place; the resource counts per kind are shown in the panel.

## 10. Clans and people inspector (US-206)
- A **side inspector** for the selected camp or person, built on the M11 form widgets: per clan `leader, members, stance, store, debts, partners`; per person `kin, opinions, grudges, items, routine, actions, properties` (brief 4.6). Each field has help (`EditorHelp`) and a schema entry.
- **Routine** uses the M11 timeline view and the same day-block data (US-196); **actions** are a subset of the interactions catalog the person may use (an allow list); **properties** are tags and values the quest rule language can read.
- The inspector shows effects from the shipped data as grey "inherited" values, and an edit becomes an override of that one field. A "reset to inherited" button removes the override.
- **Consistency checks** live in the Simulation and are shown as a list: kin cycles, a member in two clans, a debt to a clan that is not in the world, a partner who does not exist.
- The graph editor (US-173) is linked for a person's interaction graph: "Open graph" jumps to it for the selected kind.

## 11. Play the edited region (US-207)
- A **Play here** button starts the game on the edited world file with the hero at the start or at a chosen tile, using `levelFromRegion` plus the overrides, the clan and people setup, and the NPC population (US-155 spawns). Escape returns to the Editor with the edits intact.
- It runs through the **play-in-editor debugger** (US-186): the quest tracker and event log are shown; reloading the world file in the Editor while playing applies on the next Play here (not hot, D-61 Q7).
- The Quick check (US-194) runs on the world: 20 years from the setup, with the same pass criteria, so a bad setup is seen before the owner walks in.
- The saved game records the world file name and its version, so a run keeps the world it started in; opening a save whose world file changed later warns and loads the saved copy of the overrides (a run save embeds only the file name and a hash, and a changed hash is a warning, not a refusal; D-61 Q8).

## 12. Saves and migrations
- `worldVersion` starts at 1. A newer file than the game reads is refused with the version named; an older file is migrated in memory and written only on explicit save, after a `.bak` copy (as the level files do).
- The run save version is bumped once (the world file name and hash). A migration test loads a version-N save and a version-N-1 save.

## 13. Tests per story (outline)
| Story | Cases (name = test) |
|---|---|
| US-200 | Open at every zoom, Layers (draw passes), Same world (tile for tile and fingerprint), chunk cache budget (build limit per frame), minimap cache hit |
| US-201 | Setting changes the preview and the saved file (only differences), start check refuses a broken setting, seed roll is deterministic from a given clock, shipped settings produce today's fingerprint |
| US-202 | Stroke, undo, redo, round trip, regeneration keeps overrides, conflict list exact, edge wall refused |
| US-203 | River, lake and ridge macros equal their tile overrides, reachability warning |
| US-204 | Place, move, remove, tombstone survives regeneration, placed person has stable id, properties override kind data |
| US-205 | Good site verdict equals `Region::goodSite`, forced site flagged, harvest state not stored |
| US-206 | Inherited vs override display, reset, each consistency check, routine edit changes the next day's schedule |
| US-207 | Play here starts with edits, Escape returns, save embeds world name and hash, hash change warns |
| X-M12 | Exit demonstration: reshape the land, place a camp and a person, change a clan, press Play here, see it in the running game; clean-worktree verify Debug and Release, strict 3 s first-frame on the owner's PC |

## 14. Risks
| Risk | Mitigation |
|---|---|
| 256 x 256 editing stalls the frame | Chunk textures, LRU, build limit per frame, minimap cached; budget test |
| A second copy of the generator in the preview drifts from the game | Preview calls `Region` itself; "Same world" test |
| Regeneration silently moves the owner's work | Overrides never move; conflict list; no silent change |
| Placed people and the NPC population disagree on ids | Stable ids in the world file; population reads them; round-trip and reload tests |
| World file grows without bound | Cap in Game Rules, sorted run lists, tombstones only when needed |
| Dirty tree: the owner's uncommitted level and NPC edits are in the main checkout | Commit only M12 files; verify in a clean worktree of `qa` (as X-M11 did) |

## 15. Amendments
- **US-201 (D-62):** Apply writes `assets/data/sim/region.json` (changed lines only), not a `generator` section of the world file; the seed is not a field (it comes with a new game); `size` and `chunkSize` are read-only. Preview is a 128 x 128 sample (section 5 said 1 pixel a tile of the full map).
- **US-202 (D-63):** the world file is read by the Region view and the sim loader (`sim::loadWorld`, `makeWorldRegion`); the game reads it from US-207. A painted tile that equals the seed biome is not an edit. `Renderer::destroyTexture` exists (section 4 had no way to release a chunk picture). Per-conflict choices (section 7, rule 3) move to US-204 and US-205, where placed things exist.
- **US-203 (D-64):** the smooth-shore pass of section 6 is not built (it is not in the acceptance criteria). A ford is a strip of land, not a new biome. The warnings are the start and sealed cave mouths; camps join them when they exist (US-205).
- **US-204 (D-65):** a thing is a plant or object (objects ride on plants) or an animal; entries are drawn from the list, not the chunk pictures; Move is click-pick and click-drop, and drag, duplicate and box select are left out (not in the acceptance criteria); the conflict choices are two buttons for all (Fix: move, Fix: drop) and the Move and Take away tools for one; a place is named freely in the file and as a quest word (`red-cliff`) in the level. The game reads the world file from US-207.

- **US-205 (D-66):** camps are `player` or `rival` and use the generator site check unless placed anyway; rival clans start at the placed camps (the generator fills up to two); amounts live in a resource entry and a flint or wood spot of several gives one unit at a time; the region follows the entries through an overlay (`applyPlacedToRegion`). Resource counts per kind, a camp start store and a camp reachability warning are left out.
