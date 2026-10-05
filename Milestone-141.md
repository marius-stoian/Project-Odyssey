# Project Odyssey: assembly progress (141)

## AP-142 · 2026-10-06 · M10b Editor help and live data (S-US-305)

Codex v2.13, requirements v2.12.

### State
- S-US-305 DONE locally: run saves carry a level baseline (`things.json` version 3: id and hash for every placed thing; `buildings.json` version 2: the link from a level building to its store building). On load, and on Play (F1) after an Editor save under a loaded run, things the level changed or added come fresh, things it removed leave the run, and everything else keeps its saved state. The status line counts the update. A save without a baseline loads as before once and is written with one. 11 new "US-305" test cases (`tests/game/level_merge_test.cpp`).
- Plan: `docs/plans/US-305.md` (how a run, `region.json` and the level file relate, and the deviations below). Guide: "Level edits and run saves" in `docs/guides/editor.md`. Teach-back in `docs/learning-journal.md`.

### Decisions and Codex issues
- Technical: one baseline for all six kinds in `things.json` instead of a copy in each of the three save files (they are written in one autosave); `buildings.json` stores only its link; the people's save is unchanged. No save-format change beyond the baseline and that link, so no design question for the owner.
- A generated region has no level file: no baseline, no merge.
- Open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020, CI-021.

### Next
- X-M10b: verify Debug and Release, `docs/gates/M10b.md`, `docs/gates/M10b-walkthrough.md`; the owner's Pass or Fail on the walkthrough is the only stop.
