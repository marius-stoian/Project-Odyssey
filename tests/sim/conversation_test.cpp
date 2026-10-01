// US-161: the conversation runtime (lines and choices by condition, effects in order, END), the mood word and the choice of script.
#include "sim/action_runner.h"
#include "sim/conversation.h"
#include "sim/dialogue_select.h"
#include "sim/dialogue_script.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;

namespace {

// A world made of small tables, as in the rules tests.
class Table : public rules::RuleContext {
public:
    std::map<std::string, rules::Value> paths;
    std::map<std::string, long long> answers; // "has(hero,berries,1)" -> 1

    Table() {
        for (const char* root : {"hero", "npc", "target", "actor"}) paths[root] = rules::Value::ofText(root);
        paths["hero.name"] = rules::Value::ofText("Ayla");
        paths["npc.name"] = rules::Value::ofText("Tok");
        paths["target.name"] = rules::Value::ofText("Tok");
        paths["time"] = rules::Value::ofText("morning");
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

// Writes down what the runner asks of the world.
class Recorder : public rules::EffectHost {
public:
    std::vector<std::string> log;
    void setState(const rules::ThingRef& target, const std::string& state) override { log.push_back("set " + std::to_string(target.id) + " " + state); }
    void apply(const rules::Effect& effect, int actor, const rules::ThingRef& target) override {
        log.push_back(effect.source + " by " + std::to_string(actor) + " on " + std::to_string(target.id));
    }
    int ticksPerDay() const override { return 1000; }
};

rules::DlgScript script(const std::string& text, const std::string& name = "x") {
    rules::LoadReport report;
    auto parsed = rules::parseDialogue(text, name, "dialogue/" + name + ".dlg", report);
    for (const auto& d : report.errors) MESSAGE(d.line, ": ", d.message);
    REQUIRE(parsed.has_value());
    return *parsed;
}

const char* const kFixture = R"(@who elder
=== start
Elder: Hello, {hero}. I am {npc}.
Elder: It is night.   [if time == night]
-> Offer berries [if has(hero, berries, 1)] [else You have no berries] {take hero berries 1; opinion npc hero 5} => thanks
-> A secret [if flag(secret)] => thanks
-> Leave => END

=== thanks
Elder: Thank you.
-> Back => start
)";

} // namespace

TEST_CASE("US-161 A node shows the lines whose condition holds and the choices it may") {
    Table world;
    rules::Conversation talk(script(kFixture), 0, {1, 3});
    REQUIRE(talk.nodeId() == "start");
    rules::ConversationView view = talk.view(world);
    REQUIRE(view.lines.size() == 1); // the night line is left out
    CHECK(view.lines[0].speaker == "Elder");
    CHECK(view.lines[0].text == "Hello, Ayla. I am Tok."); // {hero} and {npc} are filled

    // Hidden: the secret has no [else], so it is not listed. Greyed: the berries have an [else] with the reason.
    REQUIRE(view.choices.size() == 2);
    CHECK(view.choices[0].text == "Offer berries");
    CHECK_FALSE(view.choices[0].enabled);
    CHECK(view.choices[0].reason == "You have no berries");
    CHECK(view.choices[1].text == "Leave");
    CHECK(view.choices[1].enabled);

    // The world changes and the same node shows something else.
    world.answers["has(hero,berries,1)"] = 1;
    world.answers["flag(secret)"] = 1;
    world.paths["time"] = rules::Value::ofText("night");
    view = talk.view(world);
    CHECK(view.lines.size() == 2);
    REQUIRE(view.choices.size() == 3);
    CHECK(view.choices[0].enabled);
    CHECK(view.choices[1].text == "A secret");
}

TEST_CASE("US-161 A choice runs its effects in order, then the talk moves on; END closes it") {
    Table world;
    world.answers["has(hero,berries,1)"] = 1;
    Recorder host;
    rules::ActionRunner runner;
    rules::Conversation talk(script(kFixture), 0, {1, 3});

    REQUIRE(talk.choose(0, world, runner, 0, host)); // Offer berries
    REQUIRE(host.log.size() == 2);
    CHECK(host.log[0] == "take hero berries 1 by 0 on 3");
    CHECK(host.log[1] == "opinion npc hero 5 by 0 on 3");
    CHECK(talk.nodeId() == "thanks");
    CHECK_FALSE(talk.finished());
    REQUIRE(talk.view(world).lines.size() == 1);

    REQUIRE(talk.choose(0, world, runner, 0, host)); // Back
    CHECK(talk.nodeId() == "start");
    REQUIRE(talk.choose(1, world, runner, 0, host)); // Leave (the secret is hidden, so Leave is second)
    CHECK(talk.finished());
    CHECK(host.log.size() == 2); // Back and Leave have no effects
    CHECK(talk.view(world).choices.empty());
    CHECK_FALSE(talk.choose(0, world, runner, 0, host)); // nothing to choose once it is over
}

TEST_CASE("US-161 A greyed-out or missing choice does nothing") {
    Table world; // no berries
    Recorder host;
    rules::ActionRunner runner;
    rules::Conversation talk(script(kFixture), 0, {1, 3});
    CHECK_FALSE(talk.choose(0, world, runner, 0, host)); // greyed out
    CHECK_FALSE(talk.choose(7, world, runner, 0, host));
    CHECK_FALSE(talk.choose(-1, world, runner, 0, host));
    CHECK(talk.nodeId() == "start");
    CHECK(host.log.empty());
    talk.leave(); // Esc: the talk is over and no effect happened
    CHECK(talk.finished());
    CHECK(host.log.empty());
}

TEST_CASE("US-161 An effect with a delay waits in the runner like an interaction's does") {
    Table world;
    Recorder host;
    rules::ActionRunner runner;
    rules::Conversation talk(script("=== start\nElder: Wait.\n-> Later {after 2s opinion npc hero 1} => END\n"), 0, {1, 3});
    REQUIRE(talk.choose(0, world, runner, 0, host));
    CHECK(host.log.empty());
    CHECK(runner.pending().size() == 1);
    rules::InteractionRegistry none;
    runner.tick(2 * rules::ActionRunner::kTicksPerSecond, none, host);
    REQUIRE(host.log.size() == 1);
    CHECK(host.log[0].rfind("opinion npc hero 1", 0) == 0);
}

TEST_CASE("US-161 A talk with a script of no nodes is over at once") {
    rules::DlgScript empty;
    rules::Conversation talk(empty, 0, {1, 3});
    CHECK(talk.finished());
}

TEST_CASE("US-161 The mood word is one word from the opinion and the most pressing need") {
    odysseus::sim::Needs full; // 100 each
    CHECK(rules::moodWord(60, full) == "warm");
    CHECK(rules::moodWord(40, full) == "warm");
    CHECK(rules::moodWord(20, full) == "friendly");
    CHECK(rules::moodWord(0, full) == "neutral");
    CHECK(rules::moodWord(-14, full) == "neutral");
    CHECK(rules::moodWord(-15, full) == "wary");
    CHECK(rules::moodWord(-39, full) == "wary");
    CHECK(rules::moodWord(-40, full) == "hostile");

    odysseus::sim::Needs hungry = full;
    hungry[odysseus::sim::Need::Hunger] = 10;
    CHECK(rules::moodWord(60, hungry) == "hungry"); // a pressing need shows before a good feeling
    CHECK(rules::moodWord(-20, hungry) == "hungry");
    CHECK(rules::moodWord(-60, hungry) == "hostile"); // but not before open hostility

    odysseus::sim::Needs both = full; // the lowest need wins
    both[odysseus::sim::Need::Warmth] = 20;
    both[odysseus::sim::Need::Social] = 5;
    CHECK(rules::moodWord(0, both) == "lonely");
    both[odysseus::sim::Need::Social] = 20; // a tie: the first need in the list
    CHECK(rules::moodWord(0, both) == "cold");
    CHECK(rules::moodWord(0, [&] { auto n = full; n[odysseus::sim::Need::Energy] = 24; return n; }()) == "tired");
}

TEST_CASE("US-161 The script that speaks for someone: their name, then a role, then a kind, then the priority") {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us161" / "select";
    fs::remove_all(folder.parent_path());
    fs::create_directories(folder);
    const auto write = [&](const std::string& name, const std::string& text) { std::ofstream(folder / (name + ".dlg"), std::ios::binary) << text; };
    write("a-kind", "@who person\n=== start\nX: kind\n-> Bye => END\n");
    write("b-role", "@who elder\n@priority 1\n=== start\nX: role\n-> Bye => END\n");
    write("c-role-strong", "@who elder\n@priority 5\n=== start\nX: stronger role\n-> Bye => END\n");
    write("d-name", "@who Tok\n=== start\nX: name\n-> Bye => END\n");
    write("e-bark", "@who Tok\n@bark greet\n=== start\nX: hi\n-> Bye => END\n");
    write("f-when", "@who Mara\n@when opinion(npc, hero) >= 10\n=== start\nX: friend\n-> Bye => END\n");
    rules::LoadReport report;
    const rules::DialogueLibrary library = rules::DialogueLibrary::load(folder, report);
    REQUIRE(report.errors.empty());
    Table world;
    const auto pick = [&](rules::WhoFacts who) {
        const rules::DlgScript* chosen = rules::selectScript(library, who, world);
        return chosen == nullptr ? std::string("-") : chosen->name;
    };

    CHECK(pick({"Tok", "person", {"elder"}}) == "d-name");       // the name beats the role and the kind; the bark never speaks for a Talk
    CHECK(pick({"Ira", "person", {"elder"}}) == "c-role-strong"); // two roles: the higher priority
    CHECK(pick({"Ira", "person", {}}) == "a-kind");
    CHECK(pick({"Wolf", "wolf", {}}) == "-");                     // nothing fits: the plain talk
    CHECK(pick({"Mara", "person", {}}) == "a-kind");              // @when fails: the kind script speaks
    world.answers["opinion(npc,hero)"] = 25;
    CHECK(pick({"Mara", "person", {}}) == "f-when");              // @when holds
    CHECK(pick({"MARA", "person", {}}) == "f-when");              // names do not mind capitals
}

TEST_CASE("US-161 The shipped elder script: offering berries takes one, raises opinion by 5 and shows the next node") {
    rules::LoadReport report;
    const rules::DialogueLibrary library = rules::DialogueLibrary::load(fs::path(ODYSSEUS_DATA_DIR) / "dialogue", report);
    REQUIRE(report.errors.empty());
    const rules::DlgScript* elder = library.find("elder-fire");
    REQUIRE(elder != nullptr);
    Table world;
    world.answers["has(hero,berries,1)"] = 1;
    Recorder host;
    rules::ActionRunner runner;
    rules::Conversation talk(*elder, 0, {1, 3});
    const rules::ConversationView first = talk.view(world);
    REQUIRE(first.choices.size() == 3);
    CHECK(first.choices[1].text == "Offer berries"); // the key 2
    REQUIRE(talk.choose(1, world, runner, 0, host));
    REQUIRE(host.log.size() >= 2);
    CHECK(host.log[0] == "take hero berries 1 by 0 on 3");
    CHECK(host.log[1] == "opinion npc hero 5 by 0 on 3");
    CHECK(talk.nodeId() == "thanks");
    REQUIRE(talk.view(world).lines.size() == 1);
    CHECK(talk.view(world).lines[0].text == "The clan remembers kindness.");
}
