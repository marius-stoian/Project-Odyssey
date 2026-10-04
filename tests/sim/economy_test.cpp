// US-280 Currencies per region: the economy of a region (which items are money, market prices, delivery weights) as data and as the text the Editor edits, and the
// money arithmetic of whole numbers (coins to a balance, change back as coins).
#include "sim/data.h"
#include "sim/economy.h"
#include "sim/economy_json.h"

#include <doctest/doctest.h>

#include <nlohmann/json.hpp>

namespace sim = odysseus::sim;

namespace {

sim::RegionEconomy shellsAndGold() {
    sim::RegionEconomy economy;
    economy.currencies = {{"shells", 1}, {"gold", 10}};
    return economy;
}

} // namespace

TEST_CASE("US-280 Text: item=number lists read and write the same, and a mistake is named") {
    const std::optional<sim::ItemCounts> parsed = sim::parsePairs("shells=1 gold=10, bone=3", 1, 100, nullptr);
    REQUIRE(parsed.has_value());
    CHECK(parsed->size() == 3);
    CHECK(parsed->at("gold") == 10);
    CHECK(sim::formatPairs(*parsed) == "bone=3 gold=10 shells=1"); // the written form is sorted: the same every time
    CHECK(sim::parsePairs(sim::formatPairs(*parsed), 1, 100, nullptr) == parsed);
    CHECK(sim::parsePairs("", 1, 100, nullptr)->empty());

    std::string problem;
    CHECK_FALSE(sim::parsePairs("shells", 1, 100, &problem).has_value());
    CHECK(problem.find("item=number") != std::string::npos);
    CHECK_FALSE(sim::parsePairs("Shells=1", 1, 100, &problem).has_value());
    CHECK(problem.find("item id") != std::string::npos);
    CHECK_FALSE(sim::parsePairs("shells=0", 1, 100, &problem).has_value());
    CHECK(problem.find("between 1 and 100") != std::string::npos);
    CHECK_FALSE(sim::parsePairs("shells=1.5", 1, 100, &problem).has_value());
    CHECK(problem.find("whole") != std::string::npos);
}

TEST_CASE("US-280 Money: coin items make a balance and change comes back as coins, highest value first") {
    const sim::RegionEconomy economy = shellsAndGold();
    sim::ItemCounts bag = {{"berries", 4}, {"gold", 2}, {"shells", 3}};
    CHECK(sim::coinValue(economy, bag) == 23);
    CHECK(sim::takeCoins(economy, bag) == 23);
    CHECK(bag.size() == 1); // only the berries are left in the bag
    CHECK(bag.at("berries") == 4);

    int remainder = -1;
    const sim::ItemCounts change = sim::makeChange(economy, 27, remainder);
    CHECK(remainder == 0);
    CHECK(change.at("gold") == 2);
    CHECK(change.at("shells") == 7);

    // A currency whose smallest coin is 5 cannot make 3: the remainder says so, and nothing is lost.
    sim::RegionEconomy coarse;
    coarse.currencies = {{"gold", 5}};
    const sim::ItemCounts coins = sim::makeChange(coarse, 13, remainder);
    CHECK(coins.at("gold") == 2);
    CHECK(remainder == 3);
    // No currency at all: the whole amount stays a remainder (barter only).
    sim::makeChange(sim::RegionEconomy{}, 9, remainder);
    CHECK(remainder == 9);
}

TEST_CASE("US-280 None: a region with no currency offers barter only") {
    const sim::RegionEconomy none;
    CHECK_FALSE(none.hasCurrency());
    CHECK(none.empty());
    CHECK_FALSE(none.isCurrency("shells"));
    CHECK(none.currencyValue("shells") == 0);
    const sim::RegionEconomy set = shellsAndGold();
    CHECK(set.hasCurrency());
    CHECK(set.isCurrency("shells"));
    CHECK(set.currencyValue("gold") == 10);
}

TEST_CASE("US-280 File: the economy reads from the level text and writes back the same") {
    const std::filesystem::path file = "level.json";
    const nlohmann::json text = nlohmann::json::parse(R"({"currencies": {"shells": 1}, "prices": {"flint": 4}, "resources": {"berries": 5}})");
    const sim::RegionEconomy economy = sim::economyFromJson(text, file, "economy");
    CHECK(economy.currencies.at("shells") == 1);
    CHECK(economy.prices.at("flint") == 4);
    CHECK(economy.resources.at("berries") == 5);
    CHECK(sim::economyFromJson(sim::economyToJson(economy), file, "economy") == economy);
    CHECK(sim::economyToJson(sim::RegionEconomy{}).empty()); // nothing set: nothing written

    // Mistakes name the file and the field.
    const auto message = [&](const char* json) {
        try {
            sim::economyFromJson(nlohmann::json::parse(json), file, "economy");
        } catch (const sim::DataError& error) {
            return std::string(error.what());
        }
        return std::string();
    };
    CHECK(message(R"({"currencies": {"shells": 0}})").find("economy.currencies.shells") != std::string::npos);
    CHECK(message(R"({"currencies": {"Shells": 1}})").find("item id") != std::string::npos);
    CHECK(message(R"({"coins": {}})").find("economy.coins") != std::string::npos);
    CHECK(message(R"({"prices": [1]})").find("economy.prices") != std::string::npos);
    CHECK(message(R"({"currencies": {"shells": 1.5}})").find("whole number") != std::string::npos);
}
