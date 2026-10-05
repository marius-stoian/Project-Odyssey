// US-185 Story events: the crossroads events as files with a trigger, edited as forms; and the first-day quest in place of the tutorial.
#include "game/story_event_editor.h"
#include "sim/hero_data.h"
#include "sim/hero_life.h"
#include "sim/quest_data.h"
#include "sim/world.h"

#include <doctest/doctest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
namespace sim = odysseus::sim;
using odysseus::game::StoryEventEditor;

namespace {

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

fs::path eventsFolder() { return fs::path(ODYSSEUS_DATA_DIR) / "story" / "events"; }

struct Copy {
    fs::path path;
    Copy() {
        path = fs::temp_directory_path() / ("odysseus-us185-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(path);
        fs::copy(eventsFolder(), path, fs::copy_options::recursive);
    }
    ~Copy() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

} // namespace

TEST_CASE("US-185 The story events load from their files in the order they always had") {
    const sim::HeroData data = sim::loadHeroData(ODYSSEUS_DATA_DIR);
    REQUIRE(data.events.size() == 12);
    CHECK(data.events.front().id == "wolf-at-the-fire");
    CHECK(data.events[1].id == "lost-flint");
    CHECK(data.events.back().id == "first-blood");
    CHECK(data.events[4].needsAffinity.has_value()); // river-crossing needs hunter 12
    for (const auto& event : data.events) CHECK(event.trigger.empty());
}

TEST_CASE("US-185 An event is read, written and read again; a bad trigger names itself") {
    for (const auto& entry : fs::directory_iterator(eventsFolder())) {
        std::string problem;
        const auto event = sim::parseStoryEvent(readFile(entry.path()), entry.path().string(), problem);
        REQUIRE_MESSAGE(event.has_value(), problem);
        const std::string written = sim::writeStoryEvent(*event);
        const auto again = sim::parseStoryEvent(written, entry.path().string(), problem);
        REQUIRE(again.has_value());
        CHECK(sim::writeStoryEvent(*again) == written);
        CHECK(again->options.size() == event->options.size());
    }
    std::string problem;
    std::string text = readFile(eventsFolder() / "lost-flint.json");
    text.insert(text.find("\"options\""), "\"trigger\": \"has(\",\n  ");
    CHECK_FALSE(sim::parseStoryEvent(text, "lost-flint.json", problem).has_value());
    CHECK(problem.find("trigger") != std::string::npos);
}

TEST_CASE("US-185 The form editor opens an event, changes it, saves it and keeps the old file") {
    Copy copy;
    std::vector<std::string> said;
    StoryEventEditor editor(960, 540, [&said](const std::string& m) { said.push_back(m); });
    editor.setFolder(copy.path);
    CHECK(editor.files().size() == 12);
    CHECK(editor.files().front() == "wolf-at-the-fire"); // by "order", not by name
    REQUIRE(editor.open("lost-flint"));
    CHECK_FALSE(editor.dirty());
    editor.edit().title = "The found flint";
    editor.edit().trigger = "flag(met-elder) or time == night";
    sim::AffinityValues values{};
    REQUIRE(StoryEventEditor::affinityFromText("knapper=9 trade=-2", values));
    editor.edit().options[0].affinity = values;
    CHECK(StoryEventEditor::affinityText(values) == "knapper=9 trade=-2");
    CHECK_FALSE(StoryEventEditor::affinityFromText("knapper=lots", values));
    CHECK_FALSE(StoryEventEditor::affinityFromText("swordsman=3", values));
    CHECK(editor.dirty());
    REQUIRE(editor.save());
    CHECK_FALSE(editor.dirty());
    CHECK(fs::exists(copy.path / "lost-flint.json.bak"));
    std::string problem;
    const auto saved = sim::parseStoryEvent(readFile(copy.path / "lost-flint.json"), "lost-flint.json", problem);
    REQUIRE(saved.has_value());
    CHECK(saved->title == "The found flint");
    CHECK(saved->trigger == "flag(met-elder) or time == night");
    CHECK(saved->options[0].affinity[static_cast<std::size_t>(sim::Affinity::Knapper)] == 9);

    // A trigger that is not a condition is refused and nothing is written.
    editor.edit().trigger = "has(";
    CHECK_FALSE(editor.save());
    CHECK(said.back().find("Not saved") != std::string::npos);

    // New events start from a template and are only written on Save.
    REQUIRE(editor.createNew("a-visitor"));
    CHECK_FALSE(fs::exists(copy.path / "a-visitor.json"));
    REQUIRE(editor.save());
    CHECK(fs::exists(copy.path / "a-visitor.json"));
    CHECK_FALSE(editor.createNew("a-visitor"));
    CHECK_FALSE(editor.createNew("Bad Name"));
}

TEST_CASE("US-185 A trigger that does not hold keeps the event from the year; without a checker it counts as true") {
    sim::HeroData data = sim::loadHeroData(ODYSSEUS_DATA_DIR);
    for (auto& event : data.events) event.trigger = "flag(never)";
    const auto run = [&](bool answer, bool withChecker) {
        sim::NewGame game;
        game.seed = 3;
        sim::World world(game.seed, sim::configForComfort(data, sim::loadSimConfig(ODYSSEUS_DATA_DIR), game.comfort));
        sim::HeroLife life(data, world, game);
        if (withChecker) life.setEventTrigger([answer](const std::string&) { return answer; });
        REQUIRE(life.chooseFocus(0, 1));
        return life.liveYear() != nullptr;
    };
    CHECK_FALSE(run(false, true));
    CHECK(run(true, true));
    CHECK(run(false, false));
}

TEST_CASE("US-185 The first-day quest carries the words of the tutorial") {
    sim::rules::LoadReport report;
    const auto quest = sim::rules::parseQuest(readFile(fs::path(ODYSSEUS_DATA_DIR) / "quests" / "first-day.json"), "quests/first-day.json", report, "first-day");
    REQUIRE(quest.has_value());
    REQUIRE(quest->steps.size() == 3);
    CHECK(quest->steps[0].id == "gather");
    CHECK(quest->steps[0].text.find("Gather") != std::string::npos);
    CHECK(quest->steps[1].objective.subject == "eat-berries");
    CHECK(quest->steps[2].objective.subject == "tend-fire");
    for (const auto& step : quest->steps) {
        REQUIRE(step.hint.has_value());
        CHECK(step.hint->afterSeconds == 120); // the hint after two minutes, as before
    }
    CHECK(quest->giver == "none");
    CHECK(quest->turnIn.find("first day") != std::string::npos);
}
