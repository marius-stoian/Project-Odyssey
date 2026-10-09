# Project Odyssey: assembly progress (154)

## AP-155 · 2026-10-09 · M12 World editing (S-US-202 Done)

Codex v2.13, requirements v2.12.

### Done
- S-US-202 Hand edits on top of the seed: painting tools in the Region view (Brush, Rect, Fill, Reset), one world history (Ctrl+Z, Ctrl+Y), the world file `assets/worlds/<name>.json` (seed plus only the differing tiles) with its schema and guide, tile edits in `sim::Region`, `sim::loadWorld`/`saveWorld`/`makeWorldRegion`, and `Renderer::destroyTexture` so repainted chunk pictures and the maps do not pile up textures. Merged into `qa`. Debug build of all targets: zero warnings; the 15 `US-202` cases pass, with the US-200 and US-201 cases (12), the editor, help, schema and US-19x suites and the whole luna suite (`docs/evidence/US-202/build.txt`). The full suite and the manual checks (`docs/plans/stories-M12.md#us-202`) run at X-M12.

### Decisions and Codex issues
- Delegated: D-63 Q1 to Q10. Codex issues: none new. Open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020, CI-021.

### Notes
- The owner's uncommitted level and NPC edits are still in the checkout and were not committed (wanderer.json hostile fails `US-291 Event` and the sim test `npc_kind_test`; both pass with the committed file).
- The game does not read the world file yet: US-207 (Play the edited region) does. Per-conflict choices (keep, move, remove) arrive with placed things in US-204 and US-205.
- Painting a tile the biome the seed already gives is not an edit; if the settings later change the seed land there, the tile follows it (D-63 Q3).

### Next
- S-US-203 Water and mountains: river, lake and ridge tools (macros made of tile edits), the reachability warning, and the smooth-shore pass.
