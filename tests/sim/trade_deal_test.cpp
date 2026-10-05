// US-283 The trade screen's rules: barter and the region's money, the balance bar (received against given), the purse, Haggle (one seeded try a day), and the saved balance.
#include "sim/trade_market.h"

#include <doctest/doctest.h>

namespace rules = odysseus::sim::rules;
namespace sim = odysseus::sim;

namespace {

const sim::ItemCounts kValues = {{"flint", 3}, {"fur", 5}, {"berries", 1}, {"shells", 1}};
const sim::OpinionConfig kOpinions; // the bands of the shipped opinions.json

rules::TradeProfile profile() {
    rules::TradeProfile out;
    out.stock = {{"flint", 6}, {"fur", 4}};
    return out;
}

sim::TradeMarket marketWith(sim::PriceConfig config = {}, std::uint64_t seed = 1) {
    sim::TradeMarket market(config, seed);
    REQUIRE(market.addTrader(1, profile(), 0));
    return market;
}

sim::RegionEconomy shells() {
    sim::RegionEconomy economy;
    economy.currencies = {{"shells", 1}};
    return economy;
}

sim::TradeMarket::Quote quote(const sim::TradeMarket& market, const sim::TradeMarket::Deal& deal, sim::Attitude attitude, const sim::RegionEconomy& economy = {}) {
    return market.quote(1, deal, attitude, 25, kOpinions, economy, kValues);
}

} // namespace

TEST_CASE("US-283 Trade: a friendly trader with 6 flint and a hero who gives berries worth 2 flint: the goods move") {
    sim::TradeMarket market = marketWith();
    sim::TradeMarket::Deal deal;
    deal.heroGives = {{"berries", 10}};
    deal.heroGets = {{"flint", 2}};
    // A friendly trader asks 2.7 for a flint and pays 0.55 for a berry it does not want (half of 1, and 10% more because it likes the hero).
    CHECK(market.heroPaysMilli(1, "flint", sim::Attitude::Friendly, {}, kValues) == 2700);
    CHECK(market.traderPaysMilli(1, "berries", sim::Attitude::Friendly, {}, kValues) == 550);
    const auto priced = quote(market, deal, sim::Attitude::Friendly);
    CHECK(priced.receivesMilli == 5500);
    CHECK(priced.givesMilli == 5400);
    CHECK(priced.acceptable);

    // Nine berries are not enough: the bar says by how much.
    deal.heroGives = {{"berries", 9}};
    const auto short9 = quote(market, deal, sim::Attitude::Friendly);
    CHECK_FALSE(short9.acceptable);
    CHECK(short9.problem.find("more in value") != std::string::npos);
    CHECK(market.execute(1, deal, sim::Attitude::Friendly, 25, kOpinions, {}, kValues).done == false);
    CHECK(market.stock(1, "flint") == 6); // nothing moved

    deal.heroGives = {{"berries", 10}};
    const auto done = market.execute(1, deal, sim::Attitude::Friendly, 25, kOpinions, {}, kValues);
    REQUIRE(done.done);
    CHECK(done.heroGave == sim::ItemCounts{{"berries", 10}});
    CHECK(done.heroGot == sim::ItemCounts{{"flint", 2}});
    CHECK(market.stock(1, "flint") == 4); // the trader has two fewer
    CHECK(market.stock(1, "berries") == 10); // and the berries
    CHECK(market.drift(1, "flint") == 6);   // the hero bought two: the price nudges up
    CHECK(market.drift(1, "berries") == -30);
}

TEST_CASE("US-283 Acceptance: received at least given, at the prices of the trader; equal is accepted") {
    sim::TradeMarket market = marketWith();
    sim::TradeMarket::Deal deal;
    deal.heroGets = {{"flint", 1}}; // 3000 at the plain price
    deal.heroGives = {{"fur", 2}};  // half of 5, twice: 5000? the trader pays 2500 a fur
    CHECK(quote(market, deal, sim::Attitude::Neutral).receivesMilli == 5000);
    CHECK(quote(market, deal, sim::Attitude::Neutral).acceptable);
    deal.heroGets = {{"fur", 1}}; // 5000 for a fur it sells; 2 fur given at 2500 each: exactly equal
    CHECK(quote(market, deal, sim::Attitude::Neutral).acceptable);
    CHECK(quote(market, deal, sim::Attitude::Neutral).receivesMilli == quote(market, deal, sim::Attitude::Neutral).givesMilli);
    // A suspicious trader pays less and asks more: the same deal is not enough.
    CHECK_FALSE(quote(market, deal, sim::Attitude::Suspicious).acceptable);
    // Nothing offered, nothing asked.
    CHECK_FALSE(quote(market, {}, sim::Attitude::Neutral).acceptable);
}

