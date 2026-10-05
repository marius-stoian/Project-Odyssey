// US-281 Trader stock: the trade profile of a class, a kind and a placed NPC (parse, write, merge), limited stock, the daily restock (fixed pieces and weighted
// random picks), wants, and the saved state.
#include "sim/data.h"
#include "sim/npc_kind.h"
#include "sim/trade_market.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;
namespace sim = odysseus::sim;

namespace {

rules::TradeProfile flintTrader() {
    rules::TradeProfile profile;
    profile.stock = {{"flint", 0}, {"berries", 4}};
    profile.restockPerDay = {{"flint", 2}};
    profile.wants = {"berries"};
    return profile;
}

rules::TradeProfile parseTrade(const std::string& text, std::vector<std::string>* problems = nullptr) {
    const auto parsed = rules::parseJson(text);
    REQUIRE(parsed.value.has_value());
    const rules::NpcExtras extras = rules::parseExtras(*parsed.value, [&](int, const std::string& message) {
        if (problems != nullptr) problems->push_back(message);
    });
    return extras.trade;
}

} // namespace

TEST_CASE("US-281 Profile: a trade block reads, writes back as the same text and rejects mistakes by name") {
    const rules::TradeProfile profile = parseTrade(R"({ "trade": { "stock": { "flint": 6, "fur": 2 }, "restockPerDay": { "flint": 2 }, "deliveries": 2,
        "weights": { "fur": 3 }, "wants": ["berries", "berries"], "rare": { "obsidian": "friendly" } } })");
    CHECK(profile.stock.at("flint") == 6);
    CHECK(profile.restockPerDay.at("flint") == 2);
    CHECK(profile.deliveries == 2);
    CHECK(profile.weights.at("fur") == 3);
    CHECK(profile.wants == std::vector<std::string>{"berries"}); // a duplicate is one want
    CHECK(profile.rare.at("obsidian") == "friendly");

    rules::NpcExtras extras;
    extras.trade = profile;
    const auto fields = rules::extrasFieldTexts(extras);
    REQUIRE(fields.size() == 1);
    CHECK(fields[0].first == "trade");
    CHECK(parseTrade("{ \"trade\": " + fields[0].second + " }") == profile);
    CHECK(rules::extrasFieldTexts(rules::NpcExtras{}).empty()); // nothing set: nothing written

    std::vector<std::string> problems;
    parseTrade(R"({ "trade": { "stock": { "Flint": 6 } } })", &problems);
    CHECK(problems.size() == 1);
    CHECK(problems[0].find("item id") != std::string::npos);
    problems.clear();
    parseTrade(R"({ "trade": { "restockPerDay": { "flint": -1 } } })", &problems);
    CHECK(problems.size() == 1);
    CHECK(problems[0].find("trade.restockPerDay.flint") != std::string::npos);
    problems.clear();
    parseTrade(R"({ "trade": { "rare": { "obsidian": "nice" } } })", &problems);
    CHECK(problems.size() == 1);
    problems.clear();
    parseTrade(R"({ "trade": { "price": 3 } })", &problems);
    CHECK(problems.size() == 1);
    CHECK(problems[0].find("unknown field") != std::string::npos);
}

TEST_CASE("US-281 Layers: classes, then the kind, then the placed NPC; maps merge per key and any NPC with a profile is a trader") {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us281" / "npc-classes";
    fs::remove_all(folder.parent_path());
    fs::create_directories(folder);
    std::ofstream(folder / "trader.json") << R"({ "id": "trader", "label": "Trader", "colour": "#d9a441", "icon": "coin", "tags": [],
  "dialogues": {}, "actions": { "allow": [], "deny": [] }, "trade": { "stock": { "flint": 6, "fur": 2 }, "wants": ["berries"] } })";
    std::ofstream(folder / "talker.json") << R"({ "id": "talker", "label": "Talker", "colour": "#808080", "icon": "person", "tags": [], "dialogues": {}, "actions": { "allow": [], "deny": [] } })";
    rules::LoadReport report;
    const rules::NpcClassCatalog classes = rules::NpcClassCatalog::load(folder, report);
    REQUIRE(report.errors.empty());

    rules::NpcLayer kind;
    kind.classes = std::vector<std::string>{"trader"};
    kind.extras.trade.stock = {{"fur", 5}};
    kind.extras.trade.wants = {"fur"};
    rules::NpcLayer placed;
    placed.extras.trade.stock = {{"flint", 1}};
    placed.extras.trade.deliveries = 2;

    const rules::ResolvedNpc npc = rules::resolveNpc(classes, &kind, placed);
    CHECK(npc.extras.trade.stock.at("flint") == 1); // the placed NPC wins
    CHECK(npc.extras.trade.stock.at("fur") == 5);   // the kind wins over the class
    CHECK(npc.extras.trade.wants == std::vector<std::string>{"berries", "fur"}); // the union
    CHECK(npc.extras.trade.deliveries == 2);
    CHECK(std::find(npc.tags.begin(), npc.tags.end(), "trader") != npc.tags.end());

    // A talker with no profile is no trader; give it one in its own fields and it is.
    rules::NpcLayer talkerKind;
    talkerKind.classes = std::vector<std::string>{"talker"};
    const rules::ResolvedNpc talker = rules::resolveNpc(classes, &talkerKind, {});
    CHECK(std::find(talker.tags.begin(), talker.tags.end(), "trader") == talker.tags.end());
    rules::NpcLayer own;
    own.extras.trade.stock = {{"berries", 3}};
    const rules::ResolvedNpc trader = rules::resolveNpc(classes, &talkerKind, own);
    CHECK(std::find(trader.tags.begin(), trader.tags.end(), "trader") != trader.tags.end());

    // The class file with a profile reads back as the same class.
    const rules::NpcClass* shipped = classes.find("trader");
    REQUIRE(shipped != nullptr);
    rules::LoadReport again;
    const auto reread = rules::NpcClassCatalog::parse(rules::toJson(*shipped), shipped->file, again, "trader");
    REQUIRE(reread.has_value());
    CHECK(*reread == *shipped);
}

