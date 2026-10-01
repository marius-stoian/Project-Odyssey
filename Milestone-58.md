# Project Odyssey: assembly progress (58)

## AP-059 · 2026-10-01 · US-150 Done: interaction data and the rule language

| | |
|---|---|
| Assembly plan / requirements | **v2.0** / **v2.0** |
| Repository | `qa` (story/US-150 merged) |
| Milestone | **M7 World interactions**: 1 of 7 stories built |
| Next | **S-US-151** (tags and smart objects) |

### What happened
- US-150: the language and the loader are in `src/sim/rule_*` and `interaction.*`; `assets/data/interactions/gather.json`; the owner's guide `docs/guides/interaction-data.md`; the game logs `Interactions: 1 loaded from 1 file(s), 0 error(s)` at start.
- Verification: `pwsh tools/verify.ps1 -Story US-150`, Debug and Release 0 warnings, 27 of 27 tests each. 20 new simulation cases plus 1 game case; the guide is checked against the code by a test.

### Decided by Dominus (delegated technical choices)
- Files are flat (`src/sim/rule_*.h`) instead of `src/sim/rules/`, because the layer check needs each header to include its layer's `boundary.h`. The design document is updated.
- Effect arguments are space-separated terms; a sum needs brackets (`give actor berries (1 + skill(gatherer))`).
- Our own 250-line JSON reader, because nlohmann cannot report lines.

### Still open
- CI on `qa` after the merge (see Limit.md).
- Owner's uncommitted `assets/levels/valley.json` edits are untouched.
