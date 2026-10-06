# The Data tab: every data file as a form (US-191, M11)

The **Data** button of the Editor opens any file of `assets/data/` as forms built from its schema (see [schemas.md](schemas.md)): no JSON to type, every field with its range and its help, a mistake refused with the reason. **Esc** comes back to the map.

## The screen
| Where | What |
|---|---|
| top bar | **Close** (Esc), **Save** (Ctrl+S), **Undo** (Ctrl+Z), **Redo** (Ctrl+Y), **New entry** |
| first list | the folders of `assets/data/` (`(top)` is the files at the top: weapons, plants, animals...) |
| second list | the files of the chosen folder |
| third list | the **entries** of the open file, with a **find** box above it. A catalog such as `weapons.json` has `(file)` (the settings that are not an entry: the launch speeds, the elements) and one entry per weapon. A file such as `sim/needs.json` is one entry. |
| the form | the fields of the chosen entry, one per row |
| bottom line | the open file and entry, `*unsaved` when there are changes, and the reason the last edit was refused (red) |

## The fields
| Field | How to use it |
|---|---|
| a number | click, type, **Enter**. A whole number or a decimal; one outside its range is refused with the range (`must be between 0 and 1000 (is 5000)`). |
| a word with fixed choices (`class`, `era`...) | click: the choices open as a list; pick one, or type and press **Enter** |
| `yes` / `no` | click it |
| a link (`light`, `output`, `piece`...) | click: the names of the catalog it points to are offered; typing narrows them |
| a list of words (`tags`, `tools`, `classes`) | one box, words separated by commas |
| a list whose entries hold commas (the sentences of small talk) | one box for each entry |
| a group (`dailyDecay`, `celestial`) | the heading folds and unfolds; a group the file leaves out says `(not set)`: **add** creates it |
| a list of objects (`layout` of a building) | a heading with **add**, an entry for each with **x** (remove) and **^ v** (move) |
| a map keyed by name (`materials`, `inputs` of a recipe) | a heading with **add**, an entry for each, whose name can be changed in its first box |
| a field the file leaves out | it is there, empty (or with its default); typing a value adds it to the file; **x** on a present optional field takes it out again |
| a member the schema does not know | shown as JSON text so nothing is hidden |

Rest the pointer on a field for half a second: its tooltip shows what it is for, its range and an example (the schema's `description` and `example`; the same text is in `help.json`'s coverage).

## Saving
**Ctrl+S** writes the file by *patching the text it was read from*: a changed value is replaced where it stands, a list that gained or lost entries is rewritten in the style of its neighbours, and everything else, the comments, the `note` fields, the spacing, stays byte for byte. So a change of one damage value changes one line of `weapons.json`, and your diff shows exactly that. The old file is kept as `.bak1` (three backups). After the write the game reads the file again (the sets that watch it, see Live data in [editor.md](editor.md)): a weapon you hold, the shots in the air, the palettes and the weather follow at once.

A file saved in another program while it is open here is read again if you have nothing unsaved; otherwise your changes are kept and the status line says the file changed.

## Undo
Every edit of the open file is a step: **Ctrl+Z** and **Ctrl+Y** walk back and forth, up to 200 steps. Opening another file or leaving the tab with unsaved changes asks first: **Esc** once shows the question, **Esc** again throws the changes away.

## New entries
**New entry** adds an entry to the list the file keeps its entries in (a new weapon in `weapons.json`, a new plant in `plants.json`): it gets the required fields with defaults and a name of its own (`new-entry`, `new-entry-2`...), and the form shows it. Rename it, fill it in, save.

## Copy, rename, delete (US-193)
These work on the chosen **entry** of a catalog (a weapon, a plant, an item, a building kind...). The top bar has **Copy**, **Rename**, **Delete** and **Interactions**.

| Button | What it does |
|---|---|
| **Copy** | Makes a new entry beside the chosen one with everything the same and a name of its own: `bush-copy` (or `iron sword copy` for a name with a space, `-2`, `-3` when taken). It is chosen at once; change it, then Save. An edit like any other: Ctrl+Z takes it back. |
| **Rename** | Opens a dialog: the places that use the entry listed (`hero/recipes.json:9: recipes[2].output`), and a box for the new name. **Rename** writes the new name in the entry itself and in every place that names it: the fields that link to it, the keys of the tables that name it (the items of a recipe, a building's cost, a trader's stock), the conditions, effects and objectives of rule files, quests and **dialogues** (as a whole word; `eat-berries` is another word and quoted prose is left alone), and the **levels**. All the files are written at once; if one cannot be written the ones already written are put back. Refused when the new name is taken, is not a name, or when the open file has unsaved changes (save or undo first: a rename writes files). **Esc** answers no. |
| **Delete** | Removes the entry from the open file. When something still uses it, the places are listed first and you confirm (**Delete anyway**) or press Esc. An entry nothing uses goes at once. It is an edit of the open file: nothing is written until Save, and Ctrl+Z brings it back. |
| **Interactions** | Opens the interaction graph on an interaction that targets the entry: one that lists its name in `target.kinds`, or whose `target.tags` the entry carries (the game's tags, the derived ones included). With none it opens the list. |

The tags and states of an entry are ordinary fields of its form (a list of words). A copied plant carries the same tags, so it offers the same interactions as soon as it is saved: it is in the catalog, in the Editor's plant palette and in the list of kinds a level may place.

What a rename does *not* touch: prose (what people say, journal text, help text, notes), saved runs (a run save keeps the old name: after renaming an item or a kind, start a new game; the level things of a loaded run follow the level, see Live data in editor.md), and anything that is not in the data folder or the levels folder.

## Notes for the C++ learner
`src/sim/data_document.cpp` is the document and its undo (a stack of whole copies: simple, and the files are small). `src/sim/json_patch.cpp` is the patcher: a small parser that maps where every value stands in the text, then a recursive function that copies unchanged text and rewrites only what differs. `src/sim/data_form.cpp` turns a document and a schema into rows; `src/game/data_editor.cpp` turns rows into Luna widgets. The three layers know nothing of each other's insides, so the first two run in tests without a screen. `src/sim/data_refs.cpp` finds the places a name is used (from the schemas' `ref` and `keyRef`, the levels' fixed shape and the dialogue files) and plans a rename without writing anything: you see the list first.
