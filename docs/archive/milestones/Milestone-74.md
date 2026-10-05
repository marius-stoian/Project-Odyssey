# Project Odyssey: assembly progress (74)

## AP-075 · 2026-10-01 · X-M8 exit review: Speak to NPCs is done

| | |
|---|---|
| Assembly plan / requirements | **v2.2** / **v2.4** |
| Repository | `qa` merged into `main`; tag `m8-done` |
| Milestone | **M8 Speak to NPCs: Done** |

### State
- All five exit criteria are met (`docs/gates/M8.md`): the player talks to any clan member; written conversations branch on the simulation; without a script NPCs make small talk from their memories; talk changes opinions, items and memories; NPCs talk to each other in bubbles.
- Every story (US-160..US-165) ran `tools/verify.ps1` (0 warnings, 27 of 27 ctest groups in Debug and Release) and has a green CI run on `qa` (the last, US-165: run 36882172021).
- 50 small-talk samples for the owner, with the count of repeated lines (42 different, none more than twice): `docs/gates/M8-smalltalk.md`.

### Decisions
- None requested. The owner's reading items are listed at the end of `docs/gates/M8.md`: the 50 small-talk lines and the greeting rule (at most two bubbles at once).

### Next
- K-M8b (Resolution and GPU renderer): read its prompt and ask the owner its design questions first (D-35). The P-011 milestones follow in order: M8b, M8c Lighting and shadows, M8d Buildings, M8e Building life, then K-M9.
