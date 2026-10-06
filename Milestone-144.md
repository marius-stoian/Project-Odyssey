# Project Odyssey: assembly progress (144)

## AP-145 · 2026-10-06 · M11 Data editors (S-US-190 Done)

Codex v2.13, requirements v2.12.

### State
- S-US-190 Done on `qa` (branch `story/US-190`): every file under `assets/data/` has a schema (43 schemas, `schemas/index.json`, every field with a description); the loaders check type, range and choices with file, line and field; the schema test checks coverage, links and drift against the loaders. Debug build zero warnings; its own cases pass (sim 13, game 2); the full verification and CI run at X-M11 (D-41).
- Whole Simulation test run in Debug: 348 of 349 pass. The one failure ("US-261 Shipped kinds") is caused by the owner's own uncommitted edit of `assets/data/npcs/wanderer.json` (attitude "enchanted"); I left that file alone.
- Guide `docs/guides/schemas.md`, plan `docs/plans/stories-M11.md#us-190`, teach-back in `docs/learning-journal.md`.

### Decisions and Codex issues
- No new decision: D-60 Q1 to Q4 cover it. The drift test found about a dozen optional fields no shipped file used (a plant's `height` and `shadow`, a light's `flicker`, an NPC kind's `trade` and `schedule`, an interaction's `chronicle`); they were added to the schemas.
- Codex issues: none new. Still open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020, CI-021.

### Next
- S-US-191 Schema-driven form editor (the Data tab, undo, canonical writer, live reload, catalogs by id: CI-007 and CI-021).
