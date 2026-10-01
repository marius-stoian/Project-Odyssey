// US-162: who says what: the script chosen for an NPC, the greetings, and the roles.
#include "core/random.h"
#include "sim/dialogue_select.h"
#include "sim/world.h"

#include "story_helpers.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;
namespace sim = odysseus::sim;

namespace {

// The same small world of tables as in the conversation tests: paths and function answers are set by the test.
class Table : public rules::RuleContext {
public:
    std::map<std::string, rules::Value> paths;
    std::map<std::string, long long> answers;

    Table() {
        for (const char* root : {"hero", "npc", "target", "actor"}) paths[root] = rules::Value::ofText(root);
        paths["hero.name"] = rules::Value::ofText("Ayla");
        paths["npc.name"] = rules::Value::ofText("Ama");
    }
    rules::Value path(const std::string& dotted) const override {
        const auto it = paths.find(dotted);
        return it == paths.end() ? rules::Value::ofNumber(0) : it->second;
    }
    rules::Value call(const std::string& name, const std::vector<rules::Value>& args) const override {
        std::string key = name + "(";
        for (std::size_t i = 0; i < args.size(); ++i) key += (i ? "," : "") + (args[i].isText ? args[i].text : std::to_string(args[i].number));
        key += ")";
        const auto it = answers.find(key);
        return rules::Value::ofNumber(it == answers.end() ? 0 : it->second);
    }
};

// A folder of scripts, loaded as the game loads them.
rules::DialogueLibrary libraryOf(const std::string& name, const std::vector<std::pair<std::string, std::string>>& files) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us162" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    for (const auto& [file, text] : files) std::ofstream(folder / (file + ".dlg"), std::ios::binary) << text;
    rules::LoadReport report;
    rules::DialogueLibrary library = rules::DialogueLibrary::load(folder, report);
    for (const auto& d : report.errors) MESSAGE(d.line, ": ", d.message);
    REQUIRE(report.errors.empty());
    return library;
}

std::string talk(const std::string& who, const std::string& header = "", const std::string& words = "hello") {
    return "@who " + who + "\n" + header + "=== start\nX: " + words + "\n-> Bye => END\n";
}

std::string pickTalk(const rules::DialogueLibrary& library, const rules::WhoFacts& who, const Table& world, odysseus::core::Pcg32& random) {
    const rules::DlgScript* script = rules::selectScript(library, who, world, random);
    return script == nullptr ? std::string("-") : script->name;
}

} // namespace

TEST_CASE("US-162 Specific first: a script for Ama beats the one for all elders") {
    const auto library = libraryOf("specific", {{"all-elders", talk("elder", "@priority 50\n")}, {"ama", talk("Ama")}});
    Table world;
    odysseus::core::Pcg32 random(1, 8);
    // Even though the elders' script has a far higher priority: the name is more specific than the role.
    CHECK(pickTalk(library, {"Ama", "person", {"elder"}}, world, random) == "ama");
    CHECK(pickTalk(library, {"Tok", "person", {"elder"}}, world, random) == "all-elders");
    CHECK(pickTalk(library, {"Tok", "person", {}}, world, random) == "-");
}

TEST_CASE("US-162 Opinion: a script for those who dislike the hero is used when someone does") {
    const auto library = libraryOf("opinion", {{"angry", talk("person", "@when opinion(npc, hero) < -30\n@priority 5\n", "Go away.")}, {"plain", talk("person")}});
    Table world;
    odysseus::core::Pcg32 random(1, 8);
    world.answers["opinion(npc,hero)"] = -50;
    CHECK(pickTalk(library, {"Ama", "person", {}}, world, random) == "angry");
    world.answers["opinion(npc,hero)"] = -30; // not below -30
    CHECK(pickTalk(library, {"Ama", "person", {}}, world, random) == "plain");
    world.answers["opinion(npc,hero)"] = 40;
    CHECK(pickTalk(library, {"Ama", "person", {}}, world, random) == "plain");
}

TEST_CASE("US-162 Scripts that fit equally well are chosen between by the seeded stream") {
    const auto library = libraryOf("ties", {{"a", talk("person")}, {"b", talk("person")}, {"c", talk("person")}});
    Table world;
    const auto picks = [&](std::uint64_t seed) {
        odysseus::core::Pcg32 random(seed, 8);
        std::vector<std::string> out;
        for (int i = 0; i < 30; ++i) out.push_back(pickTalk(library, {"Ama", "person", {}}, world, random));
        return out;
    };
    CHECK(picks(7) == picks(7)); // the same seed, the same choices
    CHECK(picks(7) != picks(8));
    const auto many = picks(7);
    CHECK(std::set<std::string>(many.begin(), many.end()).size() == 3); // all three get a turn

    // One number is drawn per call whatever fits, so the stream is the same with no script, one or three.
    const auto stateAfter = [&](const rules::DialogueLibrary& lib) {
        odysseus::core::Pcg32 random(7, 8);
        for (int i = 0; i < 5; ++i) pickTalk(lib, {"Ama", "person", {}}, world, random);
        return random.state();
    };
    const auto none = libraryOf("ties-none", {{"x", talk("wolf")}});
    const auto one = libraryOf("ties-one", {{"x", talk("person")}});
    CHECK(stateAfter(none) == stateAfter(one));
    CHECK(stateAfter(one) == stateAfter(library));
}

