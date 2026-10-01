// US-165: clan members near the hero talk, quarrel and court in speech bubbles, in turn, 3 seconds a line.
#include "camp.h"

#include "sim/smalltalk.h"

using namespace camp_support;

namespace {

namespace sim = odysseus::sim;

// Nobody greets the hero here (so no greeting bubbles get in the way), and two people near the fire who are not the hero.
struct Pair {
    int first = -1;
    int second = -1;
};

Pair twoNearby(Camp& camp, bool avoidElderAndChild = true) {
    const int hero = camp.odyssey.life()->personId();
    for (const sim::Person& person : camp.odyssey.clan()->people()) camp.odyssey.changeOpinion(person.id, hero, -100);
    Pair pair;
    const auto& figures = camp.odyssey.clanView().figures();
    const sim::World& world = *camp.odyssey.clan();
    const int daysPerYear = world.calendar().daysPerYear();
    int oldest = -1;
    for (const sim::Person& p : world.people()) {
        if (p.alive && (oldest < 0 || p.ageDays > world.people()[static_cast<std::size_t>(oldest)].ageDays)) oldest = p.id;
    }
    for (std::size_t i = 0; i < figures.size(); ++i) {
        const int person = static_cast<int>(i);
        if (!figures[i].present || person == hero) continue;
        if (avoidElderAndChild && (person == oldest || world.people()[i].ageYears(daysPerYear) < 12)) continue; // the shipped pair script is for those
        if (std::hypot(figures[i].x - camp.odyssey.hero().feetX(), figures[i].y - camp.odyssey.hero().feetY()) > 8 * game::kTileSize) continue;
        (pair.first < 0 ? pair.first : pair.second) = person;
        if (pair.second >= 0) break;
    }
    return pair;
}

bool saysOneOf(const std::string& text, const std::string& topic, const std::string& listener) {
    sim::rules::LoadReport report;
    const auto data = sim::rules::SmalltalkData::load(fs::path(ODYSSEUS_DATA_DIR) / "dialogue" / "smalltalk.json", "dialogue/smalltalk.json", report);
    REQUIRE(data.has_value());
    REQUIRE(data->topic(topic) != nullptr);
    for (const auto& entry : *data->topic(topic)) {
        std::string expected = entry.text;
        for (std::size_t at = expected.find("{hero}"); at != std::string::npos; at = expected.find("{hero}")) expected.replace(at, 6, listener);
        if (const std::size_t at = expected.find("{season}"); at != std::string::npos) { // any season word in its place
            const std::string head = expected.substr(0, at);
            const std::string tail = expected.substr(at + 8);
            if (text.size() > head.size() + tail.size() && text.rfind(head, 0) == 0 && text.compare(text.size() - tail.size(), tail.size(), tail) == 0) return true;
            continue;
        }
        if (expected == text) return true;
    }
    return false;
}

} // namespace

TEST_CASE("US-165 Bubbles: two friends who talk say a short line each, in turn, and each goes after 3 seconds") {
    Camp camp("exch-talk");
    const Pair pair = twoNearby(camp);
    REQUIRE(pair.second >= 0);
    sim::World* world = camp.odyssey.clanMutable();
    world->adjustOpinion(pair.first, pair.second, 50);
    world->adjustOpinion(pair.second, pair.first, 50);
    const std::string firstName = world->people()[static_cast<std::size_t>(pair.first)].name;
    const std::string secondName = world->people()[static_cast<std::size_t>(pair.second)].name;

    world->talk(pair.first, pair.second); // the social simulation at work
    camp.play(1);
    const game::Bubble* one = camp.odyssey.bubbles().of(pair.first);
    REQUIRE(one != nullptr);
    CHECK(camp.odyssey.bubbles().of(pair.second) == nullptr); // one at a time: the first speaks first
    CHECK_MESSAGE(saysOneOf(one->text, "social.talk", secondName), one->text);
    CHECK(one->text.size() <= 60); // a short line
    CHECK(one->ticksLeft == 3 * 20);

    // After 3 seconds the second speaks, and the first line is gone with the next one starting.
    int ticks = 1;
    while (camp.odyssey.bubbles().of(pair.second) == nullptr && ticks < 120) {
        camp.play(1);
        ++ticks;
    }
    const game::Bubble* two = camp.odyssey.bubbles().of(pair.second);
    REQUIRE(two != nullptr);
    CHECK(ticks >= 3 * 20); // not before 3 seconds
    CHECK(ticks <= 3 * 20 + 2);
    CHECK(camp.odyssey.bubbles().of(pair.first) == nullptr); // gone as the next line starts
    CHECK_MESSAGE(saysOneOf(two->text, "social.talk", firstName), two->text);

    // And it is over after 3 more seconds: no bubble left, nothing waiting.
    camp.play(3 * 20 + 2);
    CHECK(camp.odyssey.bubbles().all().empty());
    CHECK_FALSE(camp.odyssey.exchanges().playing());
}

