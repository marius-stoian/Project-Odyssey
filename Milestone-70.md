# Project Odyssey: assembly progress (70)

## AP-071 · 2026-10-01 · US-162 Who says what

| | |
|---|---|
| Assembly plan / requirements | **v2.1** / **v2.2** |
| Repository | `story/US-162` merged into `qa`; `main` at `m7-done` |
| Milestone | M8 Speak to NPCs, in progress (K-M8, US-160, US-161, US-162 Done) |

### State
- US-162 passed `tools/verify.ps1 -Story US-162`: zero warnings, every test passing in Debug and Release (11 new cases: 7 simulation, 4 game). CI on `qa`: the US-160 merge and P-010 are green; the US-161 merge was still running when this was written.
- Scripts that fit equally well are chosen by the seeded stream; roles elder, hunter, gatherer and child come from the world; `mood(who)` works in conditions; friendly people within 3 m greet the hero in a bubble, at most once a minute each.
- Evidence: `docs/evidence/US-162/greeting-bubbles.png` (made with a packaged copy of the game, seed 7).

### Decisions
- None requested. Delegated technical choice (Dominus): **at most two greeting bubbles at once, nearest first**, because the first version made a whole camp greet in the same instant and the screenshot showed an unreadable pile. The owner may prefer another number or another rule at X-M8 (the constant is `kGreetingBubblesAtOnce` in `src/game/bubbles.h`).

### Next
- Confirm green CI on `qa`, then S-US-163 (generated small talk and `{smalltalk.topic}`; the owner reads 50 samples at X-M8).