TEST_CASE("US-162 A greeting is a script marked @bark; a talk never uses one and a greeting never uses a talk") {
    const auto library = libraryOf("barks", {{"hello", "@who person\n@bark greet\n=== start\nFriend: Hello, {hero}.\n"}, {"chat", talk("person")}});
    Table world;
    odysseus::core::Pcg32 random(1, 8);
    CHECK(pickTalk(library, {"Ama", "person", {}}, world, random) == "chat");
    const rules::DlgScript* bark = rules::selectBark(library, {"Ama", "person", {}}, world, random);
    REQUIRE(bark != nullptr);
    CHECK(bark->name == "hello");
    CHECK(rules::barkText(*bark, world) == "Hello, Ayla.");
    CHECK(rules::selectBark(library, {"Wolf", "wolf", {}}, world, random) == nullptr);
}

TEST_CASE("US-162 A greeting says the first line whose condition holds") {
    const auto library = libraryOf("barktext", {{"hello", "@who person\n@bark greet\n=== start\nFriend: Night greetings.   [if time == night]\nFriend: Good day.\n"}});
    Table world;
    odysseus::core::Pcg32 random(1, 8);
    const rules::DlgScript* bark = rules::selectBark(library, {"Ama", "person", {}}, world, random);
    REQUIRE(bark != nullptr);
    CHECK(rules::barkText(*bark, world) == "Good day.");
    world.paths["time"] = rules::Value::ofText("night");
    CHECK(rules::barkText(*bark, world) == "Night greetings.");
}

TEST_CASE("US-162 Roles come from the world: elder is the oldest, hunter and gatherer the most skilled, child under 12") {
    sim::World world(42, story_test::realConfig());
    const int daysPerYear = world.calendar().daysPerYear();
    // Everyone starts alike, then three adults stand out: the oldest, the best hunter, the best gatherer.
    for (int id = 0; id < static_cast<int>(world.people().size()); ++id) {
        world.personMutable(id)->huntSkill = 10;
        world.personMutable(id)->gatherSkill = 10;
        world.personMutable(id)->ageDays = 25 * daysPerYear;
    }
    sim::Person* elder = world.personMutable(2);
    sim::Person* hunter = world.personMutable(3);
    sim::Person* gatherer = world.personMutable(4);
    REQUIRE((elder && hunter && gatherer));
    elder->ageDays = 90 * daysPerYear;
    hunter->ageDays = 30 * daysPerYear;
    hunter->huntSkill = 99;
    gatherer->ageDays = 30 * daysPerYear;
    gatherer->gatherSkill = 99;
    const auto has = [&](int person, const char* role) {
        const auto roles = rules::rolesOf(world, person);
        return std::find(roles.begin(), roles.end(), role) != roles.end();
    };
    CHECK(has(2, "elder"));
    CHECK_FALSE(has(3, "elder"));
    CHECK(has(3, "hunter"));
    CHECK_FALSE(has(2, "hunter"));
    CHECK(has(4, "gatherer"));
    CHECK_FALSE(has(3, "gatherer"));

    // A child is a child, and is never the best of the adults even with the best skill.
    sim::Person* child = world.personMutable(5);
    REQUIRE(child != nullptr);
    child->ageDays = 5 * daysPerYear;
    child->huntSkill = 100;
    child->gatherSkill = 100;
    CHECK(has(5, "child"));
    CHECK_FALSE(has(5, "hunter"));
    CHECK_FALSE(has(5, "gatherer"));
    CHECK(rules::rolesOf(world, 9999).empty());
}

