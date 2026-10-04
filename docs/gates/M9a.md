# Exit review M9a: NPC foundation (2026-10-05)

Codex v2.11 (X-M9a). Tests were run once for the whole milestone, at the exit (owner, 2026-10-04).

**Verify.** `pwsh tools/verify.ps1 -Story X-M9a -Config Both` on the owner's PC: Debug and Release build with zero warning lines, 27 of 27 test groups passed in each (Release includes the strict 3-second first-frame limit, D-47). Output: `docs/evidence/X-M9a/`.

Found and fixed by the one full verify (the stories were built without running the suite): the placed people were only built into the simulation after a restart, never at start-up (`OdysseyGame` constructor now calls `buildNpcPopulation`); one Debug iterator misuse in the US-268 partner-types test; one US-266 test level that put a character outside the map.

| # | Exit criterion | Result | Evidence |
|---|---|---|---|
| 1 | Every placed NPC is a person with classes, attitude and opinions | Met | US-260..264; `npc_class_editor_test`, `npc_kind_test`, `npc_people_test`, `npc_attitude_test` |
| 2 | The player talks to and confronts NPCs and sees their actions in a pop-up | Met | US-265..267; `npc_talk_test`, `npc_confront_test`, `npc_actions_test` |
| 3 | The owner edits classes, kinds and NPCs in the Editor | Met | US-260, US-268, US-269; `npc_class_editor_test`, `npc_editor_test`, `npc_kinds_tab_test` (one NPC, one kind and one class changed and followed in play) |
| 4 | The NPC test level shows all of it | Met | US-270; `npc_test_level_test`; walk-through table in `docs/guides/npc-data.md` |
| 5 | 100,000 persons run within the ADR-022 budget | Met | US-263 load test inside `npc_people_test` (Debug and Release green) |
| 6 | Release: strict 3 s first frame on the owner's PC | Met | Release 27/27 above |
| 7 | Screenshots of the Editor Kinds tab, markers and the test level | **Not produced** | GPU screenshots are manual: `docs/plans/US-269.md` step 6, `US-270.md` step 4 |

## For the owner
- Row 7 needs your PC and the window; follow the walk-through of the test level (`odysseus.exe --level assets/levels/npc-test.json`).
