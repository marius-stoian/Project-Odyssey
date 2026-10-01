# Build brief: M10 Quests, M11 Data editors, M12 World editing, M13 Politics

Mraw to Anima, 2026-10-01. Source of truth: requirements v2.1 (Round 14, D-40). Anima turns this brief into Codex v2.1 (P-010, K-M10..X-M13). Kill gate 2 (X-M6) moves **after M13**.

## 1. Goal
The owner wants the Editor to be the game's authoring tool: **story and quests, entities, world, mechanics, functions and their configuration**, all set up in the Editor and all saved as readable files he can also edit offline. Builds on M7 (shared rule language, interactions, hot reload), M8 (dialogue) and M9 (node-graph widget, attach to placed things, test-play, validation).

## 2. Owner decisions (D-40, five chat rounds, 2026-10-01)
| Topic | Owner answer |
|---|---|
| Packaging | Split into **M10 Quests and story authoring, M11 Data editors, M12 World editing**, plus **M13 Politics** (own milestone, after the editors). |
| Quests | **Authored only** (no generated quests). **One JSON file per quest + quest graph editor.** **Flat quests with prerequisites** (no chapters). **Journal + tracker + markers** (markers can be turned off). |
| Old story content | **The elder tutorial becomes the first quest**; crossroads events become story events edited in the Editor; tuning files are edited with forms. |
| Data editing | **Schema-driven forms** for every data file. |
| Region | **The procedural region edited in the Editor**: **all generator settings with a live preview**; hand edits on top of the seed: **terrain and biomes, water and mountains, things and people, camps and resources**, and per clan and person **social, economic, political and day-to-day setup, available actions and properties**. |
| Economy | **Values, stores and debts** with today's Trade mechanics (no new economy simulation). |
| Daily life | **Routines as data** per role and per person, guiding the utility AI. |
| Overrides | **Per kind** (data editor) **and per placed thing** (world). |
| Functions | **Game Rules page** with on/off switches for whole systems. |
| Testing tools | **Play-in-editor debugger.** |
| Politics | **Brought into the MVP as the third pillar with a victory.** Mechanics: **clan alliances and vassal oaths, elders' council, marriage ties, leadership challenges**, and **"economical and technological"** levers. |
| Playtest | **Kill gate 2 after M13.** |

**Dominus's reading, for the owner to confirm at K-M13:** "economical and technological" political levers = trade pacts, tribute in goods, embargoes, and sharing crafts, recipes and profession know-how with allies (Age 1), **not** the Technology pillar or its Mythic Strands, which stay cut. Political victory uses the same rule shape as Trade and Religion: vassals hold 60% of the region's people, or 50% combined over the three pillars, tunable in Game Rules.

## 3. Requirements added or changed (v2.1)
STO-04 authored quests, STO-05 story content in one place, EDT-01..EDT-06 (new prefix EDT: Editor as authoring tool, schema-driven editing, Game Rules, region editing, social/economic/political/daily world setup, play-in-editor debugger), SDC-07 daily routines, PIL-08 Politics in the MVP, MVP-08 (three pillars), MVP-09 (Politics victory), MVP-15 (authoring), ADR-020 (schemas and seed overrides), cut list (Politics leaves it), kill criteria (gate after M13).

## 4. Formats (the contract)

