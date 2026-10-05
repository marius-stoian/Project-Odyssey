// US-171 Dialogue graph editor: a .dlg file as cards and wires and back, notes kept, positions in a sidecar.
#include "game/dialogue_graph.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using namespace odysseus::game;
using odysseus::sim::rules::DlgScript;
using odysseus::sim::rules::LoadReport;

namespace {

fs::path dialogueDir() { return fs::path(ODYSSEUS_DATA_DIR) / "dialogue"; }

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

DlgScript parse(const std::string& text, const std::string& name = "test") {
    LoadReport report;
    auto script = odysseus::sim::rules::parseDialogue(text, name, "dialogue/" + name + ".dlg", report);
    REQUIRE_MESSAGE(script.has_value(), (report.errors.empty() ? std::string("no error") : report.errors.front().message));
    return *script;
}

int count(const DialogueGraph& d, const char* type) {
    int n = 0;
    for (const auto& card : d.graph.nodes()) n += card.type == type ? 1 : 0;
    return n;
}

int findCard(const DialogueGraph& d, const char* type, const std::string& firstField) {
    for (const auto& card : d.graph.nodes()) {
        if (card.type == type && !card.fields.empty() && card.fields[0] == firstField) return card.id;
    }
    return 0;
}

} // namespace

TEST_CASE("US-171 Open: elder-fire.dlg is one card per node, line and choice, and each choice is a wire to its target") {
    const DlgScript script = parse(readFile(dialogueDir() / "elder-fire.dlg"), "elder-fire");
    const DialogueGraph d = dialogueToGraph(script, {});
    CHECK(count(d, dlg_card::kNode) == 3);
    CHECK(count(d, dlg_card::kLine) == 4);
    CHECK(count(d, dlg_card::kChoice) == 5);
    CHECK(count(d, dlg_card::kCondition) == 2); // the line [if time == night] and the choice [if has(...)]
    CHECK(count(d, dlg_card::kEffect) == 1);
    CHECK(count(d, dlg_card::kComment) == 1);   // the note above "Offer berries" (the one above the file is kept in the header)
    // "Ask about the hunt" leads to the node "hunt"; "Leave" leads to END, which is no wire.
    const int ask = findCard(d, dlg_card::kChoice, "Ask about the hunt");
    const int hunt = findCard(d, dlg_card::kNode, "hunt");
    const int leave = findCard(d, dlg_card::kChoice, "Leave");
    REQUIRE(ask != 0);
    REQUIRE(hunt != 0);
    bool askToHunt = false;
    bool leaveWired = false;
    for (const auto& wire : d.graph.wires()) {
        if (wire.fromNode == ask && wire.fromPort == 1 && wire.toNode == hunt) askToHunt = true;
        if (wire.fromNode == leave && wire.fromPort == 1) leaveWired = true;
    }
    CHECK(askToHunt);
    CHECK_FALSE(leaveWired);
}

TEST_CASE("US-171 Open: every shipped .dlg file goes to the graph and back as the same text, notes included") {
    int files = 0;
    for (const auto& entry : fs::directory_iterator(dialogueDir())) {
        if (entry.path().extension() != ".dlg") continue;
        ++files;
        const std::string text = readFile(entry.path());
        const DlgScript script = parse(text, entry.path().stem().string());
        const DialogueGraph d = dialogueToGraph(script, {});
        std::vector<std::string> problems;
        const auto back = graphToDialogue(d, problems);
        INFO(entry.path().filename().string());
        REQUIRE(back.has_value());
        CHECK(odysseus::sim::rules::writeDialogue(*back) == text);
        for (const std::string& problem : problems) CHECK_MESSAGE(problem.rfind("warning:", 0) != 0, problem);
    }
    CHECK(files >= 5);
}

