// US-161: Talk opens the conversation panel; choices by mouse or key; hidden and greyed choices.
#include "camp.h"

#include "sim/dialogue_select.h"

using namespace camp_support;

namespace {

namespace sim = odysseus::sim;

// The clan member the shipped elder script speaks for, standing in the camp (the hero is never the one talked to).
int elderOf(Camp& camp) {
    const sim::World& world = *camp.odyssey.clan();
    const int hero = camp.odyssey.life()->personId();
    const auto& figures = camp.odyssey.clanView().figures();
    for (std::size_t i = 0; i < figures.size(); ++i) {
        if (static_cast<int>(i) == hero || !figures[i].present) continue;
        const auto roles = sim::rules::rolesOf(world, static_cast<int>(i));
        if (std::find(roles.begin(), roles.end(), "elder") != roles.end()) return static_cast<int>(i);
    }
    return -1;
}

game::Subject personSubjectOf(Camp& camp, int person) {
    const auto subject = game::subjectFor(camp.odyssey, {static_cast<int>(game::Subject::Kind::Person), person});
    REQUIRE(subject.has_value());
    return *subject;
}

// One tick with a key pressed, or with a left click at a point.
luna::engine::Intents keyPressed(luna::engine::Intent intent) {
    luna::engine::Intents intents;
    intents.set(intent, true, true);
    return intents;
}

luna::engine::Intents clickAt(int x, int y) {
    luna::engine::Intents intents;
    luna::engine::Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    pointer.held[static_cast<std::size_t>(luna::engine::PointerButton::Left)] = true;
    pointer.pressed[static_cast<std::size_t>(luna::engine::PointerButton::Left)] = true;
    intents.setPointer(pointer);
    return intents;
}

} // namespace

TEST_CASE("US-161 Talk: the game pauses and the panel shows the elder's name, mood, first line and numbered choices") {
    Camp camp("talk-opens");
    const int elder = elderOf(camp);
    REQUIRE(elder >= 0);
    const std::string name = camp.odyssey.clan()->people()[static_cast<std::size_t>(elder)].name;

    REQUIRE(game::startInteraction(camp.odyssey, "talk", personSubjectOf(camp, elder))); // what the menu's Talk does
    CHECK(camp.odyssey.run().screen() == game::Screen::Talk);
    CHECK(camp.odyssey.run().modal());
    camp.play(1);

    // The world stands still while the panel is open.
    const auto ticks = camp.odyssey.clan()->ticks();
    camp.play(40);
    CHECK(camp.odyssey.clan()->ticks() == ticks);
    CHECK(camp.odyssey.run().screen() == game::Screen::Talk);

    const std::vector<std::string> shown = camp.odyssey.run().shownText();
    REQUIRE_FALSE(shown.empty());
    CHECK(shown[0].rfind(name + "   (", 0) == 0); // "Name   (mood)"
    const std::string mood = sim::rules::moodWord(camp.odyssey.clan()->opinion(elder, camp.odyssey.life()->personId()), camp.odyssey.clan()->people()[static_cast<std::size_t>(elder)].needs);
    CHECK(shown[0] == name + "   (" + mood + ")");
    REQUIRE(shown.size() >= 2);
    CHECK((shown[1] == "Elder: The fire is low tonight, " + camp.odyssey.life()->name() + "." || shown[1] == "Elder: You walk like a hunter today.")); // the first line

    const auto& widgets = camp.odyssey.run().widgets();
    REQUIRE(widgets.size() == 3);
    CHECK(widgets[0].label == "1. Ask about the hunt");
    CHECK(widgets[1].label.rfind("2. Offer berries", 0) == 0);
    CHECK(widgets[2].label == "3. Leave");
}

TEST_CASE("US-161 Choose: the key 2 takes a berry, raises the elder's opinion by 5 and shows the next node") {
    Camp camp("talk-key");
    const int elder = elderOf(camp);
    REQUIRE(elder >= 0);
    const int hero = camp.odyssey.life()->personId();
    camp.odyssey.life()->give("berries", 3);
    REQUIRE(game::startInteraction(camp.odyssey, "talk", personSubjectOf(camp, elder)));
    camp.play(1);
    REQUIRE(camp.odyssey.run().widgets().size() == 3);
    CHECK(camp.odyssey.run().widgets()[1].enabled);

    const int before = camp.odyssey.clan()->opinion(elder, hero);
    camp.odyssey.update(keyPressed(luna::engine::Intent::Slot2));
    CHECK(camp.berries() == 2);
    CHECK(camp.odyssey.clan()->opinion(elder, hero) == before + 5);
    const std::vector<std::string> shown = camp.odyssey.run().shownText();
    REQUIRE(shown.size() >= 2);
    CHECK(shown[1] == "Elder: The clan remembers kindness.");
    CHECK(camp.odyssey.run().screen() == game::Screen::Talk); // the talk goes on at the next node
    REQUIRE(camp.odyssey.run().widgets().size() == 1);
    CHECK(camp.odyssey.run().widgets()[0].label == "1. Back");
}

