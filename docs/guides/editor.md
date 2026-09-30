# The Level Editor

Project Odyssey has two modes. **Game mode** plays a level. **Editor mode** stops the world so you can change it: paint the ground, place characters, and set up the level. Everything you build is saved as a file and plays at once.

## Starting
| How | What happens |
|---|---|
| `odysseus.exe` | plays your level, `assets/levels/valley.json` |
| `odysseus.exe --editor` | opens it in the Editor |
| `odysseus.exe --level assets/levels/level-1.json` | plays (or, with `--editor`, edits) another level file |
| **F2** | from the game: open the Editor. The world stops. |
| **F1** | from the Editor: play. The level starts fresh from the hero's start, as you left it. |

The corner of the screen always says which mode you are in ("GAME  F2: EDIT" or "EDITOR  F1: PLAY").

## Looking around
- **W A S D** or the **arrow keys** move the view.
- Hold the **right mouse button** and drag to slide the map, like a map on a table.
- The white square under the pointer is the cell you will change. The bottom line shows the tool, what it uses, the cell (column, row), `*unsaved` when there are changes, and the last thing done.

## The toolbar (top left)
Rest the pointer on any button to see what it does.

| Button | What it does |
|---|---|
| **Brush** | Paint the chosen ground: click one cell, or hold and drag. One drag is one step of Undo. |
| **Rect** | Fill a rectangle: press on one corner, drag to the other, let go. |
| **Fill** | Click a cell: every touching cell of the same ground becomes the chosen ground (a whole pond, a whole meadow). |
| **Erase** | Paint the level's default ground (see Level). |
| **Place** | Put a character on the map (see Characters). |
| **Select** | Pick a character, or the hero's start, to move or change it. |
| **Level** | Open the level settings (see Level settings). |
| **Grid** | Show or hide the cell lines (also **G**). |
| **Undo** | Take back the last change (also **Ctrl+Z**). Up to 100 steps. |
| **Redo** | Do it again (also **Ctrl+Y**). |
| **Save** | Save the level (also **Ctrl+S**). The last three saves are kept as backups (`.bak1` to `.bak3`), so a mistake is never final. |

## Ground
With **Brush**, **Rect**, **Fill** or **Erase** chosen, the palette on the left shows every ground: grass, path, stone, water, dirt, mossy stone, sand, snow, ice, mud, wooden floor, planks, brick, cobblestone, desert and lava. Click one to paint with it; rest the pointer on it to see its name.

**Stone, water, brick and lava are solid**: in the game nobody walks through them, and spears stop at stone.

## Characters
1. Click **Place**. The palette shows the characters: two heroes (who stand as people in your world) and ten monsters (goblin, skeleton, wolf, spider, slime, orc, troll, bat, plant monster, ghost).
2. Click one, then click the map. It stands there facing south, with its kind's HP and sword damage, and is selected at once.
3. Click **Select**, then click a character to select it (a gold frame and its name). Then:
   - drag it to move it;
   - press **R** to turn it (clockwise; monsters seen from the front look the same whichever way they face);
   - press **Delete** to remove it;
   - change its **Name**, **HP** and **Sword** damage in the panel on the right: click a field, type, press **Enter** (or click somewhere else).

In the game, take the sword (**Shift**) and strike (**E**, **Space** or **Enter**): monsters lose HP, flash red and fall at 0. A monster you hit strikes back: a red **!** appears over it, and half a second later its **Sword** damage hits you if you are still within 1.5 m, so step away in time. Your HP is shown top left; at 0 you start again at the START marker. The heroes you place just stand there for now.

## The hero's start
The hero begins where the gold **START** marker stands. With **Select**, drag the marker to move it. Press **F1** and the hero starts there.

## Level settings
Click **Level** to open the panel on the right:

| Setting | What it does |
|---|---|
| **Name** | The level's name, shown in the game's log. |
| **Width**, **Height** | The size in cells (8 to 256). Growing adds the default ground on the right and at the bottom; shrinking cuts from there. What you painted stays where it is; characters and targets that would fall outside are removed (Undo brings them back). |
| Ground list | The default ground: what **Erase** paints and what a larger level is filled with. |
| **New** | Start a new 32 x 32 level, saved as the next free `level-N.json` next to this one. |
| **Open** | Choose another level of the same folder. |
| **Close** | Close the panel. |

If you have unsaved changes, **New** and **Open** ask first: **Save** them, **Discard** them, or **Cancel**. Undo never crosses from one level into another.

## Good to know
- The tests play their own copy, `assets/levels/demo.json`; your `valley.json` is yours to change.
- A level is a readable JSON file: you can open it in any text editor.
