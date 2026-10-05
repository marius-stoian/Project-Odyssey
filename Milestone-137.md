# Project Odyssey: assembly progress (137)

## AP-138 · 2026-10-05 · M10b Editor help and live data (S-US-301)

Codex v2.13, requirements v2.12.

### State
- S-US-301 DONE locally: `SuggestList` in Luna (`src/luna/engine/ui.*`), `suggest` on `TextField` and `NumberField`, four new intents for the arrows, Tab and Escape. Debug build zero warnings, 27 of 27 test groups green (`docs/evidence/US-301/windows-debug.txt`), picture `docs/evidence/US-301/suggest-list.png`. No game field offers a list yet (US-302).
- Plan: `docs/plans/US-301.md`. Teach-back in `docs/learning-journal.md`.

### Decisions and Codex issues
- Technical: Enter accepts a row only after Up or Down (the first row is highlighted at once so Down then Tab gives the second row); new intents `ListUp`, `ListDown`, `ListTab`, `ListEscape` so W and S never move the highlight. Design document section 3 still reads right; the plan records the two changes.
- Open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020.

### Next
- S-US-302 (suggestions on every field: sources from the catalogs, files, fixed lists, numbers with default, min, max and the last 5 typed).
