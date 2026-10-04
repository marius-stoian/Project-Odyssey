// US-293 Default interactions by partner type: the partner types come from data (partner-types.json) and can grow; each type has a defaults file; a class, a kind or an NPC overrides
// them with `partnerActions`; the chooser prefers those actions when that partner is the target (a hunter hunts the deer it sees, people forage, rest, fish and pray at places).
#include "sim/data.h"
#include "sim/npc_director.h"
#include "sim/npc_kind.h"
#include "sim/partner_types.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;
namespace sim = odysseus::sim;

namespace {

sim::CalendarConfig calendar() { return sim::loadCalendarConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "calendar.json"); }
sim::NeedsConfig needs() { return sim::loadNeedsConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "needs.json"); }

const rules::InteractionRegistry& shippedInteractions() {
    static const rules::InteractionRegistry registry = [] {
        rules::LoadReport report;
        rules::InteractionRegistry loaded = rules::InteractionRegistry::load(fs::path(ODYSSEUS_DATA_DIR) / "interactions", report);
        for (const auto& error : report.errors) FAIL(error.text());
        return loaded;
    }();
    return registry;
}

rules::ScheduleConfig exactConfig() {
    rules::ScheduleConfig config;
    config.scatterPixels = 0;
    config.farPercent = 0; // these tests are about what a person does near the hero
    return config;
}

struct Wild {
    sim::NpcPopulation population{calendar(), needs()};
    sim::TradeMarket market;
    sim::NpcDirector director;

    explicit Wild(rules::ScheduleConfig config = exactConfig()) : director(std::move(config)) {
        director.setInteractions(&shippedInteractions());
        director.setMarket(&market);
        director.setSeed(5);
        population.setFocus(300, 200);
    }
    int add(int id, const sim::NpcProfile& profile, int x, int y) {
        const int index = population.add(id, "wanderer", 20 * 28, 0, x, y);
        director.setHome(index, x, y);
        director.setProfile(index, profile);
        return index;
    }
    void runTo(int day, int hour) {
        const std::uint64_t target = static_cast<std::uint64_t>(day) * static_cast<std::uint64_t>(population.ticksPerDay()) + static_cast<std::uint64_t>(hour) * static_cast<std::uint64_t>(population.ticksPerHour());
        while (population.ticks() < target) {
            population.tick();
            director.tick(population);
        }
    }
};

sim::NpcProfile profileWith(std::map<std::string, std::vector<std::string>> partnerActions, std::vector<std::string> classes = {"talker"}) {
    sim::NpcProfile profile;
    profile.classes = std::move(classes);
    profile.tags = {"npc"};
    profile.partnerActions = std::move(partnerActions);
    return profile;
}

} // namespace

TEST_CASE("US-293 Types: the partner types come from partner-types.json, the owner can add one, class:<id> is always a type") {
    const std::vector<std::string> shipped = rules::loadPartnerTypes(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "partner-types.json");
    CHECK(shipped == std::vector<std::string>{"player", "animal", "environment"});
    rules::setPartnerTypes(shipped);
    CHECK(rules::validPartnerType("animal"));
    CHECK(rules::validPartnerType("class:guard"));
    CHECK_FALSE(rules::validPartnerType("buildings"));
    // A new type added to the data file.
    const fs::path folder = fs::temp_directory_path() / "odysseus-us293";
    fs::create_directories(folder);
    std::ofstream(folder / "partner-types.json") << R"({ "version": 1, "types": ["player", "animal", "environment", "buildings"] })";
    const std::vector<std::string> extended = rules::loadPartnerTypes(folder / "partner-types.json");
    CHECK(extended.size() == 4);
    rules::setPartnerTypes(extended);
    CHECK(rules::validPartnerType("buildings"));
    CHECK(rules::partnerTypeNames().size() == 4);
    rules::setPartnerTypes({}); // the built-in three again
    CHECK_FALSE(rules::validPartnerType("buildings"));
    CHECK(rules::partnerTypeNames().size() == 3);
    // Mistakes are named.
    std::ofstream(folder / "bad.json") << R"({ "version": 1, "types": ["Buildings"] })";
    CHECK_THROWS_AS(rules::loadPartnerTypes(folder / "bad.json"), sim::DataError);
    std::ofstream(folder / "bad2.json") << R"({ "version": 1, "types": ["class"] })";
    CHECK_THROWS_AS(rules::loadPartnerTypes(folder / "bad2.json"), sim::DataError);
}

