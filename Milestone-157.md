# Project Odyssey: assembly progress (157)

## AP-158 · 2026-10-09 · M12 World editing (S-US-205 Done)

Codex v2.13, requirements v2.12.

### Done
- S-US-205 Camps and resources: Camp (player, rival) and Resource (flint, wood, berries, herd) in the Place row of the Region view; camp rules with reasons and the Anyway override; rival clans start at the placed camps (`Rivals(..., placed)`); the player camp moves the start; amounts per spot (a flint or wood spot of 50 gives 50, one at a time; what is left is kept in the run save); hidden seed spots; the region follows every entry (`applyPlacedToRegion`).

### Decisions and Codex issues
- Delegated: D-66 Q1 to Q8. Codex issues: none new. Open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020, CI-021.

### Verification
- Debug build of all targets, zero warnings (`docs/evidence/US-205/build.txt`). The 4 sim and 2 game `US-205` cases pass; so do the 119 sim and 156 game cases of the US-03x, US-04x, US-08x, US-18x, US-19x, US-20x and US-30x groups. Not run: the full suites (X-M12).

### Notes
- The owner's uncommitted level and NPC edits are still in the checkout and were not committed.
- Left out (not in the acceptance criteria): resource counts per kind in a panel, a camp's start store, a reachability warning for camps.
- The running game still reads the world file only from US-207 (D-63); the level it draws still makes one plant a spot.

### Next
- S-US-206 Clans and people inspector: per clan and per person setup, inherited versus override values, consistency checks.
