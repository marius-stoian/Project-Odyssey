# Project Odyssey: assembly progress (139)

## AP-140 · 2026-10-05 · M10b Editor help and live data (S-US-303)

Codex v2.13, requirements v2.12.

### State
- S-US-303 DONE locally: a registry of reloadable data sets (`src/game/data_reload.*`) and the game's sets (`src/game/odyssey_reload.cpp`): interactions with dialogues and quests, NPC classes, lights, plants/objects/characters, help. All or nothing; placed plants follow the new catalog by kind name; a missing kind gets a red "?" and a warning; a two-second toast and the mistakes panel of every set. Debug build zero warnings, 27 of 27 test groups green (`docs/evidence/US-303/windows-debug.txt`). Release timings (NFR-09): interactions 4.4 ms, NPC classes 3.2, lights 0.2, catalog 4.3, help 0.4 (`docs/evidence/US-303/timings-release.txt`); `missing-marker.png`.
- One existing test changed on purpose: `US-156 A bad file lists ...` waits for the new "Reloaded" toast to expire before it counts draws.
- Plan: `docs/plans/US-303.md`. Guide: "Live data" in `docs/guides/editor.md`; the "restart the game" lines of the guides now say which files reload and which apply at the next start. Teach-back in `docs/learning-journal.md`.

### Decisions and Codex issues
- **CI-021 (open, for Anima and the owner):** weapons, animals, effects, weather, tiles, materials, building kinds, hero data (items) and sim data are not swapped live: the play state holds them by pointer, index or long-lived structure. A save says "<file> applies at the next start"; nothing is half-applied; F5 leaves them alone. Each needs its own story if the owner wants it live.
- Other open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020.

### Next
- S-US-304 (file watch for outside edits: poll every 250 ms, 300 ms debounce, own-write filter, `--no-watch`), then US-305 and X-M10b.
