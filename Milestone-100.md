# Project Odyssey: assembly progress (100)

## AP-101 · 2026-10-04 · US-267 Actions and the Actions pop-up

| | |
|---|---|
| Assembly plan / requirements | **v2.11** / **v2.10** (mirror) |
| Repository | `story/US-267` merged into `qa` |
| Milestone | M9a NPC foundation, in progress |

### State
- Debug build with zero warnings; tests written, not run (one full verify at X-M9a). `tests/game/npc_actions_test.cpp`.
- Tags then allow/deny lists decide the offers (simulation), an NPC's menu hides what it cannot do for its own reasons, the Actions pop-up (key X, 'Actions...') lists everything with what it needs. Manual checks: `docs/plans/US-267.md`.

### Decisions
- Technical: 'requirements in plain words' are the `else` text of the interaction's `requires` (the owner writes 'needs: friendly or better' next to the `opinion(npc, hero) >= 10` test); no new rule function was added. The shipped Trade action arrives with M9b; the tests use a test file for it.
- Technical: a later layer's allow can lift an earlier deny, a deny inside one layer beats an allow (as decided in US-261); denied actions are not listed in the pop-up.

### Next
- S-US-268 (Editor NPC panel: classes, attitude, dialogues by partner type, actions, family), then US-269, US-270, X-M9a.
