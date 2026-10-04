// US-282 Supply and demand, and reputation: the price of a good is the region's base price times the stock-ratio curve times the drift, with the trader's attitude to the
// hero applied last; some attitudes refuse to trade; rare goods need an opinion. All whole numbers (ADR-023).
#include "sim/trade_market.h"

#include <doctest/doctest.h>

#include <algorithm>

namespace rules = odysseus::sim::rules;
namespace sim = odysseus::sim;

namespace {

const sim::ItemCounts kValues = {{"flint", 3}, {"fur", 5}, {"berries", 1}, {"shells", 1}, {"obsidian", 20}};

rules::TradeProfile profile() {
    rules::TradeProfile out;
    out.stock = {{"flint", 6}, {"fur", 4}, {"berries", 4}};
    out.wants = {"berries"};
    return out;
}

sim::TradeMarket marketWith(int id = 1) {
    sim::TradeMarket market;
    REQUIRE(market.addTrader(id, profile(), 0));
    return market;
}

long long hero(const sim::TradeMarket& market, int id, const std::string& item, sim::Attitude attitude, const sim::RegionEconomy& economy = {}) {
    return market.heroPaysMilli(id, item, attitude, economy, kValues);
}

} // namespace

TEST_CASE("US-282 Curve: the price follows target over stock, clamped between half and double") {
    sim::TradeMarket market = marketWith();
    const sim::RegionEconomy region;
    CHECK(market.stockRatioPercent(1, "flint") == 100); // stock 6 of target 6
    CHECK(market.marketMilli(1, "flint", region, kValues) == 3000);
    market.removeStock(1, "flint", 2); // stock 4: 100 * 6 / 4 = 150
    CHECK(market.stockRatioPercent(1, "flint") == 150);
    CHECK(market.marketMilli(1, "flint", region, kValues) == 4500);
    market.removeStock(1, "flint", 2); // stock 2: 300, clamped to 200
    CHECK(market.stockRatioPercent(1, "flint") == 200);
    CHECK(market.marketMilli(1, "flint", region, kValues) == 6000);
    market.removeStock(1, "flint", 2); // an empty shelf asks the most, not more
    CHECK(market.stockRatioPercent(1, "flint") == 200);
    market.addStock(1, "flint", 20); // plenty: 100 * 6 / 22 = 27, clamped to 50
    CHECK(market.stockRatioPercent(1, "flint") == 50);
    CHECK(market.marketMilli(1, "flint", region, kValues) == 1500);
}

TEST_CASE("US-282 Scarce: asked twice a day apart while the good gets scarcer, the second price is higher") {
    sim::TradeMarket market = marketWith();
    const sim::RegionEconomy region;
    const long long first = hero(market, 1, "flint", sim::Attitude::Neutral);
    market.removeStock(1, "flint", 3); // the day's trading and the region's demand take flint away
    market.dailyUpdate(1, region);     // a day passes (this profile restocks nothing)
    const long long second = hero(market, 1, "flint", sim::Attitude::Neutral);
    CHECK(second > first);
    CHECK(first == 3000);
    CHECK(second == 6000); // stock 3 of 6: ratio 200
}

TEST_CASE("US-282 Region: the region's own price replaces the value of the item") {
    sim::TradeMarket market = marketWith();
    sim::RegionEconomy region;
    region.prices = {{"flint", 8}};
    CHECK(sim::TradeMarket::basePrice(region, kValues, "flint") == 8);
    CHECK(sim::TradeMarket::basePrice(region, kValues, "fur") == 5);
    CHECK(sim::TradeMarket::basePrice(region, kValues, "mystery") == 1); // an item nobody priced still costs one
    CHECK(market.marketMilli(1, "flint", region, kValues) == 8000);
}

