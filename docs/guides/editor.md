# The Level Editor

Project Odyssey has two modes. **Game mode** plays a level. **Editor mode** stops the world so you can paint the ground, place characters and set up the level. Everything you build is saved as a file and plays at once.

## Starting
| How | What happens |
|---|---|
| `odysseus.exe` | plays your level, `assets/levels/valley.json` |
| `odysseus.exe --editor` | opens it in the Editor |
| `odysseus.exe --level assets/levels/level-1.json` | plays (or, with `--editor`, edits) another level file |
| **F2** | from the game: open the Editor. The world stops. |
| **F1** | from the Editor: play. The level starts fresh from the hero's start, as you left it. |

The corner of the screen always shows the mode ("GAME  F2: EDIT" or "EDITOR  F1: PLAY").

## Looking around
- **W A S D** or the **arrow keys** move the view.
- Hold the **right mouse button** and drag to slide the map.
- The white square under the pointer is the cell you will change. The bottom line shows the tool, what it uses, the cell (column, row), `*unsaved` when there are changes, and the last thing done.

## The toolbar (top left)
Rest the pointer on any button to see what it does.

| Button | What it does |
|---|---|
| **Brush** | Paint the chosen ground: click one cell, or hold and drag. One drag is one Undo step. |
| **Rect** | Fill a rectangle: press on one corner, drag to the other, let go. |
| **Fill** | Click a cell: every touching cell of the same ground becomes the chosen ground (a whole pond, a whole meadow). |
| **Erase** | Paint the level's default ground (see Level). |
| **Place** | Put a character on the map (see Characters). |
| **Arms** | Put a weapon pickup on the map (see Weapon pickups). |
| **Plant** | Put a plant on the map (see Plants). |
| **Light** | Put a light on the map (see Lights and the time of day). |
| **Select** | Pick a character, pickup, plant, effect, light or the hero's start, to move or change it. |
| **Data** | Open any data file of the game (weapons, plants, needs, rules...) as forms built from its schema; see [data-editor.md](data-editor.md). **Esc** comes back. |
| **Level** | Open the level settings (see Level settings). |
| **#** | The Grid button: show or hide the cell lines (also **G**). |
| **Fx** | Put a looping effect on the map (see Effects and weather). |
| **Sky** | Switch the time-of-day preview on or off (see Lights and the time of day). |
| **Undo** | Take back the last change (also **Ctrl+Z**). Up to 100 steps. |
| **Redo** | Do it again (also **Ctrl+Y**). |
| **Save** | Save the level (also **Ctrl+S**). The last three saves are kept as backups (`.bak1` to `.bak3`), so a mistake is never final. |

## Ground
With **Brush**, **Rect**, **Fill** or **Erase** chosen, the palette on the left shows every ground: grass, path, stone, water, dirt, mossy stone, sand, snow, ice, mud, wooden floor, planks, brick, cobblestone, desert and lava. Click one to paint with it; rest the pointer on it for its name.

**Stone, water, brick and lava are solid**: nobody walks through them in the game, and spears stop at stone.

## Characters
1. Click **Place**. The palette shows two heroes (who stand as people in your world) and ten monsters (goblin, skeleton, wolf, spider, slime, orc, troll, bat, plant monster, ghost).
2. Click one, then click the map. It stands there facing south, with its kind's HP and sword damage, and is selected at once.
3. Click **Select**, then click a character to select it (gold frame and name). Then:
   - drag it to move it;
   - press **R** to turn it clockwise (monsters seen from the front look the same whichever way they face);
   - press **Delete** to remove it;
   - change its **Name**, **HP** and **Sword** damage in the right panel: click a field, type, press **Enter** (or click elsewhere).

In the game, pick up the sword (see Weapon pickups), hold it (**Shift** or **1-9**) and strike (**E**, **Space** or **Enter**): monsters lose HP, flash red and fall at 0. A monster you hit strikes back: a red **!** appears over it, and half a second later its **Sword** damage hits you if you are still within 1.5 m, so step away in time. Your HP is shown top left; at 0 you start again at the START marker. Placed heroes just stand there for now.

## Weapon pickups and the hotbar
1. Click **Arms**. The palette shows the 16 starter weapons by icon, then **Sp** (the old spear throw) and **Sw** (the old plain sword slash).
2. Click one, then click the map. The weapon lies there with a shadow and is selected at once.
3. With **Select** you can click a pickup, drag it to move it, and press **Delete** to remove it. **Ctrl+Z** and **Ctrl+Y** undo and redo all of this, like characters.
4. Save with **Ctrl+S**. The level file is now version 3 and lists the pickups (and plants, effects and lights); older (version 1 and 2) files still open, without what they lack.

