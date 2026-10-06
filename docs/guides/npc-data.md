# NPC data: classes (US-260), kinds, placed NPCs

NPCs are described in layers (D-52). This guide grows with each M9a story; US-260 is the first layer.

## NPC Classes: `assets/data/npc-classes/<id>.json`

A class says what kind of person someone is. An NPC has one or more classes. One file per class; the file name is the class id. Create, edit and delete classes in the Editor (the **Class** button) or by hand. A file with a mistake is skipped and named in the log as `npc-classes/healer.json:5: unknown icon "banana"`; the other classes still load. **F5** reads the files again (all or nothing: with any mistake the good classes stay in use).

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

Older levels (versions 1 to 3) load without classes and are written as version 4 on the next save. A class name with no file is kept in the level (the game warns); deleting a class in the Editor is refused while placed NPCs use it, and the message names them.

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
- **Allow and deny:** layer by layer, allow first and then deny; a later layer overrides an earlier one, and inside one layer a deny beats an allow. So with a trader class that allows `barter` and a placed NPC that denies it, barter is denied for that NPC only. An action no layer mentions is left to the interaction's own rules.

## Placed NPCs are persons (US-262)

When a level starts, every placed character becomes a **person** of the simulation (`sim::NpcPopulation`), except animals and monsters. A character is a creature when its kind is an animal, when its classes include `monster` or `animal`, or when it is an enemy kind with no class at all. Creatures keep fighting and grazing as before. The hero's own kind is never a person.

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

Up to 100,000 persons fit. Nothing in the game loops over all of them in a frame: persons near the hero (within 800 pixels, 25 tiles) are simulated hour by hour, found through a grid of 256-pixel cells; everyone else is brought up to date when the game day ends. A person is the same at the end of a day whether near all day, far all day, or walking across the border. `NpcPopulation::near(x, y, radius)` answers "who is near" without walking the store. The numbers and budgets are in ADR-022.

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

Right-click a placed person (a person of the level, not a creature): the menu title is its name and attitude word, `Ossa (neutral)`. **Talk** is offered when the NPC has a **dialogue for the player** and the hero is within 2 m; with no dialogue there is no Talk (D-52 Q-11). The dialogue is the `player` entry of the layers (the NPC itself, then its kind file, then its classes) and names a script of `assets/data/dialogue/` by file name: `"player": "npc-trader.dlg"` is the script `npc-trader`. A name with no script is no dialogue. The conversation panel pauses the game, and its title shows the attitude word. A placed person needs no run of the hero for this: it works in any level. Clan members keep their own talk (their scripts are chosen by `@who` as before).

A placed NPC carries the tags of its classes, its kind and itself, plus `npc` and, when it has a dialogue for the player, `speaks`. `talk.json` targets the tag `speaks`, which clan members carry too. Give a script an `@who` that nobody has (`@who npc-trader`) so no clan member is ever given it.

In the script of a placed NPC: `opinion(npc, hero)` is what it thinks of the hero (-100 to 100), `mood(npc)` is its attitude word, `{opinion npc hero 5}` changes it and `{remember npc "{hero} was kind" 20}` gives it a memory (see the dialogue format guide). Other `opinion` pairs read 0 for a placed person.

## Confront (US-266)

**Confront** is its own menu for every NPC, hostile ones included: the key **C** (the NPC under the pointer, else the nearest within 6 m) or **Confront...** in the right-click menu. It lists only the interactions with `"menu": "confront"`; the ordinary menu (Talk and the rest) never lists them. Five ship, in `assets/data/interactions/`:

| Action | Effects of its file | Notes |
|---|---|---|
| `taunt` | opinion of the hero -10, friends who hear it -2, a memory | a jeer |
| `insult` | -20, friends -5, a memory | |
| `ask-for-peace` | +5, friends +1, a memory | |
| `antagonise` | -30, friends -8, a memory, `do provoke` | starts a fight |
| `de-escalate` | `do calm 15 70`, friends +2 | 70 in 100: stops an attack, +15 |

Every amount is in the file: change it and press **F5**. `opinion npc hero -20` changes what the target thinks of the hero; `do spread-opinion -5` makes everyone within `hearingTiles` (12, in `opinions.json`) who **knows** the target (they have met it or are of its family) think `-5` of the hero too. A creature with a kind file (a goblin, a deer) is an NPC too and keeps an opinion of the hero from its starting attitude. Add your own confront action with a new file that says `"menu": "confront"` and targets `npc`. NPCs may confront each other later (US-292).

## Actions and the Actions pop-up (US-267)

An NPC's actions are worked out in layers (D-52 Q-08): **tags** give the defaults (an interaction file targets tags: `"target": { "tags": ["trader"] }`), and the **allow and deny lists** (class, kind and the NPC itself, resolved as in "Precedence") fine-tune. An interaction in the resolved `deny` list is never offered for that NPC (a later deny wins). One in `allow` is offered although the target tags of its file do not match (the hero, the range and the `requires` still count): a plain wanderer that allows `trade` can trade.

**The right-click menu** of an NPC shows what the hero can do now. An action that cannot be done for a reason of the NPC's own (a failing `requires`: the attitude, an item) is **hidden**; one only out of range stays, greyed out with "Too far away". The menu always ends with **Confront...** and **Actions...**.

