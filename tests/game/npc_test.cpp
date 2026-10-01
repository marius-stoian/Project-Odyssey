// US-154: clan members and animals do things on their own with the same interactions and the same runner as the hero.
#include "camp.h"

#include "game/game_rules.h"

using namespace camp_support;

namespace {

using game::kAnimalActorBase;
using game::kPersonActorBase;
using odysseus::sim::Need;
using State = game::NpcLife::State;

// The monster of this kind in the level (the first), or -1: wolves and goblins fight, so they are enemies.
int enemyOfKind(const game::OdysseyGame& odyssey, const std::string& kind) {
    for (std::size_t i = 0; i < odyssey.enemies().size(); ++i) {
        if (odyssey.enemies()[i].kindName == kind) return static_cast<int>(i);
    }
    return -1;
}

// The harmless animal of this kind (deer, rabbit): placed characters that only stand, graze and flee.
int animalOfKind(const game::OdysseyGame& odyssey, const std::string& kind) {
    for (std::size_t i = 0; i < odyssey.bystanders().size(); ++i) {
        if (odyssey.bystanders()[i].kind == kind) return static_cast<int>(i);
    }
    return -1;
}

double metresBetween(double ax, double ay, double bx, double by) { return std::hypot(ax - bx, ay - by) / 32.0; }

} // namespace

TEST_CASE("US-154 A hungry clan member walks to a ripe plant and gathers it, and the plant shows as picked") {
    Spec spec;
    spec.plants.push_back({"wheat", 3, 0}); // 3 m east of the fire
    Camp camp("npc-hungry", {}, false, {}, spec);
    REQUIRE(camp.odyssey.plants().size() == 1);
    const game::WorldPlant& plant = camp.odyssey.plants()[0];
    const int person = camp.person();
    REQUIRE(person >= 0);
    camp.odyssey.harmPerson(person, Need::Hunger, 70); // 30 left: hungry
    const int hungerBefore = camp.odyssey.clan()->people()[static_cast<std::size_t>(person)].needs[Need::Hunger];
    REQUIRE(hungerBefore <= 40);

    bool walked = false;
    bool worked = false;
    for (int tick = 0; tick < 600 && plant.state == "ripe"; ++tick) {
        camp.play(1);
        if (const auto* mind = camp.odyssey.npcs().mind(kPersonActorBase + person)) {
            walked = walked || (mind->state == State::Walking && mind->interaction == "gather");
            worked = worked || (mind->state == State::Working && mind->interaction == "gather");
        }
    }
    CHECK(walked);                    // they walked to the plant
    CHECK(worked);                    // and the ring ran for them too (the same runner)
    CHECK(plant.state == "picked");   // the bush shows as picked
    CHECK_FALSE(plant.present());
    // Walking to it took them to the plant, and the berries they ate filled their hunger a little.
    const auto& figure = camp.odyssey.clanView().figures()[static_cast<std::size_t>(person)];
    CHECK(metresBetween(figure.x, figure.y, plant.feet.x, plant.feet.y) < 2.5);
    CHECK(camp.odyssey.clan()->people()[static_cast<std::size_t>(person)].needs[Need::Hunger] > hungerBefore);

    // It ripens again on its own, 15 s later, and the person goes back to their day.
    camp.play(310);
    CHECK(plant.state == "ripe");
    const auto* mind = camp.odyssey.npcs().mind(kPersonActorBase + person);
    REQUIRE(mind != nullptr);
    CHECK(mind->state == State::Idle);
    CHECK_FALSE(camp.odyssey.clanView().hasErrand(person));
}

TEST_CASE("US-154 Someone who is not hungry leaves the bush alone") {
    Spec spec;
    spec.plants.push_back({"wheat", 3, 0});
    Camp camp("npc-fed", {}, false, {}, spec);
    camp.play(400);
    CHECK(camp.odyssey.plants()[0].state == "ripe");
    for (const auto& [id, mind] : camp.odyssey.npcs().minds()) {
        CHECK_MESSAGE(mind.interaction != "gather", id);
    }
}

TEST_CASE("US-154 A deer grazes on grass and runs from a wolf") {
    Spec spec;
    spec.plants.push_back({"grass", 6, 0});
    spec.characters.push_back({"deer", 6, 1});
    spec.characters.push_back({"wolf", 20, 0}); // far away, beyond sight
    Camp camp("npc-deer", {}, false, {}, spec);
    const int deer = animalOfKind(camp.odyssey, "deer");
    const int wolf = enemyOfKind(camp.odyssey, "wolf");
    REQUIRE(deer >= 0);
    REQUIRE(wolf >= 0);
    const int deerActor = kAnimalActorBase + camp.odyssey.bystanders()[static_cast<std::size_t>(deer)].id;

    bool grazed = false;
    for (int tick = 0; tick < 400 && !grazed; ++tick) {
        camp.play(1);
        const auto* mind = camp.odyssey.npcs().mind(deerActor);
        grazed = mind != nullptr && mind->state == State::Working && mind->interaction == "graze";
    }
    REQUIRE(grazed); // it walked to the grass and is eating
    const auto& deerNow = camp.odyssey.bystanders()[static_cast<std::size_t>(deer)];
    const auto& grass = camp.odyssey.plants()[0];
    CHECK(metresBetween(deerNow.feet.x, deerNow.feet.y, grass.feet.x, grass.feet.y) < 2.0);

    // A wolf comes up to 3 m away from the deer.
    game::Enemy& wolfNow = camp.odyssey.enemyAt(static_cast<std::size_t>(wolf));
    wolfNow.setFeet(deerNow.feet.x + 3 * 32, deerNow.feet.y);
    const double startGap = metresBetween(deerNow.feet.x, deerNow.feet.y, wolfNow.feetX(), wolfNow.feetY());
    bool fled = false;
    for (int tick = 0; tick < 120; ++tick) {
        camp.play(1);
        const auto* mind = camp.odyssey.npcs().mind(deerActor);
        fled = fled || (mind != nullptr && mind->state == State::Fleeing);
    }
    CHECK(fled);
    const double gap = metresBetween(deerNow.feet.x, deerNow.feet.y, wolfNow.feetX(), wolfNow.feetY());
    CHECK(gap > startGap + 3.0); // it ran from the wolf
}

