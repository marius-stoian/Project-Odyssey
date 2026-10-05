# Project Odyssey: assembly progress (140)

## AP-141 · 2026-10-05 · M10b Editor help and live data (S-US-304)

Codex v2.13, requirements v2.12.

### State
- S-US-304 DONE locally: `FileWatcher` (`src/game/data_reload.*`) notices files saved outside the game and the game reads the set again by itself within about a second (a round of looks takes at most half a second, a change must be quiet for 0.3 s). The game's own writes are ignored once; the open level is reloaded when the Editor has nothing unsaved, else nothing is overwritten and the status line says the file changed. `--no-watch` turns it off. Debug build zero warnings, 27 of 27 test groups green (`docs/evidence/US-304/windows-debug.txt`). Release cost on the shipped data (194 files): average 0.164 ms a tick, 95th percentile 0.641 ms (`docs/evidence/US-304/watch-cost-release.txt`).
- Plan: `docs/plans/US-304.md`. Guide: "Files saved outside the game" and `--no-watch` in `docs/guides/editor.md`; guides and lesson 18 updated. Teach-back in `docs/learning-journal.md`.

### Decisions and Codex issues
- Technical: a round of looks at every file is spread over several ticks (20 files a tick) to keep each tick cheap; the class is off by default and `main` turns it on unless `--no-watch`, so a game object made by a test never reloads by itself (determinism).
- The cost budget is read as the average per tick (a frame pays a third of it); the tick that looks at twenty files costs up to about 0.65 ms (95th percentile).
- Open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020, CI-021.

### Next
- S-US-305 (level edits win over the run save: per-id hash baseline in `things.json` version 3, with a migration), then X-M10b.
