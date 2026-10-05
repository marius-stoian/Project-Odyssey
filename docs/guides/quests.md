# Quest data guide (US-180, M10)

A quest is one JSON file in `assets/data/quests/`, named after its id: `first-day.json` holds the quest `first-day`. Comments (`// ...` and `/* ... */`) are allowed. The Editor (US-184) writes the same file; its screen positions go to `<id>.quest.layout.json`. Conditions and effects use the rule language of the interaction files (docs/guides/interaction-data.md): one language for everything.

## A whole file

```jsonc
// The elder's first lesson.
{
  "id": "first-day",
  "title": "The First Day",
  "giver": "none",
  "requires": ["not quest(first-day) == done"],
  "start": "gather",
  "steps": {
    "gather": { "text": "Gather berries for the clan.",
                "objective": "gather berries 3", "marker": "tag:edible",
                "hint": { "after": "120s", "text": "Berry bushes grow by the stream." },
                "next": "eat" },
    "eat":    { "text": "Eat at the clan fire.", "objective": "interact eat-berries", "next": "fire" },
    "fire":   { "text": "Keep the fire alive.", "objective": "interact tend-fire", "next": "END",
                "branches": [ { "if": "flag(fire-out)", "to": "relight" } ] },
    "relight": { "text": "Light it again.", "objective": "interact tend-fire", "next": "END" }
  },
  "fail": ["hero.dead"],
  "rewards": ["opinion npc hero +10", "give hero flint 2"],
  "journal": "The elder taught me the first things a clan needs."
}
```

## The fields

| Field | Required | Meaning | Example |
|---|---|---|---|
| `id` | yes | One word of letters, digits, `-` or `_`; must equal the file name without `.json` | `"first-day"` |
| `title` | yes | The name in the journal and tracker | `"The First Day"` |
| `giver` | no (default `none`) | Who gives the quest: a person id, `role:<role>` (the first living person with that role, in id order), or `none` (the quest starts by itself as soon as it is available) | `"role:elder"` |
| `requires` | no | Prerequisites, all must hold before the quest is available (rule language; `quest(id)` and `step(id)` work here) | `["quest(first-day) == done"]` |
| `start` | yes | The id of the first step | `"gather"` |
| `steps` | yes | One entry per step, keyed by step id | see below |
| `fail` | no | Conditions; when any one holds the quest fails for good | `["hero.dead"]` |
| `rewards` | no | Effect lines run when the quest ends, aimed at the hero | `["give hero flint 2"]` |
| `journal` | no | The line the journal shows for a finished quest | `"The elder taught me..."` |
| `offer` | no | What the giver says when offering the quest; without it the game says the title and the first step | `"The clan is hungry. Will you help?"` |
| `turnIn` | no | What the giver says when the quest is handed in (the last step is `talk <giver>`) | `"Well done. The clan eats tonight."` |
| `note` | no | Your own words; kept when the Editor saves | `"tutorial"` |

### A step

| Field | Required | Meaning | Example |
|---|---|---|---|
| `text` | yes | What the tracker and journal show | `"Gather berries for the clan."` |
| `objective` | yes | What the player must do (list below) | `"gather berries 3"` |
| `next` | yes | The step that follows, or `END` to finish the quest | `"eat"` |
| `branches` | no | `[{ "if": condition, "to": step }]`; the first whose condition holds is taken instead of `next` | `[{ "if": "flag(fire-out)", "to": "relight" }]` |
| `marker` | no | Where the map marker points: `tag:<tag>`, `object:<id>`, `npc:<id>`, `place:<name>` | `"tag:edible"` |
| `hint` | no | `{ "after": "120s", "text": "..." }`: shown when the objective has not moved for that long (`s` or `m`) | `{ "after": "2m", "text": "Try the stream." }` |

### Objectives

| Objective | Met when |
|---|---|
| `talk <who>` | the hero starts a conversation with that person or character |
| `goto <place>` | the hero reaches that place |
| `gather <item> [n]` | the hero gathers n of the item (default 1) after the step started |
| `give <item> [n]` | the hero hands n of the item over |
| `craft <item> [n]` | the hero crafts n of the item |
| `interact <interaction>` | the hero finishes that interaction |
| `defeat <kind> [n]` | the hero defeats n of that kind |
| `wait <time>` | the time has passed since the step started (`30s`, `2m`, `1d`) |
| `flag <name>` | the story note is set |

Things that happened before the step started never count. More than needed counts as exactly done.

## Where a quest stands

