// US-153: the action runner (timed actions, interruption, effects that wait) in the simulation layer, headless.
#include "sim/action_runner.h"
#include "core/random.h"
#include "sim/interaction.h"
#include "sim/npc_chooser.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;

namespace {

// Records what the runner asks the game to do.
class Recorder : public rules::EffectHost {
public:
    std::vector<std::string> log;
    int perDay = 1000;

    void setState(const rules::ThingRef& target, const std::string& state) override { log.push_back("set " + std::to_string(target.id) + " " + state); }
    void apply(const rules::Effect& effect, int actor, const rules::ThingRef& target) override {
        log.push_back(effect.source + " by " + std::to_string(actor) + " on " + std::to_string(target.id));
    }
    int ticksPerDay() const override { return perDay; }
};

// A registry made of the given interaction files (id -> JSON text).
rules::InteractionRegistry registryOf(const std::string& name, const std::vector<std::pair<std::string, std::string>>& files) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us153" / name / "interactions";
    fs::remove_all(folder.parent_path());
    fs::create_directories(folder);
    for (const auto& [id, text] : files) std::ofstream(folder / (id + ".json"), std::ios::binary) << text;
    rules::LoadReport report;
    rules::InteractionRegistry registry = rules::InteractionRegistry::load(folder, report);
    for (const auto& d : report.errors) MESSAGE(d.text());
    REQUIRE(report.errors.empty());
    return registry;
}

std::string job(const std::string& id, const std::string& duration, const std::string& effects) {
    return "{ \"id\": \"" + id + "\", \"label\": \"L\", \"actors\": [\"hero\"], \"target\": { \"tags\": [\"t\"] }, \"duration\": " + duration + ", \"effects\": [" + effects + "] }";
}

} // namespace

TEST_CASE("US-153 An instant interaction acts at once") {
    const auto registry = registryOf("instant", {{"zap", job("zap", "0", "\"fx a\", \"set target.state hit\"")}});
    rules::ActionRunner runner;
    Recorder host;
    REQUIRE(runner.start(*registry.find("zap"), 0, {1, 7}, 100, host));
    CHECK(host.log == std::vector<std::string>{"fx a by 0 on 7", "set 7 hit"});
    CHECK(runner.running(0) == nullptr);
    CHECK(runner.progress(0, 100) == -1);
}

TEST_CASE("US-153 A timed interaction acts when its time is over, not before, and shows its progress") {
    const auto registry = registryOf("timed", {{"pick", job("pick", "3", "\"fx a\", \"set target.state picked\"")}});
    rules::ActionRunner runner;
    Recorder host;
    REQUIRE(runner.start(*registry.find("pick"), 0, {1, 7}, 1000, host));
    CHECK(host.log.empty());
    REQUIRE(runner.running(0) != nullptr);
    CHECK(runner.running(0)->endTick == 1060); // 3 s at 20 ticks a second
    CHECK(runner.progress(0, 1000) == 0);
    runner.tick(1030, registry, host);
    CHECK(runner.progress(0, 1030) == 50);
    CHECK(host.log.empty());
    runner.tick(1059, registry, host);
    CHECK(host.log.empty());
    CHECK(runner.progress(0, 1059) == 98);
    runner.tick(1060, registry, host);
    CHECK(host.log == std::vector<std::string>{"fx a by 0 on 7", "set 7 picked"});
    CHECK(runner.running(0) == nullptr);
    runner.tick(1200, registry, host);
    CHECK(host.log.size() == 2); // it happens once
}

TEST_CASE("US-153 A short duration still takes at least one tick") {
    const auto registry = registryOf("short", {{"blink", job("blink", "0.001", "\"fx a\"")}});
    rules::ActionRunner runner;
    Recorder host;
    REQUIRE(runner.start(*registry.find("blink"), 0, {1, 1}, 50, host));
    CHECK(host.log.empty());
    runner.tick(51, registry, host);
    CHECK(host.log.size() == 1);
}

