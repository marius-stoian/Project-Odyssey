# Story plans: M9b

Per-story plans for milestone M9b, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-280](#us-280)
- [US-281](#us-281)
- [US-282](#us-282)
- [US-283](#us-283)
- [US-284](#us-284)

---

<a id="us-280"></a>

## US-280 manual checks (currencies per region)

1. **Open.** F2, click **Level**, click **Economy...**: a panel with **Money**, **Prices**, **Goods**.
2. **Set.** In **Money** type `shells=1`, press Enter: the status line says `currencies: shells=1`. Click **Save** (Ctrl+S): `assets/levels/<level>.json` has `"economy": { "currencies": { "shells": 1 } }` and `"levelVersion": 5`.
3. **Mistake.** In **Money** type `shells=free`: the status line explains and the field shows `shells=1` again. Type `Shells=1`: refused (not an item id).
4. **Undo.** Ctrl+Z removes the currency, Ctrl+Y brings it back.
5. **None.** Clear **Money** and save: the file has no `economy` at all.
6. **Play.** F1: the level runs with the money (trades accept shells from US-283 on).

---

<a id="us-281"></a>

## US-281 manual checks (trader stock)

1. **Profile.** Edit a level by hand (the Editor panel comes with US-284): give a placed wanderer `"classes": ["trader"]` and `"trade": { "stock": { "fur": 3 }, "restockPerDay": { "flint": 1 }, "wants": ["berries"] }`.
2. **Trader.** F1: the NPC's menu (right-click) is the menu of a trader; **X** shows its actions. Without a `trade` block (and no profile in its class) it has no trade actions.
3. **Restock.** Wait one in-game day (2 minutes, or the Fast speed): the trader has 1 flint; another day, 2. Walk far away and wait: the same.
4. **Save.** In a clan level (`--clan`) the autosave at each day's end writes `trade.json` next to `npcs.json`; load the save: the stock is as it was.
5. **Mistake.** Write `"stock": { "Flint": 6 }`: the level is refused, naming the file, `characters[n]` and "item id".

---

<a id="us-282"></a>

## US-282 manual checks (supply and demand, and reputation)

1. **Scarce.** Give a trader `"stock": { "flint": 6 }` and nothing else. Open its trade screen (US-283): flint costs its value (3). Buy some flint: the next price is higher (the shelf is emptier and the drift rises). Wait a day: the drift has fallen a quarter.
2. **Reputation.** Two traders with the same stock, one `"attitude": "friendly"`, one `"suspicious"`: the friendly one asks 10% less, the suspicious one 25% more for the same good.
3. **Hostile.** A trader whose attitude is `hostile` offers no trade (US-283).
4. **Gate.** Give a trader `"rare": { "obsidian": "friendly" }` and some obsidian in stock; with a wary hero press **X** next to it: **Ask about rare goods** is greyed out with its reason. Give it gifts until it is friendly: the action is offered, and its message names the rare goods.

---

<a id="us-283"></a>

## US-283 manual checks (the trade screen for any trader)

1. **Open.** Put a trader (US-281) next to the hero; right-click it: **Trade** (a hostile or empty one has none; **X** says why). The screen shows your bag, its stock, the prices `@`, the balance bar.
2. **Barter.** Put berries on the table and take flint: the bar fills; **Deal** appears when received is at least given. Deal: the goods move, the prices have drifted (the flint is dearer, the berries cheaper), its opinion of you rose by 5.
3. **Currency.** In the Editor (**Level**, **Economy...**) set `shells=1`, give the hero shells (a gift, or the console), trade: the balance shows them, buy a fur with **Pay from balance**, **Close**: the change is back in the bag as shells.
4. **None.** In a level with no currency the screen says barter only and has no balance buttons.
5. **Haggle.** Press **Haggle**: a message with the chance and the roll; a win shows `You haggled well: 10% off today`; the button is greyed out until the next day.
6. **Rivals.** In a region game, a rival camp's **Barter** is the old screen with Propose, Pay later and the counter-offer.

---

<a id="us-284"></a>

## US-284 manual checks (the Editor trade panel)

1. **NPC.** F2, Select tool, click Tala: beside the NPC panel is the Trade section with her stock (`berries=4 flint=6 fur=2`), restock, picks, weights, wants (`berries`). Change **Stock** to `fur=3`, press Enter: the status line says `trade stock`; Ctrl+Z puts it back.
2. **Mistake.** Type `fur=lots` in **Stock**: the status line explains and the line shows the old text. Type `obsidian=nice` in **Rare**: refused (the bands are listed).
3. **Save and play.** Ctrl+S, F1, trade with Tala: the furs, the prices and the wants are as typed; wait a day: one more flint.
4. **Class.** **Class**, pick `trader`, give it `flint=4` in **Stock**, **Save**: every NPC of the class that sets no stock of its own has the flint (a trader class with no profile makes no trader).
5. **Kinds.** **Class**, **Kinds**, `wanderer`: **Wants** `berries fur`, **Save**: all wanderers want them.
6. **Walk-through.** Follow the trade steps in `docs/guides/npc-data.md` (the test level: Tala, Harn, shells).
7. **Screenshots.** `odysseus.exe --level assets/levels/npc-test.json --screenshot x.bmp --quit-after 3` with the Trade section open, into `docs/evidence/US-284/` (owner, with the GPU).
