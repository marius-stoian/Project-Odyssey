# Build brief: M9a NPC foundation, M9b Trade economy, M9c NPC life

Status: **Handed to Anima (Codex v2.10).** Owner decisions: D-52 (docs/decision-requests/D-52.md, Decided 2026-10-04). Written by Dominus (Mraw) at the owner's request, 2026-10-04. Builders follow the Codex prompts, not this brief.

## 1. Goal

The player talks to, confronts, trades with and uses actions on **every** NPC, including characters placed in a level. Placed NPCs are full persons of the simulation. The owner defines NPC Classes in the Editor and sets every NPC by hand: classes, attitude, dialogues, actions, trade and daily schedule. NPCs do the same things to each other. The simulation scales to 100,000 region-loaded NPCs by simulating in detail only near the hero.

Source of truth: Project Odyssey.docx v2.10 (Round 23) holds epics E26 (M9a), E27 (M9b), E28 (M9c), the stories below and requirements SDC-08..SDC-12, INT-09..INT-12, EDT-08, NFR-08, MVP-17.

## 2. What exists today (2026-10-04)

| Thing | Today | Where |
|---|---|---|
| Interactions | One JSON file per action, targeted by tags, right-click menu, `range`, `requires`, `duration`, `effects`, `npc` score | `docs/guides/interaction-data.md`, `src/sim/interaction.*` |
| Talk | `talk.json` targets tag `person`: only simulated clan members. `.dlg` scripts or generated small talk | `src/game/builtin_actions.cpp`, `assets/data/dialogue/` |
| Trade | `barter.json` targets tag `rival`: only rival camps. Barter screen with give and want lists, counter-offer, pay later | `RunFlow::openBarter`, `src/sim/rivals.*` |
| Placed characters | Level `characters` (kind, name, hp, swordDamage, facing); at run time combat figures (`Enemy`), subject kind `Animal`, one switch `enemy`. **No talk, no trade** | `assets/data/characters.json`, `src/game/game_rules.cpp` |
| Simulation | Clan of dozens of persons, every one simulated every day | `src/sim/` |
| Planned | M9 node-graph editors (US-170..US-175), M10 Quests | `docs/Codex.md` |

## 3. Owner decisions (D-52, summary)

- **Order:** right after the current milestone M8c; before M8d. Three milestones: M9a, M9b, M9c.
- **NPC Class** (one concept for "category" and "class"): the owner creates, edits, deletes them in the Editor. Each has a colour, an icon, default dialogues and default actions. An NPC has **one or more** classes.
- **Kind defaults:** one file per kind, `assets/data/npcs/<kind>.json`. Every NPC is then set by hand in the Editor.
- **Full persons:** placed NPCs join the simulation (needs, memories, ageing, families).
- **Scale:** up to 100,000 region-loaded NPCs; detail by distance; a compact store; relationships only for pairs that met.
- **Attitudes:** friendly, neutral, wary, hostile, scared, suspicious, enchanted, lovingly, enviously. Each NPC has its own attitude to each entity it knows (no factions). Changed by dialogue quality and frequency, gifts, trade, help events, a marriage in the family, being in the same family.
- **Talk:** dialogues per NPC, by partner type (Player, NPC Classes, Animals, Environment; the list grows later). An NPC with no script has **no Talk option**.
- **Confront:** a separate button for taunt, insult, ask for peace, antagonise, de-escalate; works on hostile NPCs too; changes the opinions of everyone involved.
- **Actions:** tags give defaults, allow and deny lists fine-tune. Denied actions are **hidden**; an **Actions pop-up** lists every action of that NPC with its requirements.
- **Trade:** barter and currency; which items are currency is set per region in the Editor; supply and demand prices; reputation gates prices and access; limited stock, daily restock; wants at full value, other goods at half.
- **Life:** full day and night schedules; NPC Class actions, Quest actions, Custom actions, Event actions, all editable in the Editor; NPCs do everything to each other.
- **Editor:** forms first (the graph editor stays in M9); markers are a colour ring plus a small icon; F5 reloads NPC data.
- **Test level:** new `assets/levels/npc-test.json` with 7 NPCs: trader, talker, wary hunter, elder, guard, goblin, deer. `valley.json` is never touched.

## 4. Data model (proposal for the architect; K-M9a design doc fixes it)