TEST_CASE("US-153 A stopped action gives nothing, and the actor is free again") {
    const auto registry = registryOf("interrupt", {{"pick", job("pick", "3", "\"fx a\"")}});
    rules::ActionRunner runner;
    Recorder host;
    REQUIRE(runner.start(*registry.find("pick"), 0, {1, 7}, 0, host));
    runner.tick(30, registry, host);
    runner.cancel(0);
    runner.tick(60, registry, host);
    runner.tick(600, registry, host);
    CHECK(host.log.empty());
    CHECK(runner.running(0) == nullptr);
    CHECK(runner.start(*registry.find("pick"), 0, {1, 7}, 600, host)); // free to start again
    runner.cancel(99);                                                // stopping someone who is not busy is harmless
}

TEST_CASE("US-153 An actor does one thing at a time; two actors may work side by side") {
    const auto registry = registryOf("busy", {{"pick", job("pick", "3", "\"fx a\"")}});
    rules::ActionRunner runner;
    Recorder host;
    REQUIRE(runner.start(*registry.find("pick"), 0, {1, 7}, 0, host));
    CHECK_FALSE(runner.start(*registry.find("pick"), 0, {1, 8}, 10, host));
    REQUIRE(runner.start(*registry.find("pick"), 3, {1, 9}, 10, host));
    runner.tick(60, registry, host);
    REQUIRE(host.log.size() == 1);
    CHECK(host.log[0] == "fx a by 0 on 7");
    runner.tick(70, registry, host);
    REQUIRE(host.log.size() == 2);
    CHECK(host.log[1] == "fx a by 3 on 9");
}

TEST_CASE("US-153 after waits for its time, in the order the effects were made") {
    const auto registry = registryOf("after", {{"wait", job("wait", "0", "\"after 15s set target.state ripe\", \"after 15s fx a\", \"after 1d fx day\", \"after 1m fx minute\"")}});
    rules::ActionRunner runner;
    Recorder host;
    REQUIRE(runner.start(*registry.find("wait"), 0, {1, 7}, 0, host));
    CHECK(host.log.empty());
    CHECK(runner.pending().size() == 4);
    runner.tick(299, registry, host);
    CHECK(host.log.empty());
    runner.tick(300, registry, host); // 15 s: both effects due now, the first made goes first
    CHECK(host.log == std::vector<std::string>{"set 7 ripe", "fx a by 0 on 7"});
    runner.tick(999, registry, host);
    CHECK(host.log.size() == 2);
    runner.tick(1000, registry, host); // one day of 1000 ticks
    REQUIRE(host.log.size() == 3);
    CHECK(host.log[2] == "fx day by 0 on 7");
    runner.tick(900, registry, host); // the clock never goes back, but an old time changes nothing
    runner.tick(1199, registry, host);
    CHECK(host.log.size() == 3);
    runner.tick(1200, registry, host); // one minute: 1200 ticks
    REQUIRE(host.log.size() == 4);
    CHECK(host.log[3] == "fx minute by 0 on 7");    CHECK(runner.pending().empty());
}

TEST_CASE("US-153 Waiting effects survive a save and a load, whatever the clock says") {
    const auto registry = registryOf("saved", {{"pick", job("pick", "0", "\"set target.state picked\", \"after 15s set target.state ripe\"")}});
    rules::ActionRunner runner;
    Recorder host;
    REQUIRE(runner.start(*registry.find("pick"), 0, {1, 7}, 100, host));
    runner.tick(200, registry, host); // 5 s later: 10 s are left
    const std::string text = runner.savePending(200);

    rules::ActionRunner loaded;
    Recorder host2;
    CHECK(loaded.loadPending(text, 5000).empty()); // a new game, a different clock
    REQUIRE(loaded.pending().size() == 1);
    CHECK(loaded.pending()[0].dueTick == 5200); // 200 ticks (10 s) from the new now
    loaded.tick(5199, registry, host2);
    CHECK(host2.log.empty());
    loaded.tick(5200, registry, host2);
    CHECK(host2.log == std::vector<std::string>{"set 7 ripe"});

    // A damaged save is reported and changes nothing else.
    rules::ActionRunner damaged;
    CHECK_FALSE(damaged.loadPending("{ not json", 0).empty());
    CHECK(damaged.pending().empty());
    CHECK_FALSE(damaged.loadPending("{\"version\": 9, \"pending\": []}", 0).empty());
    CHECK_FALSE(damaged.loadPending("{\"version\": 1, \"pending\": [{\"in\": 5, \"actor\": 0, \"kind\": 1, \"id\": 1, \"effect\": \"giv x\"}]}", 0).empty());
}