TEST_CASE("US-282 Drift: each trade nudges the price, it is capped, and it decays back day by day") {
    sim::TradeMarket market = marketWith();
    const sim::RegionEconomy region;
    const long long base = hero(market, 1, "fur", sim::Attitude::Neutral);
    CHECK(base == 5000);
    market.nudge(1, "fur", 1, true); // the hero bought one: +3%
    CHECK(market.drift(1, "fur") == 3);
    CHECK(hero(market, 1, "fur", sim::Attitude::Neutral) == 5150);
    market.nudge(1, "fur", 2, false); // the hero sold two: -6% from there
    CHECK(market.drift(1, "fur") == -3);
    market.nudge(1, "fur", 100, true); // never beyond the cap of 40
    CHECK(market.drift(1, "fur") == 40);
    CHECK(hero(market, 1, "fur", sim::Attitude::Neutral) == 7000);

    int previous = market.drift(1, "fur");
    int days = 0;
    for (std::int64_t day = 1; day <= 30 && market.drift(1, "fur") != 0; ++day, ++days) {
        market.dailyUpdate(day, region);
        CHECK(market.drift(1, "fur") < previous); // falls every day
        CHECK(market.drift(1, "fur") >= 0);
        previous = market.drift(1, "fur");
    }
    CHECK(market.drift(1, "fur") == 0);
    CHECK(days <= 14); // 40, 30, 23, 18, 14, 11, 9, 7, 6, 5, 4, 3, 2, 1, 0: a share of itself, at least one point
    CHECK(hero(market, 1, "fur", sim::Attitude::Neutral) == base); // back to the plain price
    market.nudge(1, "fur", 1, false);
    CHECK(market.drift(1, "fur") == -3);
}

TEST_CASE("US-282 Reputation: a friendly trader is cheaper than a suspicious one, a hostile one will not trade") {
    sim::TradeMarket market = marketWith();
    const long long neutral = hero(market, 1, "flint", sim::Attitude::Neutral);
    const long long friendly = hero(market, 1, "flint", sim::Attitude::Friendly);
    const long long suspicious = hero(market, 1, "flint", sim::Attitude::Suspicious);
    const long long wary = hero(market, 1, "flint", sim::Attitude::Wary);
    const long long devoted = hero(market, 1, "flint", sim::Attitude::Enchanted);
    CHECK(neutral == 3000);
    CHECK(friendly == 2700); // -10%
    CHECK(suspicious == 3750); // +25%
    CHECK(wary == 3750);
    CHECK(devoted == 2400); // -20%
    CHECK(friendly < suspicious);
    CHECK(market.refuses(sim::Attitude::Hostile));
    CHECK_FALSE(market.refuses(sim::Attitude::Wary));
    CHECK_FALSE(market.refuses(sim::Attitude::Neutral));
    CHECK(market.reputationPercent(sim::Attitude::Friendly) == -10);
    CHECK(market.reputationPercent(sim::Attitude::Wary) == 25);

    // Two traders with equal stock, one friendly and one suspicious, asked the same item.
    sim::TradeMarket two;
    REQUIRE(two.addTrader(1, profile(), 0));
    REQUIRE(two.addTrader(2, profile(), 0));
    CHECK(hero(two, 1, "fur", sim::Attitude::Friendly) < hero(two, 2, "fur", sim::Attitude::Suspicious));
}

TEST_CASE("US-282 Paying the hero: wants at full value, other goods at half, the attitude the other way round") {
    sim::TradeMarket market = marketWith();
    const sim::RegionEconomy region;
    const auto pays = [&](const std::string& item, sim::Attitude attitude) { return market.traderPaysMilli(1, item, attitude, region, kValues); };
    CHECK(pays("berries", sim::Attitude::Neutral) == 1000); // a want: full value 1
    CHECK(pays("fur", sim::Attitude::Neutral) == 2500);     // not a want: half of 5
    CHECK(pays("fur", sim::Attitude::Friendly) == 2750);    // a friendly trader pays 10% more
    CHECK(pays("fur", sim::Attitude::Suspicious) == 1875);  // a suspicious one 25% less
    CHECK(pays("berries", sim::Attitude::Neutral) > pays("berries", sim::Attitude::Suspicious));
    // The sum of a bundle is rounded once: ten berries are ten units, not ten roundings.
    CHECK(sim::TradeMarket::toUnits(10 * pays("berries", sim::Attitude::Neutral)) == 10);
    CHECK(sim::TradeMarket::toUnits(2500) == 3);
    CHECK(sim::TradeMarket::toUnits(2499) == 2);
    // The id of someone who is not a trader prices nothing.
    CHECK(market.heroPaysMilli(99, "fur", sim::Attitude::Neutral, region, kValues) == 0);
}

