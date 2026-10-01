# Project Odyssey: assembly progress (84)

## AP-085 · 2026-10-02 · US-242 Day, night and seasons

| | |
|---|---|
| Assembly plan / requirements | **v2.6** / **v2.8** |
| Repository | `story/US-242` merged into `qa` |
| Milestone | M8c Lighting and shadows, in progress (K-M8c, US-240..US-242 Done) |

### State
- `tools/verify.ps1 -Story US-242` in Debug: zero warnings, 27 of 27; CI (Release) on the push to `qa`.
- The light follows the clan's clock: night 55% blue, orange dawn and dusk, summer day 15 h, winter 8 h; sun and moon directions for shadows (US-244). Evidence: `docs/evidence/US-242/day-cycle.png`.
- CI fix on the way: the 64-light budget test (US-240) failed on the GitHub runner's software adapter; it is now judged only off GitHub.

### Decisions
- None requested. Technical: keyframes are relative to sunrise and sunset so day length changes with the season from one file; `daylight` lives in calendar.json and the simulation ignores it (no simulation change).

### Next
- S-US-243 (fires, torches and glowing effects).