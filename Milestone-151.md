# Project Odyssey: assembly progress (151)

## AP-152 · 2026-10-06 · M11 Data editors (X-M11 Done)

Codex v2.13, requirements v2.12.

### Exit result: met
- Every data file has a schema; the owner edits entities, mechanics, story tuning, game rules and daily routines in Editor forms with pickers and validation, and the running game reloads them. One new plant kind and one changed mechanic were made in the Data tab only and seen in the running game (test `X-M11 Exit`).
- Test-run table (clean worktree of `qa` at 3e8ea6d, on the owner's PC):

| Check | Debug | Release |
|---|---|---|
| Build warnings | 0 | 0 |
| ctest (headless and shards) | 22 of 22 | 22 of 22 |
| ctest (window group, strict 3 s first frame in Release) | 8 of 8 | 8 of 8 |

- Found and fixed at the exit review: a run in play, the Editor and back to the game read freed memory (older than M11). `docs/gates/M11.md` lists it, with the other details.
- `qa` merged into `main`; CI on `main` and the tag `m11-done` are recorded below.

### Decisions and Codex issues
- Decisions Dominus took as delegated in M11: D-60 Q1 to Q18 (listed in `docs/gates/M11.md` for the owner to review).
- Codex issues: none new. Open for Anima: CI-011, CI-017, CI-018, CI-019, CI-020, CI-021.

### Open for the owner
- The M10b walkthrough answer; the on-screen manual checks of M11 (docs/gates/M11.md). Neither blocks anything.

### Next
- K-M12 World editing.
