# Interaction data guide

How to change what things in the world offer and do, without touching code (US-150, INT-01, INT-03, ADR-019).

Every action in the game is one **interaction**: a small text file in `assets/data/interactions/`, named after its id (`gather.json` holds the interaction `gather`). Edit the files in any text editor. The game reads them at start; a mistake is reported as `file:line: message`, and a file with a mistake is skipped while the rest load.

> Every action in the game's right-click menu is one of these files (US-152). They carry out their work with `do`, which names an action built into the game (see "Built-in actions"); US-153 lets files spell out their own effects, with durations.

## Timed actions and things that change (US-153)

- `duration` is how long the job takes, in seconds. While it runs the hero stands still and a ring of twelve dots fills over the target. If the player **moves or attacks before the ring is full, the job stops and nothing happens**: no berries, no change to the target. With `duration` 0 (or none) the effects happen at once.
- When the job is done, the `effects` happen in the order written. `set target.state picked` changes the target's state; `after 15s set target.state ripe` puts a change on a timer, so the plant is ripe again 15 seconds later. A timer counts game time: it waits while a screen is open, and it is saved with the game.
- A plant in any state but its first (the starting one) is **hidden** until it is back to it: it cannot be seen, clicked, inspected or hit. So a picked plant disappears and comes back in the same spot (D-37).
- Saved: every plant not in its starting state, and every effect still waiting on a timer (file `things.json` in the save folder, written with the autosave). The job in progress is not saved: loading starts you with nothing under way, and nothing is lost but the time.
- Timer units: `s` (seconds), `m` (minutes), `d` (in-game days; a day is as long as the clan's calendar says).

Example, the shipped `gather.json`:

```jsonc
"duration": 3,
"requires": [ { "if": "target.state == ripe", "else": "Nothing to pick yet" } ],
"effects": [ "do gather-berries", "set target.state picked", "after 15s set target.state ripe" ]
```

## World objects: objects.json (US-155)

The things of a camp are listed in `assets/data/objects.json`, as plants are in `plants.json`. Seven ship: fire pit, knapping stone, food store, shelter, flint nodule, water source, sleeping furs. They are placed in the Editor (F2, the plant tool, the **last page** of the palette), saved in the level with the plants, and each has its own interactions in `assets/data/interactions/`.

```jsonc
{ "name": "fire pit", "frame": "fire-pit", "blocks": false, "inspect": "A ring of stones around old ash.",
  "tags":   ["object", "fire", "fire-pit"],
  "states": ["cold", "burning"] }
```

| Field | Meaning |
|---|---|
| `name` | what the Editor and menus call it; must not be a plant's name |
| `frame` | the picture the game draws (programmer art until real art is chosen): `fire-pit`, `knapping-stone`, `food-store`, `shelter`, `flint-nodule`, `water-source`, `sleeping-furs`; any other word draws a grey block, so a new object always shows |
| `blocks` | `true` makes it block walking |
| `inspect` | the line the Inspect item shows |
| `tags`, `states` | as for plants (see below); with no `tags` an object is tagged just `object`. Unlike plants, objects are never hidden by their states |

Add an entry and press F5: it is in the palette (the last page of the Plant palette) with its tags, no restart needed. Give it actions by writing interaction files that target its tags. The shipped ones:

| Object | Interactions (files) |
|---|---|
| fire pit | `light-fire`: 3 s, needs a fire drill, state `burning`, warms everyone within 6 m now and after 1 and 2 minutes, goes out after 3 |
| knapping stone | `craft-at-stone` (the same Craft as the camp's stone) |
| food store | `put-in-store` (2 berries in, state `stocked`), `take-from-store` (2 berries out, state `empty`) |
| shelter | `rest-in-shelter`: 5 s, energy +15 |
| flint nodule | `chip-flint` (needs a hammerstone: 2 flint), `pick-nodule-flakes` (by hand: 1 flint); then `chipped` for 2 minutes |
| water source | `drink`: 2 s |
| sleeping furs | `sleep-on-furs`: 8 s, energy +40 and warmth +10 |
| every object | `inspect-object` |

Two built-in actions serve them: `do warm-nearby 6 25` (everyone within 6 tiles of the thing, the hero too, gets 25 warmth) and `do restore energy 40` (the hero's need rises). The verb `fx flame` plays an effect from `effects.json` once over the thing.

## Clan members and animals act on their own (US-154)

The `npc` block of an interaction file lets clan members and animals do it by themselves, with the same file, menu rules and timed runner as the hero:

```jsonc
"npc": { "score": "need(hunger) * 2 + trait(diligent) * 10", "cooldown": 60 }
```

- **score** is a sum like a condition, but its number is what counts: the higher, the more they want to do it. A score below **30** is not worth getting up for. Scores below 0 count as 0.
- **cooldown** is how many seconds an actor rests from this interaction after doing it.
- Once a second an idle actor looks at everything within **12 m**: for a clan member the plants, for an animal the plants, the hostile creatures and the hero. It scores every interaction its kind may do to each, skips those resting, and starts the best. Ties are settled by a seeded random draw, so a world always plays out the same way.
- Far from the thing, they walk to it (people at their usual pace, animals at 1.25 m/s; a fleeing animal runs at 2.5 m/s) and start when in reach; then it runs for its `duration`, as for the hero. The hero's menu rules (`range`, `requires`) apply to them too, except that range is about doing it, not wanting it.
- **Danger.** A hostile creature within **6 m** drops whatever a clan member or animal was walking to do or doing; a clan member with a hostile close by does not set off on a new errand. Animals still look around, because they may want to flee.

What the score can read for a clan member: `need(hunger)` (and `energy`, `warmth`, `social`) is **how much the need is missing**, 0 when full to 100 when desperate, so a hungry person scores high; `trait(diligent)` is 1 when they have that trait; `distance` is the metres from them to the target. A clan member's berries are not modelled, so `has(...)` is 0 for them.

Who is who:

| Actor | Matches `actors` | Tags the score may use (`tag(...)`) |
|---|---|---|
| clan member | `person`, `clan` | `person`, `clan` |
| harmless animal (deer, rabbit) | its kind name, `animal`, `prey` | `animal`, `prey` |

The shipped files:

| File | Who | What |
|---|---|---|
| `gather` | clan members, the hero | a hungry clan member walks to a ripe plant and gathers it (they eat a little, 30 hunger); the plant is picked and ripens again after 15 s |
| `graze` | prey | walks to a patch of grass (plants tagged `grass`) and stays four seconds |
| `flee-predator` | prey | runs from a hostile creature within 6 m: `(6 - distance) * 40` |
| `flee-armed-hero` | prey | runs from a hero holding a weapon within 5 m (D-36) |
| `flee-moving-hero` | prey | runs from a hero who is moving, within 5 m (D-36): a hero standing still is no threat |

The hero as a target has the tags `hero`, `person` and, while true, `armed` (a weapon in hand) and `moving` (walking). `do flee` (a built-in for animals) makes the animal run about 8 m directly away from the thing.

## Built-in actions: `do`

Some menu items need the game itself: open the crafting screen, change what a clan member thinks of the hero, harvest a plant. Those are **built-in actions**, named with `do`:

```jsonc
"effects": [ "do open-craft knapping-stone" ]
```

A `do` that names anything the game does not have is an error at load (`do names "talks", which the game does not know (it knows: ...)`). The built-ins, each doing exactly what the game always did:

| Name | What it does |
|---|---|
| `gather-berries` | the hero gets berries, the gatherer skill grows, and the message says so (the plant itself is handled by the file: see `gather.json`) |
| `knap` | knaps flint from a flint nodule (needs a hammerstone) |
| `pick-flint` | picks flint up from a nodule by hand |
| `chop` | chops wood from a solid plant |
| `inspect` | shows the plant's own line from plants.json |
| `talk` | talks with the clan member |
| `give-berries` | gives the clan member a berry |
| `ask-to-teach <profession>` | asks the clan member to teach a profession: `do ask-to-teach hunter` |
| `open-craft <station>` | opens the crafting screen for `fire` or `knapping-stone` |
| `eat-berries` | the hero eats berries at the fire |
| `tend-camp-fire` | tends the clan's fire |
| `tend-sacred-fire` | tends the hero's sacred fire |
| `hold-ritual` | holds a ritual at the sacred fire |
| `open-barter` | opens the barter screen with a rival camp |
| `restore <need> <amount>` | raises the hero's need (hunger, energy, warmth or social): `do restore energy 40` |
| `warm-nearby <tiles> <amount>` | warmth for everyone within that many tiles of the thing, the hero too: `do warm-nearby 6 25` |
| `confront` | opens the Confront menu of the NPC (US-266); `do confront` is the "Confront..." entry of the right-click menu |
| `spread-opinion <amount>` | the persons within hearing range (`hearingTiles` of opinions.json) who know the target (have met it, or are of its family) think `amount` more of the hero (negative: less): `do spread-opinion -5` |
| `calm <gain> <percent>` | a roll against `percent` in 100: on success the NPC stops winding up to strike and thinks `gain` better of the hero; else it will not listen: `do calm 15 70` |
| `provoke` | the NPC picks a fight: an enemy winds up to strike, a peaceful person becomes an enemy |
| `graze` | for animals: stay at the grass for the length of the action, nothing else |
| `flee` | for animals: run about 8 m directly away from the thing |

### What the game's own things are tagged

| Thing | Tags | Name in `{target.name}` |
|---|---|---|
| a clan member | `person`, `clan`, and `teaches-<profession>` while they are the hero's master of that profession with no apprentice yet | their name |
| the knapping stone | `workstation`, `knapping-stone` | Knapping stone |
| the clan's fire | `fire`, `workstation`, `camp-fire` | The clan's fire |
| the sacred fire | `fire`, `sacred-fire` | Sacred fire (its name) |
| a rival camp | `camp`, `rival` | the clan's name |

`flag(sacred-fire)` is 1 once the hero has founded a sacred fire. Menu items appear in `order`, lowest first; an item that fails `range` or a `requires` is greyed out with its reason.

## Editing while the game runs: F5

Edit a file, save it, and press **F5** in the game (Game or Editor mode); a file you save in the Editor's graph editor is read again at once. The game reads the whole folder:

- No mistakes: the new data is live at once (well under a second), and the log says `Interactions reloaded: 2 from 2 file(s) in 3.1 ms`.
- Any mistake: nothing changes. The data you had stays in use, and a red panel at the top lists each mistake as `file:line: message` (the first eight; the log has all). Fix the file and press F5 again; the panel closes by itself.
- If a file has a mistake when the game *starts*, it is left out, the others load, and the same panel shows until you fix it and press F5.

F5 reads the interaction files and the conversations (`assets/data/dialogue/*.dlg`, see `docs/guides/dialogue-format.md`) together, with the quests; a mistake in any keeps all as they were. The catalogs reload too: see "Live data" in `docs/guides/editor.md`.

## A whole file

```jsonc
// Gather from a plant that is ripe.
{
  "id": "gather",
  "label": "Gather {target.name}",
  "note": "Anything you write here survives the Editor.",
  "actors": ["hero", "person"],
  "target": { "tags": ["edible", "plant"] },
  "range": 1.5,        // metres
  "duration": 3.0,     // seconds
  "requires": [
    { "if": "season != winter",     "else": "Nothing grows in winter" },
    { "if": "target.state == ripe", "else": "Nothing to pick yet" }
  ],
  "effects": [
    "give actor berries 2",
    "set target.state picked",
    "after 15s set target.state ripe"
  ],
  "npc": { "score": "need(hunger) * 2 + trait(diligent) * 10", "cooldown": 60 }
}
```

You may write `//` and `/* ... */` comments anywhere. The in-game Editor (M9) keeps the `note` field when it saves but drops `//` comments, so put lasting remarks in `note`.

## The fields

| Field | Required | Meaning |
|---|---|---|
| `id` | yes | One word of letters, digits, `-` or `_`. Must be the file name without `.json`. |
| `label` | yes | What the menu shows. `{target.name}`, `{actor.name}`, `{npc.name}` and `{hero.name}` are filled in. |
| `actors` | yes | Who may do it: `hero`, `person`, `animal`, or the name of a kind (`deer`). |
| `target` | yes | What it can be done to: `"tags"` (the thing must have all of them) and/or `"kinds"` (it must be one of them). At least one of the two. |
| `range` | no | How close the actor must be, in metres, up to 30. Default 1.5. Farther away the item is greyed out as "Too far away". |
| `duration` | no | Seconds the action takes, up to 600. 0 (default) means instant. |
| `requires` | no | A list of conditions, each `{ "if": "...", "else": "why not" }`. The first one that is false greys the item out and shows its `else` text. |
| `effects` | yes | What happens when it finishes, one line per effect, in order. |
| `npc` | no | How clan members and animals choose it on their own: `score` (an expression; higher wins) and `cooldown` (seconds before they do it again). |
| `chronicle` | no | A line for the clan's chronicle, or `null`. Tokens like `{actor.name}` are allowed. |
| `order` | no | Menu position, 0 to 1000, lower first. Default 100. |
| `menu` | no | `"confront"` puts the interaction in the Confront menu of an NPC (US-266) instead of the ordinary right-click menu. Leave out for an ordinary action. |
| `note` | no | Your own words. Kept when the Editor saves. |

Any other field is an error (`unknown field "efects"`), which catches typos.

## Tags and states: how a thing gets its actions (US-151)

An interaction does not name things one by one; it names **tags**. A thing offers every interaction whose target tags it carries all of. Give a new plant the tags `edible` and `plant` and Gather shows up in its menu with no code change.

Catalog entries in `plants.json`, `animals.json`, `weapons.json` and `characters.json` take two optional fields:

```jsonc
{ "name": "mango", "frame": "bush", "size": "tall", "blocks": false, "edible": false, "inspect": "Sweet and heavy.",
  "tags":   ["edible", "plant"],     // what it is
  "states": ["ripe", "picked"] }     // what condition it can be in; the first one is where it starts
```

- Tags are single words of letters, digits, `-` or `_`, no repeats. A mistake is reported as `plants.json: plants[0].tags[1]: ...`.
- `target.state` in a condition reads the state; `set target.state picked` changes it. A thing with no `states` has an empty state.
- When an entry has no `tags` or `states`, the game works them out from its other fields, so older files keep working. A list you write replaces the worked-out one.

| Catalog | Worked-out tags | Worked-out states |
|---|---|---|
| plants | `plant`; `edible` if edible and walk-through; `fruit-bearing` if edible and solid (it is chopped, not gathered); `solid` if it blocks; `tree` if its size is tree | `ripe`, `picked` for an edible walk-through plant; none otherwise |
| animals | `animal`, and `hostile` (fights back) or `prey` | none |
| weapons | `item`, `weapon`, its class (`sword`, `bow`...), its element if any, `starter` | none |
| characters | `hero` and `person` for the hero, `person` for friendly people, `hostile` for enemies | none |

`actors` match the same way: `hero`, `person`, `animal` or a kind name matches a thing that is that kind or carries that tag.

If an interaction targets a tag no catalog uses, the game still loads it but warns at start: `interactions/warm.json:1: unknown tag "hearth": no catalog uses it, so this will never match`.

## Conditions and scores

A condition is a small sum that comes out true or false. A score is the same kind of sum that comes out as a number.

- Numbers are whole (no decimals); text is a word or "quoted words". `season != winter` compares the season with the word winter.
- Compare with `==  !=  <  <=  >  >=`. Combine with `and`, `or`, `not`. Calculate with `+  -  *  /` (division drops the remainder; dividing by zero gives 0).
- Order of work, first to last: brackets, minus sign, `*` `/`, `+` `-`, comparisons, `not`, `and`, `or`. So `1 + 2 * 3` is 7, and `1 or 0 and 0` is 1. Use brackets when unsure.
- A number and a word are never equal, and `<` only works on numbers. Nothing in a file can crash the game: a nonsense sum just gives false or 0.
- Put spaces around a minus (`a - b`); a hyphen inside a name (`wild-berry`) is part of the name.

### Things a condition can look at

| Name | Meaning |
|---|---|
| `actor`, `target`, `npc`, `hero` | Who or what is involved. `target.state`, `target.name`, `actor.name` read a detail of one. `npc` is the person being spoken to or acted on in a conversation. |
| `time` | The part of the day: `morning`, `afternoon`, `evening` or `night`. |
| `season` | `spring`, `summer`, `autumn` or `winter`. |
| `distance` | How far apart the actor and the target are, in whole metres. |

### Functions

| Function | Meaning |
|---|---|
| `has(item, n)` | 1 when someone holds at least n of an item: has(berries, 2) for the actor, has(hero, berries, 2) for anyone |
| `need(name)` | how much a need is missing, 0 (full) to 100 (desperate): need(hunger) |
| `skill(profession)` | the actor's skill in a profession: skill(hunter) |
| `trait(name)` | 1 when the actor has the trait, else 0: trait(diligent) |
| `opinion(a, b)` | what the first thinks of the second, -100 to 100: opinion(npc, hero) |
| `mood(who)` | one word for how someone feels about the hero (warm, friendly, neutral, wary, hostile, or hungry, tired, cold, lonely): mood(npc) == wary |
| `kin(a, b)` | 1 when the two are family: kin(npc, hero) |
| `flag(name)` | a note the story has set (0 when never set): flag(met-elder) |
| `quest(id)` | where a quest stands, one word: locked, available, active, done or failed: quest(first-day) == done |
| `step(id)` | the step an active quest is on, empty when it is not active: step(first-day) == eat |
| `tag(thing, name)` | 1 when a thing carries a tag: tag(target, edible) |

Examples in a requirement:

```jsonc
{ "if": "has(berries, 1)",                 "else": "You have no berries" },
{ "if": "opinion(npc, hero) >= -20 and not kin(npc, hero)", "else": "They will not trade with you" },
{ "if": "time == night or need(energy) < 20", "else": "Not tired yet" }
```

A mistake in a sum is reported with the line, for example `unknown function "hass"` or `has() takes 2 to 3 argument(s), not 1`.

## Effects

One line per effect: a verb followed by space-separated arguments. An argument is one word, a number, "quoted text", a call such as `has(hero, berries, 1)`, or a (bracketed sum) when it has spaces: `give actor berries (1 + skill(gatherer))`. `+5` and `-5` are numbers.

| Verb | Meaning |
|---|---|
| `give` | put items in someone's hands: give actor berries 2 |
| `take` | remove items from someone: take hero berries 1 |
| `set` | change a state of a thing: set target.state picked |
| `flag` | set a story note (1 unless a value is given): flag met-elder |
| `quest` | start, complete or fail a quest: quest start first-day |
| `opinion` | change what the first thinks of the second: opinion npc hero +5 |
| `remember` | give someone a memory with a feeling: remember npc "shared berries" 20 |
| `start` | begin another interaction: start inspect target |
| `talk` | open a conversation: talk elder-fire |
| `say` | show a speech bubble: say "Hello, {hero.name}" |
| `fx` | play a visual effect from effects.json: fx leaves |
| `sound` | play a sound: sound pop |
| `do` | run an action built into the game: do give-berries |
| `after` | do an effect later (s, m or d): after 15s set target.state ripe |
| `chronicle` | write a line in the clan's chronicle: chronicle "{actor.name} shared berries" |

Notes:

- `after` takes a time and then any other effect: `15s` is seconds, `2m` minutes, `1d` in-game days. This is how a picked bush becomes ripe again.
- `give` and `take` start with who (`actor`, `target`, `npc` or `hero`); `set` needs a path like `target.state`.
- `start` must name an interaction that exists; otherwise that file is skipped with an error.
- `talk` opens a conversation script (`.dlg`, M8); until M8 exists the name is only checked for spelling.

A mistake reads like `interactions/gather.json:12: unknown effect verb "giv"`: the file, the line, and what is wrong.

## What the game does with it

1. It lists every interaction whose `actors` match who is acting and whose `target` tags match the thing.
2. Too far away, or a failing `requires`: the item is greyed out with its reason.
3. When the actor starts it, the action runs for `duration` seconds (a ring fills over the target), then the `effects` happen in order (see "Timed actions").
4. Clan members and animals score the interactions near them with `npc.score` and pick the best (US-154).

## Checks the build makes

Every shipped interaction file is loaded by the tests; a file with a mistake fails the build. Saving a file from the Editor and loading it again gives the same interaction.

## The graph editor (US-172, M9)

In the Editor, the **Rules** button opens every interaction file as a graph over the map (same canvas, keys and Save as the dialogue graph in `docs/guides/dialogue-format.md`; Esc comes back). One file is one graph:

| Card | In the file | Ports |
|---|---|---|
| **Actor** | `actors` (words separated by spaces) | out, wired to the verb's first port |
| **Verb** (yellow) | `id`, `label`, `note`, `range` (metres), `duration` (seconds), `order`, `menu` | in: actor, requires, do, npc, chronicle; out: target |
| **Target** | `target.tags` and `target.kinds` | in |
| **Needs** | one entry of `requires` (`if`, `else`) | out, wired to the verb's requires port; one card per requirement |
| **Effects** | `effects`, one per line | out, wired to the verb's do port |
| **NPC rule** | `npc.score` and `npc.cooldown` | out, wired to the verb's npc port |
| **Chronicle** | `chronicle` | out, wired to the verb's chronicle port |

Save writes the canonical text of the file (`toJson`) after the real loader has read it back, so a mistake is named and nothing is written from it. The `//` comment lines at the very top of the file are kept; comments inside the braces are not (the `note` field is the place for the owner's words). The verb id must stay the name of the file (renaming would make a new file, which this editor does not do). The previous text is kept as `<id>.json.bak`, card positions are saved in `<id>.json.layout.json`, a file changed on disk since it was opened asks once before being overwritten, and the game reloads its data like F5.

**New and Tidy.** The *name* field and **New** button of the bar start a new interaction file of that name (a verb whose id is the name, an actor, a target and one effect that says there is nothing to do yet); it is written when you press Save. **Tidy** puts the cards in a row again.
