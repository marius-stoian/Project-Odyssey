// US-264 Attitudes in the game: the attitude of a kind file or a placed NPC decides who fights the hero, persons start with it as their opinion of the hero,
// the family starts friendly, and the menu title shows the word.
#include "camp.h"

#include "game/game_rules.h"
#include "game/npc_class_book.h"
#include "sim/npc_population.h"

#include <functional>
#include <memory>

using namespace camp_support;
namespace sim = odysseus::sim;

namespace {

struct Studio {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;

    explicit Studio(const std::string& name, const std::function<void(game::Level&)>& changeLevel) : data(dataCopy(name)) {
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        changeLevel(level);
        game::saveLevel(level, definitions, data / "attitude-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "attitude-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
    }
};

game::PlacedCharacter placedAt(game::Level& level, const std::string& kind, const std::string& name, int east) {
    return {level.nextId++, kind, {level.heroStart.x + 600 + east, level.heroStart.y}, game::Facing::South, name, 60, 4, {}};
}

} // namespace

TEST_CASE("US-264 Hostile replaces the enemy switch for NPCs with a kind file") {
    Studio studio("attitude-fights", [](game::Level& level) {
        game::PlacedCharacter grub = placedAt(level, "goblin", "Grub", 0);                // monster, hostile: fights as before
        game::PlacedCharacter tame = placedAt(level, "goblin", "Tame", 40);              // a goblin that is friendly: does not fight
        tame.attitude = "friendly";
        game::PlacedCharacter grumpy = placedAt(level, "wanderer", "Grumpy", 80);        // a wanderer that is hostile: fights
        grumpy.attitude = "hostile";
        game::PlacedCharacter calm = placedAt(level, "wanderer", "Calm", 120);           // a plain wanderer: does not
        level.characters = {grub, tame, grumpy, calm};
    });
    const game::OdysseyGame& odyssey = *studio.odyssey;
    const auto& placed = odyssey.level().characters;
    CHECK(odyssey.fightsHero(placed[0]));
    CHECK_FALSE(odyssey.fightsHero(placed[1]));
    CHECK(odyssey.fightsHero(placed[2]));
    CHECK_FALSE(odyssey.fightsHero(placed[3]));
    std::vector<std::string> fighters;
    for (const auto& enemy : odyssey.enemies()) fighters.push_back(enemy.name);
    CHECK(fighters == std::vector<std::string>{"Grub", "Grumpy"});
    // Menu title: the name and the attitude word.
    CHECK(game::animalSubject(odyssey, 0).title == "Grub (hostile)");
    CHECK(game::animalSubject(odyssey, 1).title == "Grumpy (hostile)");
    CHECK(game::animalSubject(odyssey, 0).name == "Grub");
    CHECK(odyssey.attitudeWordOf(placed[3].id) == "neutral");
}

TEST_CASE("US-264 Persons start with the attitude of their kind or of their own as the opinion of the hero") {
    Studio studio("attitude-persons", [](game::Level& level) {
        game::PlacedCharacter ossa = placedAt(level, "wanderer", "Ossa", 0);
        ossa.attitude = "wary";
        game::PlacedCharacter brek = placedAt(level, "wanderer", "Brek", 40);
        level.characters = {ossa, brek};
    });
    const sim::NpcPopulation& people = studio.odyssey->npcPopulation();
    const auto& placed = studio.odyssey->level().characters;
    REQUIRE(people.size() == 2);
    CHECK(people.attitude(placed[0].id, sim::NpcPopulation::kHero) == sim::Attitude::Wary);
    CHECK(people.attitude(placed[1].id, sim::NpcPopulation::kHero) == sim::Attitude::Neutral);
    CHECK(studio.odyssey->attitudeWordOf(placed[0].id) == "wary");
    // A gift: the opinion rises by the amount and the word changes at the threshold.
    sim::NpcPopulation& mutablePeople = studio.odyssey->npcPopulationMutable();
    const int gift = studio.odyssey->npcOpinions().events.at("gift");
    CHECK(mutablePeople.event(placed[1].id, sim::NpcPopulation::kHero, "gift"));
    CHECK(people.opinion(placed[1].id, sim::NpcPopulation::kHero) == gift);
    CHECK(studio.odyssey->attitudeWordOf(placed[1].id) == "friendly");
    CHECK(studio.odyssey->attitudeWordOf(placed[0].id) == "wary"); // the other one is unchanged
}

TEST_CASE("US-264 Family: two NPCs of the same family start with the same-family opinion of each other when the level loads") {
    Studio studio("attitude-family", [](game::Level& level) {
        game::PlacedCharacter a = placedAt(level, "wanderer", "Ossa", 0);
        game::PlacedCharacter b = placedAt(level, "wanderer", "Brek", 40);
        game::PlacedCharacter c = placedAt(level, "wanderer", "Cori", 80);
        a.family = 4;
        b.family = 4;
        level.characters = {a, b, c};
    });
    const sim::NpcPopulation& people = studio.odyssey->npcPopulation();
    const auto& placed = studio.odyssey->level().characters;
    const int family = studio.odyssey->npcOpinions().sameFamily;
    CHECK(people.opinion(placed[0].id, placed[1].id) == family);
    CHECK(people.opinion(placed[1].id, placed[0].id) == family);
    CHECK(people.opinion(placed[0].id, placed[2].id) == 0);
    CHECK(people.opinionEntries() == 0); // sparse: the family opinion is the default, not an entry
    // The family field is saved in the level only when set.
    const std::string text = readText(studio.data / "attitude-level.json");
    CHECK(text.find("\"family\": 4") != std::string::npos);
    CHECK(text.find("\"family\"", text.find("\"family\"") + 1) != std::string::npos); // Brek's
    CHECK(text.find("\"family\"", text.find("\"family\"", text.find("\"family\"") + 1) + 1) == std::string::npos); // not Cori's
}
