// US-113 Court and compete for a partner (M2b story engine).
#include "sim/chronicle.h"
#include "sim/data.h"
#include "sim/save.h"
#include "sim/world.h"

#include "story_helpers.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

using odysseus::sim::EventKind;
using odysseus::sim::World;
namespace sim = odysseus::sim;

using namespace story_test;

namespace {

// A world where love is the only thing that stirs: no random quarrels, sickness or witnessed
// thefts, and a loved one who agrees always decides at once.
sim::SimConfig loveConfig() {
    sim::SimConfig config = realConfig();
    config.story.quarrel.calmPercent = 0;
    config.story.quarrel.irritablePercent = 0;
    config.story.sickness.basePerMille = 0;
    config.story.sickness.hungerPerMille = 0;
    config.story.sickness.coldPerMille = 0;
    config.story.sickness.huntWoundPerMille = 0;
    config.social.witnessPercent = 0;
    config.social.talkOpinion = 0; // small talk does not sway hearts: only courtship does
    config.life.pairPercent = 100;
    return config;
}

// Unpaired living adults of one sex who are not Kind, in id order: a Kind person hands out gifts on
// their own, which would blur the tests of courtship.
std::vector<int> unpaired(const World& world, sim::Sex sex) {
    std::vector<int> found;
    for (const auto& person : world.people()) {
        if (person.alive && person.sex == sex && person.partner < 0 && !person.has(sim::Trait::Kind) &&
            person.ageYears(world.calendar().daysPerYear()) >= 16) {
            found.push_back(person.id);
        }
    }
    return found;
}

// `receiver` comes to like `giver` better: each gift is worth giftOpinion points to the receiver.
void gifts(World& world, int giver, int receiver, int count) {
    for (int i = 0; i < count; ++i) {
        world.giveGift(giver, receiver);
    }
}

const sim::ChronicleEntry* entryBy(const World& world, EventKind kind, int who, int other) {
    for (const auto* entry : entriesOf(world, kind)) {
        if (entry->who == who && entry->other == other) {
            return entry;
        }
    }
    return nullptr;
}

bool hasCause(const World& world, const sim::ChronicleEntry& entry, EventKind kind) {
    return std::any_of(entry.causes.begin(), entry.causes.end(), [&](int id) { return world.chronicle().find(id)->kind == kind; });
}

// Days until `done` is true, at most `limit`.
template <typename Done>
int runUntil(World& world, int limit, Done done) {
    int days = 0;
    while (!done() && days < limit) {
        runDays(world, 1);
        ++days;
    }
    return days;
}

} // namespace

TEST_CASE("US-113 Courtship") {
    const sim::SimConfig config = loveConfig();
    World world(42, config);
    const auto couple = std::pair{unpaired(world, sim::Sex::Female).front(), unpaired(world, sim::Sex::Male).front()};
    const auto [woman, man] = couple;
    REQUIRE(woman >= 0);
    REQUIRE(man >= 0);
    // He likes her a great deal (60); she likes him a little (30), not yet enough to pair (50).
    gifts(world, woman, man, 4);
    gifts(world, man, woman, 2);
    const int before = world.opinion(woman, man);
    REQUIRE(before < config.life.pairOpinion);
    runDays(world, 1);
    const sim::Person& suitor = world.people()[static_cast<std::size_t>(man)];
    const sim::Person& loved = world.people()[static_cast<std::size_t>(woman)];
    CHECK(suitor.courting == woman);
    const sim::ChronicleEntry* began = entryBy(world, EventKind::Courtship, man, woman);
    REQUIRE(began != nullptr);
    MESSAGE(sim::formatEntry(*began));
    CHECK(began->text == suitor.name + " began courting " + loved.name + ".");
    // Gifts and time together: the other's opinion rises, day by day.
    CHECK(world.opinion(woman, man) >= before + config.story.courtship.opinionPerDay);
    CHECK(loved.partner < 0); // she does not agree yet
    CHECK(suitor.partner < 0);

    // They pair only when both agree: once she thinks enough of him too.
    runUntil(world, 40, [&] { return loved.partner >= 0; });
    REQUIRE(loved.partner == man);
    CHECK(world.opinion(woman, man) >= config.life.pairOpinion);
    CHECK(world.opinion(man, woman) >= config.life.pairOpinion);
    const sim::ChronicleEntry* pairing = entryBy(world, EventKind::Pairing, man, woman);
    if (pairing == nullptr) {
        pairing = entryBy(world, EventKind::Pairing, woman, man);
    }
    REQUIRE(pairing != nullptr);
    MESSAGE(sim::formatEntry(*pairing));
    CHECK(hasCause(world, *pairing, EventKind::Courtship));
    CHECK(pairing->text.find("courtship.") != std::string::npos);
    // The courtship is over: both are free of it.
    CHECK(suitor.courting == -1);
    CHECK(loved.courting == -1);
}

