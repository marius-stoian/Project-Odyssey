// US-196 Daily routines as data: a schedule block may carry weights for the tags of what a person could do, and the choice of an idle person is multiplied by them; a
// profession carries a routine in the same format; the clan's people take the routine of their profession.
#include "core/text.h"
#include "sim/data.h"
#include "sim/npc_class.h"
#include "sim/npc_director.h"
#include "sim/hero_data.h"
#include "sim/npc_schedule.h"
#include "sim/world.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;
namespace sim = odysseus::sim;

namespace {

sim::CalendarConfig calendar() { return sim::loadCalendarConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "calendar.json"); }
sim::NeedsConfig needs() { return sim::loadNeedsConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "needs.json"); }

// Two things an idle person may do, equally attractive on their own: a patrol to a post and a stroll to the garden.
const rules::InteractionRegistry& registry() {
    static const rules::InteractionRegistry loaded = [] {
        const fs::path folder = fs::temp_directory_path() / ("odysseus-us196-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())) / "interactions";
        fs::create_directories(folder);
        REQUIRE_FALSE(odysseus::core::writeTextFileSafely(folder / "patrol.json", R"({ "id": "patrol", "label": "Patrol", "actors": ["npc"], "target": { "tags": ["place", "post"] },
            "range": 2, "effects": [ "do walk-to" ], "npc": { "score": "60", "cooldown": 0 } })").has_value());
        REQUIRE_FALSE(odysseus::core::writeTextFileSafely(folder / "stroll.json", R"({ "id": "stroll", "label": "Stroll", "actors": ["npc"], "target": { "tags": ["place", "garden"] },
            "range": 2, "effects": [ "do walk-to" ], "npc": { "score": "60", "cooldown": 0 } })").has_value());
        rules::LoadReport report;
        rules::InteractionRegistry result = rules::InteractionRegistry::load(folder, report);
        for (const auto& error : report.errors) FAIL(error.text());
        return result;
    }();
    return loaded;
}

rules::ScheduleConfig exactConfig() {
    rules::ScheduleConfig config;
    config.scatterPixels = 0;
    return config;
}

sim::NpcProfile hunter() {
    sim::NpcProfile profile;
    profile.classes = {"hunter"};
    profile.tags = {"hunter", "npc"};
    profile.classActions = {"patrol", "stroll"};
    return profile;
}

struct Day {
    sim::NpcPopulation population{calendar(), needs()};
    sim::NpcDirector director;

    explicit Day(rules::Schedule schedule) : director(exactConfig()) {
        director.setInteractions(&registry());
        director.setSeed(11);
        director.setPlaces({{"gate", 800, 300, {"post"}}, {"garden", 500, 300, {"garden"}}});
        population.setFocus(300, 200);
        person = population.add(1, "wanderer", 20 * 28, 0, 200, 200);
        director.setHome(person, 200, 200);
        director.setProfile(person, hunter());
        director.setSchedule(person, std::move(schedule));
    }
    int person = 0;
    void runTo(int day, int hour) {
        const std::uint64_t target = static_cast<std::uint64_t>(day) * static_cast<std::uint64_t>(population.ticksPerDay()) + static_cast<std::uint64_t>(hour) * static_cast<std::uint64_t>(population.ticksPerHour());
        while (population.ticks() < target) {
            population.tick();
            director.tick(population);
        }
    }
};

// The hunters' routine of the story: work from 06:00 to 14:00 with a weight for what has to do with a post, idle after, asleep from 21:00.
rules::Schedule huntersRoutine(int weight) {
    rules::Schedule schedule;
    schedule.day = {{6 * 60, "work", "home", weight == 100 ? std::map<std::string, int>{} : std::map<std::string, int>{{"post", weight}}}, {14 * 60, "idle", "home", {}}, {21 * 60, "sleep", "home", {}}};
    return schedule;
}

// How many times the person chose a patrol on the hour marks from `from` up to (not with) `to`, over some days.
int patrols(Day& day, int from, int to, int days) {
    int count = 0;
    for (int d = 0; d < days; ++d) {
        for (int hour = from; hour < to; ++hour) {
            day.runTo(d, hour);
            count += day.director.lastAction(day.person) == "patrol" ? 1 : 0;
        }
    }
    return count;
}

} // namespace

TEST_CASE("US-196 Weights: a block multiplies the score of a choice by the weight of each tag it carries") {
    rules::ScheduleBlock block{0, "work", "home", {{"post", 300}, {"garden", 50}}};
    CHECK(rules::weighted(&block, {"place", "post"}, 60) == 180);
    CHECK(rules::weighted(&block, {"place", "garden"}, 60) == 30);
    CHECK(rules::weighted(&block, {"place", "post", "garden"}, 60) == 90); // each matching tag once, the product
    CHECK(rules::weighted(&block, {"place"}, 60) == 60);                    // no weight for these tags: the score as it was
    CHECK(rules::weighted(nullptr, {"post"}, 60) == 60);
    rules::ScheduleBlock none{0, "work", "home", {}};
    CHECK(rules::weighted(&none, {"post"}, 60) == 60);
    rules::ScheduleBlock zero{0, "work", "home", {{"post", 0}}};
    CHECK(rules::weighted(&zero, {"post"}, 60) == 0); // weight 0: never chosen
}

