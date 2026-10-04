// US-261 Kind defaults and placed-NPC overrides: the kind files of assets/data/npcs and the precedence of classes, kind and placed NPC.
#include "sim/npc_kind.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;

namespace {

rules::NpcClassCatalog twoClasses() {
    rules::LoadReport report;
    const fs::path folder = fs::temp_directory_path() / "odysseus-us261" / "npc-classes";
    fs::remove_all(folder.parent_path());
    fs::create_directories(folder);
    std::ofstream(folder / "trader.json") << R"({ "id": "trader", "label": "Trader", "colour": "#d9a441", "icon": "coin", "tags": ["trader"],
  "dialogues": { "player": "trader.dlg" }, "actions": { "allow": ["barter", "talk"], "deny": [] } })";
    std::ofstream(folder / "elder.json") << R"({ "id": "elder", "label": "Elder", "colour": "#b08968", "icon": "star", "tags": ["elder"],
  "dialogues": { "player": "elder.dlg", "animal": "elder-animal.dlg" }, "actions": { "allow": ["teach"], "deny": ["barter"] } })";
    const auto catalog = rules::NpcClassCatalog::load(folder, report);
    REQUIRE(report.errors.empty());
    return catalog;
}

} // namespace

TEST_CASE("US-261 Shipped kinds: every character kind and animal has a kind file that loads and round-trips") {
    rules::LoadReport report;
    const rules::NpcKindCatalog kinds = rules::NpcKindCatalog::load(fs::path(ODYSSEUS_DATA_DIR) / "npcs", report);
    for (const auto& error : report.errors) FAIL(error.text());
    CHECK(kinds.all().size() >= 61);
    const rules::NpcKind* goblin = kinds.find("goblin");
    REQUIRE(goblin != nullptr);
    CHECK(goblin->layer.attitude == "hostile"); // today's behaviour: enemies are hostile
    CHECK(goblin->layer.classes == std::optional<std::vector<std::string>>(std::vector<std::string>{"monster"}));
    REQUIRE(kinds.find("wanderer") != nullptr);
    CHECK(kinds.find("wanderer")->layer.attitude == "neutral");
    CHECK(kinds.find("hero") == nullptr);
    for (const rules::NpcKind& kind : kinds.all()) {
        rules::LoadReport again;
        const auto reread = rules::NpcKindCatalog::parse(rules::toJson(kind), kind.file, again, kind.kind);
        REQUIRE_MESSAGE(reread.has_value(), kind.kind);
        CHECK(*reread == kind);
    }
}

