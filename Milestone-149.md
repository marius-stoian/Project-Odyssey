# Project Odyssey: assembly progress (149)

## AP-150 · 2026-10-06 · M11 Data editors (S-US-196 Done)

Codex v2.13, requirements v2.12.

### State
- S-US-196 Done on `qa` (branch `story/US-196`): daily routines as data in the one schedule format. A block may carry `prefer` weights by tag; the choice of an idle placed person is multiplied by them (hunters choose what their work block prefers more often in that block); professions carry `day` and `night` in the same format and the clan's people take the routine of their profession; needs win (the interruption rule of US-290 for placed persons, `routineDangerBelow` for the clan). A 24-hour timeline in the Editor's schedule form and in the Data tab: dragging a block writes through the same setter as the form, so the form, Undo and Save agree.
- Debug build of every program and test executable: zero warnings. Own cases: sim 7 and game 4 pass; full verification and CI at X-M11 (D-41).
- Clean-worktree Debug check of `qa` after US-195 (0705ba1): build zero warnings, ctest 21 of 22 and the window group 8 of 8 passed; the one failure, `US-270 Loads clean`, was the shipped `npc-test.json` still at level version 6 (the round trip writes 7): fixed in this story.
- Guide `docs/guides/npc-data.md` ("Routines" and "The timeline"), plan `docs/plans/stories-M11.md#us-196`, teach-back in `docs/learning-journal.md`, evidence `docs/evidence/US-196/`.

### Decisions and Codex issues
- Decided by Dominus (delegated, D-60 Q11): the tags of an interaction are its id, the tags its file asks of the target and the tags of the target (no new field in the interaction files); a clan member's profession is worked out from their skills (hunter or gatherer); the timeline lives in the Editor's schedule form and in the Data tab and never writes by itself.
- Owner's uncommitted edit of `assets/data/npcs/wanderer.json` (hostile, classes talker and trader) breaks `npc_kind_test` and `US-290 Walk` in the main checkout; not touched.
- Codex issues: none new.

### Next
- S-US-192 Picture pickers and cutting frames (Should), then X-M11.
