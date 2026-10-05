# Project Odyssey: assembly progress (106)

## AP-107 · 2026-10-05 · US-281 Trader stock

| | |
|---|---|
| Assembly plan / requirements | **v2.12** (repository copy, CI-013) / **v2.10** (mirror) |
| Repository | `story/US-281` merged into `qa` |
| Milestone | M9b Trade economy, story 2 of 5 |

### State
- US-281 written: the trade profile (class, kind, placed NPC), limited stock, the daily restock with weighted random picks, wants, saved trade.json, F5 refresh. Nothing built or run yet (owner rule); the one full verify is X-M9bc.

### Decisions
- D-54 (owner, 2026-10-05) covers the design; technical choices are in the design notes and ADRs.

### Next
- US-282 (prices from base x stock curve x drift, reputation, gates), US-283 (the trade screen), US-284 (Editor trade panel), then K-M9c and US-290..US-294, then X-M9bc.
