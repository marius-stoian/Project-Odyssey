# Project Odyssey: assembly progress (55)

## AP-056 · 2026-10-01 · M6 built; waiting at kill gate 2

| | |
|---|---|
| Assembly plan / requirements | **v1.9** / **v1.9** |
| Repository | merged into local `qa`; nothing pushed since `5125364` |
| Milestone | **M6 Playtest**: 3 of 3 stories built; X-M6 needs the owner |
| Next | owner answers `docs/decision-requests/D-GATE-M6.md` |

### What happened
- US-090: the elder guides gather, eat, tend the fire (data in `tutorial.json`), hints after 2 minutes stuck, can be turned off on the New Game screen.
- US-091: CPack zip with the exe, SDL3 and assets; a packaged build finds its assets next to the exe and saves in the user's folder; a crash writes `crash-<n>.log` plus the last save to the user's `crash` folder.
- US-092: the first New Game asks about local session statistics; if agreed, one JSON file with play time and key events per session; nothing is ever sent.

### Decided by Dominus (delegated)
- D-33 (docs/decisions.md): the first-day steps, the 2-minute hint, the statistics question and file, the crash folder.

### Skipped because of "no testing"
- `tests/game/m6_test.cpp` compiled, never run; the zip never built; see `docs/gates/M6.md`.

### Stopped at a true human gate
- X-M6 (8 playtesters). The Codex ends here; M7 and M8 are not defined in it.
