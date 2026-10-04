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
