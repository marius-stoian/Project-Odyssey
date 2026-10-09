# Project Odyssey: assembly progress (156)

## AP-157 · 2026-10-09 · M12 World editing (S-US-204 Done)

Codex v2.13, requirements v2.12.

### Done
- S-US-204 Things, people and places in the region: Place, Move and Take away tools in the Region view (Thing / Person / Place, Kind list, Name, Set key=value, Fix: move, Fix: drop), entries with stable ids in the world file (`t-0001`, `p-0001`, `l-0001`), tombstones for the seed's own things, properties per group that win over the kind's data, `levelFromRegion` with the edits (the level the game plays), the catalogs `people` and `goals` and the region's names in `places` and `markers` (quest Marker, Goal, Giver and the dialogue Who field). Rules in `src/sim/world_places.*`; every action is one step of Undo.

### Decisions and Codex issues
- Delegated: D-65 Q1 to Q9. Codex issues: none new. Open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020, CI-021.

### Verification
- Debug build of all targets, zero warnings (`docs/evidence/US-204/build.txt`). The 8 sim and 7 game `US-204` cases pass; so do the 104 sim and 142 game cases of the US-18x, US-19x, US-20x and US-30x groups (one older test, `US-190 Drift`, had failed until the schema listed `forced`; fixed). Not run: the full suites (X-M12).

### Notes
- The owner's uncommitted level and NPC edits are still in the checkout and were not committed (wanderer.json hostile fails `US-291 Event` and `npc_kind_test`).
- Left out of this story (not in its acceptance criteria): drag to move, duplicate, box select, a per-line choice list for conflicts (design section 8). A new story if wanted.
- A level names its places with words, so the place "Red Cliff" is `red-cliff` in the level; a quest marker `place:Red Cliff` finds it either way.
- The game still reads the world file only from US-207 (D-63).

### Next
- S-US-205 Camps and resources: camps with the site check, forced sites, resources placed, moved and hidden, resource counts per kind.
