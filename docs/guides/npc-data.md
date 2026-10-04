# NPC data: classes (US-260), kinds, placed NPCs

NPCs are described in layers (D-52). This guide grows with each M9a story; US-260 is the first layer.

## NPC Classes: `assets/data/npc-classes/<id>.json`

A class says what kind of person someone is. An NPC has one or more classes. One file per class; the file name is the class id. Create, edit and delete them in the Editor (the **Class** button) or by hand. A file with a mistake is skipped, named in the log as `npc-classes/healer.json:5: unknown icon "banana"`, and the other classes still load. **F5** reads the files again (all or nothing: with any mistake the good classes stay in use).

| Field | Values | Meaning |
|---|---|---|
| `id` | lower-case letters, digits, `-` (at most 32); must equal the file name | the name other files use |
| `label` | text | what the player sees ("Trader") |
| `colour` | `"#rrggbb"` | the ring round the marker of an NPC of this class |
| `icon` | one of the icon set below | the small picture in the ring |
| `tags` | list of names | tags every NPC of this class carries (interactions target tags) |
| `dialogues` | `{ partner: "file.dlg" }` | default dialogue per partner type: `player`, `animal`, `environment` or `class:<id>` |
| `actions.allow` | list of interaction ids | actions this class may do |
| `actions.deny` | list of interaction ids | actions this class may never do (a deny always wins) |

Fields are written in this order. Unknown fields are mistakes.

```json
{
  "id": "healer",
  "label": "Healer",
  "colour": "#3a9a4c",
  "icon": "cross",
  "tags": ["healer"],
  "dialogues": { "player": "healer-greet.dlg", "class:guard": "healer-guard.dlg" },
  "actions": { "allow": ["talk"], "deny": ["barter"] }
}
```

**Icon set** (24, built in): person, coin, crown, shield, sword, bow, heart, skull, star, flame, leaf, paw, book, cross, hammer, pick, flask, key, eye, moon, sun, drop, tooth, wing.

**Shipped classes** (the test cast): trader, talker, hunter, elder, guard, monster, animal.

## Classes of a placed NPC (level version 4)

A placed character in a level may list its classes; the field is written only when there are some:

```json
{ "id": 3, "kind": "goblin", "name": "Ossa", "classes": ["trader", "elder"] }
```

Older levels (versions 1 to 3) load without classes and are written as version 4 on the next save. A class name that has no file is kept in the level (the game warns); deleting a class in the Editor is refused while placed NPCs use it, and the message names them.

## Editor: the Class panel

**Class** opens a panel: the list of classes, then **Id** (only for a new class), **Label**, **Colour**, **Icon** (click for the next one), **Tags**, **Talk** (`player=greet.dlg, class:guard=x.dlg`), **Allow**, **Deny** (comma lists). **New** starts a blank class, **Save** checks and writes the file at once, **Delete** removes it (refused while NPCs use it). Picking a class for a placed NPC comes with the NPC panel (US-268).

## Kind files: `assets/data/npcs/<kind>.json` (US-261)

The defaults of every NPC of one kind (a name of `characters.json` or `animals.json`). The file name is the kind. Every field except `kind` is optional; fields are written in this order. A file with a mistake is skipped and named as `npcs/goblin.json:4: attitude must be one of ...`; **F5** reads the kind files again with the classes (all or nothing).

| Field | Values | Meaning |
|---|---|---|
| `kind` | the file name | which kind this is |
| `classes` | list of class ids | the classes of NPCs of this kind |
| `attitude` | friendly, neutral, wary, hostile, scared, suspicious, enchanted, lovingly, enviously | the starting attitude |
| `tags` | list of names | tags added to the classes' tags |
| `dialogues` | `{ partner: "file.dlg" }` | default dialogue per partner type (same as a class) |
| `actions` | `{ "allow": [...], "deny": [...] }` | interactions this kind may or may not do |

```json
{
  "kind": "wanderer",
  "classes": ["talker"],
  "attitude": "neutral"
}
```

Shipped: one file for every character kind except the hero and every animal. Enemies are `monster` and `hostile`, animals are `animal`, the wanderer is `talker` and `neutral`: the behaviour the game had before the kind files.

## A placed NPC: what it sets itself (level version 4)

A placed character may set any of the same fields in the level file; only what is set is written, so a level made before US-261 loads and saves unchanged:

```json
{ "id": 3, "kind": "wanderer", "name": "Ossa", "classes": ["trader", "elder"], "attitude": "friendly",
  "tags": ["market"], "dialogues": { "player": "ossa.dlg" }, "actions": { "deny": ["barter"] } }
```

## Precedence

Layers, lowest first: the classes the NPC has (in order), its kind file, the placed NPC itself.

- **Classes:** the placed NPC's list if it has one, else the kind's. **Attitude:** placed, else kind, else `neutral`.
- **Tags:** all layers added together.
- **Dialogues:** per partner type; a higher layer replaces the lower one for that partner type.
- **Allow and deny:** layer by layer, allow first and then deny; a later layer overrides an earlier one, and inside one layer a deny beats an allow. So a trader class that allows `barter` and a placed NPC that denies it: barter is denied for that NPC only. An action no layer mentions is left to the interaction's own rules.
