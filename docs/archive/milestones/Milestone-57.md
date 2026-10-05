# Project Odyssey: assembly progress (57)

## AP-058 · 2026-10-01 · K-M7 done: M7 World interactions kicked off

| | |
|---|---|
| Assembly plan / requirements | **v2.0** / **v2.0** |
| Repository | `qa` (CI green at 820d629 and e2710f0 pushes of P-009) |
| Milestone | **M7 World interactions**: kickoff done, 0 of 7 stories built |
| Next | **S-US-150** (interaction data and the rule language) |

### Owner decisions (D-36, asked in chat at the kickoff)
- Progress look: a ring over the target.
- NPC and animal interrupts: danger (hit, hostile or predator within 6 m) and critical need spikes only.
- Animals: prey flee from predators always, and from the hero within 5 m when the hero runs or is armed.
- Regrowth: short real-time seconds (about 15 s, as today), written as `after 15s` in the data.

### What was built
- `docs/plans/M7-interactions-design.md`: layers and the `WorldView` door between Game and Simulation, the grammar (tokens and precedence), registry and matching, action runner and timers, state storage and the save bump 3 to 4, the NPC chooser, hot reload, the move of the 16 hard-coded actions, the test plan.
- `docs/decisions.md` D-36; `docs/status.md` K-M7 Done, P-009 Done.

### Decided by Dominus (delegated)
- None. Technical choices (built-in effects first in US-152, one save bump, F5 only) are recorded in the design document.
