# Project Odyssey: assembly progress (143)

## AP-144 · 2026-10-06 · M11 Data editors (K-M11)

Codex v2.13, requirements v2.12.

### State
- K-M11 Done. M10 and M10b are done and on `main` (X-M10, X-M10b ran their full verification and CI); the owner's walkthrough answer for M10b is still open (he deferred it), which blocks nothing in M11. D-40, D-41 and D-58 are Decided.
- Design: `docs/plans/M11-data-editors-design.md` (schema subset and validator, form widgets, reference index, live catalogs by id, Quick check, Game Rules, routines as profession schedules, pickers).
- Stories set to To do in order: US-190, US-191, US-193, US-194, US-195, US-196, US-192, then X-M11.
- Branch `qa` was fast-forwarded to `origin/qa` (it holds the lean-luna refactor and US-306). The owner's own edits to `assets/data/npcs/wanderer.json`, `assets/levels/level-7.json` and `assets/levels/level-5.json` are left uncommitted on purpose.

### Decisions and Codex issues
- D-60 (Decided by Dominus, delegated under D-41): the 13 questions of the design (one schema per file shape with an index, schema subset, fatal and warning errors, line scanner, canonical writer, own undo stack, live catalogs by id, rename scope, time-sliced Quick check, `play_rules`, profession routines in the US-290 format, atlas rebuild at US-192, M10b gate). The owner may override any of them.
- Codex issues: none new. Still open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020, CI-021 (this milestone resolves the technical content of CI-007 and CI-021).

### Next
- S-US-190 Schemas for every data file.