TEST_CASE("US-283 Hostile: a trader who is hostile to the hero refuses every deal") {
    sim::TradeMarket market = marketWith();
    sim::TradeMarket::Deal deal;
    deal.heroGives = {{"fur", 4}};
    deal.heroGets = {{"flint", 1}};
    const auto priced = quote(market, deal, sim::Attitude::Hostile);
    CHECK(priced.refused);
    CHECK_FALSE(priced.acceptable);
    CHECK_FALSE(market.execute(1, deal, sim::Attitude::Hostile, -80, kOpinions, {}, kValues).done);
}

TEST_CASE("US-283 Limits: the trader's stock, rare goods, coins and the balance bound a deal") {
    rules::TradeProfile rich = profile();
    rich.stock["obsidian"] = 2;
    rich.rare = {{"obsidian", "friendly"}};
    sim::TradeMarket market;
    REQUIRE(market.addTrader(1, rich, 0));
    sim::TradeMarket::Deal deal;
    deal.heroGives = {{"fur", 4}};
    deal.heroGets = {{"flint", 7}};
    CHECK(quote(market, deal, sim::Attitude::Neutral).problem.find("only 6 of flint") != std::string::npos);
    deal.heroGets = {{"obsidian", 1}};
    deal.heroGives = {{"fur", 4}};
    const auto wary = market.quote(1, deal, sim::Attitude::Wary, -45, kOpinions, {}, kValues);
    CHECK_FALSE(wary.acceptable);
    CHECK(wary.problem.find("people they like better") != std::string::npos);
    CHECK(market.quote(1, deal, sim::Attitude::Friendly, 25, kOpinions, {}, kValues).problem.find("people they like better") == std::string::npos);
    // Coins cannot be put on the table: they are in the balance.
    deal.heroGets = {{"flint", 1}};
    deal.heroGives = {{"shells", 5}};
    CHECK(quote(market, deal, sim::Attitude::Neutral, shells()).problem.find("balance") != std::string::npos);
    // The balance cannot be overdrawn.
    deal.heroGives = {};
    deal.balancePays = 4;
    CHECK(quote(market, deal, sim::Attitude::Neutral, shells()).problem.find("only 0 in your balance") != std::string::npos);
}

TEST_CASE("US-283 Currency: a hero with shells in his balance buys a fur at the shown price and the shells move") {
    sim::TradeMarket market = marketWith();
    const sim::RegionEconomy economy = shells();
    market.setHeroBalance(20);
    sim::TradeMarket::Deal deal;
    deal.heroGets = {{"fur", 1}};
    deal.balancePays = 4; // the fur is 5 at a neutral trader
    CHECK(market.quote(1, deal, sim::Attitude::Neutral, 0, kOpinions, economy, kValues).problem.find("1 more in value") != std::string::npos);
    deal.balancePays = 5;
    const auto outcome = market.execute(1, deal, sim::Attitude::Neutral, 0, kOpinions, economy, kValues);
    REQUIRE(outcome.done);
    CHECK(outcome.balancePaid == 5);
    CHECK(market.heroBalance() == 15);
    CHECK(market.purse(1) == 35); // the trader took the shells into its purse
    CHECK(market.stock(1, "fur") == 3);
    CHECK(outcome.heroGot == sim::ItemCounts{{"fur", 1}});
}

TEST_CASE("US-283 Surplus: what the hero gives beyond what he takes is paid back from the purse, up to what the purse holds") {
    const sim::RegionEconomy economy = shells();
    sim::TradeMarket market = marketWith();
    sim::TradeMarket::Deal deal;
    deal.heroGives = {{"fur", 4}}; // 4 x 2.5 = 10
    deal.heroGets = {{"flint", 1}}; // 3
    const auto priced = market.quote(1, deal, sim::Attitude::Neutral, 0, kOpinions, economy, kValues);
    CHECK(priced.acceptable);
    CHECK(priced.surplusUnits == 7);
    const auto outcome = market.execute(1, deal, sim::Attitude::Neutral, 0, kOpinions, economy, kValues);
    REQUIRE(outcome.done);
    CHECK(outcome.balanceGained == 7);
    CHECK(market.heroBalance() == 7);
    CHECK(market.purse(1) == 23);

    // A thin purse pays what it has; the rest is the trader's.
    sim::PriceConfig poor;
    poor.startPurse = 3;
    sim::TradeMarket thin = marketWith(poor);
    CHECK(thin.quote(1, deal, sim::Attitude::Neutral, 0, kOpinions, economy, kValues).surplusUnits == 3);

    // Without a currency nothing is paid back: the hero must ask for goods.
    deal.heroGets = {};
    const auto sell = quote(market, deal, sim::Attitude::Neutral);
    CHECK_FALSE(sell.acceptable);
    CHECK(sell.problem.find("cannot pay") != std::string::npos);
}

