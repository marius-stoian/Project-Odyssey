// US-290 Day and night schedules: blocks of time with an activity and a place, per class, kind or NPC; persons near the hero follow them hour by hour and are interrupted
// by hunger and danger (then resume); far persons follow them in their daily summary; the state is saved compactly.
#include "sim/data.h"
#include "sim/npc_director.h"
#include "sim/npc_kind.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;
namespace sim = odysseus::sim;

namespace {

sim::CalendarConfig calendar() { return sim::loadCalendarConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "calendar.json"); }
sim::NeedsConfig needs() { return sim::loadNeedsConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "needs.json"); }

rules::Schedule workAndSleep() {
    rules::Schedule schedule;
    schedule.day = {{6 * 60, "work", "market"}, {21 * 60, "sleep", "home"}};
    return schedule;
}

rules::ScheduleConfig exactConfig() {
    rules::ScheduleConfig config;
    config.scatterPixels = 0; // persons stand exactly on the place, so a test can compare positions
    config.activities["sleep"] = {0, 12, 0, 0};
    return config;
}

struct Day {
    sim::NpcPopulation population{calendar(), needs()};
    sim::NpcDirector director;

    explicit Day(rules::ScheduleConfig config = exactConfig()) : director(std::move(config)) {
        director.setPlaces({{"market", 500, 300, {}}, {"well", 900, 700, {"water"}}});
    }
    int addNear(int id = 1) {
        const int index = population.add(id, "wanderer", 20 * 28, 0, 100, 100);
        population.setFocus(300, 200); // the hero stands close: this person is simulated by the hour
        director.setHome(index, 100, 100);
        director.setSchedule(index, workAndSleep());
        return index;
    }
    // Runs the world to the given hour mark of a day (the director works on the hour).
    void runTo(int day, int hour) {
        const std::uint64_t target = static_cast<std::uint64_t>(day) * static_cast<std::uint64_t>(population.ticksPerDay()) + static_cast<std::uint64_t>(hour) * static_cast<std::uint64_t>(population.ticksPerHour());
        while (population.ticks() < target) {
            population.tick();
            director.tick(population);
        }
    }
};

} // namespace

TEST_CASE("US-290 Clock: HH:MM reads and writes the same, and bad times are refused") {
    CHECK(rules::parseClock("06:00") == 360);
    CHECK(rules::parseClock("23:59") == 1439);
    CHECK(rules::parseClock("00:00") == 0);
    CHECK_FALSE(rules::parseClock("24:00").has_value());
    CHECK_FALSE(rules::parseClock("6:00").has_value());
    CHECK_FALSE(rules::parseClock("06:60").has_value());
    CHECK_FALSE(rules::parseClock("noon").has_value());
    CHECK(rules::formatClock(360) == "06:00");
    CHECK(rules::formatClock(1439) == "23:59");
}

TEST_CASE("US-290 Blocks: the block in force is the last one that began, and the day wraps round midnight") {
    rules::Schedule schedule;
    schedule.day = {{6 * 60, "work", "market"}, {12 * 60, "eat", "home"}, {14 * 60, "work", "market"}, {22 * 60, "sleep", "home"}};
    CHECK(rules::activeBlock(schedule, 6 * 60, false)->activity == "work");
    CHECK(rules::activeBlock(schedule, 11 * 60 + 59, false)->activity == "work");
    CHECK(rules::activeBlock(schedule, 12 * 60, false)->activity == "eat");
    CHECK(rules::activeBlock(schedule, 13 * 60, false)->activity == "eat");
    CHECK(rules::activeBlock(schedule, 14 * 60, false)->activity == "work");
    CHECK(rules::activeBlock(schedule, 23 * 60, false)->activity == "sleep");
    CHECK(rules::activeBlock(schedule, 3 * 60, false)->activity == "sleep"); // before the first block: the last one, came over midnight
    CHECK(rules::activeBlock(rules::Schedule{}, 600, false) == nullptr);

    // A night variant replaces the day's blocks at night; with none the day's blocks hold.
    schedule.night = {{20 * 60, "patrol", "well"}, {2 * 60, "rest", "home"}};
    std::sort(schedule.night.begin(), schedule.night.end(), [](const auto& a, const auto& b) { return a.minute < b.minute; });
    CHECK(rules::activeBlock(schedule, 22 * 60, true)->activity == "patrol");
    CHECK(rules::activeBlock(schedule, 3 * 60, true)->activity == "rest");
    CHECK(rules::activeBlock(schedule, 22 * 60, false)->activity == "sleep");
    rules::Schedule dayOnly;
    dayOnly.day = {{6 * 60, "work", "market"}};
    CHECK(rules::activeBlock(dayOnly, 23 * 60, true)->activity == "work");

    const rules::ScheduleConfig config;
    CHECK(config.isNight(21));
    CHECK(config.isNight(23));
    CHECK(config.isNight(0));
    CHECK(config.isNight(5));
    CHECK_FALSE(config.isNight(6));
    CHECK_FALSE(config.isNight(20));
}

