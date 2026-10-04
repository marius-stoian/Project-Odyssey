// US-292 NPCs act on each other: the same interaction files as the hero's, with a person as the actor and another as the target: talk, trade, gift, confront, fight; opinions change
// as for the hero; deaths are final and have consequences; far persons deal once a day abstractly; the work per hour is bounded.
#include "sim/data.h"
#include "sim/npc_director.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <tuple>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;
namespace sim = odysseus::sim;

namespace {

sim::CalendarConfig calendar() { return sim::loadCalendarConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "calendar.json"); }
sim::NeedsConfig needs() { return sim::loadNeedsConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "needs.json"); }

const rules::InteractionRegistry& shippedInteractions() {
    static const rules::InteractionRegistry registry = [] {
        rules::LoadReport report;
        rules::InteractionRegistry loaded = rules::InteractionRegistry::load(fs::path(ODYSSEUS_DATA_DIR) / "interactions", report);
        for (const auto& error : report.errors) FAIL(error.text());
        return loaded;
    }();
    return registry;
}

rules::ScheduleConfig exactConfig() {
    rules::ScheduleConfig config;
    config.scatterPixels = 0;
    return config;
}

sim::NpcProfile person() {
    sim::NpcProfile profile;
    profile.tags = {"npc"};
    profile.classes = {"talker"};
    return profile;
}

rules::TradeProfile traderOf(sim::ItemCounts stock, std::vector<std::string> wants) {
    rules::TradeProfile profile;
    profile.stock = std::move(stock);
    profile.wants = std::move(wants);
    return profile;
}

struct Meeting {
    sim::NpcPopulation population{calendar(), needs()};
    sim::TradeMarket market;
    sim::NpcDirector director;

    explicit Meeting(rules::ScheduleConfig config = exactConfig()) : director(std::move(config)) {
        director.setInteractions(&shippedInteractions());
        director.setMarket(&market);
        director.setSeed(3);
        population.setFocus(300, 200);
        population.setOpinionConfig(sim::OpinionConfig{});
    }
    int add(int id, int x, int y, int family = 0) {
        const int index = population.add(id, "wanderer", 20 * 28, family, x, y);
        director.setHome(index, x, y);
        director.setProfile(index, person());
        return index;
    }
    void runTo(int day, int hour) {
        const std::uint64_t target = static_cast<std::uint64_t>(day) * static_cast<std::uint64_t>(population.ticksPerDay()) + static_cast<std::uint64_t>(hour) * static_cast<std::uint64_t>(population.ticksPerHour());
        while (population.ticks() < target) {
            population.tick();
            director.tick(population);
        }
    }
};

int count(const std::vector<sim::NpcEvent>& events, sim::NpcEvent::Kind kind) {
    return static_cast<int>(std::count_if(events.begin(), events.end(), [kind](const sim::NpcEvent& event) { return event.kind == kind; }));
}

} // namespace

TEST_CASE("US-292 Trade: two traders near each other where each wants what the other has trade when they are idle, and both stocks change") {
    Meeting meeting;
    const int a = meeting.add(1, 100, 100);
    const int b = meeting.add(2, 130, 100);
    REQUIRE(meeting.market.addTrader(1, traderOf({{"flint", 3}}, {"fur"}), 0));
    REQUIRE(meeting.market.addTrader(2, traderOf({{"fur", 2}}, {"flint"}), 0));
    (void)a;
    (void)b;
    meeting.runTo(0, 1);
    CHECK(meeting.market.stock(1, "flint") < 3);
    CHECK(meeting.market.stock(1, "fur") > 0);
    CHECK(meeting.market.stock(2, "fur") < 2);
    CHECK(meeting.market.stock(2, "flint") > 0);
    // Nothing is made or lost: three flint and two furs between them, as before.
    CHECK(meeting.market.stock(1, "flint") + meeting.market.stock(2, "flint") == 3);
    CHECK(meeting.market.stock(1, "fur") + meeting.market.stock(2, "fur") == 2);
    const auto events = meeting.director.takeEvents();
    CHECK(count(events, sim::NpcEvent::Kind::Trade) >= 1);
    // They think better of each other for it (the trade event of opinions.json).
    CHECK(meeting.population.opinion(1, 2) > 0);
    CHECK(meeting.population.opinion(2, 1) > 0);
    CHECK(meeting.director.lastAction(a) == "npc-swap");
}