TEST_CASE("US-283 Purse: it grows by a share each day up to its cap") {
    sim::PriceConfig config;
    config.startPurse = 10;
    config.purseRestock = 5;
    config.purseCap = 22;
    sim::TradeMarket market = marketWith(config);
    CHECK(market.purse(1) == 10);
    market.dailyUpdate(1, {});
    CHECK(market.purse(1) == 15);
    market.dailyUpdate(3, {});
    CHECK(market.purse(1) == 22); // capped
}

TEST_CASE("US-283 Haggle: one try a day with a seeded roll; a win is 10% off for the day, a loss costs opinion") {
    sim::PriceConfig config;
    sim::TradeMarket probe = marketWith(config, 1);
    CHECK(probe.haggleChance(25, 10) == 56);  // 20 + 25 / 4 + 10 x 3
    CHECK(probe.haggleChance(-100, 0) == 5);  // the floor
    CHECK(probe.haggleChance(100, 100) == 85); // the ceiling

    // Find a world seed that wins and one that loses for this trader and day (the roll is a pure function of seed, trader and day).
    std::uint64_t winning = 0;
    std::uint64_t losing = 0;
    bool foundWin = false;
    bool foundLoss = false;
    for (std::uint64_t seed = 0; seed < 400 && !(foundWin && foundLoss); ++seed) {
        sim::TradeMarket market = marketWith(config, seed);
        const auto result = market.haggle(1, 25, 10);
        if (result.won && !foundWin) {
            winning = seed;
            foundWin = true;
        }
        if (!result.won && !foundLoss) {
            losing = seed;
            foundLoss = true;
        }
    }
    REQUIRE(foundWin);
    REQUIRE(foundLoss);

    sim::TradeMarket won = marketWith(config, winning);
    const long long before = won.heroPaysMilli(1, "flint", sim::Attitude::Neutral, {}, kValues);
    const auto win = won.haggle(1, 25, 10);
    REQUIRE(win.tried);
    CHECK(win.won);
    CHECK(win.opinionChange == 0);
    CHECK(win.roll < win.chance);
    CHECK(won.discountPercent(1) == 10);
    CHECK(won.heroPaysMilli(1, "flint", sim::Attitude::Neutral, {}, kValues) == before * 90 / 100);
    CHECK(won.traderPaysMilli(1, "fur", sim::Attitude::Neutral, {}, kValues) == 2750); // the trader pays 10% more for the hero's goods
    // The second try the same day is refused and rolls nothing.
    CHECK_FALSE(won.canHaggle(1));
    const auto again = won.haggle(1, 25, 10);
    CHECK_FALSE(again.tried);
    // Tomorrow the discount is over and a new try is possible.
    won.dailyUpdate(1, {});
    CHECK(won.discountPercent(1) == 0);
    CHECK(won.heroPaysMilli(1, "flint", sim::Attitude::Neutral, {}, kValues) == before);
    CHECK(won.canHaggle(1));

    sim::TradeMarket lost = marketWith(config, losing);
    const auto loss = lost.haggle(1, 25, 10);
    REQUIRE(loss.tried);
    CHECK_FALSE(loss.won);
    CHECK(loss.opinionChange == -5);
    CHECK(lost.discountPercent(1) == 0);
    CHECK_FALSE(lost.canHaggle(1)); // a loss uses up the day's try as well

    // Deterministic: the same world, trader and day give the same roll.
    sim::TradeMarket twin = marketWith(config, winning);
    CHECK(twin.haggle(1, 25, 10).roll == win.roll);
}

TEST_CASE("US-283 Saved: the balance, the purse and the haggle of the day are saved and read back") {
    sim::TradeMarket market = marketWith();
    market.setHeroBalance(12);
    sim::TradeMarket::Deal deal;
    deal.heroGets = {{"fur", 1}};
    deal.balancePays = 5;
    market.setHeroBalance(20);
    REQUIRE(market.execute(1, deal, sim::Attitude::Neutral, 0, kOpinions, shells(), kValues).done);
    market.haggle(1, 0, 0);
    const std::string text = market.toText();
    sim::TradeMarket loaded(sim::PriceConfig{}, 1);
    loaded.restoreState(text);
    REQUIRE(loaded.addTrader(1, profile(), 0));
    CHECK(loaded.heroBalance() == 15);
    CHECK(loaded.purse(1) == 35);
    CHECK_FALSE(loaded.canHaggle(1));
    CHECK(loaded.toText() == text);
    CHECK(loaded.hash() == market.hash());
}
