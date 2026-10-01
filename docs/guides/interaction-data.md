# Interaction data guide

How to change what things in the world offer and do, without touching code (US-150, INT-01, INT-03, ADR-019).

Every action in the game is one **interaction**: a small text file in `assets/data/interactions/`, one file per interaction, named after its id (`gather.json` holds the interaction `gather`). Edit the files in any text editor. The game reads them when it starts, and a mistake is reported as `file:line: message` so you can fix it in seconds. A file with a mistake is skipped, the rest still load.

> Today `gather.json` and `inspect.json` ship, and the game still runs most of its actions from code. US-152 moves every built-in action into these files.

## Editing while the game runs: F5

Edit a file in any text editor, save it, and press **F5** in the game (Game or Editor mode). The game reads the whole folder again:

- No mistakes: the new data is live at once (well under a second), and the log says `Interactions reloaded: 2 from 2 file(s) in 3.1 ms`.
- Any mistake: nothing changes. The data you had stays in use, and a red panel at the top of the screen lists each mistake as `file:line: message` (the first eight; the log has all of them). Fix the file and press F5 again; the panel closes by itself.
- If a file has a mistake when the game *starts*, that file is left out, the others load, and the same panel shows until you fix it and press F5.

F5 reads the interaction files. Plants, animals, weapons and characters (the catalogs) are read only at start for now.

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
| `range` | no | How close the actor must be, in metres, up to 30. Default 1.5. Farther away the item is shown greyed out as "Too far away". |
| `duration` | no | Seconds the action takes, up to 600. 0 (the default) means instant. |
| `requires` | no | A list of conditions, each `{ "if": "...", "else": "why not" }`. The first one that is false greys the item out and shows its `else` text. |
| `effects` | yes | What happens when it finishes, one line per effect, in order. |
| `npc` | no | How clan members and animals choose it on their own: `score` (an expression; higher wins) and `cooldown` (seconds before they do it again). |
| `chronicle` | no | A line for the clan's chronicle, or `null`. Tokens like `{actor.name}` are allowed. |
| `order` | no | Menu position, 0 to 1000, lower first. Default 100. |
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
- When an entry has no `tags` or `states`, the game works them out from the entry's other fields, so older files keep working. A list you write replaces the worked-out one.

| Catalog | Worked-out tags | Worked-out states |
|---|---|---|
| plants | `plant`; `edible` if edible and walk-through; `fruit-bearing` if edible and solid (it is chopped, not gathered); `solid` if it blocks; `tree` if its size is tree | `ripe`, `picked` for an edible walk-through plant; none otherwise |
| animals | `animal`, and `hostile` (fights back) or `prey` | none |
| weapons | `item`, `weapon`, its class (`sword`, `bow`...), its element if any, `starter` | none |
| characters | `hero` and `person` for the hero, `person` for friendly people, `hostile` for enemies | none |

`actors` match in the same way: `hero`, `person`, `animal` or a kind name matches a thing that is that kind or carries that tag.

If an interaction targets a tag that no catalog uses, the game still loads it but warns at start: `interactions/warm.json:1: unknown tag "hearth": no catalog uses it, so this will never match`.

## Conditions and scores

A condition is a small sum that comes out true or false. A score is the same kind of sum that comes out as a number.

- Numbers are whole (no decimals); text is a word or "quoted words". `season != winter` compares the season with the word winter.
- Compare with `==  !=  <  <=  >  >=`. Combine with `and`, `or`, `not`. Calculate with `+  -  *  /` (division drops the remainder; dividing by zero gives 0).
- Order of work, first to last: brackets, minus sign, `*` `/`, `+` `-`, comparisons, `not`, `and`, `or`. So `1 + 2 * 3` is 7, and `1 or 0 and 0` is 1. Use brackets whenever you are unsure.
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
| `need(name)` | how full a need is, 0 to 100: need(hunger) |
| `skill(profession)` | the actor's skill in a profession: skill(hunter) |
| `trait(name)` | 1 when the actor has the trait, else 0: trait(diligent) |
| `opinion(a, b)` | what the first thinks of the second, -100 to 100: opinion(npc, hero) |
| `kin(a, b)` | 1 when the two are family: kin(npc, hero) |
| `flag(name)` | a note the story has set (0 when never set): flag(met-elder) |
| `tag(thing, name)` | 1 when a thing carries a tag: tag(target, edible) |

Examples in a requirement:

```jsonc
{ "if": "has(berries, 1)",                 "else": "You have no berries" },
{ "if": "opinion(npc, hero) >= -20 and not kin(npc, hero)", "else": "They will not trade with you" },
{ "if": "time == night or need(energy) < 20", "else": "Not tired yet" }
```

A mistake in a sum is reported with the line, for example `unknown function "hass"` or `has() takes 2 to 3 argument(s), not 1`.

## Effects

One line per effect. A line is a verb followed by arguments separated by spaces. An argument is one word, a number, "quoted text", a call such as `has(hero, berries, 1)`, or a (bracketed sum) when it has spaces: `give actor berries (1 + skill(gatherer))`. `+5` and `-5` are numbers.

| Verb | Meaning |
|---|---|
| `give` | put items in someone's hands: give actor berries 2 |
| `take` | remove items from someone: take hero berries 1 |
| `set` | change a state of a thing: set target.state picked |
| `flag` | set a story note (1 unless a value is given): flag met-elder |
| `opinion` | change what the first thinks of the second: opinion npc hero +5 |
| `remember` | give someone a memory with a feeling: remember npc "shared berries" 20 |
| `start` | begin another interaction: start inspect target |
| `talk` | open a conversation: talk elder-fire |
| `say` | show a speech bubble: say "Hello, {hero.name}" |
| `fx` | play a visual effect from effects.json: fx leaves |
| `sound` | play a sound: sound pop |
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
2. Too far away, or a failing `requires`: the item is shown greyed out with its reason.
3. When the actor starts it, the action runs for `duration` seconds, then the `effects` happen in order (US-153).
4. Clan members and animals score the interactions near them with `npc.score` and pick the best (US-154).

## Checks the build makes

Every shipped interaction file is loaded by the tests; a file with a mistake fails the build. Saving a file from the Editor and loading it again gives the same interaction.
