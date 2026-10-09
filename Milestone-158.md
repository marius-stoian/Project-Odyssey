# Project Odyssey: assembly progress (158)

## AP-159 · 2026-10-09 · M12 World editing (S-US-206 Done)

Codex v2.13, requirements v2.12.

### Done
- S-US-206 Clans and people inspector: the Inspect panel in the Region view (clan: leader, stance, store, debts, partners, members; clan member: kin, opinions, grudges), the `clans` and `people` sections of the world file (schema, round trip), the consistency list, one step of Undo per change; the setup applied when a game starts (`applyClanSetup`: meals, kin, opinions as written, grudges told by the chronicle; `applyHeroSetup`: store items and debts to rivals; `applyRivalSetup`: a rival's meals); a placed person's `allow`, `deny`, `day`, `night`, `stock` and `does` properties applied in the level, so removing Barter and changing HP changes only that person.

### Decisions and Codex issues
- Delegated: D-67 Q1 to Q9. Codex issues: none new. Open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020, CI-021.

### Verification
- Debug build of all targets, zero warnings (`docs/evidence/US-206/build.txt`). The 7 sim and 2 game `US-206` cases pass; so do the 213 sim and 165 game cases of the US-01x to US-11x, US-18x to US-20x and US-30x groups. Not run: the full suites (X-M12).

### Notes
- The owner's uncommitted level and NPC edits are still in the checkout and were not committed.
- Left out (not in the acceptance criteria): the grey inherited values and reset button, a list of a clan's people by name (they exist only once a game starts), the reading of leader, stance and partners (Politics, M13).
- The running game starts from the setup only through US-207 (Play here); the apply functions are tested on their own.
- Found by the address sanitizer: iterating `.items()` of a temporary JSON object (fixed in `world_file.cpp`).

### Next
- S-US-207 Play the edited region: Play here, Escape back, the run records the world file name and hash, the Quick check on the world.
