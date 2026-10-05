# M2d exit review: Content and combat (2026-10-01)

**Result: built and demonstrated story by story. Merge to `main`, tag, full test run and CI are deferred to the M4 gate** (owner, 2026-10-01: "Proceed without testing until reaching M4"). Nothing below was run as a full test suite after US-135 except where stated.

| Exit criterion | State | Evidence |
|---|---|---|
| The owner places weapon pickups, plants, animals and looping effects with the Editor, saves and plays the level | met (built; screenshots) | US-134 `docs/evidence/US-134/editor-weapons.png`; US-136 `editor-plants.png`; US-137 `editor-animals.png`; US-138 `editor-effects.png`; the Editor guide `docs/guides/editor.md` |
| The hero picks weapons into a 9-slot hotbar and fights with the 16 starter weapons of 8 classes, elements included | met | US-133 `docs/evidence/US-133/`, US-134 `hotbar.png`, US-135 one screenshot per element |
| Added by the owner during M2d: mouse aiming, arcs and ranged weapons (D-25, D-26) | met | US-139 `docs/evidence/US-139/`, US-140 `docs/evidence/US-140/`, US-141 `docs/evidence/US-141/`; the shooting range `assets/levels/range.json` |
| Enemies (goblins, predators, boars) strike back when hit and in reach, and die; the hero respawns at 0 HP | met | US-131 tests; US-137 `animals.png` |
| Plants block, are inspected, chopped and eaten, and regrow | met | US-136 `docs/evidence/US-136/` |
| Effects play; the weather changes by itself | met | US-132, US-138 `docs/evidence/US-138/` |
| Every M2c feature and the spear and sword demos still work | not re-run | the full test run after US-141 (25 of 25 in Debug and Release) covered them; the later stories (US-136..US-138, the hero-orientation fix) were only built |

## Checks owed at the M4 gate (closed by P-009, see docs/gates/test-debt.md)
- `tools/verify.ps1` (Debug and Release, zero warnings, all tests) for the work after US-141: the hero-orientation fix (US-139 follow-up), US-136, US-137, US-138. Their new tests were written with the code and never run: `US-139 The hero always faces the pointer`, `US-139 No flicker walking past the pointer`, `US-139 Facing with hysteresis` (these three did pass in Debug and Release on the orientation branch), `US-136 ...` (7 cases passed in Debug), `US-137 ...` (4 cases, never run), `US-138 ...` (3 cases, never run).
- The toolbar labels changed to make room for new buttons (Weapon is Arms, Grid is #); the end-to-end scripts in `tests/game/*.cmake` click by position and may need their coordinates checked.
- CI on `qa`, then merge `qa` into `main`, tag `m2d-done`.
- The owner's uncommitted `assets/levels/valley.json` edits were never touched.

## Owner notes
None yet. Notes from playing with `docs/guides/editor.md` become new stories through Anima.
