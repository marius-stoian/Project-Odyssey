# Exit review M8c: Lighting and shadows (2026-10-04)

Codex v2.11 (X-M8c). Tests were run once for the whole milestone, at the exit (owner, 2026-10-04).

**Verify.** `pwsh tools/verify.ps1 -Story X-M8c -Config Both` on the owner's PC: Debug and Release build with zero warning lines, 27 of 27 test groups passed in each (Release includes the strict 3-second first-frame limit). Output: `docs/evidence/X-M8c/windows-debug.txt`, `windows-release.txt`. The first run found three faults, all fixed and rerun: the level-version check in `level_test.cpp` (saves are now version 3), the Light tool click spot in the new test, and fire shadows appearing by day under dimming weather (now follows the sky only).

| # | Exit criterion | Result | Evidence |
|---|---|---|---|
| 1 | Lit by sun and moon through the day and seasons | Met | US-242, US-248; `sky_test`, `celestial_test` |
| 2 | Fires, torches and effects light the night | Met | US-243; `world_lights_test` |
| 3 | Weather dims and tints; storms flash | Met | US-246; `weather_light_test` |
| 4 | Sprites shaded with generated normal maps | Met | US-241; `normals_test` |
| 5 | Shadows from sun, moon and nearby fires | Met | US-244, US-245; `shadow_test` |
| 6 | Editor previews any time of day; places lights | Met | US-247; `lighting_editor_test` |
| 7 | High lighting holds 60 FPS on the target PC | **Not measured** | needs the GPU frame-time run (`docs/plans/stories-M8c.md#us-247` step 5); Low/High comparison table not yet recorded |
| 8 | Time-lapse screenshot sheet (dawn, noon, dusk, night with fires, rain) | **Not produced** | GPU screenshots are manual (`docs/plans/stories-M8c.md#us-246`, `US-247.md`) |

## For the owner
- Rows 7 and 8 need your PC with the GPU renderer; the agents could not capture them headless. Please run the manual checks and judge the look.
- Medium and High lighting are the same today; Low drops normal maps and fire shadows (docs/archive/milestones/Milestone-91.md).
- Weather light was added to `weather.json` for rain, storms, snow, fog and dust; tune the numbers freely.