TEST_CASE("US-281 Restock: a trader with 0 flint and a restock of 2 a day has 2 flint when a day has passed") {
    sim::TradeMarket market;
    REQUIRE(market.addTrader(7, flintTrader(), 0));
    CHECK(market.stock(7, "flint") == 0);
    CHECK(market.stock(7, "berries") == 4);
    market.dailyUpdate(1, sim::RegionEconomy{});
    CHECK(market.stock(7, "flint") == 2);
    market.dailyUpdate(1, sim::RegionEconomy{}); // the same day again: nothing more
    CHECK(market.stock(7, "flint") == 2);
    market.dailyUpdate(2, sim::RegionEconomy{});
    CHECK(market.stock(7, "flint") == 4);
}

TEST_CASE("US-281 Limited: a restock never goes beyond the cap, and a long absence brings at most a week of deliveries") {
    sim::TradeMarket market;
    REQUIRE(market.addTrader(1, flintTrader(), 0));
    for (std::int64_t day = 1; day <= 30; ++day) market.dailyUpdate(day, sim::RegionEconomy{});
    CHECK(market.stock(1, "flint") == market.cap(1, "flint")); // 10 by default: limited stock
    CHECK(market.cap(1, "berries") == 10);
    CHECK(market.stock(1, "berries") == 4);                    // berries are not restocked by this profile

    sim::TradeMarket away;
    REQUIRE(away.addTrader(1, flintTrader(), 0));
    away.dailyUpdate(100, sim::RegionEconomy{}); // 100 days later at once: seven days of deliveries
    CHECK(away.stock(1, "flint") == 10);
    rules::TradeProfile slow;
    slow.restockPerDay = {{"fur", 1}};
    sim::TradeMarket slowMarket;
    REQUIRE(slowMarket.addTrader(1, slow, 0));
    slowMarket.dailyUpdate(100, sim::RegionEconomy{});
    CHECK(slowMarket.stock(1, "fur") == 7);
}

TEST_CASE("US-281 Weighted: random deliveries come from the trader's goods and the region's, the same on every run") {
    rules::TradeProfile profile;
    profile.deliveries = 3;
    profile.weights = {{"flint", 1}};
    sim::RegionEconomy region;
    const auto run = [&](std::uint64_t seed, const sim::RegionEconomy& economy) {
        sim::TradeMarket market(sim::PriceConfig{}, seed);
        REQUIRE(market.addTrader(4, profile, 0));
        for (std::int64_t day = 1; day <= 2; ++day) market.dailyUpdate(day, economy);
        return market;
    };
    const sim::TradeMarket a = run(11, region);
    CHECK(a.stock(4, "flint") == 6); // the only good it has: three picks a day for two days
    // The region delivers berries too: both appear, and a second run with the same seed is identical.
    region.resources = {{"berries", 4}};
    const sim::TradeMarket b = run(11, region);
    const sim::TradeMarket c = run(11, region);
    CHECK(b.hash() == c.hash());
    CHECK(b.toText() == c.toText());
    CHECK(b.stock(4, "berries") > 0);
    CHECK(b.stock(4, "flint") + b.stock(4, "berries") == 6);
    // Another world seed brings another mix (checked over many seeds so one coincidence cannot fail it).
    bool different = false;
    for (std::uint64_t seed = 12; seed < 30; ++seed) different = different || run(seed, region).hash() != b.hash();
    CHECK(different);
}

TEST_CASE("US-281 Wants: berries count at full value, any other good at half") {
    sim::TradeMarket market;
    REQUIRE(market.addTrader(2, flintTrader(), 0));
    CHECK(market.wants(2, "berries"));
    CHECK_FALSE(market.wants(2, "fur"));
    CHECK(market.wantPercent(2, "berries") == 100);
    CHECK(market.wantPercent(2, "fur") == 50);
    CHECK(market.wantPercent(99, "berries") == 50); // not a trader: nothing is wanted
}

