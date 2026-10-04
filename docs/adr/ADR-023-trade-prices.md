# ADR-023: Trade prices: base x stock curve x drift, reputation last, whole numbers

Status: Accepted (owner decisions D-54 Q1-Q8, 2026-10-05; the numbers are Dominus's technical choices, all data-tunable in `assets/data/sim/trade.json`).

## Context
The owner wants prices that follow supply and demand and the trader's opinion of the hero, stock that is limited and comes back daily, and both barter and currency (D-54, D-52 Q-14..Q-17). The simulation must be deterministic and free of floating point (Charter rule 6): a price computed on two machines, or after a save and load, must be the same number.

## Decision
Every price is in **value units**: the `value` of an item in `assets/data/hero/items.json`, or the region's own price for it (`economy.prices` in the level). A price is computed in **thousandths** of a unit ("milli"), so a bundle of goods is added up exactly and rounded once, for showing (`toUnits`, half up).

For one piece of a good at a trader:

1. `base` = the region price, else the item value, at least 1.
2. `ratio = clamp(100 * target / max(stock, 1), 50, 200)`: the target is the trader's starting stock of that good (`defaultTarget` = 6 for a good it does not stock). Stock at the target gives 100, a shelf half empty 200, an empty shelf 200, a shelf at twice the target 50.
3. `drift`, in percent, signed: each piece the hero buys raises it by 3, each piece the hero sells lowers it by 3, never beyond 40 either way; every day it falls back toward zero by a quarter of itself (at least one point).
4. `market = base x ratio% x (100 + drift)%`.
5. The attitude of the trader to the hero is applied last, to the exact figure: `hero pays = market x (100 + percent)%` with the percents of `trade.json` (friendly -10, neutral 0, wary and suspicious +25, enchanted and lovingly -20, scared and enviously +10); `hostile` refuses to trade. What the trader pays the hero is `market x (100 - percent)% x want%`, where `want%` is 100 for a good the trader wants and 50 for any other good (D-52 Q-17).
6. A currency item (an item of the region's `currencies`) is worth its value anywhere: its price ignores stock, drift and attitude. Coins in the hero's bag become a balance when the trade screen opens and come back as coin, highest value first, when it closes (US-283).
7. A barter is accepted when what the trader receives is worth at least what it gives, both at its prices; the balance bar shows the difference live.
8. Haggle: once per trader per in-game day; chance `clamp(20 + opinion / 4 + persuasion x 3, 5, 85)` percent against a seeded roll of the world seed, the trader and the day; success is a 10% discount (the hero pays 10% less, the trader pays 10% more) for the rest of that day, failure costs 5 opinion. Persuasion is the hero's Trade affinity divided by 10.

Rare goods are offered only when the opinion of the trader of the hero is at least the start of the band named in the trader's `rare` table (bands from `opinions.json`).

## Why
- **One formula, every layer data.** The owner asked for all layers (region base, stock, drift, reputation); each is one multiplication of a percent, so the order does not change the result beyond rounding, and the file `trade.json` holds every number.
- **Milli units and one rounding.** A cheap good (a berry at value 1) at half price is half a unit; rounding each piece would make a handful of berries worth too much or too little. Summing in thousandths and rounding once keeps a bundle exact.
- **Seeded rolls, no hidden state.** The restock and the haggle use a fresh generator from (world seed, day, trader), so they do not depend on how many other rolls happened, on the frame rate or on whether the trader was near.
- **64-bit intermediates.** The largest product is about 3.5e16 (base 100,000 x 1000 x 200 x 140 x 125 x 100), far below the 9.2e18 of `long long`.

## Consequences
- Changing a number is a data edit; the tests read the shipped file where they depend on it.
- A trader with a large `stock` target sells cheaper per piece when it is overstocked and dearer when scarce: the owner sets personality through the starting stock and the restock.
- The price of a good depends on the stock of that trader only; a region-wide market (prices moved by all traders together) is a later story if the owner wants it.
