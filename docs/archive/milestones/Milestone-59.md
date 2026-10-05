# Project Odyssey: assembly progress (59)

## AP-060 · 2026-10-01 · US-151 Done: tags and smart objects

| | |
|---|---|
| Assembly plan / requirements | **v2.0** / **v2.0** |
| Repository | `qa` (story/US-151 merged) |
| Milestone | **M7 World interactions**: 2 of 7 stories built |
| Next | **S-US-156** (hot reload F5 and the validation panel) |

### What happened
- US-151: plants, animals, weapons and character kinds carry `tags` (and plants `states`), derived from the old fields when not written; unknown tags in an interaction file warn with file and tag; `GameRuleContext` lets the rule language read the real game; the plant context menu lists what the interaction files offer (Gather, Inspect) with the data's labels and reasons; new `inspect.json`.
- Verification: `pwsh tools/verify.ps1 -Story US-151`, Debug and Release 0 warnings, 27 of 27 tests each; 5 new game test cases.

### Decided by Dominus (delegated technical choices)
- A plant is tagged `edible` only when it can be gathered today; solid fruit-bearing ones are tagged `fruit-bearing`, so no plant menu changed.
- Until the action runner (US-153), the menu runs Gather and Inspect with the existing code by interaction id; other data interactions say no code runs them yet.

### Heads-up for the owner (design, not decided here)
- `gather.json` (your brief's example) says "Nothing grows in winter" and Gather's range is 1.5 m; today's code has no winter rule and reaches 2 m. In a run during winter, Gather on edible plants is now greyed out. US-152 reconciles the shipped data with today's behaviour; tell me if you want winter gathering kept.
- CI on `qa` for US-150 (1d5b939) was still running when this story was merged.
