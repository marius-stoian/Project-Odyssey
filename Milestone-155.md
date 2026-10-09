# Project Odyssey: assembly progress (155)

## AP-156 · 2026-10-09 · M12 World editing (S-US-203 Done)

Codex v2.13, requirements v2.12.

### Done
- S-US-203 Water and mountains: River (width, fords), Lake, Ridge, Cave, Ford and Dry tools in the Region view, all tile edits (one step of Undo each, no new format), the shape preview, the warning line (start without water or food, sealed cave mouth), the four-connected line and brush-stamp shapes in `sim`. Merged into `qa`. Debug build of the sim and game tests: zero warnings; the 12 `US-203` cases pass, with the US-200 to US-202 cases (29 in all `US-20*`), the editor, help, schema and US-19x suites and the whole luna suite (`docs/evidence/US-203/build.txt`). The full suite and the manual checks (`docs/plans/stories-M12.md#us-203`) run at X-M12.

### Decisions and Codex issues
- Delegated: D-64 Q1 to Q8. Codex issues: none new. Open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020, CI-021.

### Notes
- The owner's uncommitted level and NPC edits are still in the checkout and were not committed (wanderer.json hostile fails `US-291 Event` and the sim test `npc_kind_test`).
- Seed 1 has no inland mountains and seed 12 has no suitable lake; the tests pick the seed they need (the numbers were measured, see the tests).
- The smooth-shore pass of the design is not built (not in the acceptance criteria); the design is amended.

### Next
- S-US-204 Things, people and places in the region: place objects, plants, animals and NPC kinds on the land, selection and move, stable ids in the world file, properties that win over a kind's data.
