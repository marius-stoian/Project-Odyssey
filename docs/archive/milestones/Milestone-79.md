# Project Odyssey: assembly progress (79)

## AP-080 · 2026-10-01 · US-233 Every screen at the new size

| | |
|---|---|
| Assembly plan / requirements | **v2.6** / **v2.8** |
| Repository | `story/US-233` merged into `qa` |
| Milestone | M8b Resolution and GPU renderer, in progress (US-230..US-233 Done) |

### State
- `tools/verify.ps1 -Story US-233` in Debug: zero warnings, 27 of 27; CI (Release) runs on the push to `qa`.
- Run screens, menu, Settings and the dialogue panel are laid out from the interface size; contact sheet and screenshots in `docs/evidence/US-233/` for the owner's approval.

### Decisions
- None requested. Technical: panel sized to content and centred horizontally; the Editor stays at 1x (already placed from the 960 x 540 size).

### Next
- S-US-234 (frame budget at the new size, uses D-06), then X-M8b.
