# Project Odyssey: assembly progress (69)

## AP-070 · 2026-10-01 · US-161 Conversations and the dialogue panel

| | |
|---|---|
| Assembly plan / requirements | **v2.1** / **v2.2** |
| Repository | `story/US-161` merged into `qa`; `main` at `m7-done` |
| Milestone | M8 Speak to NPCs, in progress (K-M8, US-160, US-161 Done) |

### State
- US-161 passed `tools/verify.ps1 -Story US-161`: zero warnings, 27 of 27 tests in Debug and Release (13 new cases: 8 simulation, 5 game). Merged into `qa` and pushed; CI on that merge was running at the time of writing (earlier runs on `qa`: US-160 merge and P-010 were still in progress).
- Talk on a clan member now opens a pausing conversation panel when a script fits them (the shipped `elder-fire.dlg` speaks for the elder); otherwise it is the plain talk of before. The effect `opinion` and the condition `opinion(a, b)` are real. `remember` and `flag` are still only read (US-164).
- Evidence: `docs/evidence/US-161/talk-panel.png`, made with a packaged copy of the game that carried one extra `@who person` script so that someone beside the hero had one; the shipped files are unchanged.

### Decisions
- None requested. Delegated technical choices (Dominus): the panel is `Screen::Talk` of `RunFlow`; the conversation holds a copy of its script; opening a conversation does not call the old `talkTo`; `mood()` as a script function waits for US-162/163 (in no scenario of US-161).

### Next
- Confirm green CI on `qa`, then S-US-162 (who says what: seeded tie-break, barks, hunter/gatherer roles, `mood(npc)` function).
