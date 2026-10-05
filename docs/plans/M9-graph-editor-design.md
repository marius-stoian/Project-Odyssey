# M9 design: interaction and dialogue graph editor (written at K-M9, 2026-10-05)

Owner answers: D-56 (docs/decision-requests/D-56.md). Source stories: US-170, US-171, US-172, US-175, US-173, US-174. Traces: SDC-06, INT-03, INT-06, ADR-019.

## 1. Layers and files (overlap guard, D-56 Q4 and Q23)
New files only for new code; shared files get small additive edits and each story's Milestone file lists them.

| Layer | New files | Existing files touched (additive) |
|---|---|---|
| Luna Engine (game-agnostic) | `src/luna/engine/node_graph.{h,cpp}`: graph model + canvas widget (US-170) | `CMakeLists.txt` |
| Simulation | `src/sim/graph_check.{h,cpp}`: validation of a `DlgScript` and an `Interaction` (US-175); `src/sim/test_play.{h,cpp}`: throwaway-copy runner (US-174) | `CMakeLists.txt`; loaders only if a Sub-call needs one new `.dlg` line (US-171) |
| Game | `src/game/graph_commands.{h,cpp}`, `dialogue_graph.{h,cpp}`, `interaction_graph.{h,cpp}`, `graph_editor.{h,cpp}` | `editor.{h,cpp}` (two tabs), `level.{h,cpp}` (override file names), `odyssey_game.cpp` (F5 hook already exists, reuse), `CMakeLists.txt` |

Nothing that exists is rewritten: `writeDialogue`, `parseDialogue`, `toJson`, `InteractionRegistry::parse` stay the only readers and writers of the files. The graph is a view over them (ADR-019: JSON for rules, `.dlg` for speech, layout in a sidecar).

## 2. The widget (US-170)
`luna::engine::NodeGraph`: nodes (id, type name, title, preview lines, header colour, position in graph units, input and output ports by index), wires (node + port to node + port, one wire per output port), selection set. `NodeGraphView` is a `Widget`: right-drag pan, wheel zoom 25-400% around the cursor (D-56 Q7), left-drag moves nodes or draws a selection box, drag from an output port to an input port wires, Delete removes, F frames all. It draws colour-coded cards (Q5) with the `UiPainter`. It never edits the graph by itself: it calls `onEdit(name, before, after)`; the owner of the view turns that into a Command.
Undo (Q19): `GraphEditCommand : Command` (game layer) holds a `shared_ptr<NodeGraph>` plus the before/after graph and ignores the `Level&` it is given; it goes through the one `Editor::run`, so Ctrl+Z walks map and graph edits in one stack. Pan and zoom are view state, not edits.
Tests (headless, `ImageRenderer` and scripted `Pointer`s): add two nodes and wire; pan and zoom around the cursor; 10 edits undone to the start; selection box; delete removes its wires; zoom clamps at 25 and 400; frame-all.

## 3. Dialogue graph (US-171)
Cards: Line (speaker + text), Choice, Condition, Effect, Goto, Comment, Sub-call (Q9, Q10). They map onto the existing script model: a `DlgNode` is a column of Lines then Choices; a Condition card wired to a Line's or Choice's `if` port is its `condition`; an Effect card wired to a Choice's `do` port is its effects; a Choice's `next` wire is its `target` (a Goto card is a named target used to avoid long wires); a Comment card is the `notes` lines. Sub-call needs one new `.dlg` element; it is added to the format guide and loader only if the runtime can support it without touching M8 behaviour (decided inside US-171, recorded in the Milestone file).
Round-trip (Q11): the graph reads a script with `parseDialogue`, keeps every `notes` line, and saves with `writeDialogue`, the canonical form already used by shipped files; positions go to `<name>.dlg.layout.json`. A file without a sidecar gets the deterministic layered left-to-right layout (Q8). The editor stores the file's modified time and asks before overwriting a file that changed on disk.

## 4. Interaction graph (US-172)
One graph per interaction file (Q12): Actor card (`actors`) -> Verb card (id, label, range, duration, order, menu) -> Target card (`targetTags`, `targetKinds`); Requirement cards (condition + otherwise text) and Effect cards hang off the verb; an NPC-rule card holds `npc`. Saved with `toJson`, layout in `<id>.json.layout.json` beside it.

## 5. Attach and overrides (US-173)
Inspector field on a placed NPC or object: a drop-down of `.dlg` or interaction files and an Open-in-graph button (Q13). A level may carry `conversationOverrides` / `interactionOverrides` (placed id -> file under the level's own folder); absent fields leave old levels loading byte-identical (Q14, tested with the shipped levels).

## 6. Test-play (US-174)
`sim::TestPlay` clones the world slice it needs (partner NPC, hero inventory, clan stockpile, flags, clock) into a throwaway copy; the panel sets opinion, reputation and needs, inventory, skills, stockpile, flags, time of day, season, and forces random rolls (Q15); the run log lists lines shown, choices, effects applied. Nothing is written to the saved world (Q16, tested by hash before and after).

## 7. Validation (US-175)
`sim::checkDialogue` and `sim::checkInteraction`: unreachable nodes, dead ends (a node with no lines or choices leading nowhere and no END), unknown targets, unknown items, tags, flags and nodes, unknown builtins. Results carry file, node id and line; the Editor lists them live, clicking selects the node (Q17). Saving with errors warns (Q18); `tools/verify.ps1` calls a `--check-graphs` run of the game binary over every shipped file and fails on errors.

## 8. Order, tests, kill rule
US-170, US-171, US-172, US-175, US-173, US-174, then X-M9. Each story runs its own cases alone; one full verify (Debug and Release) at X-M9 (Q21). If US-170 passes 4 weeks, Dominus writes the decision request and builds the form editor instead (Q20). Evidence: headless pixel tests and `--screenshot` by the team; docs/gates/M9.md lists what the owner may check later (Q24).