**The Actions pop-up** (key **X** for the NPC under the pointer or the nearest within 6 m, or **Actions...** in the right-click menu) lists every action the NPC has, the confront actions too. Those that can be done now can be chosen; the others are greyed out with what they need, in plain words, which are the `else` text of their `requires`:

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

**Only differences are saved.** A value equal to what the NPC would inherit from its classes and kind is not written to the level, so changing the kind file later still changes this NPC. Every change is one **Undo** step (Ctrl+Z); a change that changes nothing is none. An NPC cannot be given *no* class at all: with an empty list it inherits the classes of its kind again (give it a class that does nothing, such as a new empty one, if you need that).

## Editor: the Kinds tab and the markers (US-269)

The **Class** panel has a second tab, **Kinds**. It lists every character and animal kind (not the hero) and edits `assets/data/npcs/<kind>.json` with the same rows as the NPC panel of one NPC: **classes** (click to tick), **Attitude** (click for the next word, after the last one comes `(none)`), **Tags**, **Talk** (`player=greet.dlg, class:guard=x.dlg`), **Allow**, **Deny**. **Save** checks the kind, writes its file at once and makes every placed NPC of that kind follow it, in the Editor and in play (F1), unless that NPC sets the value itself (a placed NPC always wins, see Precedence). A kind with no file shows `(no file)` and gets one when saved. A kind with no class ticked has no `classes` field. Fields are written in the order of the table above, so saving the same kind twice gives the same file; an unknown attitude, a bad Talk line or an unknown field is refused with the reason and the file is not touched.

**Markers.** In the Editor (never in play) every placed NPC with at least one class that has a file shows a marker just under its feet: a ring in the class `colour` with the class `icon` inside. With several classes the ring is split in equal arcs, one per class in the NPC's order, starting at the top and going clockwise; the icon is the first class's. A kind or an NPC with no class has no marker.

Example: a goblin with `"classes": ["monster"]` shows a red ring and a skull; a trader who is also an elder shows a ring half gold, half blue, and a coin.

## The test level and its walk-through (US-270)

`odysseus.exe --level assets/levels/npc-test.json` opens a 40 x 24 level with the seven NPCs of the D-52 cast, in a row along a path, and a stone wall at the east end. Each talking NPC has a script of its own in `assets/data/dialogue/` (`npc-<name>.dlg`, `@who npc-<name>`, so no clan member is ever given it). The hero starts at the west end. Walk east along the path.

| NPC | Kind, classes, attitude | Script | What to try |
|---|---|---|---|
| Tala | wanderer, `trader`, neutral | `npc-tala.dlg` | Right-click: the menu title says `Tala (neutral)`. Talk: praise her wares, her opinion goes up; press **X** to see the Actions pop-up explain what is greyed out. |
| Ossa | wanderer, `talker` (from the kind), neutral | `npc-ossa.dlg` | A plain wanderer: Talk and little else. |
| Harn | wanderer, `hunter`, **wary** | `npc-harn.dlg` | Actions that need friendly are greyed out with the reason. Talk kindly (+10), then mock him (-10); watch the attitude word in the title. |
| Vell | wanderer, `elder`, **friendly** | `npc-vell.dlg` | The written dialogue: her first lines change with what she thinks of you (below 10 she is guarded). |
| Gur | wanderer, `guard`, neutral | `npc-gur.dlg` | Press **C** next to him to confront; his opinion of you falls and nearby friends hear it. |
| Deer | kind `deer`, `animal`, neutral | none | No Talk (no dialogue for the player); it only grazes. |
| Goblin | kind `goblin`, `monster`, **hostile** | none | It attacks you. Set the `goblin` kind to neutral in the Editor (**Class**, **Kinds**) and press F1: it does not. |

In the Editor (F2) every NPC with a class has its ring and icon under its feet, and a click on one opens its NPC panel. Every shipped file of this level is checked by `tests/game/npc_test_level_test.cpp`: it loads with no mistake, and loading, saving and loading again gives the same text.

## Region economy: currencies, prices and goods (US-280, level version 5)

A level may carry an `economy` object. It is written only when something is set, so a level made before US-280 loads and saves unchanged apart from its version number (5). A level with no currency trades by **barter only** (US-283).

| Field | Values | Meaning |
|---|---|---|
| `currencies` | `{ item id: value }`, value 1 to 100000 | the items that are money in this region, each worth its value (in value units); any currency item is worth its value anywhere |
| `prices` | `{ item id: price }`, 1 to 100000 | the market's base price of a good; without an entry the item's own `value` (`assets/data/hero/items.json`) is the base |
| `resources` | `{ item id: weight }`, 1 to 1000 | goods the region delivers: added to every trader's own weights at the daily restock (US-281) |

```json
"economy": { "currencies": { "shells": 1 }, "prices": { "flint": 4 }, "resources": { "berries": 5, "flint": 3 } }
```

Item ids are lower-case letters, digits and `-`. Money is a whole number everywhere: there is no cent. The shipped item `shells` (kind `currency`, value 1) is the example currency; any item of `items.json` can be marked as money.

### Editor: the Economy panel

**Level** opens the level settings; its **Economy...** button opens the Economy panel with three lines, each `item=number item=number`:

