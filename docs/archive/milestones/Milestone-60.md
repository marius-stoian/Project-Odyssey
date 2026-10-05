# Project Odyssey: assembly progress (60)

## AP-061 · 2026-10-01 · US-156 Done: hot reload (F5) and the validation panel

| | |
|---|---|
| Assembly plan / requirements | **v2.0** / **v2.0** |
| Repository | `qa` (story/US-156 merged) |
| Milestone | **M7 World interactions**: 3 of 7 stories built |
| Next | **S-US-152** (the context menu from data) |

### What happened
- US-156: F5 (new intent `Reload`) reads the interaction files again, all or nothing; a red panel lists `file:line: message` in Game and Editor modes and closes after a clean reload; the time is logged (well under a second).
- Verification: `pwsh tools/verify.ps1 -Story US-156`, 0 warnings, 27 of 27 tests in Debug and Release; 4 new test cases. CI on `qa` for US-150 is green (1d5b939).

### Codex issue
- CI-007: the prompt says F5 reloads catalogs and dialogue too. Catalogs are held by raw pointers in the play state (a plain swap would dangle), and dialogue does not exist before M8. Built as the acceptance criteria and design describe (interactions only).

### Decided by Dominus (delegated technical choices)
- A reload with any mistake keeps the whole old registry (no half-new world); the swap happens at the start of the tick that sees F5.
