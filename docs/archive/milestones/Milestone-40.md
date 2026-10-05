# Project Odyssey: assembly progress (40)

## AP-041 · 2026-09-30 · M2c Done: the Level Editor

| | |
|---|---|
| Codex / requirements | **v1.7** / **v1.7** |
| Repository | `main` tagged `m2c-done` |
| Milestone | **M2c Level editor**: Done (7 of 7 stories, exit review met) |
| Next | **Your turn**: try the Editor (`odysseus.exe --editor`, guide `docs/guides/editor.md`); M3 starts only on your word |

### What happened
- US-120..US-126 Done: your art in the game; mouse, font and widgets; levels as files; F1/F2 modes; painting ground; placing characters; level settings, New/Open, and the guide.
- X-M2c: every exit criterion met ([docs/gates/M2c.md](../../gates/M2c.md)); a scripted session edited, saved, reloaded and played a level.
- Found and fixed: scripted input pressed a repeated intent on every tick (test harness only); a regression test guards it.
- Your edited `valley.json` is the game's level; the tests play `demo.json` (D-20). Your four new sprite sheets (weapons x2, nature, animals) are untouched and not committed: say what they are for.

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 5 / 5 | Done, `m1b-done` |
| M2 | Console clan simulator (KILL GATE 1) | 7 / 7 | Done, `m2-done` |
| M2b | Story engine (KILL GATE 1 retry) | 6 / 6 | Done, `m2b-done` |
| M2c | Level editor | 7 / 7 | Done, `m2c-done` |
| M3-M6 | | 0 / 28 | Waits for your word |
| | **MVP total** | **34 / 62** | |

### Decisions and codex issues
- Decided by you: D-05, D-19, D-GATE-M2b. Delegated: D-20. Codex issue CI-007 open (low: the editor's files sit in src/game/, not a sub-folder).
