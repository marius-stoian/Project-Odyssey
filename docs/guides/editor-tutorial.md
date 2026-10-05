# Editor tutorial: every feature, one hands-on lesson at a time

This tutorial walks you through every feature of the Odysseus Editor by building one small practice level, **Tutorial Camp**. Each lesson takes 5 to 15 minutes and ends with a **Check** step, so you know it worked before you move on. Do the lessons in order: later ones use what earlier ones built.

The reference for every button and field is [editor.md](editor.md). Each lesson links to the deeper guide for its topic.

| # | Lesson | Features |
|---|---|---|
| 0 | [Before you start](#0-before-you-start) | build, launch, what is safe to change |
| 1 | [Two modes and moving around](#1-two-modes-and-moving-around) | F1/F2, panning, grid, status line |
| 2 | [A practice level](#2-a-practice-level) | Level: New, Name, size, default ground, Open |
| 3 | [Painting the ground](#3-painting-the-ground) | Brush, Rect, Fill, Erase, solid grounds, Undo, Redo, Save, backups |
| 4 | [The hero's start](#4-the-heros-start) | START marker |
| 5 | [Characters and a first fight](#5-characters-and-a-first-fight) | Place, Select, move, turn, delete, Name/HP/Sword |
| 6 | [Animals](#6-animals) | palette pages, enemy and harmless animals |
| 7 | [Weapons and the hotbar](#7-weapons-and-the-hotbar) | Arms, pickups, hotbar |
| 8 | [Plants and world objects](#8-plants-and-world-objects) | Plant palette, objects page, per-plant *Own* values |
| 9 | [Effects and weather](#9-effects-and-weather) | Fx, `--weather`, `--seed` |
| 10 | [Lights and the time of day](#10-lights-and-the-time-of-day) | Light, Sky preview |
| 11 | [Buildings and prefabs](#11-buildings-and-prefabs) | Build, building panel, Prefab tab |
| 12 | [NPC Classes](#12-npc-classes) | Class panel |
| 13 | [One NPC of your own](#13-one-npc-of-your-own) | NPC panel, markers, Reset to defaults |
| 14 | [Kinds: change a whole species](#14-kinds-change-a-whole-species) | Kinds tab |
| 15 | [Economy, places and a trader's day](#15-economy-places-and-a-traders-day) | Economy panel, Places, Trade, Schedule, Does, Defaults with |
| 16 | [Writing a conversation as a graph](#16-writing-a-conversation-as-a-graph) | Talk, cards, wires, check, Test-play, Save, NPC Script |
| 17 | [Writing an interaction as a graph](#17-writing-an-interaction-as-a-graph) | Rules, Actor, Verb, Target, Needs, Effects, Chronicle, NPC rule |
| 18 | [Editing data while the game runs](#18-editing-data-while-the-game-runs-f5) | F5 reload |
| 19 | [Final exercise and clean-up](#19-final-exercise-and-clean-up) | putting it together; removing tutorial files |

---

## 0. Before you start

**Build and launch.** From the repository root, in a terminal:

```bash
cmake --build --preset windows-x64-release
```

```bash
./build/windows-x64/bin/Release/odysseus.exe --editor
```

Always start the game from the repository root, so it finds `assets/`.

**What is safe to change.** Two kinds of change happen in the Editor, and it matters which is which:

| Change | Written when | Affects |
|---|---|---|
| The **level**: ground, characters, pickups, plants, effects, lights, buildings, NPC values, economy, places | only when you press **Save** (Ctrl+S) | that one level file. Three backups (`.bak1` to `.bak3`) are kept. |
| **Shared data**: NPC classes, kinds, prefabs, `.dlg` conversations, interaction files | **at once**, when you press the panel's own **Save** | every level and the game itself |

So this tutorial makes its own level for the first kind, and gives everything of the second kind a name starting with `tut-`, so you can find and remove it in lesson 19. Where a lesson edits an existing shared file, it tells you how to put it back.

---

## 1. Two modes and moving around

**Goal:** know where you are and how to move.

1. The game opened in **Editor mode**: the corner says `EDITOR  F1: PLAY`. The world stands still.
2. Move the view with **W A S D** or the **arrow keys**. Hold the **right mouse button** and drag to slide the map.
3. Press **G** (or the **#** button) to show the cell lines. Press it again to hide them.
4. Move the pointer over the map. The white square is the cell you would change. Read the **status line** at the bottom: the tool, what it uses, the cell (column, row), `*unsaved` when there are changes, and the last thing done.
5. Rest the pointer on any toolbar button: a tip says what it does.
6. Press **F1**: Game mode. Walk around with WASD. Press **F2**: back to the Editor, the world stops again.

**Check:** you can switch modes both ways, and the corner text changes each time.

---

## 2. A practice level

**Goal:** a level of your own, so nothing here touches `valley.json`.

1. Click **Level**. The settings panel opens on the right.
2. Click **New**. A new 32 x 32 level is made and saved as the next free `level-N.json` in `assets/levels/`. Note the number the status line shows.
3. In **Name**, type `Tutorial Camp` and press **Enter**.
4. Set **Width** to `40` and **Height** to `30` (Enter after each). The level grows to the right and down.
5. In the ground list, click **grass**: it is now the default ground (what Erase paints and what a bigger level is filled with).
6. Press **Ctrl+S**.
7. Click **Open** and pick another level, say `demo.json`. Then **Open** again and pick your `level-N.json` to come back.
8. **Try the unsaved-changes question:** paint one cell (any tool from lesson 3, or skip this step for now), then click **Open**. The Editor asks: **Save**, **Discard** or **Cancel**. Click **Cancel**.
9. Click **Close**.

**Check:** the status line shows no `*unsaved`, and the level's name is Tutorial Camp. From now on, open it directly with:

```bash
./build/windows-x64/bin/Release/odysseus.exe --editor --level assets/levels/level-N.json
```

(Replace `N` with your number.)

---

## 3. Painting the ground

**Goal:** a camp with a path, a pond and a stone wall. More detail: [editor.md, Ground](editor.md#ground).

1. **Brush:** click **Brush**, pick **path** in the palette on the left, then hold the left button and drag a winding path across the level. One drag is one step of Undo.
2. **Rect:** click **Rect**, pick **water**, press on one corner and drag to the other: a pond.
3. **Fill:** click **Fill**, pick **sand**, click any water cell of the pond. The whole pond turns to sand (every touching cell of the same ground).
4. **Undo and Redo:** press **Ctrl+Z**: the pond is water again. **Ctrl+Y**: sand again. **Ctrl+Z** once more to keep the water. Up to 100 steps are kept.
5. **Erase:** click **Erase** and drag over part of your path: it becomes the default ground (grass, from lesson 2).
6. **Solid grounds:** with **Rect**, put a short row of **stone** near the pond. Stone, water, brick and lava block walking; spears stop at stone.
7. Press **Ctrl+S**.
8. Press **F1** and walk into the stone and the pond: you cannot pass. **F2** to return.

**Check:** in `assets/levels/` you see `level-N.json.bak1` next to your level. Each save keeps the last three versions, so a mistake is never final.

---

## 4. The hero's start

1. Click **Select**.
2. Find the gold **START** marker and drag it next to the path.
3. Press **F1**: the hero begins there. **F2** to return.

**Check:** the hero appears exactly where you left the marker. Save.

---

## 5. Characters and a first fight

**Goal:** place, change and fight a monster. More: [editor.md, Characters](editor.md#characters).

1. Click **Place**. The palette shows two heroes and ten monsters.
2. Click **goblin**, then click the map a few cells from START. It stands facing south and is selected at once (gold frame and its name).
3. Click **Select**, then click the goblin:
   - drag it to another cell;
   - press **R** to turn it;
   - in the panel on the right, set **Name** to `Grub`, **HP** to `20`, **Sword** to `3` (Enter after each).
4. Place a second goblin and press **Delete** to remove it.
5. Lesson 7 gives you a weapon. For now, press **F1** and walk up to Grub: a red **!** means it is about to strike, half a second later. Step away in time. Your HP is top left; at 0 you start again at START.

**Check:** Grub has your name and values after F2, F1, F2. Save.

---

## 6. Animals

1. With **Place**, use the **<** and **>** arrows under the palette. Page one is the twelve characters; the next pages are 50 animals.
2. Rest the pointer on an animal button: the tip gives its name and whether it is an **enemy**.
3. Place a **deer** (harmless) and a **grey wolf** (enemy) apart from each other.
4. Select the deer and change its **Facing**: west shows the picture turned around.

**Check:** in Game mode the wolf strikes back when hit, and every weapon passes through the deer (that comes in lesson 7). Save.

---

## 7. Weapons and the hotbar

More: [editor.md, Weapon pickups](editor.md#weapon-pickups-and-the-hotbar).

1. Click **Arms**. The palette shows 16 weapons by icon, then **Sp** (spear throw) and **Sw** (plain sword).
2. Place a sword and a spear near START.
3. With **Select**, drag one pickup elsewhere, then **Ctrl+Z** to put it back.
4. Save, then **F1**:
   - walk over each pickup: it goes into the **hotbar** (nine boxes, bottom centre);
   - press **1** or **2** to hold a slot, or **Shift** for the next filled slot;
   - attack with **E**, **Space** or **Enter**. Fight Grub and the wolf; try hitting the deer.
5. Press **F2** then **F1**: the level restarts and the pickups are back.

**Check:** you can defeat Grub. A monster at 0 HP falls.

---

## 8. Plants and world objects

More: [editor.md, Plants](editor.md#plants) and [World objects](editor.md#world-objects-us-155).

1. Click **Plant**. The palette shows 153 plants, 36 to a page; turn pages with **<** and **>** at its top.
2. Place a few flowers (you can walk over these) and one **blueberry bush** (bushes and trees block walking). Only one plant fits in a cell.
3. Go to the **last page** (`6/6 objects`): fire pit, knapping stone, food store, shelter, flint nodule, water source, sleeping furs. Place a **fire pit** and a **shelter** in the camp.
4. **Own values for one plant:** with **Select**, click your blueberry bush. Its panel has one field, **Own**. Type `gather.delay=60` and press Enter. This one bush now regrows 60 seconds after picking; other bushes keep the default. Try a mistake, `gather.speed=5`: the status line explains it and nothing changes.
5. Save, **F1**:
   - stand by a plant with empty hands and press **E** (or right-click it): its name and a line about it show;
   - hit an edible plant with a weapon: it bursts and heals you 10 HP, then regrows elsewhere 15 seconds later;
   - right-click the fire pit and the shelter to see their actions.

**Check:** the blueberry bush's **Own** field still says `gather.delay=60` after you reopen the level.

---

## 9. Effects and weather

1. Click **Fx**. Seventeen looping effects (fireflies, flame, portal, magic circle, whirlpool, mist...).
2. Place **fireflies** over the pond and a **flame** on the fire pit. **Select** moves (drag) or deletes them.
3. Save, **F1**: the effects loop.
4. Weather is not edited: it changes by itself in the game. To try one, close the game and launch:

```bash
./build/windows-x64/bin/Release/odysseus.exe --level assets/levels/level-N.json --weather "steady rain"
```

To get a different weather sequence, add `--seed 7` instead.

**Check:** the fireflies loop in Game mode; the Editor shows no weather (on purpose).

---

## 10. Lights and the time of day

More: [lighting.md, placed lights](lighting.md#lighting-quality-placed-lights-and-the-editor-preview-us-247).

1. Click **Light**. The palette lists the kinds in `assets/data/light/lights.json` (`campfire`, `torch`, ...).
2. Place a **campfire** light on the fire pit and two **torches** by the shelter. A small gold sun marks each one.
3. Click **Sky**. A slider appears right of the toolbar, with the time beside it. Drag it: left end is midnight, the middle is noon, the right end is midnight again. At night, your lights glow.
4. Click **Sky** again to switch the preview off. It is a view only: never saved, never an Undo step.

**Check:** at the slider's night positions the camp is lit only around your lights. Save.

---

## 11. Buildings and prefabs

More: [building-data.md](building-data.md).

### Placing a building

1. Click **Build**. The palette lists the kinds (Hut, Windbreak, Storage pit, Drying rack, Palisade) and, marked `*`, your prefabs.
2. Choose **Hut**. A ghost follows the pointer: red means it cannot stand there. Press **R** to turn it, then click: a finished hut.
3. With **Select**, click the hut. Its panel can:
   - turn it;
   - set the **interior** mode (follow the kind, `fade` or `map`) and the interior level (try `interior-hut`);
   - set its **owner** (a placed person);
   - make it a **blueprint** (unfinished; in the game you bring materials and build it);
   - delete it.
4. Make it a blueprint, save, **F1**, right-click the ghost: *Bring materials*, *Build* (repeat until done), *Cancel*. Press **B** to open the game's own Build menu.

### Making a prefab

5. Click **Prefab**. Pieces are on the left (the first row erases), the grid on the right.
6. Set the id to `tut-hut` and the label to `Tutorial hut`. Set a 3 x 3 size.
7. Pick **floor-straw** and click every cell. Pick **wall-wood** and click the outer cells; put a **door-wood** in one. Add **roof-thatch** on every cell. A right click erases the top piece of a cell.
8. Leave cost and seconds empty or `0` (they are then the sum of the pieces). Tick *buildable* and *known*.
9. Press **Save**. The file `assets/data/buildings/prefabs/tut-hut.json` is written at once.
10. Click **Build**: `* Tutorial hut` is in the palette. Place one.

**Check:** in Game mode, **B** lists *Tutorial hut*. Save the level.

---

## 12. NPC Classes

A **class** is a kind of person: trader, guard, elder. Classes are shared data. More: [npc-data.md, the Class panel](npc-data.md#editor-the-class-panel).

1. Click **Class**. The list of classes is at the top; under it, the form.
2. Click a shipped class, for example **trader**, and read its fields. Change nothing.
3. Click **New** and fill in:
   - **Id** `tut-ranger`, **Label** `Ranger`;
   - **Colour**: any colour you like; **Icon**: click until you like it;
   - **Tags** `ranger`;
   - leave **Talk**, **Allow** and **Deny** empty for now.
4. Press **Save**. `assets/data/npc-classes/tut-ranger.json` is written.
5. Try **Delete** on **trader**: it is refused, naming the placed NPCs that use it (in any level). Do not delete shipped classes.

**Check:** `tut-ranger` is in the list after you close and reopen the panel.

---

## 13. One NPC of your own

More: [npc-data.md, the NPC panel](npc-data.md#editor-the-npc-panel-us-268).

1. With **Place**, put down a **wanderer** (the second figure of page one; a character with a kind file is an NPC). Name it `Kira`.
2. With **Select**, click Kira. Under the properties, the **NPC panel** opens:
   - **Classes:** tick `tut-ranger`. A ring in your colour, with your icon, appears under Kira's feet. That is the **marker** (Editor only). Tick `hunter` as well: the ring splits in two arcs.
   - **Attitude:** click until it says `friendly`. A star means it differs from the kind.
   - **Family:** `1`. Persons with the same non-zero family start out fond of each other.
   - **Talks with / Script:** leave it for lesson 16.
   - **Actions:** untick one, for example `insult`. The hero can no longer do it to Kira.
3. Press **Ctrl+Z** a few times and **Ctrl+Y** back: each change is one step.
4. Place a second wanderer, change a few values, then press **Reset to defaults**: it becomes what its kind says again.

**Why only differences are saved:** a value equal to what Kira would inherit from her classes and kind is not written to the level. So if you change the kind later, Kira follows, except where she has her own value.

**Check:** save, **F1**, right-click Kira: the menu title shows her name and `(friendly)`, and the action you unticked is missing.

---

## 14. Kinds: change a whole species

A **kind** file (`assets/data/npcs/<kind>.json`) holds the defaults for every placed character of that kind, in every level.

1. Click **Class**, then the **Kinds** tab.
2. Pick **goblin**. Note its **Attitude** (`hostile`) before you touch anything.
3. Set **Attitude** to `neutral` and press **Save**.
4. **F1**: Grub no longer attacks you. **F2**.
5. **Put it back:** set **Attitude** to `hostile` and **Save** again. (If you lose track, restore the shipped file from the terminal: `git restore assets/data/npcs/goblin.json`.)

**Check:** Grub attacks you again in Game mode.

---

## 15. Economy, places and a trader's day

More: [npc-data.md, Economy](npc-data.md#editor-the-economy-panel), [Trade section](npc-data.md#editor-the-trade-section-us-284), [Schedule](npc-data.md#editor-the-schedule-form).

All lines below are plain text read when you press **Enter**. A mistake is explained in the status line and changes nothing. Each line is one step of Undo.

### The region's economy and places

1. **Level**, then **Economy...**:
   - **Money:** `shells=1`
   - **Prices:** `flint=4 berries=1`
   - **Goods:** `berries=5 flint=3`
   - **Places:** `market=10,8 pond=20,15/water` (tile column, row; tags after slashes). Use cells on your own map; `home` is built in and means where an NPC was placed.

### Kira the trader

2. Select Kira. The **Trade** section opens beside the NPC panel:
   - **Stock:** `flint=6 berries=4`
   - **Restock/day:** `flint=1`
   - **Picks/day:** `2`
   - **Weights:** `berries=3`
   - **Wants:** `fur berries`
   - **Rare:** `flint=friendly` (kept for people Kira likes)
3. **Day:** `06:00 work market; 12:00 eat home; 13:00 work pond` and **Night:** `21:00 sleep home`.
4. **Does:** `patrol` (actions Kira chooses on her own when idle on duty).
5. **Defaults with:** click until it says `animal`, then type `hunt` in the **Does** line under it: what Kira prefers to do when she meets an animal.
6. Try a mistake in **Day**: `6am work market`. Read the status line.

**Check:** save, **F1**, and watch Kira walk to the market in the morning (a game day is two minutes). Right-click her and trade: her stock and prices follow what you typed.

> Tip: the same Trade, Schedule, Does and Defaults lines exist in the **Class** panel and the **Kinds** tab. There they edit the draft, and **Save** writes the file for everyone of that class or kind.

---

## 16. Writing a conversation as a graph

More: [dialogue-format.md, the graph editor](dialogue-format.md#the-graph-editor-us-171-m9).

### Open and look

1. Click **Talk**. A full-screen canvas covers the map: the file list on the left, a toolbar on top, a side panel on the right, the check list underneath.
2. Click **npc-vell** in the list. **Frame** shows every card. Right-drag pans, the wheel zooms (25 to 400%).
3. Read the cards: blue **Node**, **Line**, **Choice**, red **If**, **Do**, **Note**, **Goto**. Close it without saving: **Esc** or **Close**.

### Make your own

4. In the **name:** field of the toolbar, type `tut-kira` and press **New**. You get a start node, a line and a way out. It exists only on screen until you save.
5. Click the **Line** card and, in the side panel, set the speaker and words: `Kira: The pond is quiet today.`
6. Click **+Choice**. Set its words to `Ask about flint`. Draw a wire from the previous card's yellow flow port to the choice's white flow port.
7. Click **+Node**, name it `flint`, and wire the choice's **to** port to it. Add a **+Line** after it: `Kira: I trade flint, if I like you.`
8. Add a **+If**, set it to `opinion(npc, hero) >= 10`, and wire it to the choice's **if** port. Give the choice an **else** reason in its side panel: `She does not know you well enough.`
9. Add a **+Do** with `opinion npc hero 5`, wired to the choice's **do** port.
10. Add a **+Note**, type `Kira only trades with friends.` and wire it to the choice. Notes are kept in the file as `#` lines.
11. Click on empty canvas: the side panel now edits the file's headers. Set **@who** to `npc-kira` so no clan member is given this script.
12. Press **Tidy** to put the cards in rows (one Undo step).

### Check it

13. Read the list under the canvas. `!` is an error, `?` a warning. Try one on purpose: delete the line after node `flint` so it is a **dead end**. The check shows an error; click it to select the card. **Ctrl+Z** to undo.

### Test-play it

14. Click **Test**. In the state field type `opinion=5` and press **Play**. The flint choice is greyed out with your else reason.
15. **Stop**, type `opinion=25`, **Play**. Take the choice: the log underneath shows `opinion of npc about hero +5`.
16. Select the `flint` node card and press **From here** to start in the middle. Nothing a test-play does is saved or added to Undo.

### Save and give it to Kira

17. Press **Save** (Ctrl+S). The game writes `assets/data/dialogue/tut-kira.dlg` and its layout `tut-kira.dlg.layout.json`, and reloads its data.
18. **Esc** back to the map. Select Kira. In **Talks with**, click until it says `player`; in **Script**, type `tut-kira.dlg` (or press **Pick** until it shows). The **Graph** button opens it again.

**Check:** save the level, **F1**, talk to Kira. Undo works across the graph and the map together: **Ctrl+Z** walks back through both.

---

## 17. Writing an interaction as a graph

An **interaction** is who may do what to what: "the hero may *wave* at a person". More: [interaction-data.md, the graph editor](interaction-data.md#the-graph-editor-us-172-m9).

1. Click **Rules**. The same canvas, now listing every interaction file. Open **gather** and read its cards: **Actor**, yellow **Verb**, **Target**, **Needs**, **Effects**, **NPC rule**, **Chronicle**.
2. In **name:** type `tut-wave` and press **New**: an actor, a verb, a target and one placeholder effect.
3. **Actor** card: `hero`.
4. **Verb** card: label `Wave`, range `5` (metres), duration `1` (seconds), note `A friendly hello from afar.`
5. **Target** card: tags `npc` (every placed person carries it).
6. **Effects** card: replace the placeholder with `opinion npc hero 2`. Add a second line with `remember npc "{hero} waved at me" 5`.
7. **+Needs**: if `mood(npc) != hostile`, else `They will not wave back`. Wire it to the verb's **requires** port.
8. **+Chronicle**: `{actor.name} waved hello`, wired to the **chronicle** port.
9. **+NPC** (look, don't add): this card holds a score and a cooldown, and only matters when the actors include clan members or NPCs. Open **patrol** to see one in use.
10. Read the check list, then **Save**. The verb id must stay `tut-wave` (the file's name).

**Check:** **Esc**, **F1**, right-click Kira: *Wave* is in the menu, and Kira's opinion of you rises. If a condition or effect is refused, Save names the mistake and writes nothing; the exact words are in [interaction-data.md, Effects](interaction-data.md#effects) and [Conditions](interaction-data.md#conditions-and-scores).

---

## 18. Editing data while the game runs (F5)

1. Keep the game running. Open `assets/data/dialogue/tut-kira.dlg` in any text editor (it is plain text).
2. Change one of Kira's lines and save the file.
3. Look at the game (Game or Editor mode): within a second a toast says **Reloaded interactions**; talk to Kira: the new line is live. (If you started the game with `--no-watch`, press **F5**.)
4. Now break it on purpose: delete a `=== ` node header and save. A red panel lists `file:line: message`, and the old data stays in use. Fix the file and save again: the panel closes.

**Check:** the game watches its data files and saves are read again by themselves; F5 reads everything again at once. Conversations, interaction files, quests, NPC classes, lights, plants, objects and characters reload live. Weapons, animals, effects, weather, tiles, buildings and the hero's data apply at the next start; the game tells you so when you save one.

## 19. Final exercise and clean-up

### Final exercise (30 minutes, no help)

Build a small market on Tutorial Camp:

1. A cobblestone square (Rect) with a palisade (Build) on one side.
2. A second trader with a class you make, `tut-merchant`, that sells `fur` and buys `berries`.
3. A schedule so the trader is at the square from 08:00 to 18:00 and home at night.
4. A two-choice conversation for the trader, test-played with `opinion=0` and `opinion=50` before you save it.
5. Two torches lit after dark (Sky preview to check).
6. Play it from dawn to night with **F1**.

### Clean-up

When you are done, remove what the tutorial added to the shared data (your level can stay or go):

| Remove | Made in |
|---|---|
| `assets/data/buildings/prefabs/tut-hut.json` | lesson 11 |
| `assets/data/npc-classes/tut-ranger.json` (and `tut-merchant.json`) | lessons 12, 19 |
| `assets/data/dialogue/tut-kira.dlg`, `.layout.json`, `.bak` | lesson 16 |
| `assets/data/interactions/tut-wave.json`, `.layout.json`, `.bak` | lesson 17 |
| `assets/levels/level-N.json` and its `.bak` files | lesson 2 (optional) |

Delete a class only after no placed NPC uses it (remove Kira first, or delete the level). To see anything else you changed in shared files:

```bash
git status --short assets/
```

---

## Keys at a glance

| Key | Where | Does |
|---|---|---|
| F1 / F2 | anywhere | play / edit |
| F5 | anywhere | read every data file again (all or nothing) |
| W A S D, arrows, right-drag | map | move the view |
| G | map | grid |
| R | map | turn the selected character or building, or the Build ghost |
| Delete | map, graph | remove the selection |
| Ctrl+Z / Ctrl+Y | map, graph | undo / redo (one shared history) |
| Ctrl+S | map, graph | save the level / save the open file |
| Esc | graph | back to the map |
| Wheel | graph | zoom |
| 1-9, Shift | game | hold a hotbar slot |
| E, Space, Enter | game | use the held weapon; with empty hands, look at a plant |
| B | game | Build menu |
| C | game | confront the NPC next to you |
| X | game | the Actions pop-up (why an action is greyed out) |