TEST_CASE("US-196 Routine: hunters choose what their work block prefers more often in that block than at other times") {
    // 06:00 to 14:00 prefers what has to do with a post (a patrol); the other hours have no weights, so a patrol and a stroll are equally likely.
    Day with(huntersRoutine(300));
    const int inBlock = patrols(with, 7, 14, 12);  // 7 hours a day
    const int outside = patrols(with, 15, 21, 12); // 6 hours a day
    CHECK(inBlock == 7 * 12);                      // 180 against 60: every choice in the block is the patrol
    CHECK(outside < 6 * 12);
    CHECK(outside > 0);                            // outside the block it is the roll of the dice
    // Without the weights the block makes no difference to the choices.
    Day without(huntersRoutine(100));
    CHECK(patrols(without, 7, 14, 12) < 7 * 12);
}

TEST_CASE("US-196 Needs win: a starving hunter in a work block eats first") {
    Day day(huntersRoutine(300));
    day.runTo(0, 9);
    day.population.setNeed(day.person, sim::Need::Hunger, 5);
    day.runTo(0, 10);
    CHECK(day.director.mode(day.person) == sim::NpcDirector::Mode::Eating);
    CHECK(day.director.activity(day.population, day.person) == "eat");
    CHECK(day.population.x(day.person) == 200); // at home, eating: the block's weights did not take them to the post
    CHECK(day.population.need(day.person, sim::Need::Hunger) > 20);
}

TEST_CASE("US-196 One format: weights read and write the same in the Editor's text, in a class file and in the director's save") {
    std::string problem;
    const auto blocks = rules::parseScheduleText("06:00 work gate prefer animal=300,edible=150; 14:00 idle; 21:00 sleep home", problem);
    REQUIRE(blocks.has_value());
    REQUIRE(blocks->size() == 3);
    CHECK((*blocks)[0].prefer == std::map<std::string, int>{{"animal", 300}, {"edible", 150}});
    CHECK((*blocks)[1].prefer.empty());
    CHECK(rules::scheduleText(*blocks) == "06:00 work gate prefer animal=300,edible=150; 14:00 idle home; 21:00 sleep home");
    CHECK(rules::parseScheduleText(rules::scheduleText(*blocks), problem) == blocks);
    // A weight is a number from 0 to 500 for a tag that is a word.
    CHECK_FALSE(rules::parseScheduleText("06:00 work prefer animal=900", problem).has_value());
    CHECK(problem.find("0 to 500") != std::string::npos);
    CHECK_FALSE(rules::parseScheduleText("06:00 work prefer animal", problem).has_value());
    CHECK_FALSE(rules::parseScheduleText("06:00 work prefer Animal=3", problem).has_value());
    // The same blocks in a class file (the US-290 format with one more key) and back.
    const std::string file = R"({ "id": "hunter", "label": "Hunter", "colour": "#8a8fa3", "icon": "shield", "tags": ["hunter"],
        "schedule": { "day": [ { "from": "06:00", "do": "work", "at": "gate", "prefer": { "animal": 300 } }, { "from": "21:00", "do": "sleep", "at": "home" } ] } })";
    rules::LoadReport report;
    const std::optional<rules::NpcClass> parsed = rules::NpcClassCatalog::parse(file, "hunter.json", report, "hunter");
    for (const auto& error : report.errors) FAIL(error.text());
    REQUIRE(parsed.has_value());
    REQUIRE(parsed->extras.schedule.day.size() == 2);
    CHECK(parsed->extras.schedule.day[0].prefer == std::map<std::string, int>{{"animal", 300}});
    const std::string written = rules::toJson(*parsed);
    CHECK(written.find("\"prefer\": { \"animal\": 300 }") != std::string::npos);
    rules::LoadReport again;
    const std::optional<rules::NpcClass> reread = rules::NpcClassCatalog::parse(written, "hunter.json", again, "hunter");
    REQUIRE(reread.has_value());
    CHECK(*reread == *parsed);
    // A mistake in a weight names the field.
    const std::string bad = R"({ "id": "hunter", "label": "Hunter", "colour": "#8a8fa3", "icon": "shield", "tags": ["hunter"], "schedule": [ { "from": "06:00", "do": "work", "prefer": { "animal": 900 } } ] })";
    rules::LoadReport broken;
    rules::NpcClassCatalog::parse(bad, "hunter.json", broken, "hunter");
    REQUIRE_FALSE(broken.errors.empty());
    CHECK(broken.errors[0].text().find("0 to 500") != std::string::npos);
    // The director's save keeps the weights, and a world with other weights is another world.
    Day day(huntersRoutine(300));
    const std::string text = day.director.toText();
    const sim::NpcDirector loaded = sim::NpcDirector::fromText(text, exactConfig());
    CHECK(loaded.schedule(day.person)->day[0].prefer == std::map<std::string, int>{{"post", 300}});
    CHECK(loaded.hash() == day.director.hash());
    Day other(huntersRoutine(200));
    CHECK(other.director.hash() != day.director.hash());
}

