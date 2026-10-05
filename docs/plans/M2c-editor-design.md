# M2c design: Level editor (Game mode and Editor mode)

Architect's design for US-120..US-126. Codex v1.7 (K-M2c); requirements v1.7 (E12); decisions D-05, D-19. Builds on the brief [M2c-editor-brief.md](M2c-editor-brief.md).

## 1. Art pipeline (US-120)
- **Library:** `stb` from vcpkg (D-13: libraries come from vcpkg; stb_image reads PNG, stb_image_write writes it). ADR-018 records why, and why the editor UI is our own toolkit rather than Dear ImGui.
- **Luna Engine:** `loadPng(path) -> Image` and `savePng(image, path)` (`src/luna/engine/image_io.{h,cpp}`), and pure image operations in `image.h`: crop, box-filter scale into a target box (keeping the aspect ratio, bottom-centre anchored for characters), mirror, and background removal (flood fill from the crop's border over pixels close to the border colour, tolerance in data).
- **Cuts as data:** `assets/sprites/cuts.json` lists, per sheet, the frames to take: `{ "name", "sheet", "rect": [x, y, w, h], "kind": "tile" | "character", "key": "border" | "none" }`. Tiles are cropped *inside* their drawn border (no background to remove) and scaled to 32x32; characters get background removal and are fitted into 32x48 with the feet on the bottom row.
- **Finding the rectangles:** the cutter has `--find <sheet> <region>`, which lists the connected blobs of non-background pixels in a region (bounding boxes, in reading order), so rectangles are measured, not guessed. Labels are small blobs under each figure and are left out by a minimum size.
- **Output:** `odysseus_atlas` (apps/atlas, Game identity) writes `assets/sprites/atlas/characters.png`, `tiles.png` and `atlas.json` (name -> cell), plus `--preview <file>`: a contact sheet of every frame on a checkerboard, for review. The atlases are committed; the game loads only them.
- **Frames per character:** the hero comes from the 8-direction sheet (columns S, SW, W, NW, N, NE, E, SE; rows = poses: stand and two steps, played as 0,1,0,2). Monsters and the 20 front-view heroes have one frame (they face south whatever their facing). The landscape pictures are not used in M2c.
- **Fallback:** a missing or damaged atlas logs the file and the reason, and the programmer art is used (the demo keeps working).

## 2. Pointer, font and widgets (US-121)
- **Platform:** new events `MouseMoved` (window pixels), `MouseButtonDown/Up` (Left, Right, Middle), `MouseWheel`, `TextInput` (printable ASCII), and more keys (F1, F2, Delete, Backspace, Z, Y, S, G, R, Ctrl, digits and letters).
- **Engine:** `Pointer { x, y (virtual pixels, or -1 outside the picture); held/pressed/released per button; wheel }`, built by the application from the window->virtual mapping (`integerScale`) and passed with the intents. `Game::update(const Intents&)` stays; `Intents` gains `pointer()`, `keys()` (shortcut presses: Ctrl+Z...) and `text()` (typed characters). Scripted input gains `--click x:y:t[:button]`, `--drag x1:y1:x2:y2:t1:t2` and `--key name:t`.
- **Input rate:** the game ticks at 20 Hz; presses and releases between ticks are kept until the next tick, and a brush stroke joins the previous and current pointer cell with a line (Bresenham) so fast drags leave no gaps.
- **Drawing UI with a texture-only renderer:** the toolkit owns one small texture: the font (5x7 ASCII glyphs, 6 px advance, drawn by code) and solid colour swatches (panel, border, highlight, text colours) stretched by drawing them tile by tile. No renderer change is needed.
- **Widgets:** `Panel`, `Button` (icon or text, hover label), `ListBox` (scrolling, wheel), `NumberField`, `TextField`. Each has `bounds`, `handle(const Pointer&, keys, text) -> bool` and `draw(UiPainter&)`: classes with virtual functions, owned by `std::unique_ptr` in their panel. Tested headless with the RecordingRenderer.

## 3. Levels (US-122)
- `assets/data/tiles.json`: tile kinds `{ name, atlas, solid }` (grass, dirt, path, stone, mossy stone, sand, snow, ice, mud, water, wooden floor, rock...). `assets/data/characters.json`: character kinds `{ name, atlas frames, directions (8 or 1), hp, swordDamage, enemy }`.
- Level file `assets/levels/<name>.json`, `levelVersion: 1`: `{ name, width, height, defaultGround, ground: [kind names row by row, run-length encoded], characters: [{ id, kind, x, y (feet, world pixels), facing, name, hp, swordDamage }], heroStart: {x, y}, nextId, targets: [...] }`. Saved like world saves (temp file, rename, 3 backups); validated on load with file and field in every error; the last good backup is used when the file is damaged.
- `Level` (src/game/level.{h,cpp}) is plain data plus validation; `TileMap` is built from it for drawing and walking. The built-in test map, the two straw targets and the demo enemy become `assets/levels/valley.json`; `--level <file>` picks another.

## 4. Modes (US-123)
- `enum class Mode { Game, Editor }` in `OdysseyGame`; F1 and F2 switch; `--editor` starts in the Editor; the mode name is drawn in the top-right corner.
- Game mode = today's play state built from the Level (hero at `heroStart`, placed characters, targets). Editor mode pauses it and edits the Level; switching back to Game rebuilds the play state from the Level.
- The editor camera pans (arrows, WASD, right-drag) at 1:1 scale.

## 5. Editor tools and history (US-124..US-126)
- Screen: tool bar at the top (brush, rectangle, fill, eraser, place, select, settings; grid toggle; save), palette on the left (tiles or characters, scrolling), properties panel on the right when something is selected; the map in between.
- Every edit is a `Command` with `apply(Level&)` and `undo(Level&)` (paint stroke: list of cells with old and new kinds; fill: the same; character add, remove, move, turn, property change; resize: old and new grids and dropped characters; hero start move). `History` keeps 100 steps; a new command clears the redo list. Ctrl+Z / Ctrl+Y; Ctrl+S saves.
- Flood fill: 4-neighbour, iterative with an explicit stack, bounded by the map.
- Characters get ids from `nextId` (never reused); selection stores the id, never a pointer.
- Resize (8..256 tiles each way): painted cells keep their coordinates; new cells get the default ground; characters outside are dropped (undoable).

## 6. Tests
- Headless (`odysseus_game_tests`): image operations and the cutter (known synthetic sheets), atlas index loading and fallback, pointer mapping, widgets, Level JSON round trips and every validation error, brush/fill/eraser, random edit sequences with undo and redo against a simple model, character commands, resize.
- End to end (`odysseus.exe` with scripted input): F2/F1, paint and save, place and strike, with screenshots and pixel checks.
- Existing US-024 and US-029 tests keep passing on `valley.json`.
