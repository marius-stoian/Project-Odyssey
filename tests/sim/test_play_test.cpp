// US-174 Test-play: a conversation played on a world of its own with chosen values.
#include "sim/dialogue_script.h"
#include "sim/test_play.h"

#include <doctest/doctest.h>

#include <string>
#include <vector>

namespace rules = odysseus::sim::rules;

namespace {

rules::DlgScript parse(const std::string& text) {
    rules::LoadReport report;
    auto script = rules::parseDialogue(text, "test", "dialogue/test.dlg", report);
    REQUIRE_MESSAGE(script.has_value(), (report.errors.empty() ? std::string("no error") : report.errors.front().message));
    return *script;
}

const char* kScript =
    "=== start\n"
    "Elder: Welcome, {hero}.   [if time == night]\n"
    "Elder: The fire is warm.\n"
    "-> Ask for help [if opinion(npc, hero) >= 20] [else They do not trust you yet] => help\n"
    "-> Offer a berry [if has(hero, berries, 1)] {take hero berries 1; opinion npc hero 5; flag fed-elder; after 5s say \"Thanks\"} => thanks\n"
    "-> Leave => END\n"
    "\n"
    "=== help\n"
    "Elder: Take this stone.\n"
    "-> Thanks {give hero stone 1} => END\n"
    "\n"
    "=== thanks\n"
    "Elder: The clan remembers.\n"
    "-> Back => start\n";

} // namespace

TEST_CASE("US-174 Branch: a choice that needs opinion 20 is offered once the owner sets the opinion to 25") {
    const auto script = parse(kScript);
    rules::TestState state;
    {
        rules::TestPlay play(script, state);
        const auto view = play.view();
        REQUIRE(view.choices.size() >= 1);
        CHECK(view.choices[0].text == "Ask for help");
        CHECK_FALSE(view.choices[0].enabled); // greyed with its reason
        CHECK(view.choices[0].reason == "They do not trust you yet");
        CHECK_FALSE(play.choose(0));
    }
    std::string problem;
    REQUIRE(rules::applyTestState(state, "opinion=25", problem));
    rules::TestPlay play(script, state);
    const auto view = play.view();
    CHECK(view.choices[0].enabled);
    CHECK(play.choose(0));
    CHECK(play.nodeId() == "help");
}

TEST_CASE("US-174 Branch: time, items, needs and flags decide what is shown") {
    const auto script = parse(kScript);
    rules::TestState state;
    std::string problem;
    {
        rules::TestPlay day(script, state);
        CHECK(day.view().lines.size() == 1); // no night greeting in the afternoon
        CHECK(day.view().choices.size() == 2); // no berry: "Offer a berry" is hidden
    }
    REQUIRE(rules::applyTestState(state, "time=night item.berries=2", problem));
    rules::TestPlay night(script, state);
    const auto view = night.view();
    REQUIRE(view.lines.size() == 2);
    CHECK(view.lines[0].text == "Welcome, Hero.");
    CHECK(view.choices.size() == 3);
}

TEST_CASE("US-174 Start anywhere: Play from here begins at the chosen node") {
    const auto script = parse(kScript);
    rules::TestPlay play(script, {}, "thanks");
    REQUIRE(play.started());
    CHECK(play.nodeId() == "thanks");
    CHECK(play.view().lines[0].text == "The clan remembers.");
    rules::TestPlay missing(script, {}, "nowhere");
    CHECK_FALSE(missing.started());
}

TEST_CASE("US-174 Effects: they change the state of the test world and are listed; waiting ones are marked later") {
    const auto script = parse(kScript);
    rules::TestState state;
    std::string problem;
    REQUIRE(rules::applyTestState(state, "item.berries=2", problem));
    rules::TestPlay play(script, state);
    REQUIRE(play.view().choices.size() == 3);
    REQUIRE(play.choose(1)); // Offer a berry
    CHECK(play.state().items.at("berries") == 1);
    CHECK(play.state().opinion == 5);
    CHECK(play.state().flags.at("fed-elder") == 1);
    const auto log = play.log();
    REQUIRE(log.size() >= 4);
    bool later = false;
    for (const auto& line : log) later = later || line.rfind("later (5 s):", 0) == 0;
    CHECK(later);
    CHECK(play.nodeId() == "thanks");
    // Taking the last berry removes it from the bag.
    REQUIRE(play.choose(0));
    REQUIRE(play.choose(1));
    CHECK(play.state().items.count("berries") == 0);
}

TEST_CASE("US-174 No side effects: a test-play that gives items leaves the starting state, the script and the data folder as they were") {
    const auto script = parse(kScript);
    const std::string before = rules::writeDialogue(script);
    rules::TestState state;
    std::string problem;
    REQUIRE(rules::applyTestState(state, "opinion=30", problem));
    const rules::TestState started = state;
    {
        rules::TestPlay play(script, state);
        REQUIRE(play.choose(0)); // help
        REQUIRE(play.choose(0)); // thanks: gives a stone
        CHECK(play.state().items.at("stone") == 1);
        CHECK(play.finished());
    }
    CHECK(state == started);                          // the state the owner typed is not changed by the play
    CHECK(rules::writeDialogue(script) == before);    // nor the script
}

TEST_CASE("US-174 State: words set the values, a mistake is named and changes nothing, and the text reads back the same") {
    rules::TestState state;
    std::string problem;
    REQUIRE(rules::applyTestState(state, "opinion=-40 hunger=10 item.berries=3 skill.hunter=2 trait.diligent flag.met tag.trader kin time=night season=winter hero=Joro npc=Ama", problem));
    CHECK(state.opinion == -40);
    CHECK(state.needs[0] == 10);
    CHECK(state.items.at("berries") == 3);
    CHECK(state.traits.count("diligent") == 1);
    CHECK(state.kin);
    CHECK(state.season == "winter");
    CHECK(state.heroName == "Joro");
    const std::string text = rules::testStateText(state);
    rules::TestState again;
    REQUIRE(rules::applyTestState(again, text, problem));
    CHECK(again == state);

    const rules::TestState untouched = state;
    CHECK_FALSE(rules::applyTestState(state, "opinion=150", problem));
    CHECK(problem.find("opinion=150") != std::string::npos);
    CHECK_FALSE(rules::applyTestState(state, "opinion=5 wobble=1", problem));
    CHECK(state == untouched); // the first word was good, but nothing changed
    CHECK_FALSE(rules::applyTestState(state, "item.berries=many", problem));
}

TEST_CASE("US-174 Mood and kin: the conversation reads the same words as the game") {
    const auto script = parse("=== start\nElder: Hm. [if mood(npc) == wary]\nElder: Cousin! [if kin(npc, hero)]\nElder: Hello.\n-> Leave => END\n");
    rules::TestState state;
    std::string problem;
    REQUIRE(rules::applyTestState(state, "opinion=-20 kin", problem));
    rules::TestPlay play(script, state);
    const auto view = play.view();
    REQUIRE(view.lines.size() == 3);
    CHECK(view.lines[0].text == "Hm.");
    CHECK(view.lines[1].text == "Cousin!");
}
