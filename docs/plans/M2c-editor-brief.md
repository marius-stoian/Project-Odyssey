# Build Brief: M2c Level Editor (Game mode and Editor mode)

Written by Mraw (the Dominus council) for Anima, 2026-09-30. Phase 1 of the Amek workflow: **Anima turns this brief into Codex v1.7** (prompts for the stories below); Mraw then assembles them in order. Requirements v1.7 must add epic E12 and these stories first (the requirements document stays the source of truth for *what*).

## Goal
The owner can switch the game between **Game mode** and **Editor mode**. In the Editor he can place new characters in the world, change level and character settings, and paint ground tiles by hand, using the sprites he added to `assets/sprites/`. What he builds is saved and plays in Game mode.

## Owner decisions (2026-09-30)
| ID | Decision |
|---|---|
| D-05 | Art source: **own art, placeholder quality for now** (the sprites in `assets/sprites/`, made with an image generator). Before any public release the owner verifies the generator's licence terms. Decided. |
| D-19 | Editor scope v1: **settings = level and character properties** (level name, map size, default ground, hero start; per character: name, HP, facing, sword damage). Placed characters **stand still with properties** (behaviours later). The work is a **new milestone M2c before M3**. Decided. |

## The art we have (survey)
| File | What it holds | Use |
|---|---|---|
| `Pixel RPG Heroes, Tiles, and Monsters.png` (1536x1024) | 2 heroes, 8 directions, 3 frames each; 11 ground tiles; 10 monsters (front) | main source: heroes, ground, monsters |
| `Retro RPG Heroes, Terrain & Monsters Sheet.png` (1536x1024) | 20 heroes and 20 monsters (front view); 22 ground tiles | more characters and tiles |
| `Fantasy RPG Terrain Tileset Grid.png` (1254x1254) | 16 large terrain tiles (grass, dirt, sand, rock, stone, wood, snow, ice, swamp, cracked, lava, water, rune) | higher-detail tiles |
| `image-gen-1(1)`, `image-gen-2(1)` (1536x1024) | a blue-scarf hero, 4 rows x 6 columns of walk frames on white | second hero animation |
| `image-gen-1(2)`, `image-gen-2(2)`, `image-gen-3`, `image-gen-4` (1672x941) | full landscapes (for example a snowy mountain valley) | backgrounds, title screen; not tiles |

These are **raster sheets with labels, dark or white backgrounds, no fixed cell grid, and about 60-125 px per character or tile**, not game-ready atlases. The game's sizes are fixed by D-04 and D-16 (characters 32x48, tiles 32x32 = 1 m). So the sheets must be **cut, cleaned (transparent background) and reduced** into atlases by a repeatable tool, with the cut rectangles kept as data.

## Architecture rules that apply (Charter)
- Rule 2: only the Platform layer touches the OS/SDL3, so **mouse events** are translated there. Rule 4: game code reads **intents**, so the mouse becomes a platform-agnostic "pointer" in the Engine (position, buttons, wheel), plus scripted pointer input for tests.
- Rule 9: Luna stays game-agnostic: the **UI toolkit** (font, panel, button, list, number field) goes in Luna Engine; the **Editor itself** is Game code (`src/game/editor/`), and **Level** data is Game data.
- Rule 7: content is data: tiles, characters and levels are JSON, validated with errors naming file and field. Levels save like ADR-010 (versioned, temp file then rename, 3 backups).
- Rule 6 / determinism: the simulation is untouched; the Editor pauses it; Game mode starts from the saved level.
- New library: **stb_image** (single header in `third_party/`, like FastNoiseLite) to read PNG files: needs **ADR-018**. Dear ImGui is *not* used for the Editor UI: it would pull SDL3 into game code; our small UI toolkit is enough for v1 (record this in the ADR).

## Stories (proposed IDs, epic E12 Level Editor, milestone M2c)
| ID | Story | Size | Depends on | Acceptance in one line |
|---|---|---|---|---|
| US-120 | Real art in the game | L | none | PNG loader; `tools/` cutter turns the sheets into atlases (`assets/sprites/atlas/`) from cut rectangles kept in JSON; hero, 12+ ground tiles and 10+ monsters drawn from the atlases instead of programmer art |
| US-121 | Pointer, font and UI toolkit | M | none | Mouse move, buttons and wheel reach the game as a pointer; scripted `--click`; a full ASCII bitmap font; panel, button, list and number-field widgets with tests |
| US-122 | Levels as data | M | US-120 | `tiles.json` and `characters.json` definitions; a versioned, validated `Level` file (name, size, ground grid, characters, hero start); the game loads a level instead of the built-in test map; today's demo enemy becomes a placed character |
| US-123 | Game mode and Editor mode | S | US-121, US-122 | F1 = Game, F2 = Editor (shown on screen); the Editor pans the camera freely and pauses the world; going back to Game starts from the level as edited |
| US-124 | Paint ground tiles | M | US-123 | Tile palette; click and drag to paint; rectangle fill and flood fill; eraser; grid overlay; undo and redo; Ctrl+S saves the level; solid tiles (water, lava, stone) block walking |
| US-125 | Place characters | M | US-123, US-124 | Character palette (heroes and monsters); place, select, move, delete, rotate to face; properties panel (name, HP, sword damage); in Game mode placed enemies take sword damage and are defeated |
| US-126 | Level and character settings | S | US-125 | Level name, map size (resize keeps the painted tiles), default ground, hero start (a movable marker); all saved and reloaded; documentation of the Editor for the owner |
| X-M2c | Exit review | - | all | Demonstration with screenshots by script (paint, place, save, reload, play); the owner is invited to try it, no kill gate |

Order: US-120 -> US-121 -> US-122 -> US-123 -> US-124 -> US-125 -> US-126 -> X-M2c. Each is testable headless where it is logic (Level model, JSON, undo and redo, brush and fill, atlas cutter output) and end-to-end in the window by scripted input and screenshots (the existing `--hold` and `--screenshot` options, plus `--click`).

## Definition of Done
The Charter's, plus: the Editor never breaks Game mode (the spear demo tests US-029 and the sword demo keep passing); zero warnings; every new data file has a load test that names file and field for each error; the owner-facing guide `docs/guides/editor.md` explains the controls in plain words.

## Risks and how the plan answers them
| Risk | Answer |
|---|---|
| The sheets are not clean atlases; cutting by hand each time is fragile | one cutter tool driven by a JSON of rectangles; output committed; the original sheets stay untouched |
| Reducing 300 px tiles to 32 px loses detail | keep D-16 (32 px); use a box filter; review screenshots; a later story can raise tile resolution if the owner wants |
| Generated art and public release | D-05 decided as placeholder; licence check recorded as a pre-release item, not a build blocker |
| An editor grows without end | scope is exactly the stories above; behaviours, paths, triggers, undo of character edits beyond one level of history are new stories |
| Existing demo code (hard-coded map, enemy, sword) | US-122 moves it into level data; the demo tests keep passing |

## Cross-discipline notes
- **Tech writer:** `docs/guides/editor.md` and a learning-journal entry per story (teach-back: parsing and validating JSON; the command pattern for undo and redo; a toolkit versus an application).
- **Tester:** the undo and redo stack and the flood fill are classic bug sources: tests with random edit sequences against a simple model.
- **UX:** a 480x270 virtual screen is small; the palette needs icon-only buttons with the name shown on hover, and the panels must not cover the map centre.
- **Marketing/lore:** the landscape images are strong candidates for the title screen later; nothing to do now.
