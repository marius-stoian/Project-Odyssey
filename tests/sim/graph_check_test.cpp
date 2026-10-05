// US-175 Graph validation: unreachable nodes, dead ends and unknown names in conversations and interactions, and the same check over every shipped file.
#include "sim/dialogue_script.h"
#include "sim/graph_check.h"
#include "sim/hero_data.h"
#include "sim/interaction.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;
using Kind = rules::GraphFinding::Kind;

namespace {

rules::DlgScript parse(const std::string& text) {
    rules::LoadReport report;
    auto script = rules::parseDialogue(text, "test", "dialogue/test.dlg", report);
    REQUIRE_MESSAGE(script.has_value(), (report.errors.empty() ? std::string("no error") : report.errors.front().message));
    return *script;
}

bool has(const std::vector<rules::GraphFinding>& findings, Kind kind, const std::string& key) {
    for (const auto& f : findings) {
        if (f.kind == kind && f.key == key) return true;
    }
    return false;
}

const rules::GraphFinding* find(const std::vector<rules::GraphFinding>& findings, Kind kind) {
    for (const auto& f : findings) {
        if (f.kind == kind) return &f;
    }
    return nullptr;
}

} // namespace

TEST_CASE("US-175 Unreachable: a node no choice leads to is listed, with the node as its place") {
    const auto script = parse("=== start\nElder: Hi.\n-> Bye => END\n\n=== orphan\nElder: Lonely.\n-> Back => start\n");
    const auto findings = rules::checkDialogue(script, {});
    CHECK(has(findings, Kind::Unreachable, "orphan"));
    CHECK_FALSE(has(findings, Kind::Unreachable, "start"));
    const auto* f = find(findings, Kind::Unreachable);
    REQUIRE(f != nullptr);
    CHECK_FALSE(f->error); // it loads fine: a warning
    CHECK(f->text() == "dialogue/test.dlg: node \"orphan\" cannot be reached: no choice leads to it");
}

TEST_CASE("US-175 Unreachable: nodes only an unreachable node leads to are listed too") {
    const auto script = parse("=== start\nElder: Hi.\n-> Bye => END\n\n=== a\nElder: A.\n-> On => b\n\n=== b\nElder: B.\n-> Back => a\n");
    const auto findings = rules::checkDialogue(script, {});
    CHECK(has(findings, Kind::Unreachable, "a"));
    CHECK(has(findings, Kind::Unreachable, "b"));
}

TEST_CASE("US-175 Dead end: a node with no choice and no END is listed; a greeting is not") {
    const auto script = parse("=== start\nElder: Hi.\n-> On => stuck\n\n=== stuck\nElder: ...\n");
    const auto findings = rules::checkDialogue(script, {});
    CHECK(has(findings, Kind::DeadEnd, "stuck"));
    CHECK_FALSE(has(findings, Kind::DeadEnd, "start"));
    CHECK(find(findings, Kind::DeadEnd)->error);

    const auto greeting = parse("@bark greet\n\n=== start\nElder: The fire keeps you well.\n");
    CHECK(rules::checkDialogue(greeting, {}).empty());
}

TEST_CASE("US-175 Unknown: an effect giving an item no catalog has lists the item and the node") {
    const auto script = parse("=== start\nElder: Hi.\n-> Share [if has(hero, berrys, 1)] {take hero berrys 1; give npc glowstone 1} => END\n");
    rules::GraphCatalog catalog;
    catalog.items = {"berries", "stick"};
    const auto findings = rules::checkDialogue(script, catalog);
    REQUIRE(has(findings, Kind::UnknownItem, "start/choice0/do"));
    REQUIRE(has(findings, Kind::UnknownItem, "start/choice0/if"));
    std::string messages;
    for (const auto& f : findings) messages += f.message + "\n";
    CHECK(messages.find("start, choice 1: unknown item \"berrys\"") != std::string::npos);
    CHECK(messages.find("unknown item \"glowstone\"") != std::string::npos);
    // With no item catalog there is nothing to check against.
    CHECK(rules::checkDialogue(script, {}).empty());
}

