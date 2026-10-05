// US-172 Interaction graph editor: an interaction file as actor, verb and target cards with requirement and effect cards, and back.
#include "game/interaction_graph.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using namespace odysseus::game;
using odysseus::sim::rules::Interaction;
using odysseus::sim::rules::InteractionRegistry;
using odysseus::sim::rules::LoadReport;

namespace {

fs::path interactionsDir() { return fs::path(ODYSSEUS_DATA_DIR) / "interactions"; }

Interaction gather() {
    LoadReport report;
    const InteractionRegistry registry = InteractionRegistry::load(interactionsDir(), report);
    const Interaction* found = registry.find("gather");
    REQUIRE(found != nullptr);
    return *found;
}

int cardOf(const luna::engine::NodeGraph& g, const char* type) {
    for (const auto& c : g.nodes()) {
        if (c.type == type) return c.id;
    }
    return 0;
}

} // namespace

TEST_CASE("US-172 Open: gather is one verb with an actor, a target, a requirement, effects and an NPC rule") {
    const luna::engine::NodeGraph g = interactionToGraph(gather(), {});
    CHECK(g.nodes().size() == 6);
    const int verb = cardOf(g, rule_card::kVerb);
    REQUIRE(verb != 0);
    CHECK(g.find(verb)->fields[0] == "gather");
    CHECK(g.find(verb)->fields[3] == "2");   // range, metres
    CHECK(g.find(verb)->fields[4] == "3");   // duration, seconds
    CHECK(g.find(cardOf(g, rule_card::kActor))->fields[0] == "hero person");
    CHECK(g.find(cardOf(g, rule_card::kTarget))->fields[0] == "edible plant");
    CHECK(g.find(cardOf(g, rule_card::kEffects))->fields.size() == 3);
    // Actor -> verb and verb -> target are wires.
    bool actorWire = false;
    bool targetWire = false;
    for (const auto& w : g.wires()) {
        if (w.fromNode == cardOf(g, rule_card::kActor) && w.toNode == verb) actorWire = true;
        if (w.fromNode == verb && w.toNode == cardOf(g, rule_card::kTarget)) targetWire = true;
    }
    CHECK(actorWire);
    CHECK(targetWire);
}

TEST_CASE("US-172 Open: every shipped interaction goes to the graph and back as the same data") {
    LoadReport report;
    const InteractionRegistry registry = InteractionRegistry::load(interactionsDir(), report);
    REQUIRE(report.errors.empty());
    int count = 0;
    for (const Interaction& original : registry.all()) {
        ++count;
        const luna::engine::NodeGraph g = interactionToGraph(original, {});
        std::vector<std::string> problems;
        std::string json;
        const auto back = graphToInteraction(g, problems, json);
        INFO(original.id);
        REQUIRE(back.has_value());
        CHECK(json == odysseus::sim::rules::toJson(original));
        for (const std::string& p : problems) CHECK_MESSAGE(p.rfind("warning:", 0) != 0, p);
    }
    CHECK(count >= 20);
}

TEST_CASE("US-172 Save: a changed range, a new effect and a new requirement reach the text") {
    luna::engine::NodeGraph g = interactionToGraph(gather(), {});
    g.find(cardOf(g, rule_card::kVerb))->fields[3] = "1.5";
    g.find(cardOf(g, rule_card::kEffects))->fields.push_back("opinion npc hero 1");
    luna::engine::GraphNode need = newRuleCard(rule_card::kRequirement);
    need.fields = {"hero.hp > 0", "You are too weak"};
    const int needId = g.add(need);
    REQUIRE(g.connect(needId, 0, cardOf(g, rule_card::kVerb), 1));
    std::vector<std::string> problems;
    std::string json;
    const auto saved = graphToInteraction(g, problems, json);
    REQUIRE(saved.has_value());
    CHECK(saved->rangeMilli == 1500);
    CHECK(json.find("\"range\": 1.5") != std::string::npos);
    CHECK(json.find("opinion npc hero 1") != std::string::npos);
    CHECK(json.find("You are too weak") != std::string::npos);
}

TEST_CASE("US-172 Save: a mistake is reported and nothing is written from it") {
    luna::engine::NodeGraph g = interactionToGraph(gather(), {});
    std::vector<std::string> problems;
    std::string json;
    g.find(cardOf(g, rule_card::kVerb))->fields[3] = "far";
    CHECK_FALSE(graphToInteraction(g, problems, json).has_value());
    REQUIRE_FALSE(problems.empty());
    CHECK(problems.front().rfind("error:", 0) == 0);

    g = interactionToGraph(gather(), {});
    g.find(cardOf(g, rule_card::kEffects))->fields[0] = "frobnicate everything";
    problems.clear();
    CHECK_FALSE(graphToInteraction(g, problems, json).has_value()); // the real loader refuses an unknown effect verb

    g = interactionToGraph(gather(), {});
    g.remove(cardOf(g, rule_card::kTarget));
    problems.clear();
    CHECK_FALSE(graphToInteraction(g, problems, json).has_value());
}

TEST_CASE("US-172 Save: a card nothing leads to is named as not saved") {
    luna::engine::NodeGraph g = interactionToGraph(gather(), {});
    g.add(newRuleCard(rule_card::kChronicle));
    std::vector<std::string> problems;
    std::string json;
    CHECK(graphToInteraction(g, problems, json).has_value());
    REQUIRE(problems.size() == 1);
    CHECK(problems[0].rfind("warning:", 0) == 0);
}

TEST_CASE("US-172 Layout: places are kept by what the card is in the file") {
    luna::engine::NodeGraph g = interactionToGraph(gather(), {});
    g.find(cardOf(g, rule_card::kTarget))->x = 400;
    g.find(cardOf(g, rule_card::kTarget))->y = 17;
    const DialogueLayout layout = interactionLayoutOf(g);
    REQUIRE(layout.count("target") == 1);
    CHECK(layout.at("target") == std::make_pair(400, 17));
    const luna::engine::NodeGraph again = interactionToGraph(gather(), layout);
    CHECK(interactionLayoutOf(again) == layout);
}

TEST_CASE("US-172 Numbers: metres and seconds are written the way the files write them") {
    CHECK(milliToText(2000) == "2");
    CHECK(milliToText(1500) == "1.5");
    CHECK(milliToText(250) == "0.25");
    CHECK(milliToText(0) == "0");
    CHECK(textToMilli("1.5") == 1500);
    CHECK(textToMilli("2") == 2000);
    CHECK(textToMilli("0.001") == 1);
    CHECK_FALSE(textToMilli("").has_value());
    CHECK_FALSE(textToMilli("1.2345").has_value());
    CHECK_FALSE(textToMilli("abc").has_value());
    CHECK_FALSE(textToMilli("1.").has_value());
}

TEST_CASE("US-172 Comments: the comment lines at the top of a file are kept, nothing else") {
    CHECK(leadingComments("// one\n// two\n{\n  // inside\n}\n") == "// one\n// two\n");
    CHECK(leadingComments("{\n}\n").empty());
    CHECK(leadingComments("\n// after a blank\n{").find("after a blank") != std::string::npos);
}