TEST_CASE("US-153 An interaction taken out of the data while it runs does nothing when it ends") {
    const auto registry = registryOf("removed", {{"pick", job("pick", "3", "\"fx a\"")}});
    const auto empty = registryOf("removed-empty", {{"other", job("other", "0", "\"fx b\"")}});
    rules::ActionRunner runner;
    Recorder host;
    REQUIRE(runner.start(*registry.find("pick"), 0, {1, 7}, 0, host));
    runner.tick(60, empty, host); // F5 replaced the data meanwhile
    CHECK(host.log.empty());
    CHECK(runner.running(0) == nullptr);
}

TEST_CASE("US-153 The same inputs give the same runner state") {
    const auto registry = registryOf("hash", {{"pick", job("pick", "2", "\"fx a\", \"after 5s fx b\"")}});
    const auto play = [&]() {
        rules::ActionRunner runner;
        Recorder host;
        runner.start(*registry.find("pick"), 0, {1, 7}, 0, host);
        runner.start(*registry.find("pick"), 4, {1, 8}, 3, host);
        runner.tick(40, registry, host);
        runner.tick(41, registry, host);
        return std::make_pair(runner.hash(), host.log);
    };
    const auto first = play();
    const auto second = play();
    CHECK(first.first == second.first);
    CHECK(first.second == second.second);
    rules::ActionRunner other;
    CHECK(other.hash() != first.first);
}

// ---- US-154: how clan members and animals choose

TEST_CASE("US-154 The best score wins, below the minimum nothing is chosen") {
    odysseus::core::Pcg32 random(1, 1);
    CHECK(rules::pickBest({10, 80, 40}, 30, random) == 1);
    CHECK(rules::pickBest({10, 20, 29}, 30, random) == std::nullopt);
    CHECK(rules::pickBest({}, 30, random) == std::nullopt);
    CHECK(rules::pickBest({30}, 30, random) == 0); // exactly the minimum is enough
    CHECK(rules::pickBest({0, 0}, 0, random).has_value());
}

TEST_CASE("US-154 Ties are broken by the seeded stream, the same way every time") {
    const std::vector<int> scores = {50, 90, 90, 90, 10};
    const auto picks = [&](std::uint64_t seed) {
        odysseus::core::Pcg32 random(seed, 3);
        std::vector<std::size_t> chosen;
        for (int i = 0; i < 40; ++i) chosen.push_back(*rules::pickBest(scores, 30, random));
        return chosen;
    };
    CHECK(picks(7) == picks(7)); // same seed, same picks
    const auto chosen = picks(7);
    for (const std::size_t i : chosen) CHECK((i == 1 || i == 2 || i == 3)); // only among the best
    bool varied = false;
    for (const std::size_t i : chosen) varied = varied || i != chosen.front();
    CHECK(varied); // and not always the first
    // The stream moves by one draw per choice whether or not there was a tie or a winner.
    odysseus::core::Pcg32 a(5, 5);
    odysseus::core::Pcg32 b(5, 5);
    rules::pickBest({10, 20}, 30, a); // nothing chosen
    rules::pickBest({90, 90}, 30, b); // a tie
    CHECK(a.state() == b.state());
}

TEST_CASE("US-154 An interaction an actor just did rests for its cooldown") {
    rules::CooldownTable table;
    CHECK(table.ready(7, "gather", 0));
    table.start(7, "gather", 100);
    CHECK_FALSE(table.ready(7, "gather", 99));
    CHECK(table.ready(7, "gather", 100));
    CHECK(table.ready(8, "gather", 50));      // others are not resting
    CHECK(table.ready(7, "graze", 50));       // nor is another interaction
    const std::uint64_t hash = table.hash();
    rules::CooldownTable same;
    same.start(7, "gather", 100);
    CHECK(same.hash() == hash);
    table.forget(7);
    CHECK(table.ready(7, "gather", 0));
    CHECK(table.hash() != hash);
}