TEST_CASE("US-113 Unrequited love") {
    // He loves her; she saw him steal and thinks ill of him: she turns him down.
    const sim::SimConfig config = loveConfig();
    World world(42, config);
    const auto couple = std::pair{unpaired(world, sim::Sex::Female).front(), unpaired(world, sim::Sex::Male).front()};
    const auto [woman, man] = couple;
    REQUIRE(woman >= 0);
    REQUIRE(man >= 0);
    gifts(world, woman, man, 4);
    world.recordTheft(man, woman);
    REQUIRE(world.opinion(woman, man) < config.story.courtship.rejectBelow);
    const int loveBefore = world.opinion(man, woman);
    runDays(world, 1);
    const sim::Person& suitor = world.people()[static_cast<std::size_t>(man)];
    const sim::ChronicleEntry* turnedDown = entryBy(world, EventKind::Rejection, man, woman);
    REQUIRE(turnedDown != nullptr);
    MESSAGE(sim::formatEntry(*turnedDown));
    CHECK(turnedDown->text == world.people()[static_cast<std::size_t>(woman)].name + " turned down " + suitor.name + "'s courtship.");
    CHECK(hasCause(world, *turnedDown, EventKind::Courtship));
    CHECK(world.people()[static_cast<std::size_t>(woman)].partner != man);
    CHECK(suitor.courting == -1);
    CHECK(world.opinion(man, woman) <= loveBefore - config.story.courtship.rejectionOpinionLoss + 1); // it hurts (+1: the day's courting)
    const sim::Memory* memory = memoryOf(suitor, sim::MemoryKind::Rejection);
    REQUIRE(memory != nullptr);
    CHECK(memory->subject == woman);
    CHECK(memory->event == turnedDown->id);
    // He does not court her again while he mends his heart.
    runDays(world, config.story.courtship.pauseDays / 2);
    CHECK(suitor.courting != woman);
    CHECK(world.people()[static_cast<std::size_t>(woman)].partner != man);
}