TEST_CASE("US-293 Defaults: one defaults file per partner type loads; a mistake leaves that file out; the registry does not take them for interactions") {
    rules::LoadReport report;
    const rules::PartnerDefaults defaults = rules::loadPartnerDefaults(fs::path(ODYSSEUS_DATA_DIR) / "interactions", report);
    for (const auto& error : report.errors) FAIL(error.text());
    REQUIRE(defaults.find("environment") != nullptr);
    CHECK(*defaults.find("environment") == std::vector<std::string>{"forage", "rest-at-shelter", "fish", "pray"});
    REQUIRE(defaults.find("animal") != nullptr);
    CHECK(defaults.find("animal")->empty());
    REQUIRE(defaults.find("class") != nullptr);
    CHECK(*defaults.find("class") == std::vector<std::string>{"npc-chat"});
    CHECK(shippedInteractions().find("defaults-environment") == nullptr);
    for (const char* id : {"forage", "rest-at-shelter", "fish", "pray", "hunt"}) CHECK_MESSAGE(shippedInteractions().find(id) != nullptr, id);

    const fs::path folder = fs::temp_directory_path() / "odysseus-us293-defaults";
    fs::remove_all(folder);
    fs::create_directories(folder);
    std::ofstream(folder / "defaults-animal.json") << R"({ "partnerType": "animal", "actions": ["hunt"] })";
    std::ofstream(folder / "defaults-wrong.json") << R"({ "partnerType": "animal", "actions": [] })";
    std::ofstream(folder / "defaults-extra.json") << R"({ "partnerType": "extra", "actions": ["Hunt"], "colour": 1 })";
    std::ofstream(folder / "talk.json") << "not a defaults file";
    rules::LoadReport mistakes;
    const rules::PartnerDefaults loaded = rules::loadPartnerDefaults(folder, mistakes);
    CHECK(loaded.actions.size() == 1);
    CHECK(loaded.find("animal") != nullptr);
    CHECK(mistakes.errors.size() >= 3);
    bool mismatch = false;
    for (const auto& error : mistakes.errors) mismatch = mismatch || error.message.find("must match the file name") != std::string::npos;
    CHECK(mismatch);
}

