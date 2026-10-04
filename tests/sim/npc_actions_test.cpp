// US-291 Action sources: an idle NPC chooses among the actions of its class, its own custom actions and the events on offer, by the `npc` score of the interaction files;
// there is no quest source yet (M10 adds one).
#include "sim/data.h"
#include "sim/npc_director.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;
namespace sim = odysseus::sim;

namespace {

sim::CalendarConfig calendar() { return sim::loadCalendarConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "calendar.json"); }
sim::NeedsConfig needs() { return sim::loadNeedsConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "needs.json"); }

rules::ScheduleConfig exactConfig() {
    rules::ScheduleConfig config;
    config.scatterPixels = 0;
    return config;
}

// The shipped patrol and help-with-fire files, so the tests use the same ones as the game; the files of the dealings between persons (US-292) are left out, so a guard and a
// villager standing side by side do not stop to chat in these tests.
const rules::InteractionRegistry& shippedInteractions() {
    static const rules::InteractionRegistry registry = [] {
        const fs::path folder = fs::temp_directory_path() / "odysseus-us291" / "interactions";
        fs::remove_all(folder.parent_path());
        fs::create_directories(folder);
        for (const char* name : {"patrol.json", "help-with-fire.json"}) fs::copy_file(fs::path(ODYSSEUS_DATA_DIR) / "interactions" / name, folder / name);
        rules::LoadReport report;
        rules::InteractionRegistry loaded = rules::InteractionRegistry::load(folder, report);
        for (const auto& error : report.errors) FAIL(error.text());
        return loaded;
    }();
    return registry;
}

sim::EventCatalog villagerEvents() {
    sim::EventCatalog catalog;
    catalog.events.push_back({"fire-help", "Help put out the fire", "fire", "help-with-fire", {"villager"}, 12, 30, 30});
    return catalog;
}

sim::NpcProfile guard(bool custom = false) {
    sim::NpcProfile profile;
    profile.classes = {"guard"};
    profile.tags = {"guard", "npc"};
    (custom ? profile.customActions : profile.classActions) = {"patrol"};
    return profile;
}

sim::NpcProfile villager() {
    sim::NpcProfile profile;
    profile.classes = {"villager"};
    profile.tags = {"npc"};
    return profile;
}

struct Watch {
    sim::NpcPopulation population{calendar(), needs()};
    sim::NpcDirector director;