TEST_CASE("US-171 Save: a node added and a choice rewired show in the canonical text, positions in the sidecar") {
    const DlgScript script = parse(readFile(dialogueDir() / "elder-fire.dlg"), "elder-fire");
    DialogueGraph d = dialogueToGraph(script, {});
    // A new node "rest" with one line, and the choice "Leave" now leads to it.
    luna::engine::GraphNode node = newDialogueCard(dlg_card::kNode);
    node.fields = {"rest"};
    describeCard(node);
    const int rest = d.graph.add(node);
    luna::engine::GraphNode line = newDialogueCard(dlg_card::kLine);
    line.fields = {"Elder", "Sleep well."};
    describeCard(line);
    const int restLine = d.graph.add(line);
    REQUIRE(d.graph.connect(rest, 0, restLine, 0));
    const int leave = findCard(d, dlg_card::kChoice, "Leave");
    REQUIRE(d.graph.connect(leave, 1, rest, 0));
    d.graph.find(rest)->x = 500;
    d.graph.find(rest)->y = 40;

    std::vector<std::string> problems;
    const auto saved = graphToDialogue(d, problems);
    REQUIRE(saved.has_value());
    const std::string text = odysseus::sim::rules::writeDialogue(*saved);
    CHECK(text.find("-> Leave => rest") != std::string::npos);
    CHECK(text.find("=== rest\nElder: Sleep well.\n") != std::string::npos);

    const std::string json = layoutToJson(layoutOf(d));
    const DialogueLayout layout = layoutFromJson(json);
    REQUIRE(layout.count("rest") == 1);
    CHECK(layout.at("rest") == std::make_pair(500, 40));
    CHECK(layout == layoutOf(d));
    // Open again with the sidecar: every card is where it was left.
    const DialogueGraph again = dialogueToGraph(*saved, layout);
    CHECK(layoutOf(again) == layout);
}

TEST_CASE("US-171 Hand edits: notes written in a text editor are still there after a change in the graph") {
    const std::string text =
        "# Top note\n"
        "@who elder\n"
        "\n"
        "# Note on start\n"
        "=== start\n"
        "# Note on the line\n"
        "Elder: Hello.\n"
        "# Note on the choice\n"
        "-> Bye => END\n";
    const DlgScript script = parse(text);
    DialogueGraph d = dialogueToGraph(script, {});
    d.graph.find(findCard(d, dlg_card::kLine, "Elder"))->fields[1] = "Hello again.";
    std::vector<std::string> problems;
    const auto saved = graphToDialogue(d, problems);
    REQUIRE(saved.has_value());
    const std::string out = odysseus::sim::rules::writeDialogue(*saved);
    CHECK(out.find("# Top note") != std::string::npos);
    CHECK(out.find("# Note on start") != std::string::npos);
    CHECK(out.find("# Note on the line\nElder: Hello again.") != std::string::npos);
    CHECK(out.find("# Note on the choice\n-> Bye => END") != std::string::npos);
}

TEST_CASE("US-171 Save: a mistake in a card is reported and the file is not written from it") {
    const DlgScript script = parse("=== start\nElder: Hi.\n-> Bye => END\n");
    DialogueGraph d = dialogueToGraph(script, {});
    d.graph.find(findCard(d, dlg_card::kChoice, "Bye"))->fields[0] = "Bye";
    luna::engine::GraphNode condition = newDialogueCard(dlg_card::kCondition);
    condition.fields = {"this is not ( an expression"};
    const int c = d.graph.add(condition);
    REQUIRE(d.graph.connect(c, 0, findCard(d, dlg_card::kChoice, "Bye"), 1));
    std::vector<std::string> problems;
    CHECK_FALSE(graphToDialogue(d, problems).has_value());
    REQUIRE_FALSE(problems.empty());
    CHECK(problems.front().rfind("error:", 0) == 0);
}

TEST_CASE("US-171 Save: a card nothing leads to is named as not saved") {
    const DlgScript script = parse("=== start\nElder: Hi.\n");
    DialogueGraph d = dialogueToGraph(script, {});
    d.graph.add(newDialogueCard(dlg_card::kCondition));
    std::vector<std::string> problems;
    CHECK(graphToDialogue(d, problems).has_value());
    REQUIRE(problems.size() == 1);
    CHECK(problems[0].rfind("warning:", 0) == 0);
}

TEST_CASE("US-171 Layout: a damaged sidecar gives an empty layout and the graph is laid out again") {
    CHECK(layoutFromJson("{ not json").empty());
    CHECK(layoutFromJson("{\"cards\": {\"start\": [1]}}").empty());
}