TEST_CASE("US-154 Prey flee a hero who holds a weapon or runs at them, but not one standing still with empty hands") {
    Spec spec;
    spec.characters.push_back({"deer", 3, 0}); // 3 m from the hero
    Camp camp("npc-hero-scare", {}, false, {}, spec);
    const int deer = animalOfKind(camp.odyssey, "deer");
    REQUIRE(deer >= 0);
    const int deerActor = kAnimalActorBase + camp.odyssey.bystanders()[static_cast<std::size_t>(deer)].id;

    // Standing still, empty-handed: no reason to run.
    bool fled = false;
    for (int tick = 0; tick < 100; ++tick) {
        camp.play(1);
        const auto* mind = camp.odyssey.npcs().mind(deerActor);
        fled = fled || (mind != nullptr && mind->state == State::Fleeing);
    }
    CHECK_FALSE(fled);

    // Holding a weapon: it runs.
    REQUIRE(camp.odyssey.pickUp("iron sword"));
    camp.odyssey.selectSlot(0);
    for (int tick = 0; tick < 60 && !fled; ++tick) {
        camp.play(1);
        const auto* mind = camp.odyssey.npcs().mind(deerActor);
        fled = mind != nullptr && mind->state == State::Fleeing;
    }
    CHECK(fled);
}

TEST_CASE("US-154 Danger drops what a clan member was doing") {
    Spec spec;
    spec.plants.push_back({"wheat", 5, 0});
    spec.characters.push_back({"goblin", 30, 0}); // far away
    Camp camp("npc-danger", {}, false, {}, spec);
    const int person = camp.person();
    REQUIRE(person >= 0);
    camp.odyssey.harmPerson(person, Need::Hunger, 70);
    bool walking = false;
    for (int tick = 0; tick < 200 && !walking; ++tick) {
        camp.play(1);
        const auto* mind = camp.odyssey.npcs().mind(kPersonActorBase + person);
        walking = mind != nullptr && mind->state == State::Walking;
    }
    REQUIRE(walking);
    // A goblin appears right next to them.
    const auto& figure = camp.odyssey.clanView().figures()[static_cast<std::size_t>(person)];
    camp.odyssey.enemyAt(static_cast<std::size_t>(enemyOfKind(camp.odyssey, "goblin"))).setFeet(figure.x + 64, figure.y);
    camp.play(2);
    const auto* mind = camp.odyssey.npcs().mind(kPersonActorBase + person);
    REQUIRE(mind != nullptr);
    CHECK(mind->state == State::Idle); // they stopped walking to the bush
    CHECK(camp.odyssey.actions().running(kPersonActorBase + person) == nullptr);
    camp.play(120); // and with the goblin next to them they do not set off again
    CHECK(mind->state == State::Idle);
    CHECK(camp.odyssey.plants()[0].state == "ripe"); // and took nothing
}

TEST_CASE("US-154 The same seed and the same inputs give the same world after thousands of ticks") {
#ifdef NDEBUG
    const int ticks = 10000;
#else
    const int ticks = 3000; // Debug is slow; Release runs the full 10,000
#endif
    struct Result {
        std::uint64_t world;
        std::uint64_t runner;
        std::uint64_t npcs;
        std::vector<std::string> states;
        std::vector<std::pair<double, double>> animals;
    };
    const auto run = [&](const std::string& name) {
        Spec spec;
        spec.plants = {{"wheat", 3, 0}, {"wheat", -4, 2}, {"grass", 6, 0}, {"grass", 7, 3}};
        spec.characters = {{"deer", 6, 1}, {"rabbit", 5, -2}, {"wolf", 12, 4}};
        Camp camp(name, {}, false, {}, spec);
        for (const auto& person : camp.odyssey.clan()->people()) {
            if (person.id % 2 == 0) camp.odyssey.harmPerson(person.id, Need::Hunger, 60);
        }
        camp.play(ticks);
        Result result{camp.odyssey.clan()->hash(), camp.odyssey.actions().hash(), camp.odyssey.npcs().hash(), {}, {}};
        for (const auto& plant : camp.odyssey.plants()) result.states.push_back(plant.state);
        for (const auto& placed : camp.odyssey.bystanders()) result.animals.push_back({static_cast<double>(placed.feet.x), static_cast<double>(placed.feet.y)});
        return result;
    };
    const Result first = run("npc-hash-a");
    const Result second = run("npc-hash-b");
    CHECK(first.world == second.world);
    CHECK(first.runner == second.runner);
    CHECK(first.npcs == second.npcs);
    CHECK(first.states == second.states);
    CHECK(first.animals == second.animals);
    CHECK(first.npcs != game::NpcLife().hash()); // something happened
}
