// US-184 Quest graph editor: a quest file as cards and wires, edited, saved in canonical form with its layout, and opened again.
#include "game/editor_history.h"
#include "game/graph_editor.h"
#include "game/level.h"
#include "game/quest_graph.h"

#include <doctest/doctest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using namespace odysseus::game;
namespace rules = odysseus::sim::rules;

namespace {

const char* const kQuestText = R"json({
  "id": "first-day",
  "title": "The First Day",
  "giver": "role:elder",
  "requires": ["not quest(first-day) == done"],
  "start": "gather",
  "steps": {
    "gather": { "text": "Gather berries.", "objective": "gather berries 3", "marker": "tag:edible",
                "hint": { "after": "120s", "text": "Try the stream." }, "next": "fire" },
    "fire":   { "text": "Keep the fire.", "objective": "interact tend-fire", "next": "END",
                "branches": [ { "if": "flag(fire-out)", "to": "gather" } ] }
  },
  "fail": ["hero.dead"],
  "rewards": ["give hero flint 2"],
  "journal": "Learned.",
  "turnIn": "Well done."
})json";

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

rules::Quest parse(const std::string& text, const std::string& id = "first-day") {
    rules::LoadReport report;
    auto quest = rules::parseQuest(text, "quests/" + id + ".json", report, id);
    REQUIRE(quest.has_value());
    return *quest;
}