| Line | Edits |
|---|---|
| **Money** | `currencies`, for example `shells=1 gold=10` |
| **Prices** | `prices`, for example `flint=4` |
| **Goods** | `resources`, for example `berries=5 flint=3` |

A line is read when you press Enter or click elsewhere. A mistake (a name that is not an item id, a number out of range, a missing `=`) is said in the status line and changes nothing. Every table is one **Undo** step; an empty line clears the table (no money: barter only); typing the same table again is no step.

## Trade: the `trade` block and the daily restock (US-281, level version 5)

Any NPC with a trade profile is a trader (D-54 Q7); the **Trader** class is only a default profile. The `trade` block can be written in a class file, a kind file and on a placed NPC. The layers merge: class, then kind, then the placed NPC; tables merge per key (the later layer wins), `wants` is the union, `deliveries` is the last one set. Every field is optional; fields are written in this order.

| Field | Values | Meaning |
|---|---|---|
| `stock` | `{ item: count }`, 0 to 9999 | the stock at the start, and the **target** the price curve measures against (US-282); `0` = the shelf starts empty |
| `restockPerDay` | `{ item: count }`, 0 to 999 | pieces delivered every day, as written |
| `deliveries` | 0 to 20 | weighted random picks a day; each pick adds `deliveryAmount` (1) piece |
| `weights` | `{ item: weight }`, 0 to 1000 | the picks are drawn from these weights plus the region's `economy.resources` (0 switches an inherited weight off) |
| `wants` | list of item ids | bought at full value; any other good at half (D-52 Q-17) |
| `rare` | `{ item: band word }` | goods offered only at an opinion at least as high as the word (US-282): hostile, wary, suspicious, neutral, friendly, enchanted, lovingly |

```json
"trade": { "stock": { "flint": 6, "fur": 2 }, "restockPerDay": { "flint": 2 }, "deliveries": 1, "weights": { "fur": 3 }, "wants": ["berries"], "rare": { "obsidian": "friendly" } }
```

**Limited stock.** A restock never brings a good above its cap: `max(starting stock x capFactor, minimumCap)` (2 and 10 in `assets/data/sim/trade.json`). A good the trader does not stock has a cap of 10. A trade may leave a trader above its cap (it keeps what you sell it); only deliveries stop at the cap.

**Daily restock.** When a new in-game day begins, every trader gets its fixed `restockPerDay` pieces and then `deliveries` weighted random picks. The random stream depends only on the world seed (the level name, or `--seed`), the day and the trader, so the same day always brings the same goods. A trader far from the hero is restocked in the same one pass (the cost is the number of traders); after a long absence at most 7 days of deliveries arrive at once (`catchUpDays`).

**Saved.** The stock, price drift and haggle day of every trader are in `trade.json` next to `npcs.json` (versioned JSON). The profiles are data and are read again at every start; the saved stock is then put back. F5 reads class and kind files again and gives every trader its new profile without touching its stock.

A class whose file has a mistake in `trade` is skipped like any class file with a mistake (`npc-classes/trader.json:7: trade.stock.Flint ...`). A level with a mistake in a placed NPC's `trade` is refused with the file, the character (`characters[0]`) and the problem.

## Trade: prices and reputation (US-282, ADR-023)

A price is the region's base price for the good, times the **stock curve**, times the **drift**, with the trader's **attitude** to the hero applied last. All whole numbers, in thousandths of a value unit; the formula and reasons are in `docs/adr/ADR-023-trade-prices.md`. Every number is in `assets/data/sim/trade.json`:

| Section | Fields | Meaning |
|---|---|---|
| `stock` | `minimumCap`, `capFactor`, `defaultTarget`, `deliveryAmount`, `catchUpDays` | restock caps and the target of a good the trader does not stock (US-281) |
| `curve` | `minPercent`, `maxPercent` | the stock curve is `100 x target / stock`, clamped to these (50 and 200) |
| `drift` | `percentPerTrade`, `maxPercent`, `decayPercentPerDay` | each piece bought or sold nudges the price (3), up to a cap (40); it decays back daily (a quarter of itself) |
| `reputation` | `percent` (by attitude word), `refuse` (list of words) | the percent added to what the hero pays: friendly -10, neutral 0, wary and suspicious +25, enchanted and lovingly -20; a word in `refuse` (hostile) will not trade |
| `wants` | `wantPercent`, `otherPercent` | what the trader pays for the hero's goods: its wants at 100%, any other good at 50% |
| `haggle` | `baseChance`, `opinionDivisor`, `perPersuasion`, `minChance`, `maxChance`, `discountPercent`, `failureOpinion` | the Haggle button of the trade screen (US-283) |
| `purse` | `start`, `restockPerDay`, `cap` | the money a trader can pay out in a currency region, in value units (US-283) |

**Reputation bands** use the same attitude word the NPC menu title shows (opinion bands of `opinions.json`): a friendly trader is cheaper than a suspicious one; a devoted one (`enchanted`, `lovingly`) unlocks rare stock.