TEST_CASE("US-290 Text: the Editor's schedule line reads and writes the same, sorted, and mistakes are named") {
    std::string problem;
    const auto blocks = rules::parseScheduleText("21:00 sleep home; 06:00 work market; 12:00 eat", problem);
    REQUIRE(blocks.has_value());
    REQUIRE(blocks->size() == 3);
    CHECK((*blocks)[0].minute == 360);
    CHECK((*blocks)[0].place == "market");
    CHECK((*blocks)[1].place == "home"); // no place: home
    CHECK(rules::scheduleText(*blocks) == "06:00 work market; 12:00 eat home; 21:00 sleep home");
    CHECK(rules::parseScheduleText(rules::scheduleText(*blocks), problem) == blocks);
    CHECK(rules::parseScheduleText("", problem)->empty());

    CHECK_FALSE(rules::parseScheduleText("6am work", problem).has_value());
    CHECK(problem.find("not a time") != std::string::npos);
    CHECK_FALSE(rules::parseScheduleText("06:00", problem).has_value());
    CHECK(problem.find("time activity place") != std::string::npos);
    CHECK_FALSE(rules::parseScheduleText("06:00 Work market", problem).has_value());
    CHECK_FALSE(rules::parseScheduleText("06:00 work market; 06:00 eat home", problem).has_value());
    CHECK(problem.find("two blocks begin at 06:00") != std::string::npos);

    rules::Schedule schedule = workAndSleep();
    CHECK(rules::setScheduleField(schedule, "night", "22:00 patrol well", problem));
    CHECK(schedule.night.size() == 1);
    CHECK(rules::scheduleFieldText(schedule, "day") == "06:00 work market; 21:00 sleep home");
    const rules::Schedule before = schedule;
    CHECK_FALSE(rules::setScheduleField(schedule, "day", "x", problem));
    CHECK_FALSE(rules::setScheduleField(schedule, "week", "06:00 work market", problem));
    CHECK(schedule == before);
}

TEST_CASE("US-290 File: a schedule reads from a class, a kind and a placed NPC, writes back the same, and the highest layer wins") {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us290" / "npc-classes";
    fs::remove_all(folder.parent_path());
    fs::create_directories(folder);
    std::ofstream(folder / "guard.json") << R"({ "id": "guard", "label": "Guard", "colour": "#8a8fa3", "icon": "shield", "tags": [], "dialogues": {}, "actions": { "allow": [], "deny": [] },
  "schedule": { "day": [ { "from": "06:00", "do": "patrol", "at": "market" }, { "from": "20:00", "do": "sleep" } ], "night": [ { "from": "21:00", "do": "patrol", "at": "well" } ] } })";
    rules::LoadReport report;
    const rules::NpcClassCatalog classes = rules::NpcClassCatalog::load(folder, report);
    REQUIRE(report.errors.empty());
    const rules::NpcClass* guard = classes.find("guard");
    REQUIRE(guard != nullptr);
    CHECK(guard->extras.schedule.day.size() == 2);
    CHECK(guard->extras.schedule.day[1].place == "home"); // "at" is optional
    CHECK(guard->extras.schedule.night.size() == 1);
    rules::LoadReport again;
    const auto reread = rules::NpcClassCatalog::parse(rules::toJson(*guard), guard->file, again, "guard");
    REQUIRE(reread.has_value());
    CHECK(*reread == *guard);

    rules::NpcLayer kind;
    kind.classes = std::vector<std::string>{"guard"};
    rules::NpcLayer placed;
    CHECK(rules::resolveNpc(classes, &kind, placed).extras.schedule == guard->extras.schedule); // the class's
    kind.extras.schedule.day = {{8 * 60, "work", "market"}};
    CHECK(rules::resolveNpc(classes, &kind, placed).extras.schedule.day.size() == 1);        // the kind's replaces it whole
    CHECK(rules::resolveNpc(classes, &kind, placed).extras.schedule.night.empty());
    placed.extras.schedule.day = {{9 * 60, "rest", "home"}};
    CHECK(rules::resolveNpc(classes, &kind, placed).extras.schedule.day[0].activity == "rest"); // the placed NPC's wins

    // Mistakes are named, with the line.
    const auto problem = [](const char* schedule) {
        rules::LoadReport mistakes;
        const std::string text = std::string(R"({ "id": "x", "label": "X", "colour": "#808080", "icon": "person", "tags": [], "dialogues": {}, "actions": { "allow": [], "deny": [] }, "schedule": )") + schedule + " }";
        REQUIRE_FALSE(rules::NpcClassCatalog::parse(text, "npc-classes/x.json", mistakes, "x").has_value());
        return mistakes.errors.empty() ? std::string() : mistakes.errors[0].message;
    };
    CHECK(problem(R"([ { "from": "6am", "do": "work" } ])").find("HH:MM") != std::string::npos);
    CHECK(problem(R"([ { "from": "06:00" } ])").find("activity word") != std::string::npos);
    CHECK(problem(R"([ { "from": "06:00", "do": "work" }, { "from": "06:00", "do": "eat" } ])").find("two blocks begin at 06:00") != std::string::npos);
    CHECK(problem(R"({ "noon": [] })").find("unknown field") != std::string::npos);
    CHECK(problem(R"(5)").find("schedule must be") != std::string::npos);
}

