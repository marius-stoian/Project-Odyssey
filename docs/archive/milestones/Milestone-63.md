# Project Odyssey: assembly progress (63)

## AP-064 · 2026-10-01 · US-155 Done: Age 1 world objects

| | |
|---|---|
| Assembly plan / requirements | **v2.0** / **v2.0** |
| Repository | `qa` (story/US-155 merged) |
| Milestone | **M7 World interactions**: 6 of 7 stories built |
| Next | **S-US-154** (NPCs and animals use interactions), then X-M7 |

### What happened
- US-155: `objects.json` with the seven objects (D-35), each with tags, states and its interactions (9 new files); programmer art by code; the Editor's plant palette gets a last page for the objects; light a fire pit with a fire drill and it warms people nearby; food store, shelter, flint nodule, water and furs work; new built-in actions `restore` and `warm-nearby`, effect verb `fx`; `World::satisfyPersonNeed`.
- Verification: `pwsh tools/verify.ps1 -Story US-155`, 0 warnings, 27 of 27 tests in Debug and Release; 6 game cases and 1 sim case added; three older tests adapted for the new catalog and palette page (reasons in `docs/plans/stories-M7.md#us-155`).

### Codex issues
- CI-008: objects ride on the plant machinery (no level format bump or migration); the objects are the last page of the plant palette. Open for Anima.

### Decided by Dominus (delegated technical choices)
- The same plant list, placement, history and saves serve the objects; an object is never hidden by its states (that rule is for plants).
