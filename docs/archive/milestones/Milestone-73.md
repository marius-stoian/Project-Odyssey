# Project Odyssey: assembly progress (73)

## AP-074 · 2026-10-01 · US-165 NPCs talk to each other

| | |
|---|---|
| Assembly plan / requirements | **v2.2** / **v2.4** |
| Repository | `story/US-165` merged into `qa`; `main` at `m7-done` |
| Milestone | M8 Speak to NPCs: all six prompts built (K-M8, US-160..US-165 Done); X-M8 is next |

### State
- US-165 passed `tools/verify.ps1 -Story US-165`: zero warnings, every test passing in Debug and Release (8 new cases: 3 simulation, 5 game).
- CI on `qa`: green through the US-163 merge; the US-164 and US-165 merge runs were last.
- Clan members near the hero now talk, quarrel, court, share and give in speech bubbles (3 s a line, in turn); the elder calls a child over with the shipped pair script.
- Evidence: `docs/evidence/US-165/bubbles-between-people.png`.

### Decisions
- None requested. Delegated technical choices (Dominus): the bubbles listen to the simulation (the chronicle plus a small queue for `talk`), never decide it; "near the camera" is within 12 m of the hero; generated lines are one per speaker from `social.<kind>`.

### Next
- Confirm green CI on `qa`, then X-M8 (the exit review of Speak to NPCs; D-35: the owner reads the 50 small-talk lines in `docs/evidence/US-163/` and judges the greeting rule of at most two bubbles at once; `qa` merges into `main` and `m8-done` is tagged after green CI), then P-011's milestones K-M8b..X-M8e.