TEST_CASE("US-292 Talk: persons who meet talk: how it went is remembered in their opinions, the words are an event to show, and they feel less lonely") {
    Meeting meeting;
    const int a = meeting.add(1, 100, 100);
    const int b = meeting.add(2, 130, 100);
    meeting.population.setNeed(a, sim::Need::Social, 40);
    meeting.runTo(0, 1);
    CHECK(meeting.population.met(1, 2));
    CHECK(meeting.population.met(2, 1));
    const auto events = meeting.director.takeEvents();
    REQUIRE(count(events, sim::NpcEvent::Kind::Talk) >= 1);
    for (const sim::NpcEvent& event : events) {
        if (event.kind == sim::NpcEvent::Kind::Talk) CHECK_FALSE(event.text.empty());
    }
    CHECK(meeting.director.lastAction(a) == "npc-chat");
    CHECK(meeting.population.need(a, sim::Need::Social) >= 40 + 10 - 1 - 1); // a conversation gives Social back (the hour of falling took a little)
    (void)b;
}

TEST_CASE("US-292 Fight: two NPCs who are hostile to each other fight when they meet; the weaker dies, the witnesses think less of the attacker and the family never forgives") {
    Meeting meeting;
    const int a = meeting.add(1, 100, 100);
    const int b = meeting.add(2, 130, 100, 7);
    meeting.add(3, 300, 100);       // a witness who knows b
    meeting.add(4, 300, 150, 7);    // b's sister
    meeting.population.adjust(1, 2, -80); // a is hostile to b; b has nothing against a, and chats first (the hour decides who is first)
    meeting.population.adjust(3, 2, 10); // the witness and b have met
    meeting.director.setCombat(a, 30, 10);
    meeting.director.setCombat(b, 20, 4);
    REQUIRE(meeting.population.attitude(1, 2) == sim::Attitude::Hostile);
    meeting.runTo(0, 1);
    CHECK(meeting.director.mode(b) == sim::NpcDirector::Mode::Dead);
    CHECK(meeting.director.mode(a) != sim::NpcDirector::Mode::Dead);
    CHECK(meeting.director.hp(a) < 30); // b struck back
    CHECK(meeting.director.hp(b) == 0);
    const auto events = meeting.director.takeEvents();
    CHECK(count(events, sim::NpcEvent::Kind::Fight) >= 1);
    REQUIRE(count(events, sim::NpcEvent::Kind::Death) == 1);
    for (const sim::NpcEvent& event : events) {
        if (event.kind == sim::NpcEvent::Kind::Death) {
            CHECK(event.actor == 2); // who died
            CHECK(event.target == 1); // and who killed them
        }
    }
    // The witness knew the victim: they think less of the killer. The family think much less.
    CHECK(meeting.population.opinion(3, 1) < 0);
    CHECK(meeting.population.opinion(4, 1) <= -30);
    // The dead do nothing more.
    meeting.runTo(0, 4);
    CHECK(meeting.director.mode(b) == sim::NpcDirector::Mode::Dead);
    CHECK(count(meeting.director.takeEvents(), sim::NpcEvent::Kind::Death) == 0);
}

TEST_CASE("US-292 Fight: when neither falls they break off with less liking for each other, and the fight goes on the next hour while they are hostile") {
    Meeting meeting;
    const int a = meeting.add(1, 100, 100);
    const int b = meeting.add(2, 130, 100);
    meeting.population.adjust(1, 2, -80);
    meeting.population.adjust(2, 1, -80);
    meeting.director.setCombat(a, 1000, 1); // both hardly hurt in six rounds
    meeting.director.setCombat(b, 1000, 1);
    meeting.runTo(0, 1);
    CHECK(meeting.director.mode(a) == sim::NpcDirector::Mode::Scheduled);
    CHECK(meeting.director.mode(b) == sim::NpcDirector::Mode::Scheduled);
    CHECK(meeting.director.hp(a) < 1000);
    CHECK(meeting.director.hp(b) < 1000);
    CHECK(meeting.population.opinion(1, 2) < -80 + 1); // not forgiven
    const int hp = meeting.director.hp(a);
    meeting.runTo(0, 3);
    CHECK(meeting.director.hp(a) < hp); // more rounds in the next hours (the cooldown of a fight is an hour)
}

TEST_CASE("US-292 Confront: a person who dislikes another confronts them, and those who can hear and know the target think less of the actor") {
    Meeting meeting;
    meeting.add(1, 100, 100);
    meeting.add(2, 130, 100);
    meeting.add(3, 300, 100);
    meeting.population.adjust(1, 2, -40); // dislike, but not yet hostile
    meeting.population.adjust(3, 2, 20);  // the witness likes the target
    const int before = meeting.population.opinion(2, 1);
    meeting.runTo(0, 1);
    CHECK(meeting.population.opinion(2, 1) < before); // the target now thinks less of the actor
    CHECK(meeting.population.opinion(3, 1) < 0);        // so does the one who heard
    CHECK(meeting.population.noteCount(1) >= 1);        // the target remembers (index 1 is person 2)
    CHECK(meeting.director.lastAction(0) == "npc-confront");
}

