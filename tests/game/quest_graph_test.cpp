// US-184 Quest graph editor: a quest file as cards and wires, edited, saved in canonical form with its layout, and opened again.
#include "game/editor_history.h"
#include "game/graph_editor.h"
#include "game/level.h"
#include "game/quest_graph.h"
#include "sim/graph_check.h"
#include "sim/quest_book.h"

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

// X-M10: the owner's loop, end to end. A quest is built in the graph editor, saved, test-played with the debugger's calls, loaded the way the game loads
// it, handed out by a giver and played to its reward.
namespace {

class Recorder : public rules::EffectHost {
public:
    std::vector<std::string> log;
    void setState(const rules::ThingRef&, const std::string& state) override { log.push_back("set " + state); }
    void apply(const rules::Effect& effect, int actor, const rules::ThingRef&) override { log.push_back(effect.source + " by " + std::to_string(actor)); }
    int ticksPerDay() const override { return 1000; }
};

class Quiet : public rules::RuleContext {
public:
    rules::Value path(const std::string&) const override { return rules::Value::ofNumber(0); }
    rules::Value call(const std::string&, const std::vector<rules::Value>&) const override { return rules::Value::ofNumber(0); }
};

} // namespace

TEST_CASE("X-M10 From scratch: build a quest in the graph, test-play it, save it, play it from a giver to the reward") {
    Folder folder;
    Rig rig(folder);
    REQUIRE(rig.editor.createNew("fetch-water"));
    const int header = rig.card(quest_card::kQuest);
    REQUIRE(header != 0);
    CHECK(rig.editor.setCardField(header, 1, "Fetch water"));
    CHECK(rig.editor.setCardField(header, 2, "role:elder"));
    CHECK(rig.editor.setCardField(header, 5, "The clan is thirsty. Will you fetch water?"));
    CHECK(rig.editor.setCardField(header, 6, "Thank you, the clan drinks tonight."));
    // The template has one step that leads to the end; add a second step in front of it and a reward.
    const int first = rig.card(quest_card::kStep, "first");
    REQUIRE(first != 0);
    CHECK(rig.editor.setCardField(first, 1, "Drink at the stream to see it is clean."));
    CHECK(rig.editor.setCardField(first, 2, "interact drink"));
    const int back = rig.editor.addCard(quest_card::kStep);
    CHECK(rig.editor.setCardField(back, 0, "return"));
    CHECK(rig.editor.setCardField(back, 1, "Tell the elder."));
    CHECK(rig.editor.setCardField(back, 2, "talk elder"));
    const int end = rig.card(quest_card::kEnd);
    REQUIRE(end != 0);
    REQUIRE(rig.editor.graph()->connect(first, 0, back, 0));
    REQUIRE(rig.editor.graph()->connect(back, 0, end, 0));
    const int rewards = rig.editor.addCard(quest_card::kRewards);
    CHECK(rig.editor.setCardField(rewards, 0, "give hero flint 2"));
    REQUIRE(rig.editor.graph()->connect(rewards, 0, header, 2));
    REQUIRE(rig.editor.save());

    // Saved, and the game's loader reads it with no complaint; the check finds nothing wrong.
    rules::LoadReport report;
    const std::vector<rules::Quest> quests = rules::loadQuests(folder.path / "quests", report);
    for (const auto& e : report.errors) MESSAGE(e.text());
    REQUIRE(report.errors.empty());
    const rules::Quest* made = nullptr;
    for (const auto& q : quests) {
        if (q.id == "fetch-water") made = &q;
    }
    REQUIRE(made != nullptr);
    CHECK(made->steps.size() == 2);
    CHECK(rules::checkQuest(*made, {}).empty());

    // Test-play with the debugger's calls: jump to the second step and finish it.
    rules::QuestBook book(quests);
    rules::ActionRunner runner;
    Recorder host;
    Quiet world;
    std::int64_t now = 100;
    book.update(world, now, 1000, runner, host);
    CHECK(book.status("fetch-water") == rules::QuestStatus::Available); // it waits for its giver
    CHECK(book.jumpTo("fetch-water", "return", now));
    CHECK(book.activeStep("fetch-water") == "return");
    CHECK(book.reset("fetch-water"));

    // Played as a player does: the giver hands it out, the hero drinks, tells the elder, and the reward arrives.
    book.update(world, now, 1000, runner, host);
    REQUIRE(book.start("fetch-water", now));
    now += 20;
    book.notify({rules::QuestObjective::Kind::Interact, "drink", 1, now});
    book.update(world, now, 1000, runner, host);
    CHECK(book.activeStep("fetch-water") == "return");
    now += 20;
    book.notify({rules::QuestObjective::Kind::Talk, "elder", 1, now});
    book.update(world, now, 1000, runner, host);
    CHECK(book.status("fetch-water") == rules::QuestStatus::Done);
    REQUIRE(host.log.size() == 1);
    CHECK(host.log[0] == "give hero flint 2 by 0");
}
