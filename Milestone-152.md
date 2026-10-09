# Project Odyssey: assembly progress (152)

## AP-153 · 2026-10-09 · M12 World editing (K-M12 and S-US-200 Done)

Codex v2.13, requirements v2.12.

### Done
- K-M12: design `docs/plans/M12-world-editing-design.md` and D-61 (12 delegated decisions, `docs/decision-requests/D-61.md`), committed on `qa` (eafe94d).
- S-US-200 The region in the Editor: Region view with zoom from the whole map to single tiles, seven layers, cached overview, chunk pictures made a few a frame. Merged into `qa`. Debug build of `luna_tests` and `odysseus_game_tests`: zero warnings; the 7 `US-200` cases pass (`docs/evidence/US-200/build.txt`). The full suite and the manual checks (`docs/plans/stories-M12.md#us-200`) run at X-M12.

### Decisions and Codex issues
- Delegated: D-61 Q1 to Q12. Codex issues: none new. Open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020, CI-021.

### Notes
- With the owner's uncommitted `assets/data/npcs/wanderer.json` (hostile) in the checkout, `US-291 Event: a fire lit...` fails; with the committed file it passes. The owner's level and NPC edits are still not mine and were not committed.
- The renderer cannot release a texture; US-202 adds that before it rebuilds a chunk picture.

### Next
- S-US-201 Generator settings with live preview.