### 4.1 Quest (`assets/data/quests/<id>.json`)
```jsonc
// The elder's first lesson (was tutorial.json).
{
  "id": "first-day",
  "title": "The First Day",
  "giver": "role:elder",                         // person id, role:<role> or none (auto-start)
  "requires": ["not quest(first-day) == done"],  // prerequisites, shared rule language
  "start": "gather",
  "steps": {
    "gather": { "text": "Gather berries for the clan.",
                "objective": "gather berries 3", "marker": "tag:edible",
                "hint": { "after": "120s", "text": "Berry bushes grow by the stream." },
                "next": "eat" },
    "eat":    { "text": "Eat at the clan fire.", "objective": "interact eat-berries", "marker": "object:clan-fire", "next": "fire" },
    "fire":   { "text": "Keep the fire alive.",  "objective": "interact tend-fire",  "next": "END",
                "branches": [ { "if": "flag(fire-out)", "to": "relight" } ] }
  },
  "fail": ["hero.dead"],
  "rewards": ["opinion role:elder hero +10", "give hero flint 2"],
  "journal": "The elder taught me the first things a clan needs."
}
```
Objectives: `talk <who>`, `goto <place>`, `gather|give|craft <item> <n>`, `interact <interaction>`, `defeat <kind> <n>`, `wait <time>`, `flag <name>`. Dialogue gains `{quest start|complete|fail <id>}` effects and `quest(<id>)`, `step(<id>)` conditions. Graph layout in `<id>.quest.layout.json`.

### 4.2 Story event (`assets/data/story/events/<id>.json`)
Crossroads events in the same shape they have today (title, text, age range, role, options with affinity and effects), plus a `trigger` condition, edited in the Editor.

### 4.3 Schema (`assets/data/schemas/<file>.schema.json`)
A small JSON Schema subset: `type`, `properties`, `items`, `minimum`/`maximum`, `enum`, `default`, `description` (help text in the form), and `ref` (`"ref": "catalog:items"` turns a field into a picker and a link check). One schema per data file; CI checks every file against its schema and every schema against its loader's fields.

### 4.4 Game Rules (`assets/data/rules/<name>.json`)
New Game presets, comfort, victory thresholds and `systems: { weather, combat, rivals, tutorial, markers, chronicle, politics: true|false }`. A new game or a level names the rules file (default `standard.json`).

### 4.5 Routines (`assets/data/sim/routines.json`)
Per role, and per person in the world: day blocks `{ from, to, prefer: [interaction tags], weight }`. Needs below their danger level always win.

### 4.6 Region world file (`assets/worlds/<name>.json`)
`{ seed, generator: {...all settings...}, overrides: { chunks: { "<cx>,<cy>": [tile changes] }, things: [...], people: [...], places: [...], camps: [...], resources: [...] }, clans: { "<clan>": { leader, members, stance, store, debts, partners } }, people: { "<id>": { kin, opinions, grudges, items, routine, actions, properties } } }`. Only differences from the seed are stored.

### 4.7 Politics (`assets/data/sim/politics.json`)
Stance rules (events and their stance changes), alliance and oath thresholds, tribute amounts, oath-break chance, council voting weights, marriage-tie bonus, challenge thresholds, lever effects; all editable in M11 forms.

## 5. Milestones and stories

### M10 Quests and story authoring (E17)
| ID | Story | Size | Priority | Depends on |
|---|---|---|---|---|
| US-180 | Quest data and runtime | L | Must | US-150, US-164 |
| US-181 | Objectives from world events | M | Must | US-180 |
| US-182 | Getting and handing in quests | M | Must | US-180, US-161 |
| US-183 | Journal, tracker and markers | M | Must | US-180 |
| US-186 | Play-in-editor debugger | M | Must | US-180 |
| US-184 | Quest graph editor | L | Must | US-180, US-170 |
| US-187 | Quest validation | S | Must | US-184 |
| US-185 | The tutorial as a quest; story events | M | Must | US-184, US-090 |

### M11 Data editors (E18)
| ID | Story | Size | Priority | Depends on |
|---|---|---|---|---|
| US-190 | Schemas for every data file | L | Must | US-150 |
| US-191 | Schema-driven form editor | L | Must | US-190 |
| US-193 | Entity editor | M | Must | US-191, US-172 |
| US-194 | Mechanics and story tuning with a quick check | M | Must | US-191 |
| US-195 | Game Rules page | M | Must | US-191 |
| US-196 | Daily routines as data | M | Must | US-154, US-191 |
| US-192 | Picture pickers and cutting frames | M | Should | US-191, US-120 |

