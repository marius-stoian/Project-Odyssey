# Project Odyssey: assembly progress (64)

## AP-065 · 2026-10-01 · US-154 Done: NPCs and animals use interactions

| | |
|---|---|
| Assembly plan / requirements | **v2.0** / **v2.0** |
| Repository | `qa` (story/US-154 merged) |
| Milestone | **M7 World interactions**: 7 of 7 stories built; X-M7 next |
| Next | **X-M7** exit review, then K-M8 |

### What happened
- US-154: clan members and animals score the interactions near them with the files' `npc.score`, walk there and do them with the same runner as the hero; harmless animals (deer, rabbits) now walk, graze and flee a wolf or an armed or moving hero; a hostile within 6 m drops what they were doing; `need(...)` now means how much is missing.
- Verification: `pwsh tools/verify.ps1 -Story US-154`, 0 warnings, 27 of 27 tests in Debug and Release; 3 sim and 6 game cases; the 10,000-tick (3,000 in Debug) same-seed determinism check passes.
- CI on `qa`: US-152 and US-153 green; US-155 (5ea7590) was running.

### Decided by Dominus (delegated technical choices)
- Fighting monsters (wolf, goblin) stay put and act as threats; only harmless animals act.
- Clan members and animals are not saved mid-errand; after a load everyone looks around again.
- One test (`US-151 Catalog entries`) used the clover, which now carries the tag `grass`; it uses lavender.
