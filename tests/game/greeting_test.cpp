// US-162: a friendly NPC the hero passes within 3 m greets them in a bubble, at most once a minute.
#include "camp.h"

using namespace camp_support;

namespace {

namespace sim = odysseus::sim;

// Who stands within 3 m of the hero and thinks well of them: the people who should greet.
std::set<int> shouldGreet(Camp& camp) {
    std::set<int> out;
    const auto& figures = camp.odyssey.clanView().figures();
    const int hero = camp.odyssey.life()->personId();
    for (std::size_t i = 0; i < figures.size(); ++i) {
        const int person = static_cast<int>(i);
        if (!figures[i].present || person == hero) continue;
        if (std::hypot(figures[i].x - camp.odyssey.hero().feetX(), figures[i].y - camp.odyssey.hero().feetY()) > 3 * game::kTileSize) continue;
        if (camp.odyssey.clan()->opinion(person, hero) < 0) continue;
        out.insert(person);
    }
    return out;
}

std::set<int> greeting(Camp& camp) {
    std::set<int> out;
    for (const game::Bubble& bubble : camp.odyssey.bubbles().all()) out.insert(bubble.person);
    return out;
}

const std::set<std::string>& shippedGreetings(const std::string& hero) {
    static std::set<std::string> words;
    words = {"Good day, " + hero + ".", "Well met, " + hero + ".", "The fire keeps you well, " + hero + "."};
    return words;
}

} // namespace

TEST_CASE("US-162 Greeting: friendly people within 3 m say a short greeting in a bubble, two at a time, the nearest first") {
    Camp camp("greet-friendly");
    const int hero = camp.odyssey.life()->personId();
    for (const sim::Person& person : camp.odyssey.clan()->people()) camp.odyssey.changeOpinion(person.id, hero, 50); // everyone is a friend
    camp.play(1);
    const std::set<int> expected = shouldGreet(camp);
    REQUIRE(expected.size() > static_cast<std::size_t>(game::kGreetingBubblesAtOnce)); // the clan stands around the fire where the hero starts
    const std::set<int> now = greeting(camp);
    CHECK(now.size() == static_cast<std::size_t>(game::kGreetingBubblesAtOnce));
    for (const game::Bubble& bubble : camp.odyssey.bubbles().all()) {
        CHECK(expected.count(bubble.person) == 1);
        CHECK_MESSAGE(shippedGreetings(camp.odyssey.life()->name()).count(bubble.text) == 1, bubble.text);
        CHECK(bubble.ticksLeft <= 3 * 20);
        CHECK(bubble.ticksLeft > 0);
    }
    // The two who speak are the nearest: nobody who waits is closer to the hero.
    const auto& figures = camp.odyssey.clanView().figures();
    const auto distance = [&](int person) { return std::hypot(figures[static_cast<std::size_t>(person)].x - camp.odyssey.hero().feetX(), figures[static_cast<std::size_t>(person)].y - camp.odyssey.hero().feetY()); };
    double farthestSpeaker = 0.0;
    for (const int person : now) farthestSpeaker = std::max(farthestSpeaker, distance(person));
    for (const int person : expected) {
        if (now.count(person) == 0) CHECK(distance(person) >= farthestSpeaker);
    }
    // When the bubbles have gone, the next ones speak.
    camp.play(3 * 20);
    CHECK_FALSE(greeting(camp).empty());
    CHECK(greeting(camp) != now);
}

TEST_CASE("US-162 Greeting: at most once a minute per person") {
    Camp camp("greet-minute");
    const int hero = camp.odyssey.life()->personId();
    for (const sim::Person& person : camp.odyssey.clan()->people()) camp.odyssey.changeOpinion(person.id, hero, 50);
    // The hero stays at the fire. Count how many times each person starts a greeting in the first 55 seconds: never twice.
    std::map<int, int> started;
    std::set<int> before;
    for (int tick = 0; tick < 55 * 20; ++tick) {
        camp.play(1);
        const std::set<int> now = greeting(camp);
        for (const int person : now) {
            if (before.count(person) == 0) ++started[person];
        }
        before = now;
    }
    CHECK_FALSE(started.empty());
    for (const auto& [person, times] : started) CHECK_MESSAGE(times == 1, "person ", person, " greeted ", times, " times");
    // Past the minute, someone greets again.
    bool again = false;
    for (int tick = 0; tick < 30 * 20 && !again; ++tick) {
        camp.play(1);
        for (const int person : greeting(camp)) {
            if (started.count(person) != 0 && before.count(person) == 0) again = true;
        }
        before = greeting(camp);
    }
    CHECK(again);
}
TEST_CASE("US-162 Greeting: those who dislike the hero keep quiet") {
    Camp camp("greet-unfriendly");
    const int hero = camp.odyssey.life()->personId();
    for (const sim::Person& person : camp.odyssey.clan()->people()) camp.odyssey.changeOpinion(person.id, hero, -80);
    camp.play(5);
    CHECK(greeting(camp).empty());
}

TEST_CASE("US-162 Greeting: the mood function and the opinion condition read the real world") {
    Camp camp("greet-mood");
    const int hero = camp.odyssey.life()->personId();
    const int person = camp.person();
    REQUIRE(person >= 0);
    const game::Subject subject = *game::subjectFor(camp.odyssey, {static_cast<int>(game::Subject::Kind::Person), person});
    const game::GameRuleContext context(camp.odyssey, subject);
    const auto moodIs = [&](const char* word) {
        const auto parsed = odysseus::sim::rules::parseExpression(std::string("mood(npc) == ") + word);
        REQUIRE(parsed.root != nullptr);
        return odysseus::sim::rules::isTrue(*parsed.root, context);
    };
    // Full needs: the mood is the feeling about the hero.
    camp.odyssey.changeOpinion(person, hero, 200);
    camp.odyssey.helpPerson(person, sim::Need::Hunger, 100);
    camp.odyssey.helpPerson(person, sim::Need::Energy, 100);
    camp.odyssey.helpPerson(person, sim::Need::Warmth, 100);
    camp.odyssey.helpPerson(person, sim::Need::Social, 100);
    CHECK(moodIs("warm"));
    camp.odyssey.changeOpinion(person, hero, -300);
    CHECK(moodIs("hostile"));
    const auto lower = odysseus::sim::rules::parseExpression("opinion(npc, hero) < -30");
    REQUIRE(lower.root != nullptr);
    CHECK(odysseus::sim::rules::isTrue(*lower.root, context));
}
