# Project Odyssey: assembly progress (68)

## AP-069 · 2026-10-01 · US-160 verified and merged; P-010 adopted

| | |
|---|---|
| Assembly plan / requirements | **v2.1** / **v2.2** |
| Repository | `qa` (US-160 merged, `f317b4d`); `main` at `m7-done` |
| Milestone | M8 Speak to NPCs, in progress (K-M8 and US-160 Done) |

### State
- `m7-done` is tagged and pushed; CI on `main` was green.
- US-160 (the `.dlg` format) passed `tools/verify.ps1 -Story US-160`: zero warnings, 27 of 27 tests in Debug and Release. `chore/k-m8` and `story/US-160` are merged into `qa` and pushed; CI on that merge was queued at the time of writing.
- P-010 adopted Codex v2.1: D-40 and D-41 were already synced into `docs/decisions.md`, `CLAUDE.md` carries v2.1; this session added P-010 and the 47 rows for K-M10..X-M14 to `docs/status.md`, and marked CI-007 and CI-008 resolved in `docs/codex-issues.md`. X-M6 now waits for X-M14.
- The CHANGELOG conflict on merge (line endings) was resolved by keeping the `qa` file and inserting the US-160 entry.
- The owner's uncommitted edits to `assets/levels/valley.json` were left untouched and uncommitted.

### Decisions
- None requested. M8 and M9 keep D-35 (per-story verify and CI); M10-M14 follow D-41 (design questions delegated to Dominus, tests at exit reviews).

### Next
- Wait for green CI on `qa` (US-160 merge, then P-010), then S-US-161 (conversation runtime and the dialogue panel; design in `docs/plans/M8-dialogue-design.md`, D-38).
