# Project Odyssey: assembly progress (62)

## AP-063 · 2026-10-01 · US-153 Done: timed actions and world state

| | |
|---|---|
| Assembly plan / requirements | **v2.0** / **v2.0** |
| Repository | `qa` (story/US-153 merged) |
| Milestone | **M7 World interactions**: 5 of 7 stories built |
| Next | **S-US-155** (new world objects), then S-US-154, X-M7 |

### Owner decisions (D-37, asked in chat at the story)
- Gathering keeps working in winter (no winter rule).
- A picked plant is hidden until it is ripe again, then reappears in the same spot.

### What happened
- US-153: the action runner (`src/sim/action_runner.*`): timed actions with a ring of dots over the target, interruption by moving or attacking (gives nothing), effects that wait (`after 15s ...`) in a deterministic order; plants have states and are hidden out of their starting state; `things.json` saved with the autosave (plant states, waiting effects); `gather.json` is now a 3 s job.
- Verification: `pwsh tools/verify.ps1 -Story US-153`, 0 warnings, 27 of 27 tests in Debug and Release; 9 runner cases and 5 game cases.

### Decided by Dominus (delegated technical choices)
- Saved state goes in a second file, `things.json`, not a clan-save version bump; waiting effects are saved relative to now; running actions are not saved.
- The built-in `gather` became `gather-berries` (berries, skill, message); the plant's fate is written in `gather.json`.
