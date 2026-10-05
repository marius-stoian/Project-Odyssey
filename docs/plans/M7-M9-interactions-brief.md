# Build brief: M7 World interactions, M8 Speak to NPCs, M9 Interaction and dialogue editor

Mraw to Anima, 2026-10-01. Source of truth: requirements v2.0 (Round 13, D-34). Anima turns this brief into Codex v2.0 (K-M7..X-M9). These milestones are built **before** kill gate 2 (X-M6), which is held afterwards on the richer game.

## 1. Goal
The owner wants game entities to interact with each other, to speak to NPCs and to interact with the world, and to configure every action, interaction and conversation **offline in human-readable files** and **inside the game Editor**.

Today every world action is hard-coded in C++ (`RunFlow::openContext` in `src/game/run_flow.cpp`: Talk, Give berries, Ask to teach, Craft, Eat, Tend the fire...). "Talk" returns one line. The clan simulation's actions (`assets/data/sim/actions.json`) are only rates. These milestones replace both with one data-driven system.

## 2. Owner decisions (D-34, 2026-10-01, question round in chat)
| Question | Owner answer |
|---|---|
| Config format | **Plain-text dialogue scripts (`.dlg`) + JSON for actions and interactions** (comments allowed, one file per thing). No new library. |
| How talking works | **Hybrid**: hand-written dialogue trees with conditions and effects that read the simulation, plus generated small talk from each NPC's memories, gossip and needs when no written line fits. Every clan member can speak. |
| Editor depth | **Full visual graph editor** (nodes and wires) for dialogue and interaction rules inside the game, plus hot reload of files edited offline. |
| Order | **Build first, playtest after**: M7, M8, M9 now; kill gate 2 (X-M6) on the result. |

D-34 closes D-08 (interactions interview, OPEN-09) and defines INT-01.

## 3. Requirements added (v2.0)
- **INT-01 Interactions**: every thing in the world offers actions; the player, NPCs and animals perform them through one system (smart objects: properties + verbs, GD-04).
- **INT-02 Tags and smart objects**: catalogs give every kind tags (edible, wooden, flammable, person, animal, workstation...); an interaction targets tags, not kinds.
- **INT-03 Human-readable configs**: actions and interactions are JSON files in `assets/data/interactions/` (comments allowed), dialogue is `.dlg` text in `assets/data/dialogue/`; every error names file, line and field; the game keeps the last good version; F5 reloads while running.
- **INT-04 Timed actions and world state**: actions take time, can be interrupted, show progress and effects, and change the target's state (a bush is picked, a fire is lit), which is saved.
- **INT-05 Entities interact with each other**: clan NPCs and animals choose interactions from the same data (utility scores in the data), so the player sees them gather, tend the fire, greet, trade, graze and flee.
- **INT-06 Interaction editor**: the Editor shows interactions as a graph (actor, verb, target, conditions, effects) and edits them; saves to the same JSON.
- **SDC-03 Talk to NPCs**: hybrid conversation (D-34) in a dialogue panel; conversations change opinions, items, memories and flags.
- **SDC-04 Dialogue script format**: `.dlg` plain text, specified in `docs/guides/dialogue-format.md`.
- **SDC-05 NPCs talk to each other**: visible speech bubbles from the same scripts; outcomes feed the M2b social simulation (quarrels, courtship, sharing).
- **SDC-06 Dialogue graph editor**: nodes and wires, round-trips with hand-edited `.dlg` files.
- **ADR-019**: one condition and effect language shared by interactions and dialogue; JSON (+ comments) for rules, `.dlg` text for speech; the graph layout lives in a sidecar file so the text stays clean.

## 4. Formats (the contract the stories build to)

