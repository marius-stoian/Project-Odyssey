# Project Odyssey: assembly progress (54)

## AP-055 · 2026-10-01 · M5 built: the vertical slice

| | |
|---|---|
| Assembly plan / requirements | **v1.9** / **v1.9** |
| Repository | merged into local `qa`; nothing pushed since `5125364` |
| Milestone | **M5**: 16 stories built |
| Next | **K-M6** (US-090..US-092; X-M6 needs playtesters, a human gate) |

### What happened
- The run exists end to end: New Game, Growing Period (focus and crossroads), mantle, professions and crafting, apprenticeship, trade, barter and debts, the sacred fire, aging and the end screen; settings, F3 overlay, run save.

### Decided by Dominus (delegated)
- D-32 (docs/decisions.md).

### Skipped because of "no testing"
- `tests/sim/hero_test.cpp` compiled, never run; `tests/game/run_test.cpp` still to write. See `docs/gates/M5.md`.