TEST_CASE("US-282 Currency: a currency item is worth its value anywhere, never repriced") {
    sim::TradeMarket market = marketWith();
    sim::RegionEconomy region;
    region.currencies = {{"shells", 1}};
    market.addStock(1, "shells", 50);
    CHECK(hero(market, 1, "shells", sim::Attitude::Wary, region) == 1000);
    CHECK(hero(market, 1, "shells", sim::Attitude::Friendly, region) == 1000);
    CHECK(market.traderPaysMilli(1, "shells", sim::Attitude::Suspicious, region, kValues) == 1000);
    market.nudge(1, "shells", 5, true);
    CHECK(hero(market, 1, "shells", sim::Attitude::Neutral, region) == 1000);
}

TEST_CASE("US-282 Gate: rare goods need the band the trader names; a wary hero is not offered them, a friendly one is") {
    rules::TradeProfile rich = profile();
    rich.stock["obsidian"] = 2;
    rich.rare = {{"obsidian", "friendly"}, {"fur", "enchanted"}};
    sim::TradeMarket market;
    REQUIRE(market.addTrader(5, rich, 0));
    const sim::OpinionConfig opinions; // the bands of the shipped opinions.json: friendly from 10, enchanted from 40
    CHECK(market.isRare(5, "obsidian"));
    CHECK_FALSE(market.isRare(5, "flint"));
    CHECK(market.rareWord(5, "obsidian") == "friendly");

    const auto offered = [&](int opinion) { return market.offeredGoods(5, opinion, opinions); };
    const auto locked = [&](int opinion) { return market.lockedGoods(5, opinion, opinions); };
    const auto has = [](const std::vector<std::string>& list, const std::string& item) { return std::find(list.begin(), list.end(), item) != list.end(); };

    // A wary hero (opinion -45): the plain goods are offered, the rare ones are not.
    CHECK(has(offered(-45), "flint"));
    CHECK_FALSE(has(offered(-45), "obsidian"));
    CHECK_FALSE(has(offered(-45), "fur"));
    CHECK(has(locked(-45), "obsidian"));
    CHECK(has(locked(-45), "fur"));
    CHECK_FALSE(has(locked(-45), "flint"));
    // Friendly begins at 10: 9 is not enough, 10 is; the fur needs 40.
    CHECK_FALSE(has(offered(9), "obsidian"));
    CHECK(has(offered(10), "obsidian"));
    CHECK_FALSE(has(offered(10), "fur"));
    CHECK(has(offered(40), "fur"));
    CHECK(locked(40).empty());
    // A rare good that is out of stock is neither offered nor shown as locked.
    market.removeStock(5, "obsidian", 2);
    CHECK_FALSE(has(offered(100), "obsidian"));
    CHECK_FALSE(has(locked(-45), "obsidian"));
}

TEST_CASE("US-282 Saved: the drift is saved and read back") {
    sim::TradeMarket market = marketWith();
    market.nudge(1, "fur", 3, true);
    market.nudge(1, "flint", 2, false);
    const std::string text = market.toText();
    sim::TradeMarket loaded;
    loaded.restoreState(text);
    REQUIRE(loaded.addTrader(1, profile(), 0));
    CHECK(loaded.drift(1, "fur") == 9);
    CHECK(loaded.drift(1, "flint") == -6);
    CHECK(loaded.toText() == text);
}
