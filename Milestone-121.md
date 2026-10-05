# Project Odyssey: assembly progress (121)

## AP-122 · 2026-10-05 · M9 Interaction and dialogue editor (US-175)

| | |
|---|---|
| Assembly plan / requirements | **v2.12** (repository copy) / **v2.10** (mirror) |
| Repository | `story/US-175`, merged into `qa` |
| Milestone | M9 Interaction and dialogue editor |

### State
- US-175 Graph validation written: headless check in `src/sim/graph_check.*`, live finding list in the graph editor, click selects the card, catalog from the game's data. 11 US-175 cases ran alone and pass in Debug (US-170..172 cases still pass). Every shipped `.dlg` and interaction file has no error with the real item list; that test is the "same check in CI". The full verify is X-M9.

### Decisions
- Dead end is an error, unreachable node a warning (the loader already accepts an unreachable node); an unknown tag is a warning (the loader's own rule). Need names are compared without regard to case, as the game reads them.
- Technical: the findings are keyed like the layout file, so a click maps to a card without new bookkeeping.

### Files changed in shared code (overlap guard)
`src/game/odyssey_game.{h,cpp}` (`syncGraphCatalog`, two calls), `src/luna/engine/node_graph.{h,cpp}` (`centerOn`), `src/game/graph_editor.{h,cpp}`, `src/game/dialogue_graph.*` and `interaction_graph.*` (key lookups), `CMakeLists.txt`, `docs/guides/dialogue-format.md`, `docs/status.md`.

### Next
- S-US-173 Attach to placed things (a dialogue per placed NPC already exists from M9a: check it; per-thing overrides are new), then US-174 Test-play, X-M9.
