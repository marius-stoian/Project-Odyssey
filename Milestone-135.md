# Project Odyssey: assembly progress (135)

## AP-136 · 2026-10-05 · M10b Editor help and live data (P-013, K-M10b)

Codex v2.13, requirements v2.12.

### State
- P-013 Done: Codex v2.13 adopted (`CLAUDE.md` names Codex v2.13 and requirements v2.12, D-15 includes M10b, `docs/status.md` lists K-M10b, US-300..US-305 and X-M10b). Requirements mirror is v2.12 with epic E29.
- K-M10b Done: owner answered the open questions in one chat round (D-59); design written in `docs/plans/M10b-editor-help-design.md`.

### Decisions and Codex issues
- D-59 (owner): tooltip after 0.4 s; suggestion list opens on focus; reload shows a 2-second toast plus the status line, a failed reload also stays in the mistakes panel with a red status line.
- Design finding: the code has about 100 fields in four editors (Level, Building, Graph, Story events), not about 41 in three. Built as D-58 Q5 says (every field in every editor); the coverage test covers all four.
- New for Anima: CI-020 (the Charter text in the Codex still says v2.11 and v2.10). Still open: CI-011, CI-017, CI-018, CI-019.

### Next
- S-US-300 (field tooltips from help.json), then US-301..US-305 and X-M10b (10-minute owner walkthrough is its only stop).