In the game the hero starts with empty hands. Walk over a pickup and its weapon goes into the first free slot of the **hotbar** (nine boxes, bottom centre); the pickup is gone until the level restarts (F2, then F1). Keys **1** to **9** hold that slot; **Shift** holds the next filled one. With all nine slots full a pickup stays where it is and "Hotbar full" flashes. **E**, **Space** or **Enter** attack with the held weapon. Your valley has no pickups until you place some; `demo.json` has the old spear and sword by the hero.

## Plants
1. Click **Plant**. The palette shows the 153 plants by picture, 36 to a page; the arrows **<** and **>** at its top turn the pages (the page number is between them). Flowers, grasses and mushrooms are small and walkable; bushes and trees are big and **block walking** like a rock (and stop shots).
2. Click a plant in the palette, then a cell of the map: the plant grows there, with its feet in the middle of the cell's bottom edge. Only one plant grows per cell.
3. With **Select** you can click a plant, drag it to another cell and press **Delete** to remove it. **Ctrl+Z** and **Ctrl+Y** undo and redo all of it.
4. In the game: walk next to a plant and press **E**, **Space** or **Enter** with empty hands (or the **right mouse button** at any time) and its name and a line about it show for three seconds. Hit it with any weapon and it is destroyed with a burst of leaves; an **edible** plant heals you 10 HP. Fifteen seconds later the same plant grows back at a random free spot inside the picture.
## World objects (US-155)
The **last page** of the Plant palette (its number says `6/6 objects`) holds the world objects: fire pit, knapping stone, food store, shelter, flint nodule, water source and sleeping furs. Place, select, move, delete and undo them exactly like plants; they are saved in the level with the plants. In the game you right-click them to use them (see `docs/guides/interaction-data.md`: "World objects"). To add your own, add an entry to `assets/data/objects.json` and restart the game.
## Animals
The character palette has six pages (arrows **<** and **>** under it): the first is the twelve characters, the others the 50 animals. Place, select, move, turn (**R**), rename and delete them like any character; their **Facing** picks the side they look toward (west shows the picture turned around). Hovering a button tells the animal's name and whether it is an enemy.

In the game these 20 animals are **enemies**: grey wolf, fox, bear, boar, wild pig, cougar, lynx, leopard, jaguar, cheetah, lion, tiger, snow leopard, hyena, jackal, rhino, hippopotamus, buffalo, bull, water buffalo. They can be hit, strike back after a half-second warning when you are within 1.5 m, and fall at 0 HP. All the others (deer, cows, rabbits, ...) are harmless: they stand where you put them, cannot be hit, and every weapon passes them. The **Sword** number in the properties panel is an animal's strike damage.
## Effects and weather
1. Click **Fx**. The palette shows the 17 looping effects (fireflies, a flame, a portal, a magic circle, a whirlpool, dark mist, ...) by their first picture. Click one, then click the map to place it. **Select** moves it (drag) or removes it (**Delete**); **Ctrl+Z** and **Ctrl+Y** undo and redo. In the game it plays in a loop.
2. Weather is not edited: in the game a random weather fades in over 3 seconds every 60 to 120 seconds, and the sky is clear about one time in three. It is only for the eyes, and the same level plays under the same weathers every time. To try a weather: `odysseus.exe --weather "steady rain"`; to choose another sequence: `--seed 7`. The Editor shows no weather.
## Lights and the time of day
1. Click **Light**. The palette lists the kinds of light in `assets/data/light/lights.json` (`campfire`, `torch`, and any you add). Click a kind, then click the map: a small gold sun marks the light. **Select** moves it (drag) or removes it (**Delete**); **Ctrl+Z** and **Ctrl+Y** undo and redo. The kind decides colour, reach and strength; to change them edit `lights.json` (see the lighting guide).
2. Placed lights are saved in the level (level version 3: a `lights` list of `{id, kind, x, y}`). Older levels (versions 1 and 2) open without lights and are written as version 3 the next time you save.
3. In the game a placed light shines after dark like a camp fire, and adds nothing in daylight.
4. **Sky** switches the **time-of-day preview** on: a slider appears right of the tool bar with the time beside it. Drag it and the level is lit as at that hour (left end midnight, middle noon, right end midnight), with its lights glowing at night. It is a view only: not saved and not an Undo step. Click **Sky** again to switch it off.

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

With unsaved changes, **New** and **Open** ask first: **Save**, **Discard**, or **Cancel**. Undo never crosses from one level into another.

## Good to know
- The tests play their own copy, `assets/levels/demo.json`; your `valley.json` is yours to change.
- A level is a readable JSON file: you can open it in any text editor.

## Tooltips and suggestions (US-300, M10b)

Rest the pointer on any field of the Level Editor panels, the Building editor, the Graph editor (dialogues, rules, quests) or the Story events list and, after 0.4 seconds, a small box says what the field is for, its range and an example. Move the pointer, or click the field, and it goes. Fields that are typed in (not buttons) all have one; a button keeps its own hint.

