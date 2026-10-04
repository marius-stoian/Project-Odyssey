// US-294 The living test level: assets/levels/npc-test.json shows one day of NPC life. Every NPC follows its schedule, Tala and Harn swap goods, and people talk. The 100,000-person
// soak is the separate sim test npc_soak_test.cpp (executable odysseus_sim_soak).
#include "camp.h"

#include "sim/npc_director.h"

#include <memory>
#include <set>

using namespace camp_support;
namespace sim = odysseus::sim;

namespace {

luna::engine::Intents pressing(luna::engine::Intent intent) {
    luna::engine::Intents intents;
    intents.set(intent, true, true);
    return intents;
}

fs::path shippedLevel() { return fs::path(ODYSSEUS_DATA_DIR).parent_path() / "levels" / "npc-test.json"; }

struct Day {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;

    explicit Day(const std::string& name) : data(dataCopy(name)) {
        const fs::path file = data / "npc-test.json";
        fs::copy_file(shippedLevel(), file);
        odyssey = std::make_unique<game::OdysseyGame>(data, file);
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
        odyssey->update(pressing(luna::engine::Intent::ModeGame));
    }
    int id(const char* name) const {
        for (const game::PlacedCharacter& placed : odyssey->level().characters) {
            if (placed.name == name) return placed.id;
        }
        FAIL("the test level has no ", name);
        return 0;
    }
    int index(const char* name) const { return odyssey->npcPopulation().indexOf(id(name)); }
};

} // namespace

TEST_CASE("US-294 Day: the schedules hold, the traders swap and the persons talk during one in-game day of the test level") {
    Day day("living-level");
    game::OdysseyGame& game = *day.odyssey;
    REQUIRE(game.level().places.size() == 5);
    std::set<std::string> seen; // every action any person chose for themselves during the day
    const char* people[] = {"Tala", "Ossa", "Harn", "Vell", "Gur"};
    for (int tick = 0; tick < 2400 - 1; ++tick) {
        game.update({});
        if (tick % 10 != 0) continue;
        for (const char* who : people) {
            const std::string last = game.npcDirector().lastAction(day.index(who));
            if (!last.empty()) seen.insert(last);
        }
    }
    CHECK(seen.count("npc-swap") == 1);
    CHECK(seen.count("npc-chat") == 1);
    // Tala and Harn swapped: each has something of the other's, and they have met.
    CHECK(game.npcPopulation().met(day.id("Tala"), day.id("Harn")));
    CHECK(game.tradeMarket().stock(day.id("Tala"), "berries") + game.tradeMarket().stock(day.id("Harn"), "berries") >= 6);
}

TEST_CASE("US-294 Day: at eleven in the morning every NPC is at what its schedule says") {
    Day day("living-level-eleven");
    game::OdysseyGame& game = *day.odyssey;
    for (int tick = 0; tick < 1150; ++tick) game.update({});
    const auto activity = [&](const char* who) { return game.npcDirector().activity(game.npcPopulation(), day.index(who)); };
    CHECK(activity("Tala") == "work");
    CHECK(activity("Harn") == "work");
    CHECK(activity("Ossa") == "work");
    CHECK(activity("Vell") == "go");
    CHECK(activity("Gur") == "patrol");
    // Tala works at the market (320, 390): she is there, give or take the scatter of a crowd.
    const int tala = day.index("Tala");
    CHECK(std::abs(game.npcPopulation().x(tala) - 320) <= 150);
    CHECK(std::abs(game.npcPopulation().y(tala) - 390) <= 150);
}

TEST_CASE("US-294 Day: the level loads with no problem in its schedules, places or actions") {
    Day day("living-level-clean");
    CHECK(day.odyssey->npcClasses().report().errors.empty());
    CHECK(day.odyssey->interactionReport().errors.empty());
    const game::PlacedCharacter* ossa = nullptr;
    for (const game::PlacedCharacter& placed : day.odyssey->level().characters) {
        if (placed.name == "Ossa") ossa = &placed;
    }
    REQUIRE(ossa != nullptr);
    CHECK(ossa->extras.does == std::vector<std::string>{"fish"});
    CHECK_FALSE(ossa->extras.schedule.empty());
}