TEST_CASE("US-293 Layers: the defaults are the lowest layer; a class, then a kind, then the NPC replaces the list of a partner type") {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us293-layers" / "npc-classes";
    fs::remove_all(folder.parent_path());
    fs::create_directories(folder);
    std::ofstream(folder / "hunter.json") << R"({ "id": "hunter", "label": "Hunter", "colour": "#7a9a4a", "icon": "bow", "tags": [], "dialogues": {}, "actions": { "allow": [], "deny": [] },
  "partnerActions": { "animal": ["hunt"], "class:guard": ["npc-chat"] } })";
    rules::LoadReport report;
    const rules::NpcClassCatalog classes = rules::NpcClassCatalog::load(folder, report);
    REQUIRE(report.errors.empty());
    const rules::NpcClass* hunter = classes.find("hunter");
    REQUIRE(hunter != nullptr);
    CHECK(hunter->extras.partnerActions.at("animal") == std::vector<std::string>{"hunt"});
    rules::LoadReport again;
    const auto reread = rules::NpcClassCatalog::parse(rules::toJson(*hunter), hunter->file, again, "hunter");
    REQUIRE(reread.has_value());
    CHECK(*reread == *hunter);

    rules::PartnerDefaults defaults;
    defaults.actions = {{"environment", {"forage", "pray"}}, {"animal", {}}};
    rules::NpcLayer kind;
    kind.classes = std::vector<std::string>{"hunter"};
    rules::NpcLayer placed;
    rules::ResolvedNpc npc = rules::resolveNpc(classes, &kind, placed, &defaults);
    CHECK(npc.extras.partnerActions.at("environment") == std::vector<std::string>{"forage", "pray"}); // from the defaults file
    CHECK(npc.extras.partnerActions.at("animal") == std::vector<std::string>{"hunt"});                // the class replaces the default for animals
    kind.extras.partnerActions = {{"animal", {"watch"}}};
    npc = rules::resolveNpc(classes, &kind, placed, &defaults);
    CHECK(npc.extras.partnerActions.at("animal") == std::vector<std::string>{"watch"});               // the kind replaces the class's
    placed.extras.partnerActions = {{"animal", {}}, {"environment", {"fish"}}};
    npc = rules::resolveNpc(classes, &kind, placed, &defaults);
    CHECK(npc.extras.partnerActions.at("animal").empty());                                            // the NPC replaces both: it does nothing special with animals
    CHECK(npc.extras.partnerActions.at("environment") == std::vector<std::string>{"fish"});

    // Mistakes in the field are named.
    std::ofstream(folder / "bad.json") << R"({ "id": "bad", "label": "Bad", "colour": "#808080", "icon": "person", "tags": [], "dialogues": {}, "actions": { "allow": [], "deny": [] },
  "partnerActions": { "robot": ["hunt"] } })";
    rules::LoadReport mistakes;
    rules::NpcClassCatalog::load(folder, mistakes);
    REQUIRE(mistakes.errors.size() == 1);
    CHECK(mistakes.errors[0].message.find("unknown partner type \"robot\"") != std::string::npos);

    // The Editor's line.
    rules::NpcExtras extras;
    std::string problem;
    CHECK(rules::setPartnerActions(extras, "animal", "hunt, watch", problem));
    CHECK(rules::partnerActionsText(extras, "animal") == "hunt watch");
    CHECK(rules::setPartnerActions(extras, "class", "npc-chat", problem));
    CHECK_FALSE(rules::setPartnerActions(extras, "robot", "x", problem));
    CHECK_FALSE(rules::setPartnerActions(extras, "animal", "Hunt", problem));
    CHECK(rules::setPartnerActions(extras, "animal", "", problem)); // empty: the type is removed
    CHECK(extras.partnerActions.count("animal") == 0);
}

TEST_CASE("US-293 Animals: a hunter whose default with animals is hunt hunts a deer it can see; a person without that default ignores it") {
    rules::ScheduleConfig config = exactConfig();
    config.huntPercent = 100;
    Wild wild(config);
    const int hunter = wild.add(1, profileWith({{"animal", {"hunt"}}}, {"hunter"}), 100, 100);
    const int bystander = wild.add(2, profileWith({}), 600, 100);
    wild.director.setAnimals({{50, "deer", 150, 120, {"animal", "prey"}}});
    wild.population.setNeed(hunter, sim::Need::Hunger, 60);
    wild.runTo(0, 1);
    CHECK(wild.director.lastAction(hunter) == "hunt");
    CHECK(wild.population.x(hunter) == 150); // it went to the deer
    CHECK(wild.population.y(hunter) == 120);
    CHECK(wild.director.lastAction(bystander).empty());
    const auto events = wild.director.takeEvents();
    REQUIRE(std::count_if(events.begin(), events.end(), [](const sim::NpcEvent& event) { return event.kind == sim::NpcEvent::Kind::Hunted; }) == 1);
    CHECK(wild.director.animals().empty()); // killed: gone from the director's list (the game removes it from the world)
    CHECK(wild.population.need(hunter, sim::Need::Hunger) > 60); // the hunt feeds

    // The chance decides: with none the deer stays.
    rules::ScheduleConfig unlucky = exactConfig();
    unlucky.huntPercent = 0;
    Wild miss(unlucky);
    const int other = miss.add(1, profileWith({{"animal", {"hunt"}}}, {"hunter"}), 100, 100);
    miss.director.setAnimals({{50, "deer", 150, 120, {"animal", "prey"}}});
    miss.runTo(0, 1);
    CHECK(miss.director.lastAction(other) == "hunt");
    CHECK(miss.director.animals().size() == 1);

    // A deer too far away is not seen (beyond lookRadius).
    Wild blind(config);
    const int shortsighted = blind.add(1, profileWith({{"animal", {"hunt"}}}, {"hunter"}), 100, 100);
    blind.director.setAnimals({{50, "deer", 100 + 400, 100, {"animal", "prey"}}});
    blind.runTo(0, 1);
    CHECK(blind.director.lastAction(shortsighted).empty());
}