The words live in `assets/data/editor/help.json`, one entry per field. The id of a field is its panel and its label in lower case: **Sword** in the NPC panel is `npc.sword`, **says:** on a dialogue line card is `graph.line.says`. A label's hint in brackets is not part of the id (`who (elder or friend):` is `event.who`), and an indented label (a field of the row above it) starts with `sub-` (`event.sub-says`). Panels: `level`, `economy`, `npc`, `plant`, `npc-life` (the Trade, Day, Night and Does rows of a placed NPC), `class`, `kind`, `building`, `prefab`, `graph.bar`, `graph.header`, `graph.<card type>`, `graph.test` and `event`.

```jsonc
{ "version": 1,
  "fields": {
    "npc.sword": { "purpose": "Damage of one sword strike by this character", "example": "4", "suggest": "number" },
    "class.tags": { "purpose": "Tags carried by NPCs of this class", "range": "words separated by commas", "example": "trader, elder", "suggest": "catalog:tags", "list": true }
  } }
```

`purpose`, `example` and `suggest` are required; `range` is optional (a number field's range is made from its own minimum and maximum). `suggest` says where the field's list of values comes from (see Suggestions below). You may write `//` comments. Edit the file in a text editor; the Editor reads it when the game starts.

If the file is missing or has a mistake, the Editor still opens, fields show no tooltip and the status line names the file and line (`help.json:4: ...`). A test checks that every field of the four editors has an entry and every entry has a field, so a new field without a line in `help.json` fails the build by name.


### Suggestions (US-301, US-302)

Click into a field that offers values and a list opens under it (above it near the bottom of the screen) with every value; type and it narrows to the values that start with what you typed, then the ones that contain it, in any letter case. **Up** and **Down** move the highlight, **Tab** takes the highlighted row, **Enter** takes it only after you moved the highlight (otherwise Enter keeps what you typed, as always), **Esc** closes the list and keeps your text, and a click on a row takes it. A list of words (commas or spaces) completes the word after the last separator and keeps the ones before it.

Where the values come from is the `suggest` of the field's entry in `help.json`. The list reads the data in use each time it opens, so a class or a file you saved a moment ago is offered at once.

| `suggest` | Offers | Example entry |
|---|---|---|
| `number` | the default of the character or class being edited (HP and Sword of the selected character: its kind's), the smallest, the largest, then the last five numbers you typed in that field this session | `"suggest": "number"` |
| `files:<folder>/<glob>` | the file names in a folder of `assets/data` that match `*`, `*.ext` or a whole name | `"suggest": "files:dialogue/*.dlg"` gives `elder-fire.dlg` for **Script** |
| `catalog:<name>` | the names of a catalog, listed below | `"suggest": "catalog:interactions", "list": true` for **Does** |
| `values:a\|b\|c` | a fixed list | `"suggest": "values:elder\|friend"` for the story event's **who** |
| `none` | nothing: the field works as before | `"suggest": "none"` |


## Live data (US-303, M10b)

The game reads its data files again while it runs, **all or nothing**: the new files are read into a copy, and only a clean copy replaces the data in use. A mistake keeps the last good data, lists `file:line: message` in a red panel at the top of the screen and a red toast ("Not reloaded: ..."), and the panel goes when the file is fixed and read again. A clean reload shows a two-second toast ("Reloaded lights"). **F5** reads every set; saving in the Editor (the graph editors, a class or a prefab) reads the set of the file it wrote.

| Set | Files | What follows |
|---|---|---|
| `interactions` | `interactions/`, `dialogue/`, `quests/` | the Editor's action lists and the graph editors' catalog |
| `npc-classes` | `npc-classes/`, `npcs/`, `sim/partner-types.json` | trade profiles, the partner defaults, schedules and the Class panel |
| `lights` | `light/lights.json` | placed lights and objects that shine, at once; the Light palette |
| `catalog` | `plants.json`, `objects.json`, `characters.json`, `weapons.json`, `animals.json`, `effects.json`, `weather.json` | placed plants take the new values of their kind (by name, keeping their ids and states); a new object is on the object page of the palette; the character palette; a weapon you hold, a shot in the air, the starter weapons and the weapon palette follow the new weapons by their names (a weapon that is gone takes its shots with it); the weather goes on under the same name (US-191) |
| `help` | `editor/help.json` | tooltips and suggestions |

A placed thing whose kind is gone (a plant or object deleted from the catalog, a light kind that left `lights.json`, a character kind that left `characters.json`) is **skipped by play**, a red **?** stands where it was placed (in the Editor and in play) and a warning names the level entry: `level "The Valley": plant #12 "oak" has no kind in plants.json or objects.json`. Nothing is deleted from the level; the thing comes back by itself when the kind comes back.

These files cannot be swapped while the game runs, because the play state holds them by number or in long-lived structures: `tiles.json` (a tile is a number in the map), `materials.json`, `buildings/`, `hero/`, `sim/`, `light/sky.json`, `light/celestial-events.json` and `story/`. A save or an outside change is **reported, never half-applied**: the toast says `<file> applies at the next start` and the new numbers are used when the game starts again. F5 leaves these alone. (`weapons.json`, `animals.json`, `effects.json` and `weather.json` were in this list until US-191.)

**Files saved outside the game (US-304).** The game also watches its data files: save one in a text editor and, within about a second, it is read again by itself, with no F5. It looks at the files a few at a time (every file about four times a second), waits until the file has been quiet for 0.3 seconds (an editor often writes a file twice), and then reloads the set that reads it, all or nothing like F5. What the game writes itself (an Editor save) is not read a second time. Temporary and backup files (`.tmp`, `.swp`, `.bak`, `.bak1`...) and hidden files are ignored.

The **open level** is watched too. If its file changes outside the Editor and you have nothing unsaved, the Editor reads it again (your view stays where it is) and a toast says `Reloaded <file>`. If you have unsaved changes, nothing is overwritten: the status line says the file changed on disk, and you choose (save to overwrite it, or open it again to take the new one). While you play, the new level waits and is read when you open the Editor; a run keeps going on the level it started with.

`odysseus.exe --no-watch` turns the watching off (the tests and the headless runs do not watch). F5 and the Editor's saves still reload.

**Level edits and run saves (US-305).** A run save remembers what the level looked like when it was saved: for every placed thing (plants and objects, characters and people, pickups, effects, lights, buildings of the level) its id and a short fingerprint (hash) of its entry in the level file. When the run is loaded, each id is compared with the level as it is now:

| In the level now | In the run |
|---|---|
| same fingerprint | the run keeps its state: a bush picked on the other side stays picked, a person keeps their age, needs and memories, a building keeps its damage |
| changed (moved, renamed, other class, other kind) or new | it comes fresh from the level, and what was waiting for it (a regrow timer, a trade stock) is dropped |
| gone (deleted in the Editor) | it is removed from the run |

The clan, the hero, the flags, the quests and what the clan built itself are not in the level, so the level never touches them. The status line says how many things the level updated: `Loaded clan.json: day 12. The level updated 2 things`. If you save the level in the Editor while a run is loaded and then press **F1**, the run is loaded again from its save with the same merge (it is not started over). A save made before this feature has no fingerprints: it loads as it always did, once, and the next autosave writes them. A generated region has no level file to edit and is not merged.
Catalogs: `npc-classes`, `npc-kinds` (characters and animals), `partner-types`, `interactions`, `interaction-fields` (`gather.delay`, `gather.duration`...), `light-kinds`, `objects`, `plants`, `characters`, `items`, `building-kinds`, `prefabs`, `quests`, `levels` (the files beside the open level), `tags`, `places` (of the open level) and `markers` (`tag:edible`, `npc:ossa`, `place:market`, `object:...` for a quest step's marker). `"list": true` on a field of words (**Tags**, **Allow**, **Deny**, **Does**, **Wants**) makes the list complete one word at a time. Every entry of `help.json` needs a `suggest` (write `none` for a field that offers nothing); an unknown source is a mistake of the file and the test of the coverage names it.
## A conversation for a character, own values for a plant (US-173)

**Conversation.** Select a placed NPC: in its NPC panel the row *Talks with* names a partner type and the *Script* field the `.dlg` file used with that partner (type a name like `elder-fire.dlg`, or press **Pick** to go to the next conversation of the dialogue folder). **Graph** opens that conversation in the graph editor. Talking to the character in the game starts it. The pick is saved in the level (`dialogues`) as before; only a difference from its classes and kind is kept.

**Own values for one plant.** Select a placed plant with the Select tool: its panel has one field, *Own*, with values of an interaction changed for this plant only, as `gather.delay=60 gather.duration=2.5` (seconds). `delay` is the wait of every `after` effect of that interaction (for a berry bush, the regrow time: `gather.delay=60` makes this bush ripe again 60 s after it was picked, while other bushes keep their 15 s); `duration` is how long the job takes. A mistake (an interaction that does not exist, a field other than `delay` or `duration`, a value that is not a number of seconds) is reported in the status line and changes nothing. Each change is one Undo step. In the level file a plant carries `"overrides": [ { "interaction": "gather", "field": "delay", "value": 60 } ]`; a plant with none has no `overrides` key, so a level saved before this feature is saved exactly as it was. An unknown field or a value out of range stops the level from loading and names `plants[n].overrides[m].field` or `.value`.
