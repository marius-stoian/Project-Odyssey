# Project Odyssey: assembly progress (119)

## AP-120 · 2026-10-05 · M9 Interaction and dialogue editor (US-171)

| | |
|---|---|
| Assembly plan / requirements | **v2.12** (repository copy) / **v2.10** (mirror) |
| Repository | `story/US-171`, merged into `qa` |
| Milestone | M9 Interaction and dialogue editor |

### State
- US-171 Dialogue graph editor written: `.dlg` to cards and back (every shipped file round-trips to the same text), full-screen editor behind a new **Talk** button, layout sidecar, Save with backup and an ask-once overwrite guard, graph edits in the Editor's one History. 14 US-171 cases ran alone and pass in Debug; the full verify is X-M9.
- Evidence: `docs/evidence/US-171/dialogue-graph.png`.

### Decisions and Codex issues
- CI-014 (open, for Anima): D-56 differs from Codex v2.12 in zoom range, card model and tabs; the Sub-conversation call card the owner chose (Q10) has no story and needs a new `.dlg` element plus runtime support. Not built.
- Technical: the editor is a new class `GraphEditor` beside the Editor (like `BuildingEditor`); a card keeps its data in `GraphNode::fields`, so an undo of a text edit works; wires route around when they run backwards; nodes lay out in rows.

### Files changed in shared code (overlap guard)
`src/game/editor.{h,cpp}` (Talk button, a graph-editor early branch in update and render, no `unsaved_` for undo while the graph shows), `src/game/odyssey_game.cpp` (two lines: folder and reload callback), `CMakeLists.txt`, `docs/guides/dialogue-format.md` (a section added), `docs/codex-issues.md`, `docs/status.md`.

### Next
- S-US-172 Interaction graph editor, then US-175, US-173, US-174, X-M9.