TEST_CASE("US-292 Gift: a person with goods who is fond of another gives them a gift, and is thought the better of for it") {
    Meeting meeting;
    meeting.add(1, 100, 100);
    meeting.add(2, 130, 100);
    REQUIRE(meeting.market.addTrader(1, traderOf({{"fur", 2}}, {}), 0));
    REQUIRE(meeting.market.addTrader(2, traderOf({}, {}), 0));
    meeting.population.adjust(1, 2, 60);
    const int before = meeting.population.opinion(2, 1);
    meeting.runTo(0, 1);
    CHECK(meeting.market.stock(1, "fur") == 1);
    CHECK(meeting.market.stock(2, "fur") == 1);
    CHECK(meeting.population.opinion(2, 1) > before);
    CHECK(count(meeting.director.takeEvents(), sim::NpcEvent::Kind::Gift) == 1);
}

TEST_CASE("US-292 Far: persons far from the hero have a dealing once a day, abstractly, with the same effects and nothing to show") {
    rules::ScheduleConfig config = exactConfig();
    config.farPercent = 100;
    Meeting meeting(config);
    meeting.population.setFocus(-9000, -9000);
    meeting.add(1, 6000, 6000);
    meeting.add(2, 6030, 6000);
    meeting.runTo(1, 0); // a whole day: each is visited once, at the tick of the day that is its index
    CHECK(meeting.population.met(1, 2)); // they talked, though nobody saw it
    CHECK(count(meeting.director.takeEvents(), sim::NpcEvent::Kind::Talk) == 0);

    // A far fight is settled at once, and a death is reported whoever is near.
    Meeting war(config);
    war.population.setFocus(-9000, -9000);
    const int a = war.add(1, 6000, 6000);
    const int b = war.add(2, 6030, 6000);
    war.population.adjust(1, 2, -80);
    war.population.adjust(2, 1, -80);
    war.director.setCombat(a, 100, 50);
    war.director.setCombat(b, 10, 1);
    war.runTo(1, 0);
    CHECK(war.director.mode(b) == sim::NpcDirector::Mode::Dead);
    CHECK(count(war.director.takeEvents(), sim::NpcEvent::Kind::Death) == 1);
}

TEST_CASE("US-292 Budget: at most maxPerHour dealings are begun on one hour mark") {
    rules::ScheduleConfig config = exactConfig();
    config.maxPerHour = 1;
    Meeting meeting(config);
    for (int pair = 0; pair < 3; ++pair) {
        meeting.add(10 * pair + 1, 100, 100 + 30 * pair);
        meeting.add(10 * pair + 2, 130, 100 + 30 * pair);
    }
    meeting.runTo(0, 1);
    const auto events = meeting.director.takeEvents();
    CHECK(count(events, sim::NpcEvent::Kind::Talk) == 1); // six persons, six who might talk, one budget
}

TEST_CASE("US-292 Same: two runs give the same persons, opinions, stocks and director") {
    const auto run = [] {
        Meeting meeting;
        for (int i = 0; i < 12; ++i) meeting.add(i + 1, 100 + 20 * (i % 4), 100 + 30 * (i / 4));
        REQUIRE(meeting.market.addTrader(1, traderOf({{"flint", 3}}, {"fur"}), 0));
        REQUIRE(meeting.market.addTrader(2, traderOf({{"fur", 2}}, {"flint"}), 0));
        meeting.population.adjust(5, 6, -80);
        meeting.population.adjust(6, 5, -80);
        meeting.runTo(1, 6);
        return std::make_tuple(meeting.population.hash(), meeting.director.hash(), meeting.market.hash());
    };
    CHECK(run() == run());
}

TEST_CASE("US-292 Save: the hit points and the dead are saved and read back") {
    Meeting meeting;
    const int a = meeting.add(1, 100, 100);
    const int b = meeting.add(2, 130, 100);
    meeting.population.adjust(1, 2, -80);
    meeting.population.adjust(2, 1, -80);
    meeting.director.setCombat(a, 30, 10);
    meeting.director.setCombat(b, 20, 4);
    meeting.runTo(0, 1);
    const std::string text = meeting.director.toText();
    const sim::NpcDirector loaded = sim::NpcDirector::fromText(text, exactConfig());
    CHECK(loaded.hash() == meeting.director.hash());
    CHECK(loaded.mode(b) == sim::NpcDirector::Mode::Dead);
    CHECK(loaded.hp(a) == meeting.director.hp(a));
    CHECK(loaded.toText() == text);
}

TEST_CASE("US-292 Files: the dealing files load, name the actor word npc and need a person as their target") {
    for (const char* id : {"npc-chat", "npc-swap", "npc-gift", "npc-confront", "npc-fight"}) {
        const rules::Interaction* interaction = shippedInteractions().find(id);
        REQUIRE_MESSAGE(interaction != nullptr, id);
        CHECK(interaction->actors == std::vector<std::string>{"npc"});
        CHECK(std::find(interaction->targetTags.begin(), interaction->targetTags.end(), "npc") != interaction->targetTags.end());
        CHECK(interaction->npc.has_value());
    }
}