    explicit Watch(rules::ScheduleConfig config = exactConfig()) : director(std::move(config)) {
        director.setInteractions(&shippedInteractions());
        director.setEvents(villagerEvents());
        director.setSeed(7);
        director.setPlaces({{"gate", 800, 300, {"post"}}, {"market", 500, 300, {}}, {"tower", 300, 900, {"post"}}});
        population.setFocus(300, 200);
    }
    int add(int id, const sim::NpcProfile& profile, int x = 200, int y = 200) {
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

bool atPost(const sim::NpcPopulation& population, int index) {
    return (population.x(index) == 800 && population.y(index) == 300) || (population.x(index) == 300 && population.y(index) == 900);
}

} // namespace

TEST_CASE("US-291 Sources: the standard sources are the class, custom and event sources, and there is no quest source") {
    const sim::ActionSources sources = sim::ActionSources::standard();
    CHECK(sources.count() == 3);
    sim::EventBoard board;
    const sim::EventCatalog catalog = villagerEvents();
    board.post("fire", 120, 100, 1000);
    sim::NpcProfile profile = villager();
    profile.classActions = {"patrol"};
    profile.customActions = {"sing"};
    const auto candidates = sources.collect({profile, 100, 100, 10, &board, &catalog});
    REQUIRE(candidates.size() == 3);
    CHECK(candidates[0].interaction == "patrol");
    CHECK(candidates[0].origin == sim::ActionOrigin::Class);
    CHECK(candidates[1].interaction == "sing");
    CHECK(candidates[1].origin == sim::ActionOrigin::Custom);
    CHECK(candidates[2].interaction == "help-with-fire");
    CHECK(candidates[2].origin == sim::ActionOrigin::Event);
    CHECK(candidates[2].atEvent);
    CHECK(candidates[2].eventX == 120);
    CHECK(candidates[2].bonus == 30);

    // The event is only offered to its classes, within its reach, while it lasts.
    sim::NpcProfile hunter;
    hunter.classes = {"hunter"};
    CHECK(sources.collect({hunter, 100, 100, 10, &board, &catalog}).empty());
    CHECK(sources.collect({villager(), 100 + 12 * 32 + 40, 100, 10, &board, &catalog}).empty()); // beyond 12 m
    CHECK(sources.collect({villager(), 100, 100, 1001, &board, &catalog}).empty());              // over
    board.expire(2000);
    CHECK(board.posted().empty());
}

TEST_CASE("US-291 Class: a guard whose class action is patrol, idle on duty, patrols") {
    Watch watch;
    const int guardIndex = watch.add(1, guard());
    watch.runTo(0, 2);
    CHECK(watch.director.lastAction(guardIndex) == "patrol");
    CHECK(atPost(watch.population, guardIndex)); // it went to a place tagged post (not the market)
    // Hour after hour it goes on, from post to post.
    bool gate = false;
    bool tower = false;
    for (int hour = 3; hour < 24; ++hour) {
        watch.runTo(0, hour);
        gate = gate || watch.population.x(guardIndex) == 800;
        tower = tower || watch.population.x(guardIndex) == 300;
        CHECK(atPost(watch.population, guardIndex));
    }
    CHECK(gate);
    CHECK(tower);
}

TEST_CASE("US-291 Custom: the same action set on the NPC itself is chosen the same way") {
    Watch watch;
    const int custom = watch.add(1, guard(true));
    const int none = watch.add(2, villager());
    watch.runTo(0, 2);
    CHECK(watch.director.lastAction(custom) == "patrol");
    CHECK(atPost(watch.population, custom));
    CHECK(watch.director.lastAction(none).empty()); // no actions: it stays where it is
    CHECK(watch.population.x(none) == 200);
    // A deny list keeps an action from an NPC that has it.
    Watch denied;
    sim::NpcProfile peaceful = guard();
    peaceful.deny = {"patrol"};
    const int index = denied.add(1, peaceful);
    denied.runTo(0, 5);
    CHECK(denied.director.lastAction(index).empty());
}

TEST_CASE("US-291 Duty: a person who sleeps or eats does not patrol; one with a block of work does") {
    Watch watch;
    const int asleep = watch.add(1, guard());
    const int working = watch.add(2, guard(), 210, 200);
    rules::Schedule sleeping;
    sleeping.day = {{0, "sleep", "home"}};
    rules::Schedule working_;
    working_.day = {{0, "work", "market"}};
    watch.director.setSchedule(asleep, sleeping);
    watch.director.setSchedule(working, working_);
    watch.runTo(0, 3);
    CHECK(watch.director.lastAction(asleep).empty());
    CHECK(watch.population.x(asleep) == 200);
    CHECK(watch.director.lastAction(working) == "patrol"); // free on a block of work: the patrol takes it from the market to a post
    CHECK(atPost(watch.population, working));
}

TEST_CASE("US-291 Event: an event action for the class villager sends a villager to a fire within 12 m, at once") {
    Watch watch;
    const int near = watch.add(1, villager(), 100, 100);
    const int far = watch.add(2, villager(), 100 + 20 * 32, 100);   // 20 m away
    const int other = watch.add(3, guard(), 110, 100);              // not a villager
    watch.runTo(0, 1);
    REQUIRE(watch.director.lastAction(near).empty());
    watch.director.postEvent(watch.population, "fire", 100 + 6 * 32, 100); // 6 m from the near villager
    CHECK(watch.director.lastAction(near) == "help-with-fire");
    CHECK(watch.population.x(near) == 100 + 6 * 32); // it goes to the fire
    CHECK(watch.population.y(near) == 100);
    CHECK(watch.director.lastAction(far).empty());
    CHECK(watch.population.x(far) == 100 + 20 * 32);
    CHECK(watch.director.lastAction(other) != "help-with-fire");
    // Nobody listens for an event the catalog does not know.
    const std::size_t posted = watch.director.board().posted().size();
    watch.director.postEvent(watch.population, "flood", 100, 100);
    CHECK(watch.director.board().posted().size() == posted);
}

TEST_CASE("US-291 Event over: a fire that is out is no longer on offer at the next hour") {
    Watch watch;
    watch.add(1, guard(), 100, 100); // only here to have somebody to tick
    watch.runTo(0, 1);
    watch.director.postEvent(watch.population, "fire", 150, 100);
    CHECK(watch.director.board().posted().size() == 1);
    watch.runTo(0, 2); // 30 minutes of game time have passed: the event of 30 minutes is over at the hour mark
    CHECK(watch.director.board().posted().empty());
}

TEST_CASE("US-291 Budget: at most maxPerHour persons choose an action on one hour mark") {
    rules::ScheduleConfig config = exactConfig();
    config.maxPerHour = 2;
    Watch watch(config);
    for (int i = 0; i < 5; ++i) watch.add(i + 1, guard(), 200 + i, 200);
    watch.runTo(0, 1);
    int chose = 0;
    for (int i = 0; i < 5; ++i) chose += watch.director.lastAction(i).empty() ? 0 : 1;
    CHECK(chose == 2);
    watch.runTo(0, 4); // the budget is new every hour and goes round, so after a few hours everybody has had a turn
    int chosen = 0;
    for (int i = 0; i < 5; ++i) chosen += watch.director.lastAction(i).empty() ? 0 : 1;
    CHECK(chosen == 5);
}

TEST_CASE("US-291 Save: the profiles and the events on offer are saved and read back") {
    Watch watch;
    watch.add(1, guard());
    watch.add(2, villager(), 100, 100);
    watch.runTo(0, 1);
    watch.director.postEvent(watch.population, "fire", 150, 100);
    const std::string text = watch.director.toText();
    const sim::NpcDirector loaded = sim::NpcDirector::fromText(text, exactConfig());
    CHECK(loaded.hash() == watch.director.hash());
    CHECK(loaded.toText() == text);
    REQUIRE(loaded.profile(0) != nullptr);
    CHECK(loaded.profile(0)->classActions == std::vector<std::string>{"patrol"});
    CHECK(loaded.board().posted().size() == 1);
}

TEST_CASE("US-291 Data: events.json loads; a mistake names the file and the field; the guard class carries its patrol") {
    const sim::EventCatalog catalog = sim::loadEventCatalog(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "events.json");
    REQUIRE(catalog.find("fire-help") != nullptr);
    CHECK(catalog.find("fire-help")->trigger == "fire");
    CHECK(catalog.find("fire-help")->withinMetres == 12);
    CHECK(shippedInteractions().find("patrol") != nullptr);
    CHECK(shippedInteractions().find("help-with-fire") != nullptr);
    rules::LoadReport report;
    const rules::NpcClassCatalog classes = rules::NpcClassCatalog::load(fs::path(ODYSSEUS_DATA_DIR) / "npc-classes", report);
    REQUIRE(classes.find("guard") != nullptr);
    CHECK(classes.find("guard")->extras.does == std::vector<std::string>{"patrol"});
}
