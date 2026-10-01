# Exit review M8b: Resolution and GPU renderer (2026-10-01)

Codex v2.6 (X-M8b), requirements v2.8. Result: **all criteria met** (the last row needs the owner's eyes, see "For the owner").

| # | Exit criterion | Result | Evidence |
|---|---|---|---|
| 1 | The game renders at 960x540 through Luna's SDL_GPU renderer, with the old renderer as fallback | Met | US-230 (`docs/evidence/US-230/`: GPU and SDL pictures identical, 0 of 921,600 pixels differ); US-231 (virtual screen 960x540) |
| 2 | Looks the same at 2x camera zoom | Met | `docs/evidence/X-M8b/old-vs-new.png`: the world is identical, side by side (old 480x270 picture and new 960x540 at 2x zoom); US-232 |
| 3 | Windowed sizes, borderless and exclusive full screen work | Met | US-231: tests `US-231 ...` (presentation rectangle, Whole and Fill scaling, settings round trip), window tests in the 27 groups; Settings screen buttons |
| 4 | Every screen and editor panel is laid out for the new size | Met, with one note | US-233: `docs/evidence/US-233/contact-sheet.png`; test `US-233 Screens fit the interface at both UI scales`. The Editor keeps its 1x layout (placed from the 960x540 size). |
| 5 | 60 FPS at 1080p on the target mid-range PC (D-06) | Met on the dev PC, estimated for the minimum PC | US-234: 10 minutes, 500 people, 1080p, Release: 59.9 FPS, GPU 0.20 ms, draw 0.14 ms (`docs/evidence/US-234/perf-run-10min.txt`); scaled to the D-06 PC about 1.4 ms of 16.7 ms (`docs/plans/US-234.md`). Not yet measured on real minimum hardware. |

**Release check on the owner's PC.** `pwsh tools/verify.ps1 -Story X-M8b -Config Release`: build with zero warnings, 27 of 27 test groups passed, including the strict 3-second first-frame limit (the limit is 3 s unless running on GitHub, D-47). Output: `docs/evidence/X-M8b/windows-release.txt`.

**CI.** Green on `qa` for every M8b story (US-230 to US-233; the US-234 run is on the exit commit). CI on `main` is confirmed after the merge (see Milestone-81.md).

## For the owner
- At the default UI scale (1x) the HUD, hotbar and panels are half the size they were at 480x270 (see the right-hand picture in `old-vs-new.png`). UI scale 2x in Settings brings the old size back. If 2x should be the default, say so: it is one number (`uiScale` default in `settings.h`).
- US-234 found that the daily autosave stalls one frame (25 to 35 ms) with 500 people. Not in scope; a candidate for a background write.
- The minimum-PC frame figures are estimates until someone runs the 10-minute test on an RX 6600 / RTX 3060 class PC (`odysseus.exe --level assets/levels/camp.json --clan --people 500 --perf --renderer gpu --quit-after 600`).