TEST_CASE("US-290 Follow: an NPC scheduled to work at the market from 06:00 and sleep at home from 21:00 is at the market at 10:00 and at home at 22:00") {
    Day day;
    const int person = day.addNear();
    day.runTo(0, 10);
    CHECK(day.population.x(person) == 500);
    CHECK(day.population.y(person) == 300);
    CHECK(day.director.activity(day.population, person) == "work");
    CHECK(day.director.mode(person) == sim::NpcDirector::Mode::Scheduled);
    day.runTo(0, 22);
    CHECK(day.population.x(person) == 100);
    CHECK(day.population.y(person) == 100);
    CHECK(day.director.activity(day.population, person) == "sleep");
    // Asleep through the night it gets energy back every hour (the sleep of schedule.json).
    CHECK(day.population.need(person, sim::Need::Energy) > 0);
    // Early next morning it is still at home; at 08:00 it is at the market again.
    day.runTo(1, 3);
    CHECK(day.population.x(person) == 100);
    day.runTo(1, 8);
    CHECK(day.population.x(person) == 500);
}

TEST_CASE("US-290 Interrupt: a very hungry NPC at 10:00 goes home to eat and returns to the market afterwards") {
    Day day;
    const int person = day.addNear();
    day.runTo(0, 9);
    day.population.setNeed(person, sim::Need::Hunger, 5);
    day.runTo(0, 10);
    CHECK(day.director.mode(person) == sim::NpcDirector::Mode::Eating);
    CHECK(day.director.activity(day.population, person) == "eat");
    CHECK(day.population.x(person) == 100); // at home, eating
    CHECK(day.population.need(person, sim::Need::Hunger) > 20);
    day.runTo(0, 11);
    CHECK(day.director.mode(person) == sim::NpcDirector::Mode::Scheduled);
    CHECK(day.population.x(person) == 500); // back at the market
    CHECK(day.population.y(person) == 300);
}

TEST_CASE("US-290 Danger: a hostile near sends the NPC home; when it is gone the schedule resumes") {
    Day day;
    const int person = day.addNear();
    day.runTo(0, 10);
    CHECK(day.population.x(person) == 500);
    day.director.setDanger(person, true);
    day.runTo(0, 11);
    CHECK(day.director.mode(person) == sim::NpcDirector::Mode::Fleeing);
    CHECK(day.population.x(person) == 100);
    day.runTo(0, 12);
    CHECK(day.director.mode(person) == sim::NpcDirector::Mode::Fleeing); // still in danger
    day.director.setDanger(person, false);
    day.runTo(0, 13);
    CHECK(day.director.mode(person) == sim::NpcDirector::Mode::Scheduled);
    CHECK(day.population.x(person) == 500);
}

TEST_CASE("US-290 Night: the night variant is used from the night hour of schedule.json") {
    Day day;
    const int person = day.addNear();
    rules::Schedule schedule;
    schedule.day = {{6 * 60, "work", "market"}};
    schedule.night = {{21 * 60, "patrol", "well"}};
    day.director.setSchedule(person, schedule);
    day.runTo(0, 12);
    CHECK(day.population.x(person) == 500);
    day.runTo(0, 22);
    CHECK(day.population.x(person) == 900);
    CHECK(day.director.activity(day.population, person) == "patrol");
}