TEST_CASE("US-261 Precedence: classes, then the kind, then the placed NPC; allow and deny merge and a later deny wins") {
    const rules::NpcClassCatalog classes = twoClasses();
    rules::NpcLayer kind;
    kind.classes = std::vector<std::string>{"trader"};
    kind.attitude = "wary";
    kind.tags = {"local"};
    kind.dialogues = {{"player", "kind.dlg"}};

    SUBCASE("a placed NPC with nothing set is its kind, on top of its classes") {
        const rules::ResolvedNpc npc = rules::resolveNpc(classes, &kind, {});
        CHECK(npc.classes == std::vector<std::string>{"trader"});
        CHECK(npc.attitude == "wary");
        CHECK(npc.tags == std::vector<std::string>{"local", "trader"});
        CHECK(npc.dialogues.at("player") == "kind.dlg"); // the kind overrides the class default
        CHECK(npc.action("barter") == rules::ActionState::Allowed);
        CHECK(npc.action("fly") == rules::ActionState::Unset);
    }
    SUBCASE("trade is denied for the placed NPC that denies it, and only for it") {
        rules::NpcLayer placed;
        placed.deny = {"barter"};
        const rules::ResolvedNpc denied = rules::resolveNpc(classes, &kind, placed);
        const rules::ResolvedNpc other = rules::resolveNpc(classes, &kind, {});
        CHECK(denied.denied("barter"));
        CHECK(denied.action("talk") == rules::ActionState::Allowed);
        CHECK_FALSE(other.denied("barter"));
        CHECK(other.action("barter") == rules::ActionState::Allowed);
    }
    SUBCASE("the placed NPC replaces classes and attitude; the new classes bring their defaults") {
        rules::NpcLayer placed;
        placed.classes = std::vector<std::string>{"elder", "trader"};
        placed.attitude = "friendly";
        const rules::ResolvedNpc npc = rules::resolveNpc(classes, &kind, placed);
        CHECK(npc.classes == std::vector<std::string>{"elder", "trader"});
        CHECK(npc.attitude == "friendly");
        CHECK(npc.tags == std::vector<std::string>{"elder", "local", "trader"});
        // elder denies barter, trader (later) allows it: the later class overrides.
        CHECK(npc.action("barter") == rules::ActionState::Allowed);
        CHECK(npc.action("teach") == rules::ActionState::Allowed);
        CHECK(npc.dialogues.at("animal") == "elder-animal.dlg");
    }
    SUBCASE("the classes' own deny holds until a later layer allows it again") {
        rules::NpcLayer elderKind;
        elderKind.classes = std::vector<std::string>{"elder"};
        CHECK(rules::resolveNpc(classes, &elderKind, {}).denied("barter"));
        rules::NpcLayer placed;
        placed.allow = {"barter"};
        CHECK(rules::resolveNpc(classes, &elderKind, placed).action("barter") == rules::ActionState::Allowed);
        placed.deny = {"barter"}; // inside one layer a deny beats an allow
        CHECK(rules::resolveNpc(classes, &elderKind, placed).denied("barter"));
    }
    SUBCASE("no kind file, an unknown class") {
        rules::NpcLayer placed;
        placed.classes = std::vector<std::string>{"ghost-class"};
        const rules::ResolvedNpc npc = rules::resolveNpc(classes, nullptr, placed);
        CHECK(npc.classes == std::vector<std::string>{"ghost-class"});
        CHECK(npc.attitude == "neutral");
        CHECK(npc.tags.empty());
    }
}

TEST_CASE("US-261 Mistakes: the kind file is named with its line; the other kinds load") {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us261b" / "npcs";
    fs::remove_all(folder.parent_path());
    fs::create_directories(folder);
    std::ofstream(folder / "goblin.json") << "{\n  \"kind\": \"goblin\",\n  \"classes\": [\"monster\"],\n  \"attitude\": \"grumpy\"\n}\n";
    std::ofstream(folder / "wolf.json") << "{\n  \"kind\": \"wolf\",\n  \"attitude\": \"hostile\"\n}\n";
    rules::LoadReport report;
    const rules::NpcKindCatalog kinds = rules::NpcKindCatalog::load(folder, report);
    REQUIRE(report.errors.size() == 1);
    CHECK(report.errors[0].text().find("npcs/goblin.json:4:") == 0);
    CHECK(report.errors[0].message.find("attitude") != std::string::npos);
    CHECK(kinds.find("goblin") == nullptr);
    CHECK(kinds.find("wolf") != nullptr);

    const auto problem = [](const std::string& text) {
        rules::LoadReport r;
        rules::NpcKindCatalog::parse(text, "npcs/x.json", r, "x");
        return r.errors.empty() ? std::string() : r.errors.front().text();
    };
    CHECK(problem(R"({ "kind": "y" })").find("must match the file name") != std::string::npos);
    CHECK(problem(R"({ "kind": "x", "moods": 1 })").find("moods") != std::string::npos);
    CHECK(problem(R"({ "kind": "x", "classes": "trader" })").find("classes") != std::string::npos);
    CHECK(problem(R"({ "kind": "x", "dialogues": { "robot": "a.dlg" } })").find("partner type") != std::string::npos);
    CHECK(problem(R"({ "kind": "x", "actions": { "allow": [1] } })").find("allow") != std::string::npos);
    CHECK(problem(R"({ "kind": "x" })").empty());
    CHECK(rules::attitudeNames().size() == 9);
}