### 4.1 Interaction file (`assets/data/interactions/<id>.json`)
```jsonc
// Gather from a plant that is ripe.
{
  "id": "gather",
  "label": "Gather {target.name}",
  "actors": ["hero", "person"],              // hero, person, animal, or a kind name
  "target": { "tags": ["edible", "plant"] },
  "range": 1.5,                               // metres
  "duration": 3.0,                            // seconds; 0 = instant
  "requires": [
    { "if": "season != winter",       "else": "Nothing grows in winter" },
    { "if": "target.state == ripe",   "else": "Nothing to pick yet" }
  ],
  "effects": [
    "give actor berries 2",
    "set target.state picked",
    "after 15s set target.state ripe",
    "fx leaves"
  ],
  "npc": { "score": "need(hunger) * 2 + trait(diligent) * 10", "cooldown": 60 },
  "chronicle": null                           // or a line, e.g. "{actor} shared berries with {target}"
}
```
- **Conditions** (shared with dialogue): comparisons, `and`, `or`, `not`, and functions `has(item, n)`, `need(name)`, `skill(profession)`, `trait(name)`, `opinion(a, b)`, `kin(a, b)`, `flag(name)`, `tag(thing, name)`, `time`, `season`, `distance`.
- **Effects** (one verb per line): `give`, `take`, `set`, `flag`, `opinion`, `remember` (a sim memory with a feeling), `start` (another interaction), `talk` (open a dialogue), `say` (a bubble), `fx`, `sound`, `after <t>` (delayed effect), `chronicle`.
- Full reference: `docs/guides/interaction-data.md` (written in US-150, with examples for every verb).

### 4.2 Dialogue script (`assets/data/dialogue/<name>.dlg`)
```text
# The elder at the clan fire. Lines starting with # are notes.
@who elder                 # kind, role (elder, hunter, child...) or a placed character id
@when opinion(npc, hero) >= -20
@priority 10

=== start
Elder: The fire is low tonight, {hero}.   [if time == night]
Elder: You walk like a hunter today.
-> Ask about the hunt                      => hunt
-> Offer berries [if has(hero, berries, 1)] {take hero berries 1; opinion npc hero +5; remember npc "{hero} shared berries"} => thanks
-> Leave                                   => END

=== hunt
Elder: {smalltalk.hunt}
-> Back => start
```
- `=== id` starts a node; `Speaker: text` is a line (`[if ...]` makes it optional); `-> text [if ...] {effects} => node` is a choice; `END` closes the conversation; `{...}` in text is a token (names, needs, memories, `smalltalk.<topic>` from the generator).
- The parser reports `file:line: message`; the writer prints the same text back (canonical form), keeping `#` notes.
- The graph editor stores node positions in `<name>.dlg.layout.json` beside the script.

### 4.3 Things and tags
Catalog entries (plants, animals, characters, items, new world objects) gain `"tags": [...]` and, where they have state, `"states": [...]` with a default. Placed things in a level may override interactions and attach a dialogue (`"dialogue": "elder-fire"`).

## 5. Milestones and stories

### M7 World interactions (epic E14)
Exit: every action in today's game comes from data; the player and the NPCs act on things with timed, visible, saved results; files edited offline reload with F5.
| ID | Story | Size | Priority | Depends on |
|---|---|---|---|---|
| US-150 | Interaction data, the condition and effect language, validation and the reference guide | L | Must | US-060, D-34 |
| US-151 | Tags and states on every catalog; things advertise their interactions (smart objects) | M | Must | US-150 |
| US-152 | The context menu built from data; every hard-coded action moved to JSON with unchanged behaviour | L | Must | US-151, US-061 |
| US-153 | Timed actions: progress, interruption, effects, target state that is saved | M | Must | US-152 |
| US-154 | NPCs and animals choose interactions from the same data (utility scores) and are seen doing them | L | Must | US-153, US-030 |
| US-155 | New world objects for Age 1 (fire pit, knapping stone, store, shelter, flint nodule, water, sleeping furs) placeable in the Editor | M | Should | US-151 |
| US-156 | Hot reload (F5) and the validation panel: errors with file:line, last good data kept | M | Must | US-150 |