TEST_CASE("US-165 Quarrel: a quarrel near the hero is shown as angry lines and both opinions fall") {
    Camp camp("exch-quarrel");
    const Pair pair = twoNearby(camp);
    REQUIRE(pair.second >= 0);
    sim::World* world = camp.odyssey.clanMutable();
    const int firstBefore = world->opinion(pair.first, pair.second);
    const int secondBefore = world->opinion(pair.second, pair.first);
    world->quarrel(pair.first, pair.second); // started by the social simulation: the opinions are its doing
    CHECK(world->opinion(pair.first, pair.second) < firstBefore);
    CHECK(world->opinion(pair.second, pair.first) < secondBefore);

    camp.play(1);
    REQUIRE(camp.odyssey.exchanges().playing());
    const auto lines = camp.odyssey.exchanges().current();
    REQUIRE(lines.size() == 2);
    for (const game::ExchangeLine& line : lines) {
        const std::string listener = world->people()[static_cast<std::size_t>(line.person == pair.first ? pair.second : pair.first)].name;
        CHECK_MESSAGE(saysOneOf(line.text, "social.quarrel", listener), line.text);
    }
    CHECK(lines[0].person != lines[1].person);
    CHECK(camp.odyssey.bubbles().of(lines[0].person) != nullptr);
}

TEST_CASE("US-165 A script for the two of them is used: the elder calls a child") {
    Camp camp("exch-pair");
    const Pair pair = twoNearby(camp, false);
    REQUIRE(pair.second >= 0);
    sim::World* world = camp.odyssey.clanMutable();
    const int daysPerYear = world->calendar().daysPerYear();
    // Make one of them the oldest of the clan and the other a child.
    for (const sim::Person& p : world->people()) world->personMutable(p.id)->ageDays = 30 * daysPerYear;
    world->personMutable(pair.first)->ageDays = 90 * daysPerYear;
    world->personMutable(pair.second)->ageDays = 6 * daysPerYear;
    world->talk(pair.first, pair.second);
    camp.play(1);
    REQUIRE(camp.odyssey.exchanges().playing());
    const auto lines = camp.odyssey.exchanges().current();
    REQUIRE(lines.size() == 2);
    CHECK(lines[0].person == pair.first);
    CHECK(lines[0].text == "Come here, little one.");
    CHECK(lines[1].person == pair.second);
    CHECK(lines[1].text == "Yes, elder!");
}

TEST_CASE("US-165 The hero's own talk and people far from the hero make no bubbles; the same two are not queued twice; others wait their turn") {
    Camp camp("exch-queue");
    const Pair pair = twoNearby(camp);
    REQUIRE(pair.second >= 0);
    sim::World* world = camp.odyssey.clanMutable();
    const int hero = camp.odyssey.life()->personId();
    world->talk(hero, pair.first); // the hero talking has a panel, not bubbles
    camp.play(1);
    CHECK_FALSE(camp.odyssey.exchanges().playing());
    CHECK(camp.odyssey.bubbles().all().empty());

    world->talk(pair.first, pair.second);
    world->talk(pair.second, pair.first); // the same two again: not queued twice
    camp.play(1);
    REQUIRE(camp.odyssey.exchanges().playing());
    CHECK(camp.odyssey.exchanges().waiting() == 0);

    // A third and a fourth person talking while the first exchange plays wait their turn.
    int third = -1;
    int fourth = -1;
    const auto& figures = camp.odyssey.clanView().figures();
    for (std::size_t i = 0; i < figures.size(); ++i) {
        const int person = static_cast<int>(i);
        if (!figures[i].present || person == hero || person == pair.first || person == pair.second) continue;
        if (std::hypot(figures[i].x - camp.odyssey.hero().feetX(), figures[i].y - camp.odyssey.hero().feetY()) > 8 * game::kTileSize) continue;
        (third < 0 ? third : fourth) = person;
        if (fourth >= 0) break;
    }
    if (fourth >= 0) {
        world->talk(third, fourth);
        camp.play(1);
        CHECK(camp.odyssey.exchanges().waiting() == 1);
        camp.play(2 * 3 * 20 + 4); // the first exchange is over: the next begins
        CHECK(camp.odyssey.exchanges().waiting() == 0);
        CHECK(camp.odyssey.exchanges().playing());
        CHECK(camp.odyssey.bubbles().of(third) != nullptr);
    }
}

TEST_CASE("US-165 What happened before a save is not shown again") {
    Camp camp("exch-old");
    const Pair pair = twoNearby(camp);
    REQUIRE(pair.second >= 0);
    camp.odyssey.clanMutable()->quarrel(pair.first, pair.second); // before the exchanges are told about the load below
    camp.odyssey.exchanges().reset(camp.odyssey.clan()->chronicle().entries().size());
    camp.play(2);
    CHECK_FALSE(camp.odyssey.exchanges().playing());
    CHECK(camp.odyssey.bubbles().all().empty());
}
