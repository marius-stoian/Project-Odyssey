# Project Odyssey: assembly progress (91)

## AP-92 · 2026-10-04 · US-247 Lighting in the Editor and quality settings

| | |
|---|---|
| Assembly plan / requirements | **v2.11** / **v2.10** (mirror) |
| Repository | `story/US-247` merged into `qa` |
| Milestone | M8c Lighting and shadows, in progress |

### State
- Debug build with zero warnings (all targets, tests compile). Tests written, not run: the owner asked (2026-10-04) to test M8c only once, at the exit X-M8c. `tests/game/lighting_editor_test.cpp` (preview, place, level version 3, quality).
- Editor: Sky slider (view only) and Light tool (place, select, move, delete, undo). Level version 3 with `lights`; older levels load. Quality: Low = no normal maps and no fire shadows. GPU screenshots and frame times manual: `docs/plans/US-247.md`.

### Decisions
- Technical, not a design question: the Codex acceptance says Low switches off fire shadows and normal maps, while the design note M8c section 8 says Low = no shadows at all. Followed the Codex (sun and moon shadows stay on Low, as US-245 already shipped). Medium and High are the same today; High is the quality the D-06 60 FPS target is held on. If the owner wants Medium to differ, raise it at X-M8c.
- Existing tests that expected "levelVersion": 2 after a save now expect 3 (pickups, plants).

### Next
- X-M8c: the one full verify for M8c (`tools/verify.ps1 -Story X-M8c -Config Both`), exit evidence in docs/gates/M8c.md, merge qa into main, tag m8c-done. K-M9a stops on CI-012.