### M8 Speak to NPCs (epic E15)
Exit: the player talks to any clan member; written conversations branch on the simulation; without a script NPCs make small talk from their memories; talk changes opinions, items and memories; NPCs talk to each other in bubbles.
| ID | Story | Size | Priority | Depends on |
|---|---|---|---|---|
| US-160 | The `.dlg` format: parser with file:line errors, canonical writer, format guide | L | Must | US-150 |
| US-161 | Conversation runtime and the dialogue panel (name, mood, text, numbered choices; game paused) | L | Must | US-160 |
| US-162 | Who says what: selection by id, role, kind, opinion and priority; greetings and barks | M | Must | US-161 |
| US-163 | Generated small talk from memories, gossip, needs, the season and opinion | M | Must | US-161, US-012 |
| US-164 | Conversations are remembered: memories, gossip, chronicle lines, flags saved | M | Must | US-161 |
| US-165 | NPCs talk to each other in speech bubbles; outcomes feed quarrels, courtship and sharing | M | Should | US-162, US-154 |

### M9 Interaction and dialogue editor (epic E16)
Exit: the owner opens any dialogue or interaction in the Editor as a graph, edits it, test-plays it, saves it, and the file still reads well in a text editor.
| ID | Story | Size | Priority | Depends on |
|---|---|---|---|---|
| US-170 | Node-graph widget for the Luna UI: pan, zoom, nodes, ports, wires, selection, undo | L | Must | US-121, US-126 |
| US-171 | Dialogue graph editor: nodes, lines, choices, conditions, effects; saves `.dlg` + layout sidecar | L | Must | US-170, US-160 |
| US-172 | Interaction graph editor: actor, verb, target, condition and effect blocks; saves JSON | L | Must | US-170, US-150 |
| US-173 | Attach dialogues and interaction overrides to placed things in a level | M | Must | US-171, US-172 |
| US-174 | Test-play a conversation or interaction from the Editor with chosen conditions | M | Should | US-171, US-172 |
| US-175 | Graph validation: unreachable nodes, dead ends, unknown items, tags and nodes, listed and clickable | S | Must | US-171, US-172 |

Each milestone opens with K-M7/K-M8/K-M9 (design and test plan) and closes with X-M7/X-M8/X-M9 (exit review). Then X-M6, kill gate 2.

## 6. Architecture rules
- The condition and effect language, the interaction registry and the dialogue runtime live in the **Simulation** layer (`src/sim/`), with no Engine or SDL includes (ADR-016). The panels and the graph editor live in **Game** (`src/game/`); the node-graph widget in **Luna Engine** (game-agnostic).
- Deterministic: NPC choices use the simulation's seeded random; same seed and inputs give the same world hash (existing CI test extended).
- Things are referred to by id, never by pointer (as in the Editor, US-124).
- Existing behaviour is the regression baseline: US-152 must pass every M5 and M6 test unchanged.
- Data errors never crash the game: the last good data stays loaded, the panel shows what is wrong.

## 7. Definition of Done additions
- Every new data field is in the guide (`interaction-data.md`, `dialogue-format.md`) with an example.
- Every shipped JSON and `.dlg` file passes the validator in CI.
- A round-trip test: load, save, load gives the same data for every shipped file.

## 8. Risks
| Risk | Mitigation |
|---|---|
| The graph editor is the biggest UI work so far | US-170 alone first, tested headless; kill to "form editor" if it passes 4 weeks |
| Editor saves lose hand-written JSON `//` comments | `"note"` fields survive saves; the guide says so; `.dlg` notes are kept by the writer |
| Two sources of actions (sim rates, interactions) drift | US-154 moves the sim's action choice onto interactions; `actions.json` keeps only tuning numbers |
| Generated small talk reads robotic | Templates in data, at least three variants per topic, the owner reads 50 samples at X-M8 |

## 9. Open for Anima (TBD)
- Exact list of Age 1 world objects (US-155): Mraw proposes the seven above; the owner may change it at K-M7.
- Whether the dialogue panel pauses the game or slows it to 1/4 (Mraw default: pause).