TEST_CASE("US-113 Rivals") {
    // One woman, two suitors. She likes the first better (45 against 30); both love her (60).
    // Courtship warms her by 2 a day, so the first is the first to win her, after 3 days.
    auto play = [](int quarrelPercent, int& woman, int& winner, int& rival) {
        sim::SimConfig config = loveConfig();
        config.story.rivals.quarrelPercent = quarrelPercent;
        World world(42, config);
        const auto women = unpaired(world, sim::Sex::Female);
        const auto men = unpaired(world, sim::Sex::Male);
        REQUIRE(women.size() >= 1);
        REQUIRE(men.size() >= 2);
        woman = women.front();
        winner = men[0];
        rival = men[1];
        gifts(world, woman, winner, 4);
        gifts(world, woman, rival, 4);
        gifts(world, winner, woman, 3);
        gifts(world, rival, woman, 2);
        runUntil(world, 20, [&] { return world.people()[static_cast<std::size_t>(woman)].partner >= 0; });
        return world;
    };
    int woman = 0;
    int winner = 0;
    int rival = 0;
    World world = play(100, woman, winner, rival);
    REQUIRE(world.people()[static_cast<std::size_t>(woman)].partner == winner); // the better-liked suitor wins
    const sim::ChronicleEntry* pairing = entryBy(world, EventKind::Pairing, woman, winner);
    if (pairing == nullptr) {
        pairing = entryBy(world, EventKind::Pairing, winner, woman);
    }
    REQUIRE(pairing != nullptr);

    // The rival grows jealous, and says so.
    const sim::ChronicleEntry* jealousy = entryBy(world, EventKind::Jealousy, rival, winner);
    REQUIRE(jealousy != nullptr);
    MESSAGE(sim::formatEntry(*jealousy));
    CHECK(jealousy->aux == woman);
    CHECK(jealousy->text == world.people()[static_cast<std::size_t>(rival)].name + " grew jealous of " +
                                world.people()[static_cast<std::size_t>(winner)].name + ", who won " +
                                world.people()[static_cast<std::size_t>(woman)].name + ".");
    CHECK(std::find(jealousy->causes.begin(), jealousy->causes.end(), pairing->id) != jealousy->causes.end());
    CHECK(hasCause(world, *jealousy, EventKind::Courtship)); // their own courtship is part of the reason
    CHECK(world.opinion(rival, winner) <= -world.config().story.rivals.opinionLoss);
    // The one not chosen remembers it for life.
    const sim::Person& passedOver = world.people()[static_cast<std::size_t>(rival)];
    const sim::Memory* memory = memoryOf(passedOver, sim::MemoryKind::Rejection);
    REQUIRE(memory != nullptr);
    CHECK(memory->subject == woman);
    CHECK(memory->major);
    CHECK(memory->event == jealousy->id);
    CHECK(passedOver.courting == -1);

    // And with a quarrel chance of 100% they quarrel over her; the quarrel names the reason.
    const sim::ChronicleEntry* quarrel = entryBy(world, EventKind::Quarrel, rival, winner);
    REQUIRE(quarrel != nullptr);
    MESSAGE(sim::formatEntry(*quarrel));
    CHECK(quarrel->text.find("over their love for " + world.people()[static_cast<std::size_t>(woman)].name) != std::string::npos);
    CHECK(std::find(quarrel->causes.begin(), quarrel->causes.end(), jealousy->id) != quarrel->causes.end());

    // With a chance of 0% the jealousy stays silent: no quarrel comes of it.
    int woman2 = 0;
    int winner2 = 0;
    int rival2 = 0;
    World calm = play(0, woman2, winner2, rival2);
    const sim::ChronicleEntry* jealousy2 = entryBy(calm, EventKind::Jealousy, rival2, winner2);
    REQUIRE(jealousy2 != nullptr);
    for (const auto* q : entriesOf(calm, EventKind::Quarrel)) {
        CHECK(std::find(q->causes.begin(), q->causes.end(), jealousy2->id) == q->causes.end());
    }
}

TEST_CASE("US-113 Parting") {
    // Partners quarrel three times: each thinks 36 points worse of the other, below the threshold.
    World world(42, loveConfig());
    world.pair(5, 6);
    for (int i = 0; i < 3; ++i) {
        world.quarrel(5, 6);
    }
    REQUIRE(world.opinion(5, 6) < world.config().story.parting.partingOpinion);
    REQUIRE(world.people()[5].partner == 6);
    runDays(world, 1);
    const sim::ChronicleEntry* parting = entryBy(world, EventKind::Parting, 5, 6);
    REQUIRE(parting != nullptr);
    MESSAGE(sim::formatEntry(*parting));
    CHECK(parting->text == world.people()[5].name + " and " + world.people()[6].name + " parted after a bitter quarrel.");
    CHECK(hasCause(world, *parting, EventKind::Quarrel)); // the chronicle records why
    CHECK(world.people()[5].partner == -1);
    CHECK(world.people()[6].partner == -1);
}

TEST_CASE("US-113 The one who falls out of love leaves") {
    // 6 sees 5 steal: 6 thinks ill of 5 (5 still likes 6). One is enough.
    World world(42, loveConfig());
    world.pair(5, 6);
    world.recordTheft(5, 6);
    REQUIRE(world.opinion(6, 5) < world.config().story.parting.partingOpinion);
    REQUIRE(world.opinion(5, 6) >= world.config().story.parting.partingOpinion);
    runDays(world, 1);
    const sim::ChronicleEntry* parting = entryBy(world, EventKind::Parting, 5, 6);
    REQUIRE(parting != nullptr);
    MESSAGE(sim::formatEntry(*parting));
    CHECK(parting->text == world.people()[5].name + " and " + world.people()[6].name + " parted over stolen meat.");
    CHECK(hasCause(world, *parting, EventKind::Theft));
    CHECK(world.people()[5].partner == -1);
    CHECK(world.people()[6].partner == -1);
}

