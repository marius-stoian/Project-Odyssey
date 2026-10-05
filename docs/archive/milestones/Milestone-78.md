# Project Odyssey: assembly progress (78)

## AP-079 · 2026-10-01 · US-232 Camera zoom and UI scale

| | |
|---|---|
| Assembly plan / requirements | **v2.6** / **v2.8** |
| Repository | `story/US-232` merged into `qa` |
| Milestone | M8b Resolution and GPU renderer, in progress (K-M8b, US-230, US-231, US-232 Done) |

### State
- `tools/verify.ps1 -Story US-232` in Debug: zero warnings, 27 of 27 test groups; CI (Release) runs on the push to `qa`.
- Camera zoom 1x/2x (default 2x) and UI scale 1x/2x (default 1x) through `ScaledRenderer`; settings buttons, keys + and - and the mouse wheel; saved in `settings.json`. GPU screenshots in `docs/evidence/US-232/`.
- Standing rule recorded: the owner told Mraw to stop asking for permissions; the Dominus skill and memory carry it.

### Decisions
- None requested. Technical: a scaling decorator over the Renderer interface instead of per-widget scaling (one place, crisp whole-pixel blocks); the Editor stays at 1x until US-233.

### Next
- S-US-233 (every screen and editor at 960x540), then US-234 and X-M8b.