**Rare goods.** A trade profile's `rare` table names goods and the lowest band word that unlocks each (for example `"obsidian": "friendly"`, opinion 10 or more). Below it the good is not offered. The action **Ask about rare goods** (`assets/data/interactions/rare-goods.json`) appears for a trader with a `rare` table; when the hero does not stand high enough for everything kept back, the Actions pop-up (key **X**) lists it greyed out with the reason `Rare goods are kept for people they like better`. Its tags are given by the game: `has-rare-goods` (a rare table) and `rare-open` (nothing is kept back from this hero).

Currency items are never repriced: shells worth 1 cost 1 from a friendly trader and from a suspicious one.

## Trade: the trade screen (US-283)

**Trade** (`assets/data/interactions/trade.json`, tag `trader`, range 3 m) opens the trade screen of a placed NPC that has a trade profile; a rival camp keeps its own **Barter** with its counter-offer and pay-later. The action is hidden for a trader with nothing to trade and for a hostile one; the Actions pop-up (**X**) lists it with the reason (`They have nothing to trade`, `They will not trade with you: they are hostile`).

The screen, top to bottom:

| Part | What it shows |
|---|---|
| Title | `TRADE with Tala (friendly)`: the attitude word decides the prices (ADR-023) |
| Money line | in a currency region your **balance**, the trader's **purse** and the money here (`Shells = 1`); in a region with no currency `This region has no money: barter only.` |
| **You give** | your bag (coins are in the balance) with, on each button, how many you put on the table of how many you have, and `@` what the trader pays for one piece (a `-` button takes one back) |
| **You take** | the trader's stock with how many you take of how many it has and `@` what you pay for one piece; a `*` marks a good the trader wants; rare goods you do not stand high enough for are not listed (`They keep back: ... (needs friendly)`) |
| Pay from balance | `-5 -1 +1 +5`: units of your balance paid into the deal (currency regions only) |
| Balance bar | `They receive X   They give Y   [####....]`, live: received is your goods at what the trader pays plus the balance you pay; given is its goods at what you pay. **Deal** is available when received is at least given; otherwise the line says what is missing (`They want 0.45 more in value.`) |
| **Haggle (n%)** | one try per trader per in-game day: the chance from opinion and persuasion (your Trade affinity / 10), a seeded roll; a win is 10% off for the rest of the day, a loss costs 5 opinion; the roll and the chance are shown |

**Money.** When the screen opens, every coin item of the region's currencies in your bag becomes your balance; when it closes (Close, or the menu key) the balance goes back as coin, highest value first; a remainder no coin can make waits in the balance for the next visit (`trade.json` keeps it). Any currency item is worth its value anywhere and is never repriced. In a currency region, what you gave beyond what you took is paid back to your balance from the trader's purse, up to what the purse holds (30 at the start, +5 a day, at most 60: `purse` in `trade.json`); where there is no currency nothing is paid back, so ask for goods.

**After a deal** the goods move between your bag and the trader's stock, the prices drift (ADR-023), and the trader's opinion of you rises by the `trade` event of `opinions.json` (+5).

## Editor: the Trade section (US-284)

An NPC panel (select a placed NPC with the Select tool) has a **Trade** section below it (so it never covers the figure you are editing), and the **Class** panel and the **Kinds** tab have the same six lines under **Deny**. Each line is plain text:

