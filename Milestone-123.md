# Project Odyssey: assembly progress (123)

## AP-124 · 2026-10-05 · M9 Interaction and dialogue editor (US-174)

| | |
|---|---|
| Assembly plan / requirements | **v2.12** (repository copy) / **v2.10** (mirror) |
| Repository | `story/US-174`, merged into `qa` |
| Milestone | M9 Interaction and dialogue editor |

### State
- US-174 Test-play written: a headless test world and conversation player (`src/sim/test_play.*`), `Conversation::jumpTo`, and the Test card of the graph editor (values as words, Play, From here, numbered choices, effect log). It plays the graph on screen, saved or not, and writes nothing. 12 US-174 cases ran alone and pass in Debug (the cases of US-170..US-175 still pass); the full verify is X-M9.
- All six M9 stories (US-170, US-171, US-172, US-175, US-173, US-174) are written. Next is the exit review X-M9.
- Evidence: `docs/evidence/US-174/test-play.png`.

### Decisions and Codex issues
- Not built, said openly: forced random rolls (D-56 Q15, last item) because neither language has a roll to force today; test-play of an interaction (the Codex also names the action runner). Both are added to CI-014 for Anima and the owner.
- Technical: the test world copies nothing from the game: values come from the owner's words, so it cannot touch the saved world even by mistake.

### Files changed in shared code (overlap guard)
`src/sim/conversation.h` (one method, `jumpTo`), `src/game/graph_editor.{h,cpp}` (US-171's own files), `CMakeLists.txt`, `docs/guides/dialogue-format.md`, `docs/codex-issues.md`, `docs/status.md`.

### Next
- X-M9: the one full verify (Debug and Release, `tools/verify.ps1 -Story X-M9 -Config Both`), `docs/gates/M9.md`, merge qa into main, tag m9-done, CI on main.