// ---- the routines of the professions and the clan

namespace {

// A schedule that holds all day with these weights.
rules::Schedule always(std::map<std::string, int> prefer) {
    rules::Schedule schedule;
    schedule.day = {{0, "work", "home", std::move(prefer)}};
    return schedule;
}

// How many of the living adults (a child has no profession, so no routine) are asleep at the hour marks of the night of the first day.
int sleepers(sim::World& world, int fromHour, int toHour) {
    int count = 0;
    const std::uint64_t perHour = static_cast<std::uint64_t>(world.calendar().ticksPerDay() / 24);
    for (int hour = fromHour; hour < toHour; ++hour) {
        const std::uint64_t target = static_cast<std::uint64_t>(hour) * perHour + 1;
        if (world.ticks() < target) world.runTicks(target - world.ticks());
        for (const sim::Person& person : world.people()) count += person.alive && person.ageYears(world.calendar().daysPerYear()) >= 12 && person.action == sim::Action::Sleep ? 1 : 0;
    }
    return count;
}

} // namespace

TEST_CASE("US-196 Professions carry a routine in the one schedule format") {
    const sim::HeroData data = sim::loadHeroData(ODYSSEUS_DATA_DIR);
    const sim::Profession* hunter = data.profession("hunter");
    REQUIRE(hunter != nullptr);
    REQUIRE(hunter->schedule.day.size() == 3);
    CHECK(hunter->schedule.day[0].minute == 6 * 60);
    CHECK(hunter->schedule.day[0].activity == "work");
    CHECK(hunter->schedule.day[0].prefer == std::map<std::string, int>{{"hunt", 200}});
    CHECK(data.profession("flint-knapper")->schedule.empty()); // a profession without a routine has none
    // The same reader reads a class file's schedule and a profession's: the same text gives the same schedule.
    const std::string text = R"({ "day": [ { "from": "06:00", "do": "work", "at": "gate", "prefer": { "animal": 300 } } ], "night": [ { "from": "21:00", "do": "sleep" } ] })";
    std::vector<std::string> problems;
    const rules::Schedule fromText = rules::scheduleFromJson(text, problems);
    CHECK(problems.empty());
    rules::LoadReport report;
    const std::optional<rules::NpcClass> parsed = rules::NpcClassCatalog::parse(R"({ "id": "scout", "label": "Scout", "colour": "#8a8fa3", "icon": "shield", "tags": ["scout"], "schedule": )" + text + " }", "scout.json", report, "scout");
    for (const auto& error : report.errors) FAIL(error.text());
    REQUIRE(parsed.has_value());
    CHECK(parsed->extras.schedule == fromText);
    // A mistake in a profession's routine names the file and the field.
    const fs::path folder = fs::temp_directory_path() / ("odysseus-us196-prof-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::copy(ODYSSEUS_DATA_DIR, folder, fs::copy_options::recursive);
    std::string professions = *odysseus::core::readTextFile(folder / "hero" / "professions.json");
    professions.replace(professions.find("\"hunt\": 200"), 11, "\"hunt\": 900");
    REQUIRE_FALSE(odysseus::core::writeTextFileSafely(folder / "hero" / "professions.json", professions).has_value());
    CHECK_THROWS_WITH(sim::loadHeroData(folder), doctest::Contains("professions[0].day"));
    std::error_code error;
    fs::remove_all(folder, error);
}

TEST_CASE("US-196 The clan's people take the routine of their profession") {
    sim::SimConfig plain = sim::loadSimConfig(ODYSSEUS_DATA_DIR);
    sim::SimConfig guided = plain;
    guided.routines["hunter"] = always({{"sleep", 0}});   // nobody of the clan may sleep: a weight of 0 takes the action off the list
    guided.routines["gatherer"] = always({{"sleep", 0}});
    sim::World free(5, plain);
    sim::World held(5, guided);
    const int freeSleepers = sleepers(free, 23, 24);
    const int heldSleepers = sleepers(held, 23, 24);
    CHECK(freeSleepers > 5);   // at 23:00 the clan sleeps
    CHECK(heldSleepers == 0);  // with the routine nobody does, while their energy holds
    // Without routines the weights change nothing: the same world as before the story.
    sim::World again(5, plain);
    CHECK(sleepers(again, 23, 24) == freeSleepers);
    CHECK(again.hash() == free.hash());
}

TEST_CASE("US-196 Needs win: a clan member whose energy is in danger sleeps although the routine forbids it") {
    sim::SimConfig guided = sim::loadSimConfig(ODYSSEUS_DATA_DIR);
    guided.routines["hunter"] = always({{"sleep", 0}});
    guided.routines["gatherer"] = always({{"sleep", 0}});
    sim::World rested(5, guided);
    CHECK(sleepers(rested, 2, 3) == 0); // well rested at two in the night: the routine holds
    sim::World world(5, guided);
    for (const sim::Person& person : world.people()) world.drainPersonNeed(person.id, sim::Need::Energy, 95); // exhausted: below the danger level
    CHECK(sleepers(world, 2, 3) > 5);
}
