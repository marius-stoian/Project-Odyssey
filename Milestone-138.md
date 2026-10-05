# Project Odyssey: assembly progress (138)

## AP-139 · 2026-10-05 · M10b Editor help and live data (S-US-302)

Codex v2.13, requirements v2.12.

### State
- S-US-302 DONE locally: every field of the four editors whose `help.json` entry names a source gets its list of values (numbers with default, limits and the last five typed; files; seventeen catalogs; fixed values). Debug build zero warnings, 27 of 27 test groups green (`docs/evidence/US-302/windows-debug.txt`), screenshot `docs/evidence/US-302/list-npc-hp.png`.
- Plan: `docs/plans/US-302.md`. Guide: "Suggestions (US-301, US-302)" in `docs/guides/editor.md`. Teach-back in `docs/learning-journal.md`.

### Decisions and Codex issues
- Technical: sources are functions the game gives `EditorHelp` (`OdysseyGame::suggestionNames`, `Editor::numberDefault`); fields are wired in `apply`; `suggest` is required in every `help.json` entry; a list of words completes the word after the last comma or space.
- The acceptance criteria name a "Classes" text field that does not exist (classes are ticked in a list); the list-completion rule is tested on Allow, Deny, Does and Tags with the class and interaction catalogs.
- Open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020.

### Next
- S-US-303 (live reload of every data file: reload registry, all or nothing, placed things follow, missing kind marker).
