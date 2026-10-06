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

## Notes for the C++ learner
`src/sim/data_document.cpp` is the document and its undo (a stack of whole copies: simple, and the files are small). `src/sim/json_patch.cpp` is the patcher: a small parser that maps where every value stands in the text, then a recursive function that copies unchanged text and rewrites only what differs. `src/sim/data_form.cpp` turns a document and a schema into rows; `src/game/data_editor.cpp` turns rows into Luna widgets. The three layers know nothing of each other's insides, so the first two run in tests without a screen.
