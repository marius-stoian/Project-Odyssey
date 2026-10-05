# Exit review M9b: trade economy (2026-10-05)

Codex v2.12 (X-M9bc, one exit for M9b and M9c, D-54). The tests were run once for both milestones, at the exit (owner, 2026-10-05).

**Verify.** `pwsh tools/verify.ps1 -Story X-M9bc -Config Both`: Debug and Release build with zero warning lines, 27 of 27 test groups passed in each. Output: `docs/evidence/X-M9bc/`.

Found and fixed by the one full verify (the stories were built without running the suite): the loaders of `trade.json`, `schedule.json` and `events.json` read their sections from the wrong object; the trade screen title had a double space; the Trade panel of the Editor covered the figure being edited (it now sits below the NPC panel); one shadowed variable (warning as error); one test whose level data changed (Tala's restock check). No test, threshold or budget was weakened.

| # | Exit criterion | Result | Evidence |
|---|---|---|---|
| 1 | Coin items and an abstract balance; item-value currencies (US-280) | Met | `economy_test`, `economy_editor_test` |
| 2 | Limited stock, daily weighted restock (US-281) | Met | `trade_market_test`, `trade_stock_test` |
| 3 | Prices from base x stock curve x drift, reputation last (US-282) | Met | `trade_price_test`, ADR-023 |
| 4 | Trade screen, Haggle, reputation bands, rare goods (US-283) | Met | `trade_deal_test`, `trade_screen_test`, `trade_gate_test` |
| 5 | Editor trade panel and the test-level traders (US-284) | Met | `trade_editor_test` |
| 6 | Screenshots of the trade screen and the Editor Trade panel | **Not produced** | GPU screenshots are manual (`docs/plans/stories-M9b.md#us-283`, `US-284.md`) |

## For the owner
- Row 6 needs your PC and the window.