| Line | Type | Example | Meaning |
|---|---|---|---|
| **Stock** | `item=number ...` | `flint=6 fur=2` | the stock at the start and the target of the price curve; `flint=0` is an empty shelf |
| **Restock/day** | `item=number ...` | `flint=1` | pieces delivered every day |
| **Picks/day** | a number 0 to 20, or empty | `2` | weighted random deliveries a day |
| **Weights** | `item=number ...` | `fur=3` | how likely each good is in the random deliveries (0 to 1000; the region's **Goods** from the Economy panel add theirs) |
| **Wants** | items separated by spaces or commas | `berries fur` | goods the trader buys at full value (the rest at half) |
| **Rare** | `item=band ...` | `obsidian=friendly` | goods kept for people it likes: hostile, wary, suspicious, neutral, friendly, enchanted, lovingly |

A line is read when you press Enter or click elsewhere. A mistake is said in the status line and changes nothing (`trade stock: "fur": the number after = must be whole`).

- **NPC panel**: the section shows and edits the NPC's **own** values; the class and kind add theirs (the title says what the NPC trades in all, in its hint). A layer can add or change an entry but not remove one a lower layer gives: set a stock or a restock to `0`, or a weight to `0`, to switch an inherited one off. Every line is one **Undo** step; typing the same text again is none.
- **Class panel and Kinds tab**: the lines edit the draft; **Save** writes the file (`"trade": {...}` in the order of the table of US-281) and every NPC of the class or kind that does not set the value itself follows it, in the Editor and in play (F1).

## The test level: the trader and the wary hunter (US-284)

`assets/levels/npc-test.json` is now a small market: the region has `shells` for money (value 1) and delivers berries (weight 3) and flint (weight 2) to its traders. **Tala** (trader, neutral) has flint 6, fur 2 and berries 4, restocks a flint a day, makes one random delivery a day (fur, or the region's berries and flint) and wants berries. **Harn** (hunter, **wary**) has fur 4 and a spearhead, restocks a fur a day, wants flint and keeps the spearhead for people who are friendly. Walk-through of the trade steps (added to the table of the US-270 walk-through):

| Step | Do | Expect |
|---|---|---|
| 1 | Right-click Tala: **Trade** | the screen: your bag, her stock with prices, the balance bar; `Money here: Shells = 1` |
| 2 | Give berries (she wants them: full value), take flint | the bar fills, **Deal** works, the goods move |
| 3 | Pick up some shells, trade again, pay from the balance, **Close** | the change comes back as shells |
| 4 | Press **X** next to Harn | **Ask about rare goods** is greyed out: `Rare goods are kept for people they like better` |
| 5 | Trade with Harn: **Haggle** once | a chance and a roll; a second try today is greyed out |
| 6 | Talk kindly to Harn until he is friendly, press **X** again | **Ask about rare goods** is offered; its message names the spearhead; the trade screen lists it |
| 7 | Wait one in-game day, trade with Tala again | one more flint than yesterday (plus the random delivery) |

## Places and schedules (US-290, level version 5)

### Places: the `places` of a level

A level may name spots of its map (written only when there are some). Schedules send people to them, and the environment interactions of M9c use their tags.

| Field | Values | Meaning |
|---|---|---|
| `name` | lower-case letters, digits, `-`; not `home` | what schedules say in `at` |
| `x`, `y` | world pixels, inside the level | where it is |
| `tags` | list of words | what is there: `forage`, `shelter`, `water`, `shrine`, anything you want to target |

```json
"places": [ { "name": "market", "x": 656, "y": 336 }, { "name": "grove", "x": 976, "y": 400, "tags": ["forage", "shelter"] } ]
```

`home` is built in: the spot where the NPC was placed. In the Editor the **Places** line of the Economy panel (**Level**, **Economy...**) is `market=20,10 grove=30,12/forage/shelter`: tile numbers (the place is the middle of the tile) and tags after slashes. A mistake (no `=`, `home`, a name twice, a spot outside the level) is said and changes nothing; the line is one Undo step.

### Schedules: the `schedule` block

A `schedule` can be written in a class file, a kind file and on a placed NPC. The layers do **not** merge: the schedule of the highest layer that has one wins as a whole (the NPC, else its kind, else its classes in order).

```json
"schedule": [ { "from": "06:00", "do": "work", "at": "market" }, { "from": "21:00", "do": "sleep", "at": "home" } ]
```

or, when the night differs, `"schedule": { "day": [ ... ], "night": [ ... ] }`.

| Field | Values | Meaning |
|---|---|---|
| `from` | `"HH:MM"` | when the block begins; blocks are sorted by it, two cannot begin together |
| `do` | an activity word, or the id of an interaction | what the person does |
| `at` | a place name or `home` (default) | where |

A block lasts until the next begins and the day wraps round midnight, so the last block of the day holds until the first one. The night variant is used between `nightFromHour` and `nightToHour` of `assets/data/sim/schedule.json` (21 and 6); with no night blocks the day blocks hold at night too.

**Activity words** are the keys of `activities` in `schedule.json`: `sleep` (+12 Energy an hour), `rest` (+4 Energy), `eat` (+25 Hunger), `work`, `idle`, `go`, `patrol` (nothing back). Add your own word with the needs it restores. An interaction id is also an activity. An unknown place or activity is logged with the NPC's name when the level loads (`Schedule of Tala: 12:00: "square" is not a place of this level`) and the person stays at home for it.

### How it runs

The schedule is checked **on the hour** (every 100 ticks): persons near the hero (the near radius of ADR-022) go to the place of the block and get back what the activity restores; persons far from the hero follow it in their daily summary: each is visited once a day, at the tick of the day that is their index, so no tick visits the whole crowd (ADR-022 addendum). In play the figure of a person walks 2 pixels a tick to where its schedule sends it, round what is in the way (after ten seconds without progress it is put at its goal); its menu is where it stands.

**Interruptions** (D-54 Q10): a person near the hero whose Hunger is below `eat.below` (20) goes to `eat.place` (home) and eats (`eat.restore`, +40 an hour) until no longer hungry; danger (a hostile creature within 6 m) sends it to `dangerPlace` (home) until it is gone; a fight (US-292) keeps it fighting. The next hour after an interruption the person simply follows its schedule again: it goes back to the market.

`npc-life.json` next to `npcs.json` keeps the schedules, homes and modes (run-length coded, so a crowd with one schedule saves in a few hundred bytes plus two numbers of home per person).

### Routines: weights on a block and the routines of professions (US-196)
A routine is a schedule. This story adds one key to a block, `prefer`, and one more place a schedule can live: a **profession**. There is no second format and no second scheduler.

```json
{ "from": "06:00", "do": "work", "at": "home", "prefer": { "animal": 300, "edible": 150 } }
```
`prefer` is a weight in whole percent for each **tag** of what a person could do (100 = neutral, 0 to 500). While the block is in force the score of every choice is multiplied by the weight of each tag it carries: a hunter's morning block with `"animal": 300` makes everything that has to do with animals three times as attractive, so hunters choose it more often in that block than at other times. A block guides choices; it does not name one. A weight of 0 takes it off the list. The tags of a choice are the **id of the interaction**, the **tags its file asks of its target** and the **tags of the target itself** (a place's tags, an animal's kind and tags, `place`, `animal`, `npc`...), each counted once. Needs come first: a hungry person goes to eat and danger sends them home before any choice is made (the interruption rule of US-290), so a starving hunter in a work block eats.

In the Editor's text line the weights come last: `06:00 work market prefer animal=300,edible=150; 21:00 sleep home`. A weight over 500, a tag that is not a word or a missing number is refused with the reason.

**Profession routines.** `hero/professions.json` entries may carry `day` and `night` in exactly the same block format (the same reader loads them as loads a class file):
```json
{ "id": "hunter", "name": "Hunter", ..., "day": [ { "from": "06:00", "do": "work", "prefer": { "hunt": 200 } }, { "from": "14:00", "do": "rest" }, { "from": "21:00", "do": "sleep" } ] }
```
The people of the **clan** have no schedule of their own: each takes the routine of their profession. A grown-up whose hunting is better than their gathering is a `hunter`, otherwise a `gatherer` (a child has none). In the clan the tags are the actions: `work` (gather and hunt), `gather`, `hunt`, `sleep`, `warm`, `talk`, `gift`, `steal`, `rest`, `wander`; the weights multiply the score of the action the clan's utility AI computes. The needs win here too: while any need is below `routineDangerBelow` (20, `sim/actions.json`) the weights are left aside. `hunter` and `gatherer` ship with a routine (work 06:00 to 14:00 weighted towards hunting or gathering, then rest and talk); a profession with no `day` or `night` changes nothing.

### The timeline
Under the Day and Night lines of the Schedule form (the NPC panel, the Class panel, the Kinds tab) and above the form of a routine in the Data tab (a profession's `day` and `night`, a class's `schedule.day`) a **bar of 24 hours** shows one block for each entry. Drag the **left edge** of a block: it moves in steps of a quarter hour and never past its neighbours; let go and the time is written. The bar writes through the same setter as the text line (the Data tab: the same edit as typing the time into the field), so the form shows the change at once, it is one step of Undo, and Save writes it in the file's own format (in the Data tab only the `from` line changes). Rest the pointer on a block for its time, activity and place.

### Editor: the Schedule form

The Trade section below the NPC panel has two more lines, **Day** and **Night**, and the Class panel and the Kinds tab have them under the trade lines. Type `06:00 work market; 21:00 sleep home` (time, activity, place; the place is optional and then `home`; separate blocks with `;`). Press Enter: the status line says `schedule day`; a mistake (`6am`, a missing activity, two blocks at one time) is said and changes nothing. For an NPC each line is one Undo step and edits its **own** schedule (which replaces its kind's and its classes' whole); for a class or kind **Save** writes it. An empty **Night** means the day blocks hold at night.

## Action sources: class, custom and event actions (US-291)

An NPC that is **free** (idle, or on a block of `work`, `go`, `patrol`; the list is `free` in `schedule.json`) chooses something to do on its own, once an hour (and at once when an event concerns it). It takes candidates from three sources (D-54 Q11; there is **no quest source** in M9c: M10 adds one and changes this schema):

| Source | Where it is set | Meaning |
|---|---|---|
| **Class actions** | `"does": ["patrol"]` in a class file | what every NPC of the class does when free |
| **Custom actions** | `"does"` in a kind file or on a placed NPC | the NPC's own list; layers add up (a class's, then the kind's, then the NPC's) |
| **Event actions** | `assets/data/sim/events.json` | an event of the world offers an action to the NPCs it concerns |

`does` is a list of interaction ids; an id that is no interaction, or has no `npc` block, is logged with the NPC's name when the level loads and never chosen. An NPC's own `deny` list (US-267) also keeps it from doing an action itself.

**The choice** uses the same interaction files as the hero: the `npc` block of a file gives its `score` (the rule language; `need(hunger)`, `opinion(actor, target)`, `tag(target, post)`...) and its `cooldown` in seconds. Every candidate is scored against everything it could be done to (the places of the level, or the spot of an event); the best score wins, a tie by a roll of the world seed; below 30 nobody gets up for it. An event candidate gets the `bonus` of its event added. At most `maxPerHour` (64, `schedule.json`) persons choose on one hour mark, and the turn goes round so nobody is left out. The files an NPC acts with have `"actors": ["npc"]`: `patrol.json` (target: a place tagged `post`, effect `do walk-to`) and `help-with-fire.json` (target: the event). The word `walk-to` takes the person to the thing.

### events.json

```json
{ "version": 1, "events": [ { "id": "fire-help", "label": "Help put out the fire", "trigger": "fire", "action": "help-with-fire",
    "classes": ["talker", "elder", "trader", "hunter"], "withinMetres": 12, "forMinutes": 30, "bonus": 30 } ] }
```

| Field | Meaning |
|---|---|
| `trigger` | the event of the world: `fire` is posted when a fire pit is lit |
| `action` | the interaction it offers |
| `classes` | the classes it concerns (none listed: every NPC) |
| `withinMetres` | how near the NPC must be (1 to 200) |
| `forMinutes` | how long it is on offer, in game minutes |
| `bonus` | added to the score of the action |

A fire lit within 12 m of a talker sends her to it **at once**; a fire that went out more than 30 game minutes ago is no longer on offer at the next hour mark.

### Editor

The **Does** line (under the Schedule lines of the Trade section below the NPC panel, the Class panel and the Kinds tab) takes interaction ids separated by spaces: `patrol sing`. For an NPC it is its custom actions (one Undo step); for a class its class actions, for a kind its custom actions (Save writes the file). A mistake (an id that is not lower-case words) is said and changes nothing.

## NPCs act on each other (US-292, D-54 Q12, Q13)

A free person (see US-291) looks at the neighbour standing close and may do to them what the interaction files allow. The files are the same kind as the hero's: the `actors` word is `npc`, the target is a person (`"target": { "tags": ["npc"] }`), and the `npc` block gives the score and cooldown. The shipped ones:

| File | When | What it does |
|---|---|---|
| `npc-chat.json` | always (score `30 + need(social)`, so lonelier persons chat more) | `do chat`: a roll of the quality of the talk (-2 to 2) changes both opinions as `opinions.json` says, both get a little Social back, and near the hero the words show in a bubble |
| `npc-swap.json` | each has a piece of what the other wants (the game gives the target the tag `can-swap`) | `do swap`: one piece each way between their stocks (US-281), both think better of the other (the `trade` event) |
| `npc-gift.json` | the actor has goods (tag `has-goods`) and likes the target (opinion over 25) | `do gift`: one piece of its stock to the other, who thinks better of it (the `gift` event) |
| `npc-confront.json` | the actor dislikes the target (opinion under -20) but is not hostile | the numbers of the hero's taunt: the target thinks 10 less of the actor, the actor 2 less of the target, the target remembers it, those who can hear and know the target think 2 less of the actor |
| `npc-fight.json` | the actor is hostile to the target (`mood(actor) == hostile`) | `do fight`: strike and strike back with the hit points and sword damage of the two (the placed character's `hp` and `swordDamage`; `dealings.defaultHp` and `defaultDamage` for any other person), `dealings.fightRoundsPerHour` rounds an hour near the hero |

**Consequences.** When a fight begins, the persons who can hear it (the `hearingTiles` of `opinions.json`) and know the victim think `dealings.witnessOpinion` (6) less of the attacker. A **death is final**: the person is dead for the simulation, their figure leaves the world, nobody can talk to or trade with them, and their family near the place think `dealings.griefOpinion` (30) less of the killer. If neither falls, they break off, each thinking 3 less of the other, and the fight goes on the next hour while they are hostile.

**The same rules, far away (D-54 Q13).** A person far from the hero is visited once a day (US-290); with the chance `dealings.farPercent` (5) a day it has a dealing with a neighbour who is far too, chosen by the same scores and carried out with the same effects, settled at once (a far fight runs `farFightRounds` rounds), with no animation and no bubble. A seeded roll of the world seed, the day and the person decides, so the same world always lives the same way. Deaths anywhere are reported to the game.

**Bounded work.** At most `maxPerHour` persons begin a dealing or an action on one hour mark, and the turn goes round so nobody is left out; the neighbour is found through the grid cell of the person (a few places of the cell are looked at, never the crowd). The 100,000-person soak of US-294 measures the cost.

**Effects an NPC carries out** in the files: `opinion who whom n`, `remember who "text" feeling` (with `{actor.name}` and `{target.name}`), `give who item n` and `take who item n` (the stock of a trader), and `do` with `walk-to`, `restore <need> <amount>`, `chat`, `swap`, `gift`, `fight` and `spread-opinion n`. `who` is `actor`, `target` or `hero`.

**`dealings` in `schedule.json`** holds every number: `meetRadius` (96 pixels), `fightRoundsPerHour`, `farFightRounds`, `witnessOpinion`, `griefOpinion`, `farPercent`, `preferBonus` (US-293), `defaultHp`, `defaultDamage`, `chatSocial` and the `chatter` sentences.

## Default interactions by partner type (US-293, D-54 Q14, Q16)

An NPC meets different kinds of partner: the player, other NPCs (by class), animals, the environment (places), and any kind you add. For each it can have default **dialogues** (US-265) and default **actions**: the interactions it prefers when that partner is the target.

### Partner types: `assets/data/sim/partner-types.json`

```json
{ "version": 1, "types": ["player", "animal", "environment", "buildings"] }
```

`player`, `animal` and `environment` are always there; add your own words to the list and they are valid everywhere a partner type is named (the dialogues of classes, kinds and NPCs, `partnerActions`) and appear in the Editor. `class:<id>` is always a type, one for every NPC class. A file that lists `class` is refused (`class` is the key for "every NPC class", below).

### Defaults: `assets/data/interactions/defaults-<type>.json`

One file per type: `defaults-class.json`, `defaults-animal.json`, `defaults-environment.json`. The file name says the type.

```json
{ "partnerType": "environment", "actions": ["forage", "rest-at-shelter", "fish", "pray"] }
```

`class` is the default for meeting any NPC class (shipped: empty, because a bonus for every class would put chatting above a useful swap); `animal` is empty (an NPC does nothing special with animals unless its class says so); `environment` lists the four environment actions below. A file with a mistake is left out and named in the log (`interactions/defaults-animal.json:2: partnerType "x" must match the file name "animal"`). The interaction registry skips these files. **F5** reads them again.

### `partnerActions`: a class, a kind or an NPC overrides

```json
"partnerActions": { "animal": ["hunt"], "class:guard": ["npc-chat"] }
```

In a class file, a kind file or on a placed NPC. Keys are partner types (`class` means every NPC class, `class:<id>` one class). The **list of the highest layer that has the type replaces** the lower layers' list for that type as a whole: defaults, then the classes, then the kind, then the NPC. An empty list (`"environment": []`) means "nothing special with that partner". The shipped hunter class has `"animal": ["hunt"]`.

### What the chooser does with them

- **Environment and animals.** The actions of the `environment` and `animal` lists are extra candidates of a free NPC, aimed at the places of the level (and the animals it can see within `dealings.lookRadius`, 12 m) and given the bonus `dealings.preferBonus` (40). Which place fits is the file's target tags: `forage.json` needs a place tagged `forage` (+25 Hunger, score `need(hunger) - 30`: only a hungry person goes), `rest-at-shelter.json` a `shelter` (+20 Energy), `fish.json` `water` (+20 Hunger), `pray.json` a `shrine` (+15 Social). A hungry person goes foraging at a grove, a tired one rests in the hut. `hunt.json` needs an animal tagged `prey`: the hunter goes to it and `dealings.huntPercent` (50) decides whether it is killed; the game removes the animal from the world; the hunt gives Hunger back.
- **Partners that are persons.** When the neighbour is a person of a class that the NPC has a `partnerActions` list for, an interaction in that list gets the bonus; so an NPC that prefers `npc-chat` with talkers chats even when a swap would score higher.
- The places are the `places` of the level (US-290); the animals are told to the director by the game as they move.

### Editor

Under the **Does** line (Trade section below the NPC panel, Class panel, Kinds tab) there is **Defaults with: animal** (a click goes to the next partner type, ending with `class`) and a **Does** line for that type: interaction ids separated by spaces. A type you added to `partner-types.json` is in the list the next time the game starts. An empty line removes the type from the NPC's own lists (the layer below shows again). For an NPC one Undo step; **Save** writes the class or kind.

## The living test level and the soak (US-294, D-54 Q15)

`assets/levels/npc-test.json` now shows a day of NPC life. Five **places** (market 320,390 `market`; grove 700,180 `forage`; hut 160,640 `shelter`; pond 1100,620 `water`; shrine 900,220 `shrine`) and a schedule for each of the five people. A game day is two minutes; press **F1** (game mode) and the speed keys, and watch.

| Time | Tala (trader) | Harn (hunter, wary) | Ossa (talker, does `fish`) | Vell (elder, does `pray`) | Gur (guard) |
|---|---|---|---|---|---|
| 06:00 | works at the market | at home | works at the pond | at home | patrols the market |
| 08:00 | at the market | works at the market | at the pond | at home | patrols |
| 10:00 | at the market | at the market | at the pond | goes to the shrine | patrols |
| 12:00 | eats at home | eats at home | eats at home | eats at home | patrols |
| 13:00 | works at the market | at home | works at the grove | at home | patrols |
| 15:00 | at the market | at home | at the grove | goes to the market | patrols |
| 18:00 | at the market | at home | at the grove | at the market | at home |
| 20:00 | rests at the hut | at home | at the grove | at the market | at home |
| 21:00-22:00 | sleeps at home | sleeps (the night variant) | sleeps at home | sleeps at home | sleeps at home |

Harn also carries 2 berries now, so Tala (wants berries, has flint) and Harn (wants flint, has berries) can **swap** when they meet at the market between 08:00 and 12:00. Ossa and Vell chat when they meet; at 12:00 they all walk home to eat. The deer (far in the south-east, out of sight) and the goblin have no schedule; move the deer next to Harn to see him hunt it. Because Tala may swap a flint for berries, her flint count is no longer "6 plus a restock" after a day.

**Walk-through of one day.**
1. Start the level, **F1**, speed 4. At 06:00 Tala goes to the market, Ossa to the pond, Gur starts patrolling.
2. Around 08:00 to 12:00 Tala and Harn stand at the market and swap (their last action is `npc-swap`); open the trade screen with Tala: berries have gone up, flint down.
3. At 12:00 everybody goes home and eats (hunger goes back up).
4. At 10:00 Vell goes to the shrine, at 15:00 to the market, where she and Tala chat; a bubble with a few words shows.
5. At 21:00 everybody sleeps at home. Harn has a night variant in the Schedule form of the Editor (21:00 sleep).
6. Leave the level for a day (stand far away with the Editor camera): near persons are simulated by the hour, far ones in the daily pass; both land on the same schedule when you return.

### The soak (`odysseus_sim_soak`)

`tests/sim/npc_soak_test.cpp` builds 100,000 persons (three schedules, 1,000 traders, three places) with the shipped interaction files, runs **30 in-game days** headless and hashes the saved state (population, director and market). It runs twice with the same seed: the two hashes must be identical. In Release it also checks the ADR-022 budget: no day above 100 ms of CPU time in all, no single tick above 8 ms. In Debug (with AddressSanitizer) only the repeatable hash is checked, because Debug is many times slower. The second case shows that a different seed gives a different hash, so the hash really covers the run.

Run only the soak: `ctest --preset windows-x64-release -R odysseus_sim_soak` (it takes minutes in Debug).
