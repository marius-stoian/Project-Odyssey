// US-264 Attitudes and opinions: the nine words from an opinion number, events with their amounts from opinions.json, sparse storage for pairs that met.
#include "sim/npc_population.h"
#include "sim/opinion.h"

#include "sim/data.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>

namespace sim = odysseus::sim;
namespace fs = std::filesystem;

namespace {

fs::path dataDir() { return fs::path(ODYSSEUS_DATA_DIR); }
sim::NpcPopulation make(int count = 3) {
    sim::NpcPopulation population(sim::loadCalendarConfig(dataDir() / "sim" / "calendar.json"), sim::loadNeedsConfig(dataDir() / "sim" / "needs.json"));
    population.setOpinionConfig(sim::loadOpinionConfig(dataDir() / "sim" / "opinions.json"));
    for (int i = 1; i <= count; ++i) population.add(i, "wanderer", 20 * 28, 0, 100 * i, 100);
    return population;
}

} // namespace

TEST_CASE("US-264 Words: the nine attitude words, from the opinion number or from fear and envy") {
    const sim::OpinionConfig config = sim::loadOpinionConfig(dataDir() / "sim" / "opinions.json");
    CHECK(sim::attitudeFor(config, -100, sim::Mood::None) == sim::Attitude::Hostile);
    CHECK(sim::attitudeFor(config, -60, sim::Mood::None) == sim::Attitude::Hostile);
    CHECK(sim::attitudeFor(config, -59, sim::Mood::None) == sim::Attitude::Wary);
    CHECK(sim::attitudeFor(config, -29, sim::Mood::None) == sim::Attitude::Suspicious);
    CHECK(sim::attitudeFor(config, -9, sim::Mood::None) == sim::Attitude::Neutral);
    CHECK(sim::attitudeFor(config, 0, sim::Mood::None) == sim::Attitude::Neutral);
    CHECK(sim::attitudeFor(config, 9, sim::Mood::None) == sim::Attitude::Neutral);
    CHECK(sim::attitudeFor(config, 10, sim::Mood::None) == sim::Attitude::Friendly);
    CHECK(sim::attitudeFor(config, 40, sim::Mood::None) == sim::Attitude::Enchanted);
    CHECK(sim::attitudeFor(config, 70, sim::Mood::None) == sim::Attitude::Lovingly);
    CHECK(sim::attitudeFor(config, 100, sim::Mood::None) == sim::Attitude::Lovingly);
    CHECK(sim::attitudeFor(config, 100, sim::Mood::Scared) == sim::Attitude::Scared); // fear wins over liking
    CHECK(sim::attitudeFor(config, -100, sim::Mood::Enviously) == sim::Attitude::Enviously);
    // The names round-trip, and every word is used.
    for (std::size_t i = 0; i < sim::kAttitudeCount; ++i) {
        const auto attitude = static_cast<sim::Attitude>(i);
        CHECK(sim::attitudeFromName(sim::attitudeName(attitude)) == attitude);
    }
    CHECK_FALSE(sim::attitudeFromName("grumpy").has_value());
    // Each starting attitude means an opinion that belongs to its word.
    for (const sim::Attitude word : {sim::Attitude::Friendly, sim::Attitude::Neutral, sim::Attitude::Wary, sim::Attitude::Hostile, sim::Attitude::Suspicious,
                                    sim::Attitude::Enchanted, sim::Attitude::Lovingly}) {
        CHECK(sim::attitudeFor(config, sim::startOpinion(config, word), sim::startMood(word)) == word);
    }
    CHECK(sim::attitudeFor(config, sim::startOpinion(config, sim::Attitude::Scared), sim::startMood(sim::Attitude::Scared)) == sim::Attitude::Scared);
    CHECK(sim::attitudeFor(config, sim::startOpinion(config, sim::Attitude::Enviously), sim::startMood(sim::Attitude::Enviously)) == sim::Attitude::Enviously);
}

