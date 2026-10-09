# Project Odyssey: assembly progress (153)

## AP-154 · 2026-10-09 · M12 World editing (S-US-201 Done)

Codex v2.13, requirements v2.12.

### Done
- S-US-201 Generator settings with live preview: Settings panel in the Region view (14 fields, Preview beside the old map in 4 ticks, Apply patching only the changed lines of `region.json`, Revert), `sim::regionConfigProblems`, and the model of hand edits with its conflict finder (`sim::RegionEdits`, `findConflicts`). Merged into `qa`. Debug build of `odysseus_sim_tests` and `odysseus_game_tests`: zero warnings; the 9 `US-201` cases pass, with the 6 `US-200` cases and the editor (30), Help/US-30x (51) and US-19x (44) suites (`docs/evidence/US-201/build.txt`). The full suite and the manual checks (`docs/plans/stories-M12.md#us-201`) run at X-M12.

### Decisions and Codex issues
- Delegated: D-62 Q1 to Q9. Codex issues: none new. Open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020, CI-021.

### Notes
- The owner's uncommitted level and NPC edits are still in the checkout and were not committed (wanderer.json hostile fails `US-291 Event`; it passes with the committed file).
- Every Preview and every Apply uploads a texture the renderer cannot release; US-202 adds `Renderer::destroyTexture` before it rebuilds chunk pictures.

### Next
- S-US-202 Hand edits on top of the seed (L): the world file `assets/worlds/<name>.json`, tile overrides, brushes, one world history, regeneration with the conflict list, `Renderer::destroyTexture`.
