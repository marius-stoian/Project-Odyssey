# Project Odyssey: assembly progress (136)

## AP-137 · 2026-10-05 · M10b Editor help and live data (S-US-300)

Codex v2.13, requirements v2.12.

### State
- S-US-300 DONE locally: every text and number field of the Level, Building, Graph and Story event editors shows purpose, range and example after the pointer rests on it for 0.4 s (`assets/data/editor/help.json`, 127 entries). Debug build zero warnings, 27 of 27 test groups green (`docs/evidence/US-300/windows-debug.txt`), screenshot `docs/evidence/US-300/tooltip-npc-sword.png`.
- Plan and checks: `docs/plans/US-300.md`. Guide: "Tooltips and suggestions" in `docs/guides/editor.md`. Teach-back in `docs/learning-journal.md`.

### Decisions and Codex issues
- Technical (design document updated): field ids are made from the panel and the label (`npc.sword`), not hand-written at each call site; a coverage test tours every panel state.
- The design counted about 41 fields in three editors; there are 127 in four (the Story events list is the fourth). Built to D-58 Q5 (every field).
- Open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020.

### Next
- S-US-301 (suggestion list widget in Luna: filter, keys, placement), then US-302..US-305 and X-M10b.