TEST_CASE("US-264 Gift: a neutral NPC's opinion of the hero rises by the gift amount and its word changes when a threshold is crossed") {
    sim::NpcPopulation population = make();
    const int gift = population.opinionConfig().events.at("gift");
    REQUIRE(gift == 15);
    CHECK(population.opinion(1, sim::NpcPopulation::kHero) == 0);
    CHECK(population.attitude(1, sim::NpcPopulation::kHero) == sim::Attitude::Neutral);
    CHECK_FALSE(population.met(1, sim::NpcPopulation::kHero));
    CHECK(population.event(1, sim::NpcPopulation::kHero, "gift"));
    CHECK(population.opinion(1, sim::NpcPopulation::kHero) == gift);
    CHECK(population.attitude(1, sim::NpcPopulation::kHero) == sim::Attitude::Friendly); // 15 crosses the threshold of 10
    CHECK(population.met(1, sim::NpcPopulation::kHero));
    // Another person is untouched; further gifts climb and are clamped at 100.
    CHECK(population.opinion(2, sim::NpcPopulation::kHero) == 0);
    for (int i = 0; i < 10; ++i) population.event(1, sim::NpcPopulation::kHero, "gift");
    CHECK(population.opinion(1, sim::NpcPopulation::kHero) == 100);
    CHECK(population.attitude(1, sim::NpcPopulation::kHero) == sim::Attitude::Lovingly);
    CHECK(population.event(1, sim::NpcPopulation::kHero, "insult"));
    CHECK(population.opinion(1, sim::NpcPopulation::kHero) == 80);
    CHECK_FALSE(population.event(1, sim::NpcPopulation::kHero, "no-such-event"));
    CHECK_FALSE(population.event(99, sim::NpcPopulation::kHero, "gift")); // nobody with that id
}

TEST_CASE("US-264 Start: the given attitude is the starting opinion of the hero, fear and envy are moods") {
    sim::NpcPopulation population(sim::loadCalendarConfig(dataDir() / "sim" / "calendar.json"), sim::loadNeedsConfig(dataDir() / "sim" / "needs.json"));
    population.setOpinionConfig(sim::loadOpinionConfig(dataDir() / "sim" / "opinions.json"));
    population.add(1, "goblin", 100, 0, 0, 0, sim::Attitude::Hostile);
    population.add(2, "deer", 100, 0, 0, 0, sim::Attitude::Scared);
    population.add(3, "wanderer", 100, 0, 0, 0, sim::Attitude::Lovingly);
    CHECK(population.attitude(1, sim::NpcPopulation::kHero) == sim::Attitude::Hostile);
    CHECK(population.attitude(2, sim::NpcPopulation::kHero) == sim::Attitude::Scared);
    CHECK(population.attitude(3, sim::NpcPopulation::kHero) == sim::Attitude::Lovingly);
    CHECK(population.startAttitude(2) == sim::Attitude::Scared);
    // A scared person who is helped stays scared until the mood is cleared.
    population.event(2, sim::NpcPopulation::kHero, "help");
    CHECK(population.attitude(2, sim::NpcPopulation::kHero) == sim::Attitude::Scared);
    population.setMood(2, sim::NpcPopulation::kHero, sim::Mood::None);
    CHECK(population.attitude(2, sim::NpcPopulation::kHero) == sim::Attitude::Friendly); // -10 + 20 = 10
    CHECK(population.startAttitude(99) == sim::Attitude::Neutral);
}

TEST_CASE("US-264 Family: two persons of the same family start with the same-family opinion of each other") {
    sim::NpcPopulation population = make(4);
    population.setFamily(0, 7);
    population.setFamily(1, 7);
    population.setFamily(2, 8);
    const int family = population.opinionConfig().sameFamily;
    REQUIRE(family == 20);
    CHECK(population.opinion(1, 2) == family);
    CHECK(population.opinion(2, 1) == family);
    CHECK(population.attitude(1, 2) == sim::Attitude::Friendly);
    CHECK(population.opinion(1, 3) == 0); // another family
    CHECK(population.opinion(3, 4) == 0); // family 8 and no family
    CHECK(population.opinion(4, 1) == 0); // no family at all never counts as the same
    CHECK(population.opinionEntries() == 0); // nothing is stored for the family: it is the default
    population.event(1, 2, "gift"); // now they have "met": the entry starts from the family opinion
    CHECK(population.opinion(1, 2) == family + 15);
    CHECK(population.opinion(2, 1) == family); // the other side did not change
}

TEST_CASE("US-264 Sparse: 100,000 persons where 10 pairs have met hold exactly 10 opinion entries") {
    sim::NpcPopulation population = make(0);
    for (int i = 1; i <= 100000; ++i) population.add(i, "wanderer", 20 * 28, i % 50, i % 1000, i / 1000);
    CHECK(population.opinionEntries() == 0);
    for (int pair = 0; pair < 10; ++pair) population.event(1 + pair * 1000, 2 + pair * 1000, "trade");
    CHECK(population.opinionEntries() == 10);
    population.event(1, 2, "gift"); // the same pair again: still ten
    CHECK(population.opinionEntries() == 10);
    // Reading an opinion never creates an entry.
    for (int i = 1; i < 2000; ++i) (void)population.opinion(i, i + 1);
    CHECK(population.opinionEntries() == 10);
}

