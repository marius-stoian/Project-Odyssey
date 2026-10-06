# Schemas: every data file described by data

Every file under `assets/data/` has a **schema** in `assets/data/schemas/`. A schema says what each field is: its type, its range, its choices, the catalog it points to, and a line of help text. Three things read it:

- **The loaders.** A value of the wrong type or out of range stops the load of that file, and the message names the file, the line, the field and what is allowed. Unknown fields only warn.
- **The schema test** (`tests/sim/schema_test.cpp`, "US-190 ..."). Every shipped file passes its schema, every link names an entry of a catalog, and every loader reads the same fields its schema describes.
- **The Data tab** of the Editor (US-191): the form of a file is built from its schema, and the help text of each field is the schema's `description` and `example`.

## What an error looks like
```text
assets/data/weapons.json:23: weapons[2].damage: must be between 0 and 1000 (is -4)
assets/data/sim/needs.json:3: maximum: must be between 10 and 1000 (is 5)
assets/data/interactions/gather.json:9: range: must be a number (is text)
```
The line is the line of the field. `weapons[2].damage` is the path: the third weapon, its damage. A link to something that does not exist is listed by the test the same way: `assets/data/hero/recipes.json:9: recipes[2].output: "no-such-item" is not in the catalog "items"`.

## Where the schemas live
| File | What it is |
|---|---|
| `schemas/index.json` | Which schema covers which file: a list of `{ "match": "sim/needs.json", "schema": "sim-needs" }`. A `*` stands for any run of letters but a slash (`interactions/*.json`); the first match wins. A text file (`dialogue/*.dlg`) is `{ "match": ..., "text": true }`: its own parser reads it. |
| `schemas/<name>.schema.json` | One schema. A file that exists once (`weapons.json`) has its own; the folders that hold one file per entry (`interactions`, `npc-classes`, `npcs`, `quests`, `story/events`, `buildings/prefabs`) have one schema for the whole folder. |

A file with no entry in `index.json` fails the test ("has no schema").

## The keys of a schema
A schema is JSON (comments allowed) with these keys on any node:

| Key | Meaning | Example |
|---|---|---|
| `type` | `object`, `array`, `string`, `number`, `integer` or `boolean` | `"type": "integer"` |
| `properties` | the named fields of an object, in the order the form shows them | `"properties": { "hp": { ... } }` |
| `additionalProperties` | a schema for every member not named (a map keyed by id), or `true` for "anything" | `"additionalProperties": { "type": "integer" }` |
| `items` | the schema of each element of an array | `"items": { "type": "string" }` |
| `required` | names of fields that must be there | `"required": ["id", "title"]` |
| `minimum`, `maximum` | the range of a number | `"minimum": 0, "maximum": 1000` |
| `minItems`, `maxItems`, `maxLength` | sizes | `"maxLength": 32` |
| `enum` | the allowed values | `"enum": ["small", "tall", "tree"]` |
| `default` | the value when the file leaves it out | `"default": 3` |
| `description` | the help text: what the field is for (**every field needs one**) | `"description": "Hit points"` |
| `example` | one value as it would be typed | `"example": "iron sword"` |
| `ref` | `"catalog:<name>"`: the value is the name of an entry of that catalog; the form shows a picker and the test checks the link | `"ref": "catalog:items"` |
| `keyRef` | on a map: `"catalog:<name>"`: every key of the map is the name of an entry of that catalog (the items of a recipe `{ "flint": 2 }`); the test checks the links and a rename changes the keys | `"keyRef": "catalog:items"` |
| `format` | how a text is read: `rule` (a rule-language condition), `effect` (an effect line), `colour` (`#rrggbb`), `frame` (an atlas frame), `time` (`HH:MM`) | `"format": "colour"` |

At the root of a schema also:

| Key | Meaning |
|---|---|
| `title` | the path of the file, for people |
| `provides` | which values of the file make up a catalog: `{ "catalog": "items", "at": "items[].id" }` (the `id` of every element of `items`), `"at": "materials.*"` (the member names), `"at": "id"` (a value at the top). A file may provide several catalogs; many files may provide one (`tags`). |
| `loaders` | the source files (and functions: `src/game/level.cpp#loadDefinitions,readTags`) that read the file; the drift test compares their fields with the schema |
| `externalFields` | names the loaders read in a way the drift test cannot see (a table of names such as the seven affinities); keep it short |

**Note fields.** A member called `note` or `comment`, or one that starts with `_` or `//`, is always accepted: it is for you, and the game ignores it. Any other member the schema does not know is a warning at load ("is not a field this file knows") and a failure in the test: it is probably a typo.

**Free-form objects.** An object schema with no `properties` and no `additionalProperties` accepts any members (for example a topic of `smalltalk.json` holds sentences or `{ "text", "mood" }`).

## Adding a field or a file
1. A new field in a file the game already reads: add it to the schema with a `description` and an `example` (and a `minimum`/`maximum` or `enum` where the loader has one). Run `odysseus_sim_tests "-tc=US-190*"`: the drift test names a field the loader reads and the schema lacks, and the coverage test names a value that does not fit.
2. A new kind of file: write `<name>.schema.json`, add its line to `index.json`, and name the loader in `loaders`. The test fails until all three are there.
3. A new catalog: add `provides` to the schema of the file that holds the entries; any other schema may then say `"ref": "catalog:<name>"`.

## What the loaders do with it
The game and `odysseus_headless` install the schemas once at start (`sim::schema::installFromFolder`). After that, `readJsonFile` and every reader of the rule files (interactions, NPC classes and kinds, quests, buildings, partner defaults, small talk) check each file as they read it:
- a type, range, choice or missing-field mistake makes the loader refuse that file with the message above (the others still load; a rule file with a mistake is left out like one with any other mistake);
- an unknown field is a warning in the log, once per file;
- no schemas installed (a unit test reading a temporary file), or a file outside `assets/data/`: nothing is checked.

A broken `schemas/` folder is said in the log and never stops the game: the loaders then check what they always checked.

## The files and their schemas
| Data | Schema |
|---|---|
| `weapons.json`, `plants.json`, `objects.json`, `animals.json`, `effects.json`, `weather.json`, `characters.json`, `tiles.json`, `materials.json` | the same name |
| `sim/<file>.json` (actions, calendar, clan, events, life, names, needs, opinions, partner-types, region, schedule, social, story, trade) | `sim-<file>` |
| `hero/<file>.json` (hero, items, professions, recipes, activities, tutorial) | `hero-<file>` |
| `light/<file>.json` (lights, sky, celestial-events) | `light-<file>` |
| `buildings/kinds.json`, `pieces.json`, `prefabs/*.json` | `building-kinds`, `building-pieces`, `building-prefab` |
| `interactions/*.json`, `interactions/defaults-*.json` | `interaction`, `interaction-defaults` |
| `npc-classes/*.json`, `npcs/*.json` | `npc-class`, `npc-kind` |
| `quests/*.json`, `story/events/*.json` | `quest`, `story-event` |
| `dialogue/smalltalk.json`, `dialogue/*.dlg` | `smalltalk`; the `.dlg` files are text with their own parser |
| `editor/help.json` | `editor-help` |

## For the owner learning C++
`src/sim/schema.cpp` is a small interpreter: it walks a document and a schema together, and `check` is one function that calls itself for each child. `src/sim/json_text.cpp` keeps a second, simpler reader that only remembers which line each value is on, because the parsed document forgets that.
