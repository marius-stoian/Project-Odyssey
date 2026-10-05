# Exit review M9c: NPC life (2026-10-05)

Codex v2.12 (X-M9bc, one exit for M9b and M9c, D-54). Same single verify as `docs/gates/M9b.md`: Debug and Release, zero warning lines, 27 of 27 test groups in each; output `docs/evidence/X-M9bc/`.

Found and fixed by the one full verify: the director returned early for a person who had only default actions; `hunt` was missing from the game's built-in action names; place tags (forage, shrine) gave unknown-tag warnings; the shipped class default (`npc-chat`) outscored a useful swap, so it is now empty; the environment actions are scored by need (a satisfied person no longer wanders to a shrine at work); the deer of the test level is out of sight of the hunter; the US-267 test waits for the figure to walk to its new place. No test, threshold or budget was weakened.

| # | Exit criterion | Result | Evidence |
|---|---|---|---|
| 1 | Day and night schedules, interruptions (US-290) | Met | `npc_schedule_test`, `schedule_editor_test` |
| 2 | Class, custom and event actions, no quest hook (US-291) | Met | `npc_actions_test`, `npc_life_game_test` |
| 3 | NPCs talk, trade, give, confront and fight each other; far persons by a daily roll (US-292) | Met | `npc_interact_test`, `npc_dealings_game_test` |
| 4 | Default interactions by partner type (US-293) | Met | `npc_defaults_test`, `npc_defaults_game_test` |
| 5 | The test level shows one day (US-294) | Met | `living_level_test` |
| 6 | Soak: 100,000 persons x 30 days, same seed gives the same save hash, ADR-022 budget | **Skipped by owner** | Owner decision 2026-10-05 (D-54 follow-up): kept as `odysseus_sim_soak`, label `soak`, not run in verify or CI; run by hand with `ctest --preset windows-x64-release -L soak`. The one-day 100,000-person budget test of US-263 still runs and passed |
| 7 | Screenshots of the living test level | **Not produced** | GPU screenshots are manual (`docs/plans/stories-M9c.md#us-294` step 4) |

## For the owner
- Row 7 needs your PC and the window; row 6 is yours to run when you want it.