TEST_CASE("US-264 Dialogue: quality and frequency change the opinion") {
    sim::NpcPopulation population = make();
    population.talked(1, sim::NpcPopulation::kHero, 2); // great: 6
    CHECK(population.opinion(1, sim::NpcPopulation::kHero) == 6);
    population.talked(1, sim::NpcPopulation::kHero, 1); // good 3 + 1 for talking within 3 days
    CHECK(population.opinion(1, sim::NpcPopulation::kHero) == 10);
    population.talked(1, sim::NpcPopulation::kHero, -2); // awful -8 + 1
    CHECK(population.opinion(1, sim::NpcPopulation::kHero) == 3);
    // Days later the bonus is gone.
    population.runTicks(static_cast<std::uint64_t>(sim::loadCalendarConfig(dataDir() / "sim" / "calendar.json").ticksPerDay) * 10);
    population.talked(1, sim::NpcPopulation::kHero, 1);
    CHECK(population.opinion(1, sim::NpcPopulation::kHero) == 6);
    population.talked(2, 1, 0); // a plain talk between two persons is worth nothing but the two have met
    CHECK(population.met(2, 1));
    CHECK(population.opinion(2, 1) == 0);
}

TEST_CASE("US-264 Saved: opinions are saved and loaded, in the same order, with the same hash") {
    sim::NpcPopulation population = make(5);
    population.setFamily(0, 3);
    population.setFamily(1, 3);
    population.event(1, sim::NpcPopulation::kHero, "gift");
    population.event(3, 2, "help");
    population.setMood(4, sim::NpcPopulation::kHero, sim::Mood::Scared);
    population.talked(5, 1, 2);
    const std::string text = population.toText();
    sim::NpcPopulation loaded = sim::NpcPopulation::fromText(text, sim::loadCalendarConfig(dataDir() / "sim" / "calendar.json"), sim::loadNeedsConfig(dataDir() / "sim" / "needs.json"));
    loaded.setOpinionConfig(population.opinionConfig());
    CHECK(loaded.opinionEntries() == 4);
    CHECK(loaded.opinion(1, sim::NpcPopulation::kHero) == 15);
    CHECK(loaded.opinion(3, 2) == 20);
    CHECK(loaded.attitude(4, sim::NpcPopulation::kHero) == sim::Attitude::Scared);
    CHECK(loaded.opinion(5, 1) == 6);
    CHECK(loaded.hash() == population.hash());
    CHECK(loaded.toText() == text);
}

TEST_CASE("US-264 Data: opinions.json is read and its mistakes are named") {
    const sim::OpinionConfig config = sim::loadOpinionConfig(dataDir() / "sim" / "opinions.json");
    CHECK(config.sameFamily == 20);
    CHECK(config.talk[4] == 6);
    CHECK(config.events.at("trade") == 5);

    const fs::path folder = fs::temp_directory_path() / "odysseus-us264";
    fs::create_directories(folder);
    const auto problem = [&](const std::string& text) {
        std::ofstream(folder / "o.json", std::ios::binary | std::ios::trunc) << text;
        try {
            sim::loadOpinionConfig(folder / "o.json");
        } catch (const odysseus::sim::DataError& error) {
            return std::string(error.what());
        }
        return std::string();
    };
    CHECK(problem(R"({"version":1,"bands":{"wary":-100}})").find("bands.wary") != std::string::npos); // not above the band before it
    CHECK(problem(R"({"version":1,"bands":{"hostile":-90}})").find("bands.hostile") != std::string::npos);
    CHECK(problem(R"({"version":1,"start":{"grumpy":3}})").find("start.grumpy") != std::string::npos);
    CHECK(problem(R"({"version":1,"events":{"gift":"lots"}})").find("events.gift") != std::string::npos);
    CHECK(problem(R"({"version":1,"sameFamily":900})").find("sameFamily") != std::string::npos);
    CHECK(problem(R"({"version":1,"talk":{"good":500}})").find("talk.good") != std::string::npos);
    CHECK(problem(R"({"version":2})").find("version") != std::string::npos);
    CHECK(problem(R"({"version":1})").empty()); // everything else has defaults
}