### M12 World editing (E19)
| ID | Story | Size | Priority | Depends on |
|---|---|---|---|---|
| US-200 | The region in the Editor | L | Must | US-040, US-123 |
| US-201 | Generator settings with live preview | M | Must | US-200, US-191 |
| US-202 | Hand edits on top of the seed | L | Must | US-200 |
| US-203 | Water and mountains | M | Must | US-202 |
| US-204 | Things, people and places in the region | M | Must | US-202, US-155 |
| US-205 | Camps and resources | M | Must | US-202, US-041 |
| US-206 | Clans and people inspector | L | Must | US-204, US-196, US-173 |
| US-207 | Play the edited region | S | Must | US-206, US-186 |

### M13 Politics (E20)
| ID | Story | Size | Priority | Depends on |
|---|---|---|---|---|
| US-210 | The political model | L | Must | US-206, US-195 |
| US-211 | Alliances and vassal oaths | L | Must | US-210 |
| US-212 | Elders' council | M | Must | US-210 |
| US-213 | Marriage ties | M | Must | US-210, US-113 |
| US-214 | Leadership challenges | M | Must | US-212 |
| US-215 | Economic and technological levers | M | Must | US-211, US-062 |
| US-216 | Political victory and diplomacy | M | Must | US-211, US-195, US-161 |

Acceptance criteria (three Gherkin scenarios each) are in requirements v2.1, section 12.9, and in the backlog workbook. Estimates (opt / likely / pess weeks): M10 9/13/18, M11 9/13/18, M12 10/15/21, M13 9/13/18.

## 6. Architecture rules
- Quest runtime, schemas and their validator, routines, region overrides and politics live in the **Simulation** layer (headless, deterministic, saved); forms, graphs, the region view and the debugger live in **Game**; any new widget (form fields, minimap) goes to **Luna Engine**, game-agnostic.
- One rule language (ADR-019) for quests, dialogue, interactions, story events, routines and politics.
- One validator (ADR-020) used at load, in CI and in the Editor; errors name file, line and field; bad data never crashes the game.
- Region edits are overrides on the seed (ADR-020, ADR-010): small files, regeneration keeps them, conflicts are listed.
- The debugger and Play here are Editor tools; they never write to saves or data unless the owner saves.
- Every save, level and world format change bumps its version with a migration.

## 7. Definition of Done additions
Every new data file has a schema and a guide page (`docs/guides/quests.md`, `schemas.md`, `world-editing.md`, `politics.md`); every shipped file passes the validator in CI; round-trip tests for every format; M5-M9 behaviour is the regression baseline.

## 8. Council notes and risks
| Risk | Mitigation |
|---|---|
| Politics re-enters the MVP: the largest scope change since M1b, and it adds a third victory to balance | M13 comes last, on top of editable rules; the victory thresholds live in Game Rules; a headless balance check (US-194) runs 10 seeds per pillar |
| Kill gate 2 moves after M13: the playtest slips by about a year at the plan's pace, and five milestones go untested by players | At each exit review (X-M10..X-M13) the owner plays 30 minutes and writes one line on fun; Mraw keeps the M6 package building so a playtest can be pulled forward at any time |
| Region editing at 256 x 256 tiles in the Editor must stay smooth | Chunk streaming and a cached minimap (US-200); performance budget in the design document |
| Schemas drift from the loaders | CI test compares each schema with its loader's fields |
| Six data systems (quests, events, schemas, rules, routines, politics) can overwhelm the owner | One Data tab, one validator, guide pages, help text from schemas |

## 9. Open for Anima (TBD)
- The "economical and technological" reading in section 2 (confirm with the owner at K-M13).
- The default rules file name and whether levels may carry their own rules (Mraw default: `standard.json`; levels may name one).