TEST_CASE("US-290 Far: persons far from the hero follow the schedule in their daily summary") {
    Day day;
    // 2,000 persons far from the hero. Each is visited once a day at the tick of the day that is their index: the 15th hour is 1,500 and over.
    for (int i = 0; i < 2000; ++i) {
        const int index = day.population.add(i + 1, "wanderer", 20 * 28, 0, 5000 + i, 5000);
        day.director.setHome(index, 5000 + i, 5000);
        day.director.setSchedule(index, workAndSleep());
    }
    day.population.setFocus(-9000, -9000); // the hero is far from everybody, and from the market: nobody is ever near
    day.runTo(0, 24); // one full day
    // Person 1,999 was visited at hour 19 (work at the market); person 5 at hour 0 (the night block: home).
    CHECK(day.population.x(1999) == 500);
    CHECK(day.population.x(5) == 5005);
    // A person at index 1,500 or more works; a person in the first 600 ticks (hours 0 to 5) is at home; one in hours 6 to 20 is at the market.
    CHECK(day.population.x(900) == 500);   // hour 9
    CHECK(day.population.x(550) == 5550);  // hour 5
    CHECK(day.population.x(1250) == 500);  // hour 12
    CHECK(day.population.x(2000 - 1) == 500);
}

TEST_CASE("US-290 Save: the director saves and loads to the same state, compactly for a crowd") {
    Day day;
    const int near = day.addNear(1);
    for (int i = 0; i < 3000; ++i) {
        const int index = day.population.add(100 + i, "wanderer", 20 * 28, 0, 6000 + i, 6000);
        day.director.setHome(index, 6000 + i, 6000);
        day.director.setSchedule(index, workAndSleep());
    }
    day.population.setFocus(300, 200);
    day.director.setDanger(near, true);
    day.runTo(0, 13);
    const std::string text = day.director.toText();
    const sim::NpcDirector loaded = sim::NpcDirector::fromText(text, exactConfig());
    CHECK(loaded.hash() == day.director.hash());
    CHECK(loaded.toText() == text);
    CHECK(loaded.mode(near) == sim::NpcDirector::Mode::Fleeing);
    CHECK(loaded.schedule(near) != nullptr);
    CHECK(loaded.homeX(5) == 6004);
    CHECK(text.size() < 3001 * 20); // run-length coded schedules and modes, two numbers of home per person

    CHECK_THROWS_AS(sim::NpcDirector::fromText("{", exactConfig()), sim::DataError);
    CHECK_THROWS_AS(sim::NpcDirector::fromText(R"({"version": 7})", exactConfig()), sim::DataError);
}

TEST_CASE("US-290 Same: two runs give the same persons and the same director") {
    const auto run = [] {
        Day day;
        day.addNear(1);
        for (int i = 0; i < 300; ++i) {
            const int index = day.population.add(10 + i, "wanderer", 20 * 28, 0, 3000 + 7 * i, 2000 + 3 * i);
            day.director.setSchedule(index, workAndSleep());
        }
        day.runTo(2, 5);
        return std::make_pair(day.population.hash(), day.director.hash());
    };
    CHECK(run() == run());
}

TEST_CASE("US-290 Problems: a schedule that names a place or an activity that is not there is reported") {
    rules::Schedule schedule;
    schedule.day = {{6 * 60, "work", "market"}, {12 * 60, "dance", "square"}, {20 * 60, "sleep", "home"}};
    const std::set<std::string> places = {"market"};
    const std::set<std::string> activities = {"work", "sleep"};
    const auto problems = rules::scheduleProblems(schedule, places, activities, {});
    REQUIRE(problems.size() == 2);
    CHECK(problems[0].find("square") != std::string::npos);
    CHECK(problems[1].find("dance") != std::string::npos);
    CHECK(rules::scheduleProblems(schedule, {"market", "square"}, activities, {"dance"}).empty());
}

TEST_CASE("US-290 Settings: the shipped schedule.json loads") {
    const rules::ScheduleConfig config = rules::loadScheduleConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "schedule.json");
    CHECK(config.nightFromHour == 21);
    CHECK(config.eatBelow == 20);
    CHECK(config.knownActivity("sleep"));
    CHECK(config.knownActivity("work"));
    CHECK(config.activities.at("sleep")[static_cast<std::size_t>(sim::Need::Energy)] == 12);
}