TEST_CASE("US-175 Unknown: needs, built-in actions, conversations and interactions") {
    const auto script = parse("=== start\nElder: Hi. [if need(hungr) > 10]\n-> A {do launch-rocket} => END\n-> B {talk nobody; start nothing} => END\n");
    rules::GraphCatalog catalog;
    catalog.builtins = {"give-berries"};
    catalog.dialogues = {"elder-fire"};
    catalog.interactions = {"gather"};
    const auto findings = rules::checkDialogue(script, catalog);
    CHECK(has(findings, Kind::UnknownNeed, "start/line0/if"));
    CHECK(has(findings, Kind::UnknownBuiltin, "start/choice0/do"));
    CHECK(has(findings, Kind::UnknownDialogue, "start/choice1/do"));
    CHECK(has(findings, Kind::UnknownInteraction, "start/choice1/do"));
    const auto good = parse("=== start\nElder: Hi. [if need(hunger) > 10]\n-> A {do give-berries} => END\n");
    CHECK(rules::checkDialogue(good, catalog).empty());
}

TEST_CASE("US-175 Interactions: the same names are checked in requirements, effects, the NPC score and the target tags") {
    rules::LoadReport report;
    const auto interaction = rules::InteractionRegistry::parse(
        "{ \"id\": \"t\", \"label\": \"T\", \"actors\": [\"hero\"], \"target\": { \"tags\": [\"nothing-has-this\"] }, \"range\": 1, \"duration\": 0, \"order\": 1,"
        " \"requires\": [ { \"if\": \"has(berrys, 1)\", \"else\": \"no\" } ], \"effects\": [ \"give actor glowstone 1\" ],"
        " \"npc\": { \"score\": \"need(hungr)\", \"cooldown\": 5 } }",
        "interactions/t.json", report, "t");
    REQUIRE(interaction.has_value());
    rules::GraphCatalog catalog;
    catalog.items = {"berries"};
    catalog.tags = {"plant"};
    const auto findings = rules::checkInteraction(*interaction, catalog);
    CHECK(has(findings, Kind::UnknownItem, "requirement0"));
    CHECK(has(findings, Kind::UnknownItem, "effects0"));
    CHECK(has(findings, Kind::UnknownNeed, "npcrule"));
    CHECK(has(findings, Kind::UnknownTag, "target"));
    CHECK_FALSE(find(findings, Kind::UnknownTag)->error);
}

TEST_CASE("US-175 Shipped: no shipped conversation or interaction has an error, with the game's real items") {
    const fs::path data = fs::path(ODYSSEUS_DATA_DIR);
    rules::GraphCatalog catalog;
    const auto hero = odysseus::sim::loadHeroData(data);
    for (const auto& item : hero.items) catalog.items.insert(item.id);
    REQUIRE_FALSE(catalog.items.empty());

    rules::LoadReport dialogueReport;
    const auto library = rules::DialogueLibrary::load(data / "dialogue", dialogueReport);
    REQUIRE(dialogueReport.errors.empty());
    for (const auto& script : library.all()) catalog.dialogues.insert(script.name);
    rules::LoadReport interactionReport;
    const auto registry = rules::InteractionRegistry::load(data / "interactions", interactionReport);
    REQUIRE(interactionReport.errors.empty());
    for (const auto& interaction : registry.all()) catalog.interactions.insert(interaction.id);

    int checked = 0;
    for (const auto& script : library.all()) {
        for (const auto& f : rules::checkDialogue(script, catalog)) {
            CHECK_MESSAGE(!f.error, f.text());
        }
        ++checked;
    }
    for (const auto& interaction : registry.all()) {
        for (const auto& f : rules::checkInteraction(interaction, catalog)) {
            CHECK_MESSAGE(!f.error, f.text());
        }
        ++checked;
    }
    CHECK(checked > 50);
}
