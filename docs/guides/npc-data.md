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

## Placed NPCs are persons (US-262)

When a level starts, every placed character becomes a **person** of the simulation (`sim::NpcPopulation`), except animals and monsters: a character is a creature when its kind is an animal, when its classes include `monster` or `animal`, or when it is an enemy kind with no class at all. Creatures keep fighting and grazing as before. The hero's own kind is never a person.

A person is kept in a compact store (one array per field; see ADR-022 in US-263) and has:

| What | How |
|---|---|
| `id` | the id of the placed character in the level, the same in the save |
| age | starts at 20 years plus a fixed spread by id (so the same level always starts the same people); +1 every game day |
| needs | hunger, energy, warmth, social, 0 to 100; each day they fall by the daily rate of `needs.json` (warmth faster in winter); a need that ends the day under 50 is restored (hunger 40, energy 40, warmth 30, social 20): the land, the fire and the neighbours provide, so nobody starves in the test level |
| family | a family id (0 = none); the Editor sets it in US-268 |
| memories | the last six: "an ordinary day", "a good day" or "a hard day" every night by how the needs are, and "met the hero" when the hero stands within 48 pixels (once a day) |

A game day is the `ticksPerDay` of `calendar.json` (2400 ticks, two minutes). Persons do not move yet (schedules come in M9c). They are saved with the autosave in `npcs.json` (versioned JSON, written to a temporary file and renamed, three backups) and read back with it; the same ticks always give the same persons (the population has its own hash).

## Crowds: the store, the grid and detail by distance (US-263, ADR-022)

Up to 100,000 persons fit. Nothing in the game loops over all of them in a frame: the persons near the hero (within 800 pixels, 25 tiles) are simulated hour by hour, found through a grid of 256-pixel cells; everyone else is brought up to date when the game day ends. A person is the same at the end of a day whether they were near all day, far all day, or walked across the border. `NpcPopulation::near(x, y, radius)` answers "who is near" without walking the store. The numbers and the budgets are in ADR-022.

## Attitudes and opinions (US-264): `assets/data/sim/opinions.json`

Every person has an **opinion**, a whole number from -100 to 100, of the hero and of every person they have met. The **attitude word** follows from it. There are no factions: each pair has its own number. Only pairs that have **met** are stored: an entry appears the first time something happens between two (a gift, a talk, a trade); until then the opinion is what it would be by itself (the starting attitude for the hero, the same-family opinion for a person of the same family, else 0). Reading an opinion never creates an entry.

| Word | Opinion | Notes |
|---|---|---|
| hostile | -100 to -60 | fights the hero when it has a kind file (below) |
| wary | -59 to -30 | |
| suspicious | -29 to -10 | |
| neutral | -9 to 9 | |
| friendly | 10 to 39 | |
| enchanted | 40 to 69 | |
| lovingly | 70 to 100 | |
| scared, enviously | any | a **mood**: fear and envy win over the number until something clears them |

`opinions.json` (every part is optional and has a default; a mistake names file and field):

| Field | Meaning |
|---|---|
| `bands` | where each band begins (`hostile` must be -100, each next one higher) |
| `start` | the opinion of the hero a given starting attitude means (`wary`: -45, `friendly`: 25...); `scared` and `enviously` also start the mood |
| `events` | what each event is worth, by name: `gift`, `trade`, `help`, `marriage-in-family`, `insult`, `theft`; add your own names |
| `sameFamily` | the opinion two persons of the same family start with of each other |
| `talk` | the worth of a conversation by quality (`awful`, `bad`, `plain`, `good`, `great`) and `frequencyBonus` / `frequencyDays`: talking again within the days adds the bonus |

```json
{ "version": 1,
  "events": { "gift": 15, "trade": 5, "help": 20, "marriage-in-family": 10, "insult": -20, "theft": -30 },
  "sameFamily": 20,
  "talk": { "awful": -8, "bad": -4, "plain": 0, "good": 3, "great": 6, "frequencyBonus": 1, "frequencyDays": 3 } }
```

**Starting attitude.** The `attitude` of the kind file, or the one a placed NPC sets, is how the NPC starts to think of the hero. **Family.** A placed NPC may set `"family": 4`; persons with the same non-zero family id start with the same-family opinion of each other. Both are saved in the level only when set.

**Who fights.** An NPC whose kind has a kind file fights the hero when its attitude is `hostile`: the old `enemy` switch of `characters.json` and `animals.json` only decides for kinds without a kind file. So a goblin set to `friendly` stands by, and a wanderer set to `hostile` fights. **Menu title.** The right-click menu of such an NPC shows the word: `Grub (hostile)`.

The save (`npcs.json`, version 3) holds the starting attitude of each person and the opinions of the pairs that met.

## Talking to a placed NPC (US-265)

Right-click a placed person (a person of the level, not a creature): the menu title is its name and attitude word, `Ossa (neutral)`. **Talk** is offered when the NPC has a **dialogue for the player** and the hero is within 2 m; with no dialogue there is no Talk (D-52 Q-11). The dialogue is the `player` entry of the layers (the NPC itself, then its kind file, then its classes) and names a script of `assets/data/dialogue/` by its file name: `"player": "npc-trader.dlg"` is the script `npc-trader`. A name that has no script is no dialogue. The conversation panel pauses the game, and its title shows the attitude word. A placed person needs no run of the hero for this: it works in any level. Clan members keep their own talk (their scripts are chosen by `@who` as before).

A placed NPC carries the tags of its classes, its kind and itself, plus `npc` and, when it has a dialogue for the player, `speaks`. `talk.json` targets the tag `speaks`, which clan members carry too. Give a script an `@who` that nobody has (`@who npc-trader`) so no clan member is ever given it.

