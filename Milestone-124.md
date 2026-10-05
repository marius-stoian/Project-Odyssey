# Project Odyssey: assembly progress (124)

## AP-125 · 2026-10-05 · M9 Interaction and dialogue editor (beta plan, documentation only)

| | |
|---|---|
| Assembly plan / requirements | **v2.12** (repository copy) / **v2.10** (mirror) |
| Repository | `qa`, documentation only |
| Milestone | M9 Interaction and dialogue editor |

### State
- Dominus wrote the owner's beta test plan for after M9: `docs/plans/Beta-test-after-M9.md`. No code, data or test changed.
- X-M9 (the one full verify, `docs/gates/M9.md`, merge qa into main, tag m9-done, CI on main) has not run yet and still comes first.

### Decisions and Codex issues
- CI-016 for Anima: the Codex has no beta step. Proposed: B-M9 between X-M9 and K-M10, with the plan's exit rule as the condition for starting K-M10.
- Not changed on purpose: the requirements (Drive master, v2.10) and the Codex (Drive master, v2.12). Both are edited on Drive, not here; the plan needs no requirement change.

### Files changed
`docs/plans/Beta-test-after-M9.md` (new), `docs/README.md`, `README.md`, `docs/plans/M9-graph-editor-design.md`, `docs/codex-issues.md`, `CHANGELOG.md`, `Limit.md`, `Milestone-124.md`.

### Next
- X-M9, then the owner runs the beta plan on `main`, then fix stories for the findings, then K-M10.
