# M9a-M9c design: NPC classes, persons, talk, confront, trade, life

Written at K-M9a (2026-10-04). Owner decisions: D-52 (docs/decision-requests/D-52.md) and the K-M9a answers below. Brief: docs/plans/M9a-npc-roles-brief.md. A format change is a design question for the owner.

## K-M9a owner answers (2026-10-04)
| Question | Answer |
|---|---|
| Confront key | C |
| Actions pop-up key | X |
| Class icons | A small built-in icon set (about 24 pixel icons); the owner picks one per class in the Editor |
| Test cadence | Tests are written per story and compile; the full verify (Debug and Release) runs once at X-M9a. Rerun only failing cases after a failure |

CI-012 is resolved: requirements v2.10 hold E26-E28 and US-260..US-294 (the Codex still says E17-E19 in CI-012's text; the ids are E26-E28).

## 1. Formats (fixed here; a change is a design question)
- `assets/data/npc-classes/<id>.json`: `id`, `label`, `colour` ("#rrggbb"), `icon` (name from the icon set), `tags` (list), `dialogues` (map partner type to .dlg file), `actions` {`allow`, `deny`} (lists of interaction ids). Keys written in this order.
- `assets/data/npcs/<kind>.json`: `kind`, `classes` (list of class ids), `attitude` (one of the nine words), optional `dialogues`, `actions`.
- Placed NPC in a level (level version bump when US-261 adds it): `id`, `kind`, `name`, `x`, `y`, optional `classes`, `attitude`, `dialogues`, `actions`; only differences from the kind.
- Partner types for dialogues: `player`, `class:<id>`, `animal`, `environment`.
- Precedence: class defaults, then kind file, then the placed NPC. Allow lists and deny lists merge in that order; a later deny wins.
- Attitude words: friendly, neutral, wary, hostile, scared, suspicious, enchanted, lovingly, enviously, derived from an integer opinion per pair (thresholds in US-264).

## 2. Layers
Simulation (src/sim, deterministic, integers, saved): class catalog, person store, opinions, detail by distance. Game (src/game): Editor tabs and forms, markers, Actions pop-up, Confront menu. Nothing new touches SDL.

## 3. Person store and ADR-022 outline (written in US-263)
Struct-of-arrays store, no per-person heap objects in hot loops, a spatial grid for "who is near", opinions in a sparse map keyed by (holder, target) created on first meeting. Detail by distance: near the hero full ticks, far one coarse summary per in-game day. Budget set by the 100,000-person load test; a miss goes to the owner as a decision request.

## 4. Order and tests
US-260, 261, 262, 263 (load test early), 264, 265, 266, 267, 268, 269, 270, X-M9a. Test level: assets/levels/npc-test.json (7 NPCs: trader, talker, wary hunter, elder, guard, goblin, deer). `assets/levels/valley.json` is never touched.

## M9b: trade economy (written at K-M9b, 2026-10-05; owner answers D-54 Q1-Q8)

### Formats (fixed here; a change is a design question)
- **Region economy** in the level (level version 5), written only when it holds something: `"economy": { "currencies": { "shells": 1 }, "prices": { "flint": 4 }, "resources": { "flint": 3, "berries": 5 } }`. `currencies`: item id to value (a whole number from 1 to 100000). `prices`: the region market's base price of a good in value units, overriding the item's own `value` (items.json). `resources`: delivery weights of the region's goods, added to a trader's own weights at the daily restock (D-54 Q6). A level with no currency trades by barter only.
- **Trade profile** (a `trade` block of a class file, a kind file or a placed NPC; layers merge class, then kind, then the NPC; maps merge per key, a later layer wins, `wants` is the union): `{ "stock": { "flint": 6 }, "restockPerDay": { "flint": 2 }, "deliveries": 2, "weights": { "fur": 3, "flint": 5 }, "wants": ["berries"], "rare": { "obsidian": "friendly" } }`. Written in this order. `stock` is the starting stock and the target of the stock-ratio curve; `restockPerDay` is delivered every day as written; `deliveries` is the number of weighted random picks a day (each adds one piece), drawn from `weights` plus the region's `resources`; `wants` are bought at full value, other goods at half (D-52 Q-17); `rare` names goods that are only offered at an opinion at least as high as the band word (devoted goods: `enchanted`).
- **Trade settings** `assets/data/sim/trade.json`: the stock curve, the drift, the reputation bands, the want percentages, the haggle odds, the stock cap. Whole numbers only.
- **Saved state**: `trade.json` next to `npcs.json` in the save folder: per trader the stock, the drift per good, the day of the last restock and the day of the last haggle, plus the hero's balance.

### Prices (ADR-023)
All in value units, integers, percents:
1. `base` = the region's price of the good (`prices`, else the item's `value`, at least 1).
2. Stock ratio: `ratio = clamp(100 * target / max(stock, 1), 50, 200)`; the target is the profile's starting stock for the good (a good the profile does not stock has target `defaultTarget`). Stock at the target gives 100, an empty shelf 200, a shelf at twice the target 50.
3. Drift: each trade of the good moves the trader's drift by `percentPerTrade` (the hero buying raises it, the hero selling lowers it, per unit, clamped to +-`maxPercent`); every day it falls back toward 0 by `decayPercentPerDay` of itself (at least one point).
4. `market = base * ratio / 100 * (100 + drift) / 100` (round half up, at least 1).
5. Reputation last: the trader's attitude to the hero decides a percent (`hostile` refuses to trade, `wary` and `suspicious` +25, `friendly` -10, `enchanted` and `lovingly` -20, the rest 0). The hero pays `market * (100 + percent) / 100`; the trader pays the hero `market * (100 - percent) / 100`, and then times the want percentage (100 for a want, 50 for any other good).
6. Barter is accepted when the trader's received value is at least the value it gives, both at its prices. Currency is a good worth its item value, never repriced; the balance bar is received minus given.
7. Haggle: one try per trader per in-game day. The chance is `clamp(base + opinion / opinionDivisor + persuasion * perPersuasion, min, max)` percent against a roll of the world seed, the trader id and the day; success gives -`discountPercent` on what the hero pays and +`discountPercent` on what the trader pays for the rest of that day; failure costs the opinion `failureOpinion`.

