# Project Odyssey: assembly progress (150)

## AP-151 · 2026-10-06 · M11 Data editors (S-US-192 Done)

Codex v2.13, requirements v2.12.

### State
- S-US-192 Done on `qa` (branch `story/US-192`): in the Data tab a **...** picker on every frame field shows the atlas as pictures and writes the choice; an effect, weather, weapon, plant or animal selected in the form plays its frames beside it; the **Cut tool** takes a rectangle of one of the owner's sheets, names it, adds it to `cuts.json` or `content-cuts.json` and cuts the atlas again with the same library code `odysseus_atlas` uses. The running game reads the new atlas at its next start.
- Debug build of every program and test executable: zero warnings. Own cases: game 5 pass; full verification and CI at X-M11 (D-41).
- Clean-worktree check of `qa` after US-196 (8dcf0a0), Debug: build zero warnings, ctest 22 of 22 passed.
- **All stories of M11 are Done** (S-US-190, 191, 193, 194, 195, 196, 192). Next is the exit review X-M11.
- Guide `docs/guides/data-editor.md` ("Pictures"), plan `docs/plans/US-192.md`, teach-back in `docs/learning-journal.md`, evidence `docs/evidence/US-192/`.

### Decisions and Codex issues
- Decided by Dominus (delegated, D-60 Q12): the atlas is cut again by a library call, not by running the tool; the Cut tool serves `cuts.json` and `content-cuts.json` (the frame fields of the data files name content frames); a new content cut is written with `"key": "flood"`; the running game reads the new atlas at its next start.
- Codex issues: none new.

### Next
- X-M11 Exit review: clean-worktree full verify (Debug and Release), the first-frame time on the owner's PC, the exit demonstration, `docs/gates/M11.md`, merge qa into main, CI on main, tag `m11-done`.
