# Project Odyssey: assembly progress (120)

## AP-121 · 2026-10-05 · M9 Interaction and dialogue editor (US-172)

| | |
|---|---|
| Assembly plan / requirements | **v2.12** (repository copy) / **v2.10** (mirror) |
| Repository | `story/US-172`, merged into `qa` |
| Milestone | M9 Interaction and dialogue editor |

### State
- US-172 Interaction graph editor written: every shipped interaction round-trips to the same data, edits are checked by the real loader, the **Rules** button opens it, one `GraphEditor` serves both kinds. 11 US-172 cases ran alone and pass in Debug (the 15 cases of US-170 and US-171 still pass); the full verify is X-M9.
- Evidence: `docs/evidence/US-172/interaction-graph.png`.

### Decisions
- Technical: the verb id is the file name, so this editor edits files and does not create or rename them; comments inside the braces of a JSON file are not kept by the canonical text (the leading `//` lines are). Both are in the guide.
- The panel rebuild is deferred to the next tick (a rebuild from inside its own button would destroy the running button).

### Files changed in shared code (overlap guard)
`src/game/editor.{h,cpp}` (Rules button, folder argument), `src/game/odyssey_game.cpp` (one line), `src/game/graph_editor.{h,cpp}` (US-171's own files), `CMakeLists.txt`, `docs/guides/interaction-data.md` (a section added), `docs/status.md`.

### Next
- S-US-175 Graph validation, then US-173, US-174, X-M9.
