# Project Odyssey: assembly progress (56)

## AP-057 · 2026-10-01 · P-009: Codex v2.0 adopted, test debt paid

| | |
|---|---|
| Assembly plan / requirements | **v2.0** / **v2.0** |
| Repository | local `qa` (main fast-forwarded into it first); pushed with this snapshot |
| Milestone | between M5 and M7 |
| Next | **K-M7** (ask the owner its design questions first, D-22 and D-35) |

### What happened
- Switched to `qa`, fast-forwarded it to `main` (the "Merge qa: window tests" commit).
- `docs/status.md`: Codex v2.0, P-009, and K-M7, S-US-150..156, X-M7, K-M8, S-US-160..165, X-M8, K-M9, S-US-170..175, X-M9 (To do) added before K-M6; X-M6 stays Blocked until X-M9 is done.
- `docs/decisions.md`: D-08 Decided (answered by D-34); D-34 present; D-35 added.
- Ran `pwsh tools/verify.ps1 -Story P-009`: Debug and Release both build with 0 warning lines; **25 of 25 tests pass in each** (evidence `docs/evidence/P-009/`). No failures, so no fixes and no test changes.
- `docs/gates/test-debt.md` records the run; the owed checks in `docs/gates/M2d.md`, `M4.md`, `M5.md`, `M6.md` are marked as closed by it where a test run was the debt.

### Decided by Dominus (delegated)
- None. D-35 was decided by the owner.

### Still open
- Owner's uncommitted edits to `assets/levels/valley.json` (extra wanderers) are not touched or committed by this step.
- CI on `qa` is green ([run 36832983954](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36832983954)). One test fix was needed on the way (`US-029` flight run timing, commit `9a4b200`), and the owner asked for shorter CI: parallel non-window tests, three game-test shards, thinner Debug-only grids (commit `820d629`). Details in `docs/gates/test-debt.md`.
- M5 `run_test.cpp` never written; M6 zip, soak and stability checks; X-M6 waits for X-M9.
