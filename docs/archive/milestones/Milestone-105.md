# Project Odyssey: assembly progress (105)

## AP-106 · 2026-10-05 · US-280 Currencies per region

| | |
|---|---|
| Assembly plan / requirements | **v2.12** (repository copy, CI-013) / **v2.10** (mirror) |
| Repository | `story/US-280` merged into `qa` |
| Milestone | M9b Trade economy, story 1 of 5 |

### State
- Kickoff K-M9b done: the owner's answers for M9b and M9c are D-54 (`docs/decision-requests/D-54.md`; the owner named it D-53, which M9a had used). Design notes extended in `docs/plans/M9-npc-design.md`. Codex amended to v2.12 in the repository copy (CI-013).
- US-280 written: region economy (currencies, prices, resources) in the level (level version 5), the Economy panel in the Editor, item `shells`. Owner rule: nothing is built or tested until M9b and M9c are both implemented; the one full verify is X-M9bc.

### Decisions
- D-54 (owner, 2026-10-05). Technical: persuasion = Trade affinity / 10; prices in item-value units; schedules on the hour (see D-54 and the design notes).

### Next
- US-281 (trader stock, restock, wants), US-282 (prices and reputation), US-283 (the trade screen), US-284 (Editor trade panel), then K-M9c and US-290..US-294, then the one full verify X-M9bc.