In the script of a placed NPC: `opinion(npc, hero)` is what it thinks of the hero (-100 to 100), `mood(npc)` is its attitude word, `{opinion npc hero 5}` changes it and `{remember npc "{hero} was kind" 20}` gives it a memory (see the dialogue format guide). Other `opinion` pairs read 0 for a placed person.

## Confront (US-266)

**Confront** is its own menu for every NPC, hostile ones included: the key **C** (the NPC under the pointer, else the nearest within 6 m) or **Confront...** in the right-click menu. It lists the interactions with `"menu": "confront"`, and only those; the ordinary menu (Talk and the rest) never lists them. Five ship, in `assets/data/interactions/`:

| Action | Effects of its file | Notes |
|---|---|---|
| `taunt` | opinion of the hero -10, friends who hear it -2, a memory | a jeer |
| `insult` | -20, friends -5, a memory | |
| `ask-for-peace` | +5, friends +1, a memory | |
| `antagonise` | -30, friends -8, a memory, `do provoke` | starts a fight |
| `de-escalate` | `do calm 15 70`, friends +2 | 70 in 100: stops an attack, +15 |

Every amount is in the file: change it and press **F5**. `opinion npc hero -20` changes what the target thinks of the hero; `do spread-opinion -5` makes everyone within `hearingTiles` (12, in `opinions.json`) who **knows** the target (they have met it or are of its family) think `-5` of the hero too. A creature with a kind file (a goblin, a deer) is an NPC too and keeps an opinion of the hero from its starting attitude. Add your own confront action with a new file that says `"menu": "confront"` and targets `npc`. NPCs may confront each other later (US-292).

## Actions and the Actions pop-up (US-267)

Which actions an NPC has is worked out in layers (D-52 Q-08): the **tags** give the defaults (an interaction file targets tags: `"target": { "tags": ["trader"] }`), and the **allow and deny lists** (class, kind and the NPC itself, resolved as in "Precedence" above) fine-tune. An interaction in the resolved `deny` list is never offered for that NPC (a later deny wins). One in `allow` is offered although the target tags of its file do not match (the hero, the range and the `requires` still count): a plain wanderer that allows `trade` can trade.

**The right-click menu** of an NPC shows what the hero can do now. An action that cannot be done for a reason of the NPC's own (a `requires` that fails: the attitude, an item) is **hidden**; one that is only out of range stays, greyed out with "Too far away". The menu always ends with **Confront...** and **Actions...**.

**The Actions pop-up** (the key **X** for the NPC under the pointer or the nearest within 6 m, or **Actions...** in the right-click menu) lists every action the NPC has, the confront actions too. The ones that can be done now can be chosen; the others are greyed out with what they need, in plain words, which are the `else` text of their `requires`:

```json
"requires": [ { "if": "opinion(npc, hero) >= 10", "else": "needs: friendly or better" } ]
```

Write `opinion(npc, hero)` against the bands of the table above (friendly is 10, enchanted 40, lovingly 70, suspicious from -29, wary from -59). Denied actions are not the NPC's actions, so they are not in the pop-up.

## Editor: the NPC panel (US-268)

Select a placed character that has a kind file (the **Select** tool, a click on it): below the properties panel (name, HP, sword) the **NPC panel** opens:

| Part | What it does |
|---|---|
| **Classes** | a list of every class with `[x]` for the ones the NPC has; a click ticks or unticks one. Several are allowed. |
| **Attitude** | a click goes to the next of the nine words; a star means it differs from the kind. |
| **Family** | a number; persons with the same non-zero family start with the same-family opinion of each other. 0 = none. |
| **Talks with** / **Script** | one dialogue per partner type: a click on the button goes to the next partner type (the player, animals, the environment, and one `class:<id>` for every NPC class, from data so the list grows with your classes); the field names the `.dlg` script. An empty field is the default. |
| **Actions** | a tick for every interaction of the registry; click to deny or allow it for this NPC (a denied action is not offered). |
| **Reset to defaults** | forgets everything this NPC sets itself: it is what its kind says again. |

**Only differences are saved.** A value equal to what the NPC would inherit from its classes and its kind is not written to the level, so changing the kind file later still changes this NPC. Every change is one step of **Undo** (Ctrl+Z); a change that changes nothing is none. An NPC cannot be given *no* class at all: with an empty list it inherits the classes of its kind again (give it a class that does nothing, such as a new empty one, if you need that).

## Editor: the Kinds tab and the markers (US-269)

The **Class** panel has a second tab, **Kinds**. It lists every character and animal kind (not the hero) and edits `assets/data/npcs/<kind>.json` with the same rows as the NPC panel of one NPC: **classes** (click to tick), **Attitude** (click for the next word, after the last one comes `(none)`), **Tags**, **Talk** (`player=greet.dlg, class:guard=x.dlg`), **Allow**, **Deny**. **Save** checks the kind, writes its file at once and makes every placed NPC of that kind follow it, in the Editor and in play (F1), unless that NPC sets the value itself (a placed NPC always wins, see Precedence). A kind with no file shows `(no file)` and gets one when saved. A kind with no class ticked has no `classes` field. Fields are written in the order of the table above, so saving the same kind twice gives the same file; an unknown attitude, a bad Talk line or an unknown field is refused with the reason and the file is not touched.

**Markers.** In the Editor (never in play) every placed NPC that has at least one class with a file shows a marker just under its feet: a ring in the class `colour` and the class `icon` inside it. With several classes the ring is split in equal arcs, one per class in the NPC's order, starting at the top and going clockwise; the icon is the first class's. A kind or an NPC with no class has no marker.

Example: a goblin with `"classes": ["monster"]` shows a red ring and a skull; a trader who is also an elder shows a ring half gold, half blue, and a coin.