TEST_CASE("US-281 Saved: the stock after a trade is the same after save and load, also for traders the level no longer has") {
    sim::TradeMarket market;
    REQUIRE(market.addTrader(3, flintTrader(), 0));
    REQUIRE(market.addTrader(8, flintTrader(), 0));
    market.dailyUpdate(1, sim::RegionEconomy{});
    CHECK(market.removeStock(3, "flint", 1) == 1);
    CHECK(market.addStock(3, "fur", 2) == 2);
    CHECK(market.removeStock(3, "fur", 5) == 2); // only what there is
    const std::string text = market.toText();

    sim::TradeMarket loaded;
    loaded.restoreState(text);
    REQUIRE(loaded.addTrader(3, flintTrader(), 5)); // the data is read again: the saved stock comes back
    CHECK(loaded.stock(3, "flint") == 1);
    CHECK(loaded.stock(3, "berries") == 4);
    CHECK(loaded.hash() != market.hash()); // trader 8 is not registered yet
    REQUIRE(loaded.addTrader(8, flintTrader(), 5));
    CHECK(loaded.toText() == text);
    CHECK(loaded.find(3)->restockDay == 1);

    sim::TradeMarket partial;
    partial.restoreState(text);
    CHECK(partial.toText() == text); // unclaimed states are kept in the next save

    CHECK_THROWS_AS(loaded.restoreState("not json"), sim::DataError);
    CHECK_THROWS_AS(loaded.restoreState(R"({"version": 9, "traders": []})"), sim::DataError);
}

TEST_CASE("US-281 Far: many traders restock in one pass, near or far") {
    sim::TradeMarket market;
    for (int id = 1; id <= 2000; ++id) REQUIRE(market.addTrader(id, flintTrader(), 0));
    market.dailyUpdate(1, sim::RegionEconomy{});
    for (int id = 1; id <= 2000; id += 199) CHECK(market.stock(id, "flint") == 2);
    CHECK(market.traderCount() == 2000);
}

TEST_CASE("US-281 Settings: the shipped trade.json loads and holds the numbers of ADR-023") {
    const sim::PriceConfig config = sim::loadPriceConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "trade.json");
    CHECK(config.curveMinPercent == 50);
    CHECK(config.curveMaxPercent == 200);
    CHECK(config.reputation[static_cast<std::size_t>(sim::Attitude::Wary)] == 25);
    CHECK(config.reputation[static_cast<std::size_t>(sim::Attitude::Friendly)] == -10);
    CHECK(config.refuses[static_cast<std::size_t>(sim::Attitude::Hostile)]);
    CHECK_FALSE(config.refuses[static_cast<std::size_t>(sim::Attitude::Wary)]);
    CHECK(config.wantPercent == 100);
    CHECK(config.otherPercent == 50);
}

TEST_CASE("US-284 Fields: the six trade fields of the Editor read and write the same text, and a mistake changes nothing") {
    rules::TradeProfile profile;
    std::string problem;
    CHECK(rules::setTradeField(profile, "stock", "fur=3 flint=0", problem));
    CHECK(rules::setTradeField(profile, "restock", "flint=1", problem));
    CHECK(rules::setTradeField(profile, "picks", " 2 ", problem));
    CHECK(rules::setTradeField(profile, "weights", "fur=3, berries=0", problem));
    CHECK(rules::setTradeField(profile, "wants", "berries, fur berries", problem));
    CHECK(rules::setTradeField(profile, "rare", "obsidian=friendly spearhead=enchanted", problem));
    CHECK(profile.stock == sim::ItemCounts{{"flint", 0}, {"fur", 3}});
    CHECK(profile.deliveries == 2);
    CHECK(profile.weights.at("berries") == 0); // 0 switches an inherited weight off
    CHECK(profile.wants == std::vector<std::string>{"berries", "fur"});
    for (const std::string& field : rules::tradeFieldNames()) {
        rules::TradeProfile again;
        CHECK_MESSAGE(rules::setTradeField(again, field, rules::tradeFieldText(profile, field), problem), field);
        CHECK(rules::tradeFieldText(again, field) == rules::tradeFieldText(profile, field));
    }
    CHECK(rules::setTradeField(profile, "picks", "", problem)); // empty: not set
    CHECK(profile.deliveries == -1);

    const rules::TradeProfile before = profile;
    CHECK_FALSE(rules::setTradeField(profile, "stock", "fur", problem));
    CHECK_FALSE(rules::setTradeField(profile, "picks", "21", problem));
    CHECK_FALSE(rules::setTradeField(profile, "picks", "two", problem));
    CHECK_FALSE(rules::setTradeField(profile, "wants", "Fur", problem));
    CHECK_FALSE(rules::setTradeField(profile, "rare", "obsidian", problem));
    CHECK_FALSE(rules::setTradeField(profile, "rare", "obsidian=kind", problem));
    CHECK_FALSE(rules::setTradeField(profile, "colour", "red", problem));
    CHECK(problem.find("not a trade field") != std::string::npos);
    CHECK(profile == before);
}