### Money (D-54 Q1, Q2)
Coin items of the region's currencies in the hero's bag become a balance when the trade screen opens; the unspent balance goes back as coin when it closes, greedily from the highest value (a remainder below the smallest coin stays in the balance for next time). The balance is kept in `trade.json`.

### Order and tests
US-280 currencies, US-281 stock and restock, US-282 prices and reputation, US-283 the screen, US-284 the Editor panel. Tests: tests/sim/economy_test.cpp, trade_market_test.cpp, trade_price_test.cpp; tests/game/economy_editor_test.cpp, trade_screen_test.cpp, trade_editor_test.cpp.

## M9c: NPC life (written at K-M9c, 2026-10-05; owner answers D-54 Q9-Q16)

### Formats
- **Places** in the level (level version 5): `"places": [ { "name": "market", "x": 640, "y": 320, "tags": ["trade"] } ]` (world pixels). `home` is built in: the spot where the NPC was placed. A place may carry tags (forage, shelter, water, shrine) so the environment interactions have targets.
- **Schedule** (a `schedule` block of a class, kind or placed NPC; the whole block of the highest layer that has one wins): a list of blocks `{ "from": "06:00", "do": "work", "at": "market" }`, or `{ "day": [...], "night": [...] }` when the night differs. A block lasts until the next block begins; the day wraps. `do` is an activity word (`work`, `eat`, `sleep`, `rest`, `idle`, `go`, `patrol`) or an interaction id; `at` is a place name or `home`. Night is between `nightFromHour` and `nightToHour` of `assets/data/sim/schedule.json`.
- **Action sources** (US-291, no quest source until M10): `does` (a list of interaction ids) in a class file is that class's actions, in a kind file or a placed NPC it is the custom actions; event actions come from `assets/data/sim/events.json`: `{ "id": "fire", "label": "Help put out the fire", "action": "help-with-fire", "classes": ["villager"], "withinMetres": 12, "forMinutes": 30 }`. The chooser takes class, custom and event candidates, and scores them with the `npc` block of their interaction files.
- **Partner types** `assets/data/sim/partner-types.json`: `{ "types": ["player", "animal", "environment", "buildings"] }` plus one `class:<id>` per class. Defaults per partner type: `assets/data/interactions/defaults-<type>.json` (`class`, `animal`, `environment`): `{ "partnerType": "animal", "actions": ["watch-animal"] }`; a class, kind or placed NPC overrides with `partnerActions`: `{ "animal": ["hunt"] }` (and `dialogues`). The chooser prefers these actions when that partner is the target.
- **NPC-to-NPC interactions** are the ordinary interaction files with `"actors": ["npc"]` and a `npc` block (talk, trade, gift, fight, confront, plus hunt, forage, rest and pray for the environment). Effects are the same verbs; the simulation carries out `opinion`, `remember`, `give`, `take` and the `do` words talk, trade, gift, fight, hunt, forage, rest, pray, walk-to and spread-opinion.

### Simulation
- `sim::NpcDirector` owns the life of the persons of an `NpcPopulation`: schedule blocks on the hour for the persons near the focus; interruptions (hunger below `eatBelow` sends them home to eat, danger sends them home, a fight keeps them fighting) and then the schedule resumes; the persons far from the focus follow the schedule in their daily summary, one slice of the population per tick (every person once a day, at a tick fixed by their index), so no tick pays for everybody (ADR-022 addendum).
- NPC-to-NPC: the director picks, for an idle person, the best scoring interaction against one neighbour (found through the grid), applies its effects, and bounds the work per hour (`maxPerHour`). A far person rolls once a day (seeded by world seed, person and day) and, on success, resolves one interaction with a neighbour abstractly.
- Fights: rounds of strike and strike back with the persons' hit points and sword damage (the player's numbers); a death is final, the family and witnesses within hearing distance change their opinion; the director reports deaths so the game removes the figure.
- Persistence: `npc-life.json` (schedule positions, modes, hit points, the dead) with the autosave.

### Order and tests
US-290 schedules, US-291 action sources, US-292 NPCs act on each other, US-293 partner defaults, US-294 the living test level and the soak (100,000 persons, 30 game days, the same save hash twice). Tests: tests/sim/npc_schedule_test.cpp, npc_actions_test.cpp, npc_interact_test.cpp, npc_defaults_test.cpp, npc_soak_test.cpp; tests/game/schedule_editor_test.cpp, npc_life_game_test.cpp, npc_living_level_test.cpp.