TEST_CASE("US-293 Environment: people forage, rest, fish and pray at the places of the level that are tagged for it, by what they need") {
    Wild wild;
    wild.director.setPlaces({{"grove", 700, 100, {"forage"}}, {"hut", 100, 700, {"shelter"}}, {"pond", 700, 700, {"water"}}, {"shrine", 400, 700, {"shrine"}}});
    const sim::NpcProfile everyone = profileWith({{"environment", {"forage", "rest-at-shelter", "fish", "pray"}}});
    const int hungry = wild.add(1, everyone, 100, 100);
    const int tired = wild.add(2, everyone, 250, 100);
    const int lonely = wild.add(3, everyone, 400, 100);
    wild.population.setNeed(hungry, sim::Need::Hunger, 20);
    wild.population.setNeed(tired, sim::Need::Energy, 10);
    wild.population.setNeed(lonely, sim::Need::Social, 10);
    wild.runTo(0, 1);
    CHECK(wild.director.lastAction(hungry) == "forage");
    CHECK(wild.population.x(hungry) == 700);
    CHECK(wild.director.lastAction(tired) == "rest-at-shelter");
    CHECK(wild.population.x(tired) == 100);
    CHECK(wild.population.y(tired) == 700);
    CHECK(wild.director.lastAction(lonely) == "pray");
    CHECK(wild.population.x(lonely) == 400);
    CHECK(wild.population.need(hungry, sim::Need::Hunger) >= 20 + 25 - 2); // foraging gave Hunger back
    CHECK(wild.population.need(tired, sim::Need::Energy) >= 10 + 20 - 2);

    // With only a pond left to eat from, the hungry one fishes.
    Wild pond;
    pond.director.setPlaces({{"pond", 700, 700, {"water"}}});
    const int angler = pond.add(1, everyone, 100, 100);
    pond.population.setNeed(angler, sim::Need::Hunger, 20);
    pond.runTo(0, 1);
    CHECK(pond.director.lastAction(angler) == "fish");
    // A person whose own list for the environment is empty does none of it.
    Wild idle;
    idle.director.setPlaces({{"grove", 700, 100, {"forage"}}});
    const int stayer = idle.add(1, profileWith({{"environment", {}}}), 100, 100);
    idle.population.setNeed(stayer, sim::Need::Hunger, 20);
    idle.runTo(0, 1);
    CHECK(idle.director.lastAction(stayer).empty());
    CHECK(idle.population.x(stayer) == 100);
}

TEST_CASE("US-293 Prefer: an action an NPC prefers for the kind of partner it meets beats a better scoring one") {
    Wild wild;
    const int fond = wild.add(1, profileWith({{"class:talker", {"npc-chat"}}}), 100, 100);
    const int plain = wild.add(2, profileWith({}), 130, 100);
    REQUIRE(wild.market.addTrader(1, [] { rules::TradeProfile profile; profile.stock = {{"flint", 3}}; profile.wants = {"fur"}; return profile; }(), 0));
    REQUIRE(wild.market.addTrader(2, [] { rules::TradeProfile profile; profile.stock = {{"fur", 3}}; profile.wants = {"flint"}; return profile; }(), 0));
    wild.runTo(0, 1);
    // The plain one swaps (60 against a chat of 30); the one who prefers a chat with talkers chats (30 + 40 = 70 against 60).
    CHECK(wild.director.lastAction(plain) == "npc-swap");
    CHECK(wild.director.lastAction(fond) == "npc-chat");
}

TEST_CASE("US-293 Save: the profiles with their partner actions are saved and read back") {
    Wild wild;
    wild.add(1, profileWith({{"animal", {"hunt"}}, {"class", {"npc-chat"}}}, {"hunter"}), 100, 100);
    wild.runTo(0, 1);
    const std::string text = wild.director.toText();
    const sim::NpcDirector loaded = sim::NpcDirector::fromText(text, exactConfig());
    REQUIRE(loaded.profile(0) != nullptr);
    CHECK(loaded.profile(0)->partnerActions.at("animal") == std::vector<std::string>{"hunt"});
    CHECK(loaded.hash() == wild.director.hash());
}