`locked` (prerequisites do not hold yet) -> `available` -> `active` -> `done` or `failed`. A `giver: none` quest goes from available to active by itself. `failed` and `done` are final (the Editor's debugger, US-186, can reset a quest). In rules: `quest(first-day) == done` and `step(first-day) == eat`; as effects: `quest start first-day`, `quest complete first-day`, `quest fail first-day`.

## Mistakes

A quest with a mistake is left out; the others load. The message names the file, the line and the problem:

```
quests/first-day.json:14: unknown step "nowhere"
quests/first-day.json:9: "many" is not a count from 1 to 9999
```

## Saves

The state of every quest (status, step, progress, time on the step) is saved with the world in `things.json` (version 2). Older saves load with every quest locked. A quest or step that no longer exists in the data loses its progress with a note in the log.

## What the game reports (US-181)

Only what the hero does counts; clan members and other people doing the same thing never advance a quest.

| Objective | The game reports it when |
|---|---|
| `talk <who>` | a conversation with that person opens; `<who>` is their name in lower case with `-` for spaces (`old-tok`), one of their roles (`elder`) or their kind |
| `goto <place>` | the hero is within three tiles of a named place of the level (`places` in the level file), named in lower case with `-` |
| `gather <item> [n]` | items enter the hero bag, except crafted ones (picking, gathering, a `give hero ...` effect, a reward) |
| `give <item> [n]` | a conversation or interaction effect `take hero <item> n` removes items from the bag |
| `craft <item> [n]` | a recipe produces the item |
| `interact <interaction>` | the hero finishes that interaction (a timed one counts when it ends, not when it is stopped) |
| `defeat <kind> [n]` | the hero strikes the blow that defeats a character of that kind (the kind name of characters.json, lower case) |
| `wait <time>`, `flag <name>` | no report needed: the clock and the story notes are looked at once a second |

## Seeing quests: journal, tracker, markers (US-183)

- **Journal:** the key J (or the Journal tab of the Esc menu) lists Active, Done and Failed quests. An active quest shows its current step; a done quest shows its `journal` line.
- **Tracker:** top right of the screen, the active quest whose objective moved last: title, step text, progress `2 / 3` and, after the hint time without progress, the hint.
- **Markers:** a gold `v` over the target of the tracked step's `marker` (`tag:`, `object:`, `npc:`, `place:`; the nearest to the hero), or an arrow at the screen edge pointing the way. Settings -> Quest markers On/Off (`markers` in `settings.json`, 1 or 0, default 1).

## Testing quests: Play here and the debugger (US-186)

- **Play here:** in the Editor press P with the pointer over the map. The game starts with the hero at the pointer; Esc (or F2) returns to the Editor, which has not changed.
- **Debugger (Debug builds):** F10 opens a panel top right. Pick a quest, then Start, Complete (gives the rewards), Fail, Reset, or step back and forward; toggle the flags and add the items the quest looks at; change the nearest NPC opinion of the hero by 10; jump an hour, a day or a season; teleport to the marker. The game reflects it at once.
- **Why not?** The panel lists the conditions in the way of the next move (prerequisites, branches, failure rules), each with `[yes]` or `[no]` as of now, and the objective with its progress.

## The quest graph (US-184)

Editor -> **Quests** opens the quests of `assets/data/quests/` as a graph: a **quest** card (id, title, giver, journal, offer, turn-in), a **step** card per step, and wires. The quest card's output leads to the first step; each step's output leads to the next step, or to an **end** card (the quest is finished). A **branch** card (`+If`) has two outputs: the first goes to the step it belongs to (its Branches port), the second to the step it leads to. **Needs**, **Fails** and **Rewards** cards hold one line each and wire into the quest card's three inputs. Save writes the file in canonical form (your `//` comments at the top are kept, comments inside are dropped) and the positions to `<id>.quest.layout.json`; a quest with a mistake is not written and the status bar says what and where. The id stays the file name. Undo is shared with the map edits.

## The quest check (US-187)

The Quests tab lists what looks wrong under the canvas, live as you edit; a click selects the card. The game logs the same list when it loads, and a test runs it on every shipped quest (errors fail the build).

| Finding | Kind | Meaning |
|---|---|---|
| step never reached | warning | no step or branch leads to it; it loads fine |
| dead end / no END | error | from here the quest can never finish (a loop with no way out, or no step leads to `END`) |
| unknown item, interaction | error | an objective, reward or `has(...)` names something the game does not have |
| unknown person, place, kind | warning | a person, place or creature the loaded level does not have (people of a generated region are made at run time) |
| unknown quest | error | `quest(x)`, `step(x)` or a `quest ...` reward names a quest that does not exist |
| quests need each other | error | quest A requires B done, B requires A: neither can start (a prerequisite under `not` does not count) |