TEST_CASE("US-113 Partings without a grudge say the love faded") {
    sim::SimConfig config = loveConfig();
    config.story.parting.partingOpinion = 100; // everyone falls short of it: only the words are tested
    World world(42, config);
    world.pair(5, 6);
    runDays(world, 1);
    const sim::ChronicleEntry* parting = entryBy(world, EventKind::Parting, 5, 6);
    REQUIRE(parting != nullptr);
    CHECK(parting->text == world.people()[5].name + " and " + world.people()[6].name + " parted as their love faded.");
    CHECK(parting->causes.empty());
}

TEST_CASE("US-113 Steady partners stay") {
    World world(42, loveConfig());
    world.pair(5, 6);
    runDays(world, 30);
    CHECK(world.people()[5].partner == 6);
    CHECK(world.people()[6].partner == 5);
    CHECK(entriesOf(world, EventKind::Parting).empty());
}

TEST_CASE("US-113 Courtship keeps the clan's rules") {
    World world(7, realConfig());
    std::size_t courtships = 0;
    for (int day = 1; day <= 20 * 28; ++day) {
        runDays(world, 1);
        for (const auto& person : world.people()) {
            if (person.alive && person.partner >= 0) {
                const auto& partner = world.people()[static_cast<std::size_t>(person.partner)];
                REQUIRE(partner.alive);
                REQUIRE(partner.partner == person.id);
                REQUIRE(person.courting == -1); // nobody courts once paired
            }
            if (person.courting < 0) {
                continue;
            }
            ++courtships;
            const auto& loved = world.people()[static_cast<std::size_t>(person.courting)];
            REQUIRE(person.alive);
            REQUIRE(person.courting != person.id);
            REQUIRE(loved.alive);
            REQUIRE(loved.partner < 0);
            REQUIRE(person.sex != loved.sex);
            REQUIRE((person.mother < 0 || person.mother != loved.mother)); // not siblings
            REQUIRE(person.mother != loved.id);
            REQUIRE(person.father != loved.id);
            REQUIRE(loved.mother != person.id);
            REQUIRE(loved.father != person.id);
        }
    }
    MESSAGE(courtships, " courting days seen in 20 years; ", entriesOf(world, EventKind::Courtship).size(), " courtships begun, ",
            entriesOf(world, EventKind::Rejection).size(), " rejections, ", entriesOf(world, EventKind::Jealousy).size(),
            " jealousies, ", entriesOf(world, EventKind::Parting).size(), " partings");
    CHECK(courtships > 0);
}

TEST_CASE("US-113 Saved courtships") {
    // Save in the middle of a courtship: it must come back exactly, and go on exactly the same.
    const std::filesystem::path folder = std::filesystem::temp_directory_path() / "odysseus-us113";
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    const std::filesystem::path file = folder / "clan.json";
    World world(42, loveConfig());
    const auto couple = std::pair{unpaired(world, sim::Sex::Female).front(), unpaired(world, sim::Sex::Male).front()};
    const auto [woman, man] = couple;
    gifts(world, woman, man, 4);
    gifts(world, man, woman, 1);
    runDays(world, 2);
    REQUIRE(world.people()[static_cast<std::size_t>(man)].courting == woman);
    sim::saveWorld(world, file);
    sim::LoadedWorld loaded = sim::loadWorld(file, loveConfig());
    CHECK(loaded.world.hash() == world.hash());
    CHECK(loaded.world.people()[static_cast<std::size_t>(man)].courting == woman);
    CHECK(loaded.world.people()[static_cast<std::size_t>(man)].courtDays == world.people()[static_cast<std::size_t>(man)].courtDays);
    runDays(world, 28);
    runDays(loaded.world, 28);
    CHECK(loaded.world.hash() == world.hash());
}
