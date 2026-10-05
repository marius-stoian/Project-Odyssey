# Story plans: M2c

Per-story plans for milestone M2c, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-120](#us-120)
- [US-121](#us-121)
- [US-122](#us-122)
- [US-123](#us-123)
- [US-124](#us-124)
- [US-125](#us-125)
- [US-126](#us-126)

---

<a id="us-120"></a>

## Plan US-120: Real art in the game

Codex v1.7, prompt S-US-120. Design: [M2c editor design](M2c-editor-design.md) section 1. Traces to D-05, D-19, D-04, D-16.

### What was built
| File | Layer | What |
|---|---|---|
| `src/luna/engine/image_io.{h,cpp}` | Engine | `loadPng`, `savePng` over stb (vcpkg `stb`, ADR-018) |
| `src/luna/engine/image_ops.{h,cpp}` | Engine | `crop`, `fitInto` (box filter, bottom-anchored), `mirrored`, `removeBackground`, `opaqueBounds`, `findBlobs`, `colourDistance` |
| `src/game/art.{h,cpp}` | Game | cut list (`cuts.json`, validated), `cutAtlas`, `saveAtlas`, `loadAtlas`, `contactSheet`, `makeArtSet` (atlas or programmer art) |
| `apps/atlas/main.cpp` | Program `odysseus_atlas` | writes `assets/sprites/atlas/`; `--preview`; `--find` measures figures in a sheet |
| `assets/sprites/cuts.json`, `assets/sprites/atlas/` | Data | 74 character frames (hero and wanderer: S, E and N views x 8 frames, W mirrored; 10 monsters), 16 tiles |
| `src/game/odyssey_game.cpp` | Game | draws hero, ground and enemy from the art set; logs "Art: the owner's atlas" or why not |

Finding: the "8 directions" sheet holds three views (front, side, back) with eight animation frames each, not eight directions; the diagonals use the side view they move towards.

### Tests
| Scenario | Test |
|---|---|
| Atlas | `US-120 Atlas` (synthetic sheet: background removed, 32x48 on the bottom row, mirror, tile trimmed to 32x32, save and load), `US-120 The committed atlas is the owner's sheets, cut`, `US-120 Cut list errors name the field` |
| Heroes and ground | `US-120 Heroes and ground` (the art set is the owner's, in the drawing layouts) |
| Missing art | `US-120 Missing art` (no atlas; a damaged picture: the file is named, programmer art stands in) |

### Manual check results (2026-09-30)
Contact sheet [contact-sheet.png](../evidence/US-120/contact-sheet.png): every frame is a whole figure or tile, no labels or background left. Game screenshot [game-own-art.png](../evidence/US-120/game-own-art.png): grass, path, stone, the hero and the goblin enemy from the owner's art.

---

<a id="us-121"></a>

## Plan US-121: Point, click and read on screen

Codex v1.7, prompt S-US-121. Design: [M2c editor design](M2c-editor-design.md) section 2. Traces to D-19, ARC-03.

### What was built
| File | Layer | What |
|---|---|---|
| `src/luna/platform/events.h`, `sdl_events.cpp`, `window.cpp` | Platform | mouse move, buttons, wheel (flipped wheels corrected), typed text (printable ASCII); keys F1, F2, Delete, Backspace, Ctrl, Z, Y, G, R; mouse in real pixels on high-DPI screens; text input started with the window |
| `src/luna/engine/input.{h,cpp}` | Engine | intents ModeGame, ModeEditor, Undo, Redo, Save, Delete, ToggleGrid, Rotate, Erase, Confirm (Ctrl chords: Ctrl+S saves without walking); `Pointer` in virtual pixels (held, pressed and released since the last tick, wheel); typed text; scripted pointer and typing |
| `src/luna/engine/application.{h,cpp}` | Engine | the pointer area follows the window size; scripted clicks, drags, hovers and typing |
| `src/luna/engine/ui.{h,cpp}` | Engine | 5x7 bitmap font (95 printable characters) and 12 colours on one texture; `UiPainter`; widgets `Button` (hint on hover), `ListBox` (wheel, scroll bar), `NumberField`, `TextField`, `Panel`; `ImageRenderer` (draws into a picture, for tests) |
| `apps/odysseus/main.cpp` | Program | `--click`, `--drag`, `--point`, `--type`; every intent usable with `--hold` |

Game code still reads only intents (the pointer is part of them): `US-021 Game reads only intents` passes unchanged.

### Tests
| Scenario | Test |
|---|---|
| Pointer | `US-121 Pointer` (virtual pixels, black bars, a click between ticks, right button, wheel, Ctrl chords, tool keys, typing, scripted input), `US-121 SDL mouse and text become Luna events` |
| Text | `US-121 Text` (the A glyph pixel by pixel, every character has a glyph, drawn pixel-exact) |
| Widgets | `US-121 Widgets` (button click and hint, list select and scroll, number field digits, limits and wheel, text field length, panels keep clicks from the world) |

### Manual check results (2026-09-30)
[ui-showcase.png](../evidence/US-121/ui-showcase.png): all 95 glyphs in all 12 colours, and each widget, at 480x270: readable, the hint box above everything.

---

<a id="us-122"></a>

## Plan US-122: Levels as data

Codex v1.7, prompt S-US-122. Design: [M2c editor design](M2c-editor-design.md) section 3. Traces to D-19, ARC-08.

### What was built
| File | What |
|---|---|
| `assets/data/tiles.json` | 16 ground kinds (name, atlas frame, solid); the first four keep the old tile numbers (grass, path, stone = the old rock, water) |
| `assets/data/characters.json` | 12 character kinds (hero, wanderer, 10 monsters): frames, 8 or 1 directions, HP, sword damage, enemy or not |
| `assets/levels/valley.json` | the demo that used to be code: the 64x64 test map, hero start, two straw targets, the goblin |
| `src/game/level.{h,cpp}` | `Definitions`, `Level` (plain data), `readLevelFile` (every error names file and field), `loadLevel` (falls back to .bak1-.bak3), `saveLevel` (temp file, rename, 3 backups), `buildTileMap`; ground stored as runs per row |
| `src/game/odyssey_game.{h,cpp}`, `enemy.{h,cpp}` | the game loads a level; enemies are the level's placed characters (any number, each with its own art); the sword hits the nearest in reach |
| `src/game/art.{h,cpp}` | the ground strip follows tiles.json; the whole character atlas and its red copy; `ArtSet::frame` |
| `apps/odysseus/main.cpp` | `--level <file>` |

### Tests
| Scenario | Test |
|---|---|
| Load | `US-122 Load` (valley.json equals the old built-in demo; the game plays it; another level with other ground, hero start and characters) |
| Round trip | `US-122 Round trip` (every cell, character and setting identical after save and load; a second save keeps a backup) |
| Damaged | `US-122 Damaged` (a broken file: the last good backup loads and the note names the file; each broken field is named) |
| The demo still works | `US-024 Walk to the rock`, `US-029 Throw in the game` and the spear tests, now on valley.json |

### Manual check results (2026-09-30)
`odysseus.exe` and `odysseus.exe --level <snowfield>` start from their files (the log names the level and file).

---

<a id="us-123"></a>

## Plan US-123: Game mode and Editor mode

Codex v1.7, prompt S-US-123. Design: [M2c editor design](M2c-editor-design.md) section 4. Traces to D-19.

### What was built
| File | What |
|---|---|
| `src/game/editor.{h,cpp}` | `Editor`: edits the game's Level; its own camera pans with the move keys (8 px a tick) or a right-button drag, at 1:1; draws the level, placed characters, straw targets and the hero start (framed, "START") |
| `src/game/odyssey_game.{h,cpp}` | `Mode { Game, Editor }`; F1 / F2 (`switchMode`); in the Editor the world is paused; back in Game `resetPlay` rebuilds map, camera, hero, targets, enemies and weapons from the level; the mode is shown in the top-right corner ("GAME  F2: EDIT" / "EDITOR  F1: PLAY") |
| `apps/odysseus/main.cpp` | `--editor`; the last argument is no longer skipped; a missing value is an error |

Codex issue CI-007: the editor lives in `src/game/editor.{h,cpp}` rather than `src/game/editor/`, because ADR-016's boundary rule has no sub-folders.

### Tests
| Scenario | Test |
|---|---|
| Switch | `US-123 Switch` (F2: the hero stands still, the game clock stops, the keys pan 8 px a tick, a right drag pans with the pointer, the editor draws) |
| Back to play | `US-123 Back to play` (a new hero start, a painted tile and a moved goblin with other HP are played; the hero walks) |
| Game untouched | `US-123 Game untouched` (F1 in Game does nothing; walking, spears; a trip through the Editor restores the level's play state), and the existing US-024 and US-029 end-to-end tests |
| In the window | ctest `US-123 Modes in the game` (F2 and F1 by script; log and screenshots) |

### Manual check results (2026-09-30)
[editor.png](../evidence/US-123/editor.png): the valley in Editor mode with the hero start framed and labelled, the goblin, and "EDITOR  F1: PLAY" in the corner. [game.png](../evidence/US-123/game.png): back in Game mode, the hero walking.

---

<a id="us-124"></a>

## Plan US-124: Paint ground tiles

Codex v1.7, prompt S-US-124. Design: [M2c editor design](M2c-editor-design.md) section 5. Traces to D-19.

### What was built
| File | What |
|---|---|
| `src/game/editor_history.{h,cpp}` | `Command` (apply, undo, name), `PaintCommand` (cell changes), `History` (100 steps; a new edit forgets the redo list); pure tools `lineCells` (Bresenham), `rectangleFill`, `floodFill` (4 neighbours, explicit stack) |
| `src/game/editor.{h,cpp}` | tools Brush (click or drag; a drag between ticks is joined by a line; one command per stroke), Rectangle (drag corner to corner, preview outline), Fill, Eraser (the default ground); toolbar (Brush, Rect, Fill, Erase, Grid, Undo, Redo, Save, with hints); tile palette (every kind of tiles.json as an icon, its name on hover); grid (G); Ctrl+Z, Ctrl+Y, Ctrl+S; the cell under the pointer outlined; status line (tool, ground, cell, unsaved, last action) |

Solid kinds of tiles.json (stone, water, brick, lava) block walking in Game mode through `buildTileMap`.

### Tests
| Scenario | Test |
|---|---|
| Paint | `US-124 Paint` (a click; a jump of six cells in one tick paints all six in one stroke; eraser; the palette never paints the map; water east of the start blocks the hero in Game mode) |
| Fill | `US-124 Fill` (a dragged rectangle; a flood fill turns the whole pond and only the pond to sand; one step each, undone in one) |
| Undo | `US-124 Undo` (5 random sequences of 120 edits, undos and redos checked against the remembered levels; at most 100 steps) |
| Save | `US-124 Save` (Ctrl+S writes the file and keeps a backup; G toggles the grid); ctest `US-124 Paint in the game` (palette, map click and Ctrl+S by script in the real window) |

### Manual check results (2026-09-30)
[editor-painting.png](../evidence/US-124/editor-painting.png): a scripted session: dirt dragged across, stone painted, the palette hint "sand", the status line "Brush stone (25, 30) *unsaved paint stone (2 cells)".

---

<a id="us-125"></a>

## Plan US-125: Place characters

Codex v1.7, prompt S-US-125. Design: [M2c editor design](M2c-editor-design.md) section 5. Traces to D-19.

### What was built
| File | What |
|---|---|
| `src/game/editor_history.{h,cpp}` | `CharactersCommand`: the character list before and after, and the next free id (ids are never given twice, even after an undo) |
| `src/game/editor.{h,cpp}` | tools Place and Select; a character palette (every kind of characters.json, shown by its figure, its name on hover); place by click (facing south, the kind's HP and sword damage, a capitalised name, a new id, selected at once); select by clicking the figure (the top one), move by dragging (one step of Undo), R turns clockwise, Delete removes; a properties panel for the selected character (Name, HP, Sword); the selected figure framed in gold with its name; every edit undoable |
| `src/game/odyssey_game.{h,cpp}` | placed characters that are not enemies (hero, wanderer) stand in Game mode; placed enemies take sword hits, flash red and fall at 0 HP; in the Editor the mode label moves to the bottom right |
| `assets/levels/demo.json` | the original demo level, played by every test (D-20); the owner's edited `valley.json` stays the game's level |

### Tests
| Scenario | Test |
|---|---|
| Place | `US-125 Place` (a skeleton where clicked, facing south, with its kind's defaults and a new id; selected; Undo removes it and the id is not reused) |
| Edit | `US-125 Edit` (select by click; drag to move, one step; R turns; HP typed into the panel; name and sword damage; saved and read back; Delete and Undo) |
| Play | `US-125 Play` (a placed goblin with 12 HP takes 5 per swing, flashes and is defeated; a placed wanderer stands as a bystander); ctest `US-125 Place in the game` (Place, goblin, a click on the map, Ctrl+S, F1, Shift, strike, by script in the real window) |

### Manual check results (2026-09-30)
[editor-placing.png](../evidence/US-125/editor-placing.png): the character palette with the hint "orc", a placed goblin selected (gold frame and name), its properties panel, a bat placed below.

---

<a id="us-126"></a>

## Plan US-126: Level and character settings

Codex v1.7, prompt S-US-126. Design: [M2c editor design](M2c-editor-design.md) section 5. Traces to D-19.

### What was built
| File | What |
|---|---|
| `src/game/level.{h,cpp}` | `resized`: painted cells keep their places, new cells get the default ground, characters and targets outside are dropped, the hero start moves inside |
| `src/game/editor_history.h` | `LevelCommand` (the level before and after: resize, name, default ground, hero start) |
| `src/game/editor.{h,cpp}` | toolbar button Level; a settings panel (Name, Width, Height 8-256, default ground list, New, Open, Close); the START marker dragged with Select (one step of Undo); New (a 32 x 32 level saved as the first free `level-N.json`) and Open (the levels of the folder), which ask "Save the changes first?" (Save, Discard, Cancel) when there are unsaved changes; Undo never crosses into another level; dialogs dim the world and take the pointer |
| `src/luna/engine/ui.{h,cpp}` | `UiPainter::setScreen` / `keepOnScreen`: hover hints stay inside the screen |
| `src/game/odyssey_game.h` | the game plays the level the Editor has open |
| `docs/guides/editor.md` | the owner's guide: every control in plain words |

### Tests
| Scenario | Test |
|---|---|
| Settings | `US-126 Settings` (the width typed into the panel; resize keeps painted cells and drops what falls outside; name; default ground fills a larger level; saved and reloaded; five steps of Undo back to the start) |
| Hero start | `US-126 Hero start` (the marker dragged 64 px left and 32 down, one step; F1 starts the hero there) |
| Other levels | `US-126 Other levels` (New asks with unsaved changes: Cancel keeps, Discard makes level-1.json; Open; Save then open; the game plays the Editor's level) |
| Guide | `US-126 Guide` (every control named in docs/guides/editor.md) |
| Hints | `US-126 Hints stay on screen` |

### Manual check results (2026-09-30)
The guide was read against the running Editor, control by control. [level-settings.png](../evidence/US-126/level-settings.png), [unsaved-question.png](../evidence/US-126/unsaved-question.png).
