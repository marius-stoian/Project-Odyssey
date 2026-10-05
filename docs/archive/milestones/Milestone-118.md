# Project Odyssey: assembly progress (118)

## AP-119 · 2026-10-05 · M9 Interaction and dialogue editor (K-M9, US-170)

| | |
|---|---|
| Assembly plan / requirements | **v2.12** (repository copy) / **v2.10** (mirror) |
| Repository | `story/US-170`, merged into `qa` |
| Milestone | M9 Interaction and dialogue editor |

### State
- Audit before building: M9a, M9b and M9c were already built and tagged; M8d and M8e finished before K-M9 began. M9 here means the graph editor (US-170..US-175). Nothing from earlier milestones was rewritten: US-170 adds only new files plus three source lines in `CMakeLists.txt`.
- K-M9 answered by the owner (24 questions, `docs/decision-requests/D-56.md`); design in `docs/plans/M9-graph-editor-design.md`.
- US-170 Node-graph widget written: `src/luna/engine/node_graph.{h,cpp}` (model, canvas, layered layout), `src/game/graph_commands.h` (graph edits as Commands in the one History). Its 10 cases ran alone and pass in Debug; the full verify is X-M9 (owner, D-56 Q21).

### Decisions
- Owner, D-56 Q7: zoom range is 25-400%, not the 50-200% written in the Codex US-170 prompt. For Anima's next Codex version.
- Technical: the widget lives in `src/luna/engine/node_graph.*` (the UI toolkit is one file, `ui.cpp`, so there is no `ui/` folder). Cards take their header colour from the UI sheet's colour list; a graph edit is a whole-graph before/after Command like the other list commands.

### Files changed in shared code (overlap guard, D-56 Q4)
`CMakeLists.txt` (three source lines), `docs/decisions.md` (row D-56), `docs/status.md` (K-M9 Done).

### Next
- S-US-171 Dialogue graph editor, then US-172, US-175, US-173, US-174 and X-M9.