TEST_CASE("US-162 The shipped greetings are loaded and fit the elder and anyone else") {
    rules::LoadReport report;
    const rules::DialogueLibrary library = rules::DialogueLibrary::load(fs::path(ODYSSEUS_DATA_DIR) / "dialogue", report);
    REQUIRE(report.errors.empty());
    CHECK(library.find("greet-elder") != nullptr);
    CHECK(library.find("greet-friend") != nullptr);
    CHECK(library.find("greet-friend-warm") != nullptr);
    Table world;
    world.answers["opinion(npc,hero)"] = 5;
    odysseus::core::Pcg32 random(1, 8);
    const rules::DlgScript* elder = rules::selectBark(library, {"Ama", "person", {"elder"}}, world, random);
    REQUIRE(elder != nullptr);
    CHECK(elder->name == "greet-elder");
    CHECK(rules::barkText(*elder, world) == "The fire keeps you well, Ayla.");
    std::set<std::string> said;
    for (int i = 0; i < 20; ++i) {
        const rules::DlgScript* other = rules::selectBark(library, {"Tok", "person", {}}, world, random);
        REQUIRE(other != nullptr);
        said.insert(rules::barkText(*other, world));
    }
    CHECK(said == std::set<std::string>{"Good day, Ayla.", "Well met, Ayla."});
}

namespace {

std::string pairScript(const std::string& first, const std::string& second, const std::string& kind = "", const std::string& more = "") {
    return (kind.empty() ? "" : "@bark " + kind + "\n") + "@pair " + first + " " + second + "\n" + more + "=== start\n" + first + ": Hello.\n" + second + ": Hi.\n";
}

std::string pickPair(const rules::DialogueLibrary& library, const rules::WhoFacts& a, const rules::WhoFacts& b, const std::string& kind, const Table& world, odysseus::core::Pcg32& random) {
    const rules::DlgScript* script = rules::selectPair(library, a, b, kind, world, random);
    return script == nullptr ? std::string("-") : script->name;
}

} // namespace

TEST_CASE("US-165 Two people talking: the script that fits both, by kind of event, then how well it names them, then priority") {
    const auto library = libraryOf("pairs", {{"elder-child", pairScript("elder", "child")},
                                              {"any-any", pairScript("person", "person")},
                                              {"ama-child", pairScript("Ama", "child", "talk", "@priority 1\n")},
                                              {"quarrel-any", pairScript("person", "person", "quarrel")},
                                              {"courtship-ama", pairScript("person", "Ama", "courtship")}});
    Table world;
    odysseus::core::Pcg32 random(1, 8);
    const rules::WhoFacts elder{"Tok", "person", {"elder"}};
    const rules::WhoFacts child{"Lia", "person", {"child"}};
    const rules::WhoFacts ama{"Ama", "person", {}};
    CHECK(pickPair(library, elder, child, "talk", world, random) == "elder-child");  // 2 + 2 beats 1 + 1
    CHECK(pickPair(library, ama, child, "talk", world, random) == "ama-child");      // 3 + 2: her own name
    CHECK(pickPair(library, child, elder, "talk", world, random) == "any-any");      // the order matters: this is not elder then child
    CHECK(pickPair(library, elder, child, "quarrel", world, random) == "quarrel-any");
    CHECK(pickPair(library, elder, ama, "courtship", world, random) == "courtship-ama");
    CHECK(pickPair(library, ama, elder, "courtship", world, random) == "-");         // she is the first, not the one wooed
    CHECK(pickPair(library, elder, child, "sharing", world, random) == "-");         // no script for that kind
    // A talk or a greeting never uses a pair script.
    CHECK(pickTalk(library, ama, world, random) == "-");
    CHECK(rules::selectBark(library, ama, world, random) == nullptr);
}

TEST_CASE("US-165 Pair scripts that fit equally well are chosen by the seeded stream, one draw a call") {
    const auto library = libraryOf("pair-ties", {{"a", pairScript("person", "person")}, {"b", pairScript("person", "person")}});
    Table world;
    const rules::WhoFacts someone{"Tok", "person", {}};
    std::set<std::string> seen;
    odysseus::core::Pcg32 random(5, 8);
    for (int i = 0; i < 20; ++i) seen.insert(pickPair(library, someone, someone, "talk", world, random));
    CHECK(seen == std::set<std::string>{"a", "b"});
    odysseus::core::Pcg32 none(7, 8);
    odysseus::core::Pcg32 some(7, 8);
    const auto empty = libraryOf("pair-none", {{"x", talk("wolf")}});
    pickPair(empty, someone, someone, "talk", world, none);
    pickPair(library, someone, someone, "talk", world, some);
    CHECK(none.state() == some.state());
}

TEST_CASE("US-165 The world says who has just talked, and does not keep it for ever") {
    sim::World world(42, story_test::realConfig());
    CHECK(world.takeTalks().empty());
    world.talk(3, 4);
    world.talk(5, 6);
    const auto talks = world.takeTalks();
    REQUIRE(talks.size() == 2);
    CHECK(talks[0].speaker == 3);
    CHECK(talks[0].listener == 4);
    CHECK(talks[1].speaker == 5);
    CHECK(world.takeTalks().empty()); // taken
    for (int i = 0; i < 100; ++i) world.talk(3, 4); // nobody takes them: only the newest are kept
    CHECK(world.takeTalks().size() == 32);
}
