# Project Odyssey: assembly progress (146)

## AP-147 · 2026-10-06 · M11 Data editors (S-US-193 Done)

Codex v2.13, requirements v2.12.

### State
- S-US-193 Done on `qa` (branch `story/US-193`): in the Data tab an entry of a list can be **copied**, **renamed** and **deleted**. A rename lists every place that names the entry (schema links, rule texts, the entry's own file, the levels, the dialogue files), asks, and then writes all the files at once or none; the files are patched, so only the lines that name the entry change. A delete of a used entry lists the places and waits for "Delete anyway"; Ctrl+Z brings it back. **Interactions** in the top bar opens the interaction graph on the first interaction of the entry's kind. A copied plant is a plant of the game at once.
- Debug build of every program and test executable: zero warnings. Own cases: sim 41 and game 25 of the US-19x cases pass; the full verification and CI run at X-M11 (D-41).
- Clean-worktree check of `qa` at 2cbba96 (after US-191), Debug: build exit 0, ctest 22 of 22 and window group 8 of 8 passed.
- Guide `docs/guides/data-editor.md` ("Copy, rename, delete"), plan `docs/plans/stories-M11.md#us-193`, teach-back in `docs/learning-journal.md`, evidence `docs/evidence/US-193/`.

### Decisions and Codex issues
- No new owner-facing question. D-60 Q8 followed. A rename is refused while the open file has unsaved changes (it writes files); saved runs are not rewritten.
- Codex issues: none new.

### Next
- S-US-194 Mechanics forms, story forms and Quick check (mechanics data made live).