```jsonc
// assets/data/npc-classes/trader.json  (US-260, written by the Editor)
{ "id": "trader", "label": "Trader", "colour": "#d9a441", "icon": "coin",
  "tags": ["trader"],
  "dialogues": { "player": "trader-greet.dlg", "class:guard": "trader-guard.dlg" },
  "actions": { "allow": ["trade", "talk", "give-gift"], "deny": [] } }

// assets/data/npcs/wanderer.json  (US-261: defaults for a kind)
{ "kind": "wanderer", "classes": ["wanderer"], "attitude": "neutral" }

// in the level, a placed NPC (US-261, US-268): only what differs
{ "id": 3, "kind": "wanderer", "name": "Ossa", "x": 1067, "y": 973,
  "classes": ["trader", "elder"], "attitude": "friendly",
  "dialogues": { "player": "ossa.dlg" },
  "actions": { "deny": ["ask-to-teach"] },
  "trade": { "stock": { "flint": 6, "furs": 2 }, "wants": ["berries"], "restockPerDay": { "flint": 2 } },   // M9b
  "schedule": [ { "from": "06:00", "do": "work", "at": "market" }, { "from": "21:00", "do": "sleep", "at": "home" } ] } // M9c
```

- Precedence: class defaults, then kind file, then the placed NPC. Allow and deny lists merge in that order; a later deny wins.
- Persons: one record per NPC in a struct-of-arrays store (id, kind, classes bitset, family, needs, position, detail level); opinions in a sparse map keyed by (holder, target) pair, created on first meeting. Integers only, deterministic, saved.
- Detail by distance: "near" (full ticks), "far" (one coarse summary per in-game day). The threshold and budget are an ADR (ADR-022).

## 5. Stories

### M9a NPC foundation (epic E26)
Exit: every placed NPC is a person with classes, an attitude and opinions; the player talks to and confronts them, sees their actions in a pop-up; the owner edits classes, kinds and NPCs in the Editor; the test level shows all of it; 100,000 persons load and run within budget.

| Id | Story | Size |
|---|---|---|
| US-260 | NPC Classes: create, edit, delete in the Editor; colour, icon, tags, default dialogues and actions | M |
| US-261 | Kind defaults and placed-NPC overrides; precedence; F5 reload | M |
| US-262 | Placed NPCs are full persons (needs, memories, ageing, family) | L |
| US-263 | The NPC store and detail by distance; a 100,000-person load test; ADR-022 | L |
| US-264 | Attitudes and opinions: nine words, per pair, changed by the D-52 events, saved | M |
| US-265 | Talk with placed NPCs: dialogues by partner type; no script, no Talk | M |
| US-266 | Confront: taunt, insult, ask for peace, antagonise, de-escalate, with consequences | M |
| US-267 | Actions: tags plus allow and deny; denied hidden; the Actions pop-up with requirements | S |
| US-268 | Editor NPC panel: classes, attitude, dialogues by partner type, actions | L |
| US-269 | Editor kinds tab and map markers (colour ring and icon) | M |
| US-270 | NPC test level, 7 NPCs, walk-through checklist | S |

### M9b Trade economy (epic E27)
Exit: the player trades with any trader by barter or the region's currency; prices follow supply and demand and the trader's opinion; stock is limited and restocks daily; the owner sets currencies and stock in the Editor.

| Id | Story | Size |
|---|---|---|
| US-280 | Currencies per region, set in the Editor | S |
| US-281 | Trader stock: limited, daily restock, wants at full value, other goods at half; saved | M |
| US-282 | Supply and demand prices; reputation gates prices and access | M |
| US-283 | Trade screen for any trader (barter and currency); rival camps keep working | L |
| US-284 | Editor trade panel; the test level's trader and wary hunter set up | M |

### M9c NPC life (epic E28)
Exit: NPCs follow day and night schedules, do their class, custom and event actions, and do everything to each other (talk, fight, trade, gift, confront) with the same files; the owner edits all of it in the Editor.

| Id | Story | Size |
|---|---|---|
| US-290 | Day and night schedules: data and the Editor schedule form | L |
| US-291 | Action sources: class, custom and event actions; a quest-action hook that M10 fills | M |
| US-292 | NPCs act on each other with every interaction | L |
| US-293 | Default interactions by partner type: NPC Classes, Animals, Environment (list can grow) | M |
| US-294 | Living test level: schedules, NPC-to-NPC trade and talk; 100,000-person soak test | S |

## 6. Architecture notes
- Persons, opinions, stock, prices and schedules are **Simulation** (deterministic, integers, saved). Editor forms, menus, the Actions pop-up and markers are **Game**. Nothing new touches SDL.
- One subject path for every NPC: clan members, placed NPCs and rival camps share tags and menus; no per-NPC special cases in `builtin_actions.cpp`.
- 100,000 persons: no per-person heap objects in hot loops; no O(n^2) scans (spatial grid for "who is near"); opinions only for pairs that met.

## 7. Risks
- **Scale (high).** Full persons at 100,000 is the largest technical risk in the project. US-263 comes early with a load test; if it misses its budget, the owner decides between lower counts and coarser far detail.
- **Quest actions** need M10 (Quests), built later. M9c only defines the hook; M10 fills it.
- **Rival barter** must keep working when the screen is generalised (US-283).
- **Order:** M9a-M9c delay M8d and M8e (buildings) by their size, as the owner chose.

## 8. Questions answered
All in docs/decision-requests/D-52.md (Q-01..Q-24, S-01..S-04).