struct Folder {
    fs::path path;
    Folder() {
        path = fs::temp_directory_path() / ("odysseus-us184-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(path / "quests");
        std::ofstream(path / "quests" / "first-day.json") << kQuestText;
    }
    ~Folder() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

struct Rig {
    History history;
    std::vector<std::string> said;
    int savedCalls = 0;
    GraphEditor editor;
    explicit Rig(const Folder& folder)
        : editor(960, 540, [this](std::unique_ptr<Command> c) { history.record(std::move(c)); }, [this](const std::string& m) { said.push_back(m); }) {
        editor.setFolders(folder.path, folder.path, [this] { ++savedCalls; }, folder.path / "quests");
        editor.showKind(GraphEditor::Kind::Quest);
    }
    int card(const char* type, const std::string& first = {}) {
        for (const auto& c : editor.graph()->nodes()) {
            if (c.type == type && (first.empty() || (!c.fields.empty() && c.fields[0] == first))) return c.id;
        }
        return 0;
    }
};

} // namespace

TEST_CASE("US-184 A quest becomes cards and wires and writes back as the same quest") {
    const rules::Quest quest = parse(kQuestText);
    const luna::engine::NodeGraph graph = questToGraph(quest, {});
    std::vector<std::string> problems;
    std::string json;
    const auto back = graphToQuest(graph, problems, json);
    REQUIRE(back.has_value());
    CHECK(problems.empty());
    CHECK(json == rules::writeQuest(quest));
    CHECK(back->steps.size() == 2);
    CHECK(back->steps[1].branches.size() == 1);
    CHECK(back->steps[0].hint->afterSeconds == 120);
    CHECK(back->turnIn == "Well done.");
    CHECK(back->rewards[0].source == "give hero flint 2");
}

TEST_CASE("US-184 Positions survive: the layout is keyed by what a card is, not by its number") {
    const luna::engine::NodeGraph graph = questToGraph(parse(kQuestText), {});
    DialogueLayout layout = questLayoutOf(graph);
    CHECK(layout.count("quest") == 1);
    CHECK(layout.count("step:gather") == 1);
    CHECK(layout.count("branch:fire:0") == 1);
    layout["step:fire"] = {777, 333};
    const luna::engine::NodeGraph again = questToGraph(parse(kQuestText), layout);
    CHECK(questLayoutOf(again)["step:fire"] == std::pair<int, int>{777, 333});
}

TEST_CASE("US-184 Mistakes in the graph are named and nothing is written") {
    luna::engine::NodeGraph graph = questToGraph(parse(kQuestText), {});
    std::vector<std::string> problems;
    std::string json;
    // Cut the wire that leads out of the second step: it leads nowhere.
    int fire = 0;
    for (const auto& c : graph.nodes()) {
        if (c.type == quest_card::kStep && c.fields[0] == "fire") fire = c.id;
    }
    REQUIRE(graph.disconnect(fire, 0));
    CHECK_FALSE(graphToQuest(graph, problems, json).has_value());
    REQUIRE_FALSE(problems.empty());
    CHECK(problems.front().find("leads nowhere") != std::string::npos);

    // A loose card is a warning, not an error.
    luna::engine::NodeGraph loose = questToGraph(parse(kQuestText), {});
    loose.add(newQuestCard(quest_card::kFail));
    problems.clear();
    CHECK(graphToQuest(loose, problems, json).has_value());
    REQUIRE(problems.size() == 1);
    CHECK(problems[0].rfind("warning:", 0) == 0);

    // The real loader has the last word: a goal that is not an objective.
    luna::engine::NodeGraph bad = questToGraph(parse(kQuestText), {});
    for (auto& c : bad.nodes()) {
        if (c.type == quest_card::kStep && c.fields[0] == "gather") c.fields[2] = "dance";
    }
    problems.clear();
    CHECK_FALSE(graphToQuest(bad, problems, json).has_value());
    REQUIRE_FALSE(problems.empty());
    CHECK_MESSAGE(problems.back().find("unknown objective") != std::string::npos, problems.back());
    CHECK(problems.size() == 1); // the steps that point at the broken one are not blamed as well
}

TEST_CASE("US-184 The editor opens a quest, edits a step, saves it with its layout and shares one Undo") {
    Folder folder;
    Rig rig(folder);
    CHECK(rig.editor.files() == std::vector<std::string>{"first-day"});
    REQUIRE(rig.editor.open("first-day"));
    CHECK_FALSE(rig.editor.dirty());
    const int gather = rig.card(quest_card::kStep, "gather");
    REQUIRE(gather != 0);
    CHECK(rig.editor.setCardField(gather, 2, "gather berries 5"));
    CHECK(rig.editor.dirty());
    REQUIRE(rig.editor.save());
    CHECK(rig.savedCalls == 1);
    CHECK_FALSE(rig.editor.dirty());

    const std::string text = readFile(folder.path / "quests" / "first-day.json");
    CHECK(text.find("\"objective\": \"gather berries 5\"") != std::string::npos);
    REQUIRE(fs::exists(folder.path / "quests" / "first-day.quest.layout.json"));
    CHECK(fs::exists(folder.path / "quests" / "first-day.json.bak"));
    // The saved file loads in the game's own loader, and the sidecar is not mistaken for a quest.
    rules::LoadReport report;
    const std::vector<rules::Quest> loaded = rules::loadQuests(folder.path / "quests", report);
    CHECK(report.errors.empty());
    REQUIRE(loaded.size() == 1);
    CHECK(loaded[0].steps[0].objective.amount == 5);

    CHECK(rig.history.canUndo());
    Level level;
    CHECK(rig.history.undo(level)); // the graph edit and a map edit are one stack
    const int again = rig.card(quest_card::kStep, "gather");
    CHECK(rig.editor.graph()->find(again)->fields[2] == "gather berries 3");
}

TEST_CASE("US-184 A new quest is made in the editor and only written on Save; the id cannot be renamed") {
    Folder folder;
    Rig rig(folder);
    REQUIRE(rig.editor.createNew("lost-flint"));
    CHECK(rig.editor.dirty());
    CHECK_FALSE(fs::exists(folder.path / "quests" / "lost-flint.json"));
    REQUIRE(rig.editor.save());
    CHECK(fs::exists(folder.path / "quests" / "lost-flint.json"));
    const int header = rig.card(quest_card::kQuest);
    REQUIRE(header != 0);
    CHECK(rig.editor.setCardField(header, 0, "other-name"));
    CHECK_FALSE(rig.editor.save());
    CHECK(rig.said.back().find("must stay") != std::string::npos);
    CHECK_FALSE(rig.editor.createNew("first-day")); // exists already
}