TEST_CASE("US-161 Choose with the mouse does the same; Esc and a Leave choice walk away") {
    Camp camp("talk-mouse");
    const int elder = elderOf(camp);
    REQUIRE(elder >= 0);
    const int hero = camp.odyssey.life()->personId();
    camp.odyssey.life()->give("berries", 1);
    REQUIRE(game::startInteraction(camp.odyssey, "talk", personSubjectOf(camp, elder)));
    camp.play(1);
    const luna::engine::Rect area = camp.odyssey.run().widgets()[1].area;

    const int before = camp.odyssey.clan()->opinion(elder, hero);
    camp.odyssey.update(clickAt(area.x + area.width / 2, area.y + area.height / 2));
    CHECK(camp.berries() == 0);
    CHECK(camp.odyssey.clan()->opinion(elder, hero) == before + 5);

    // Back, then Leave ends the talk and the world runs again.
    camp.odyssey.run().press(camp.odyssey, game::RunFlow::kTalkChoiceBase); // Back
    camp.odyssey.run().press(camp.odyssey, game::RunFlow::kTalkChoiceBase + 2); // Leave
    CHECK(camp.odyssey.run().screen() == game::Screen::None);
    CHECK(camp.odyssey.run().conversation() == nullptr);
    const auto ticks = camp.odyssey.clan()->ticks();
    camp.play(5);
    CHECK(camp.odyssey.clan()->ticks() > ticks);

    // Esc leaves too, with no effect.
    camp.odyssey.life()->give("berries", 1);
    REQUIRE(game::startInteraction(camp.odyssey, "talk", personSubjectOf(camp, elder)));
    camp.play(1);
    camp.odyssey.update(keyPressed(luna::engine::Intent::OpenMenu));
    CHECK(camp.odyssey.run().screen() == game::Screen::None);
    CHECK(camp.berries() == 1);
}

TEST_CASE("US-161 Hidden: a greyed choice cannot be picked, and without an [else] a choice is not shown at all") {
    // No berries: the shipped script greys the offer out with its reason.
    Camp camp("talk-hidden");
    const int elder = elderOf(camp);
    REQUIRE(elder >= 0);
    const int hero = camp.odyssey.life()->personId();
    REQUIRE(camp.berries() == 0);
    REQUIRE(game::startInteraction(camp.odyssey, "talk", personSubjectOf(camp, elder)));
    camp.play(1);
    REQUIRE(camp.odyssey.run().widgets().size() == 3);
    CHECK_FALSE(camp.odyssey.run().widgets()[1].enabled);
    CHECK(camp.odyssey.run().widgets()[1].label == "2. Offer berries  (You have no berries)");
    const int before = camp.odyssey.clan()->opinion(elder, hero);
    camp.odyssey.update(keyPressed(luna::engine::Intent::Slot2)); // nothing happens
    CHECK(camp.odyssey.clan()->opinion(elder, hero) == before);
    CHECK(camp.odyssey.run().widgets().size() == 3);

    // A script of the owner's, written next to the shipped one: its second choice has a condition and no [else], so it is left out.
    const fs::path data = dataCopy("talk-hidden-own");
    writeText(data / "dialogue" / "own.dlg",
              "@who person\n=== start\nTalker: Hello.\n-> Gift [if has(hero, berries, 1)] => END\n-> Wave => END\n");
    std::filesystem::remove(data / "dialogue" / "elder-fire.dlg");
    Camp own("talk-hidden-own", data);
    const int person = own.person();
    REQUIRE(person >= 0);
    REQUIRE(game::startInteraction(own.odyssey, "talk", personSubjectOf(own, person)));
    own.play(1);
    REQUIRE(own.odyssey.run().widgets().size() == 1);
    CHECK(own.odyssey.run().widgets()[0].label == "1. Wave");
}

TEST_CASE("US-161 Without a script for them, Talk is the plain talk of before") {
    const fs::path data = dataCopy("talk-plain");
    std::filesystem::remove(data / "dialogue" / "elder-fire.dlg");
    Camp camp("talk-plain", data);
    const int person = camp.person();
    REQUIRE(person >= 0);
    REQUIRE(game::startInteraction(camp.odyssey, "talk", personSubjectOf(camp, person)));
    CHECK(camp.odyssey.run().screen() == game::Screen::None);
    CHECK_FALSE(camp.odyssey.run().message().empty());
}
