// US-266 Confront: a separate menu (and the key C) with taunt, insult, ask for peace, antagonise and de-escalate; the amounts are data, the words are heard by
// friends in range, and the outcome can start or stop a fight.
#include "camp.h"

#include "game/game_rules.h"
#include "game/npc_class_book.h"
#include "sim/npc_population.h"

#include <functional>
#include <memory>

using namespace camp_support;
namespace sim = odysseus::sim;

namespace {

luna::engine::Intents pressing(luna::engine::Intent intent) {
    luna::engine::Intents intents;
    intents.set(intent, true, true);
    return intents;
}

struct Studio {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;

    Studio(const std::string& name, const std::function<void(game::Level&)>& changeLevel, const std::function<void(const fs::path&)>& adjust = {}) : data(dataCopy(name)) {
        if (adjust) adjust(data);
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        changeLevel(level);
        game::saveLevel(level, definitions, data / "confront-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "confront-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
    }
    // The confront menu of a placed character: its labels.
    std::vector<std::string> confrontMenu(int id) {
        std::optional<game::Subject> subject = game::npcSubject(*odyssey, id);
        if (!subject) {
            for (std::size_t i = 0; i < odyssey->enemies().size(); ++i) {
                if (odyssey->enemies()[i].id == id) subject = game::animalSubject(*odyssey, i);
            }
        }
        REQUIRE(subject.has_value());
        REQUIRE(odyssey->run().openConfront(*odyssey, *subject));
        std::vector<std::string> labels;
        for (const auto& entry : odyssey->run().contextEntries()) labels.push_back(entry.reason.empty() ? entry.label : entry.label + " | " + entry.reason);
        return labels;
    }
    void choose(std::size_t index) { odyssey->run().press(*odyssey, game::RunFlow::kContextBase + static_cast<int>(index)); }
};

game::PlacedCharacter placed(game::Level& level, const std::string& kind, const std::string& name, int east, int south = 0) {
    return {level.nextId++, kind, {level.heroStart.x + east, level.heroStart.y + south}, game::Facing::South, name, 60, 4, {}};
}

} // namespace

TEST_CASE("US-266 Separate: the Confront key offers the five confront actions and Talk is not among them") {
    Studio studio("confront-separate", [](game::Level& level) {
        game::PlacedCharacter ossa = placed(level, "wanderer", "Ossa", 60);
        ossa.classes = {"trader"};
        ossa.attitude = "friendly";
        ossa.dialogues = {{"player", "npc-confront-talk.dlg"}};
        level.characters = {ossa};
    }, [](const fs::path& data) { writeText(data / "dialogue" / "npc-confront-talk.dlg", "@who npc-nobody\n\n=== start\nOssa: Hello.\n-> Leave => END\n"); });
    const int ossa = studio.odyssey->level().characters[0].id;
    CHECK(studio.odyssey->attitudeWordOf(ossa) == "friendly");
    // The ordinary right-click menu has Talk and a way into the Confront menu, but none of the five.
    const game::PlacedCharacter* placedOssa = studio.odyssey->placedCharacter(ossa);
    REQUIRE(placedOssa != nullptr);
    REQUIRE(studio.odyssey->run().openContext(*studio.odyssey, placedOssa->feet.x, placedOssa->feet.y - 20));
    std::vector<std::string> ordinary;
    for (const auto& entry : studio.odyssey->run().contextEntries()) ordinary.push_back(entry.label);
    CHECK(ordinary == std::vector<std::string>{"Talk", "Confront...", "Actions..."});
    studio.odyssey->run().close();

    // The key C: the five actions, in the order of their files, and no Talk.
    studio.odyssey->update(pressing(luna::engine::Intent::Confront));
    REQUIRE(studio.odyssey->run().screen() == game::Screen::Context);
    std::vector<std::string> labels;
    for (const auto& entry : studio.odyssey->run().contextEntries()) labels.push_back(entry.label);
    CHECK(labels == std::vector<std::string>{"Taunt", "Insult", "Ask for peace", "Antagonise", "De-escalate"});
    CHECK(studio.odyssey->run().contextTitle() == "Confront Ossa (friendly)");
    // The right-click entry opens the same menu.
    studio.odyssey->run().close();
    REQUIRE(studio.odyssey->run().openContext(*studio.odyssey, placedOssa->feet.x, placedOssa->feet.y - 20));
    studio.odyssey->run().press(*studio.odyssey, game::RunFlow::kContextBase + 1); // Confront...
    REQUIRE(studio.odyssey->run().screen() == game::Screen::Context);
    CHECK(studio.odyssey->run().contextEntries().size() == 5);
    CHECK(studio.odyssey->run().contextEntries()[0].label == "Taunt");
}

TEST_CASE("US-266 Insult: the opinion of the target and of its friends who hear it fall by the data amounts") {
    Studio studio("confront-insult", [](game::Level& level) {
        game::PlacedCharacter target = placed(level, "wanderer", "Ossa", 60);
        game::PlacedCharacter friendA = placed(level, "wanderer", "Brek", 100);
        game::PlacedCharacter friendB = placed(level, "wanderer", "Cori", 140);
        game::PlacedCharacter stranger = placed(level, "wanderer", "Dax", 180);
        game::PlacedCharacter farFriend = placed(level, "wanderer", "Eno", 60, -32 * 30); // 30 m north: out of hearing (the hero starts at y 1048, so 40 m south would leave the level)
        target.family = friendA.family = friendB.family = farFriend.family = 4;
        level.characters = {target, friendA, friendB, stranger, farFriend};
    });
    const auto& characters = studio.odyssey->level().characters;
    const sim::NpcPopulation& people = studio.odyssey->npcPopulation();
    const int hero = sim::NpcPopulation::kHero;
    const auto before = [&](std::size_t i) { return people.opinion(characters[i].id, hero); };
    for (std::size_t i = 0; i < 5; ++i) CHECK(before(i) == 0);

    const std::vector<std::string> menu = studio.confrontMenu(characters[0].id);
    REQUIRE(menu.size() == 5);
    CHECK(menu[1] == "Insult");
    studio.choose(1);
    // The data amounts: -20 for the target, -5 for every friend who hears it. The stranger does not know the target; the far friend cannot hear.
    CHECK(before(0) == -20);
    CHECK(before(1) == -5);
    CHECK(before(2) == -5);
    CHECK(before(3) == 0);
    CHECK(before(4) == 0);
    // Only the pairs that heard have an entry now: the target and the two friends.
    CHECK(people.opinionEntries() == 3);
    // The target remembers.
    const int ossa = people.indexOf(characters[0].id);
    REQUIRE(people.noteCount(ossa) >= 1);
    CHECK(people.note(ossa, people.noteCount(ossa) - 1).text.find("insulted me") != std::string::npos);
    CHECK(people.attitude(characters[0].id, hero) == sim::Attitude::Suspicious); // -20 is in the band from -29

    // The amounts come from the file: a changed file changes the outcome after F5.
    writeText(studio.data / "interactions" / "insult.json",
              "{\n  \"id\": \"insult\",\n  \"label\": \"Insult\",\n  \"actors\": [\"hero\"],\n  \"target\": { \"tags\": [\"npc\"] },\n  \"range\": 8,\n  \"order\": 20,\n  \"menu\": \"confront\",\n"
              "  \"effects\": [ \"opinion npc hero -50\" ]\n}\n");
    studio.odyssey->update(pressing(luna::engine::Intent::Reload));
    studio.confrontMenu(characters[0].id);
    studio.choose(1);
    CHECK(before(0) == -70);
    CHECK(before(1) == -5); // no spreading in the new file
}

TEST_CASE("US-266 Calm down: a hostile NPC about to attack stops and thinks better of the hero when the check succeeds") {
    const auto certain = [](const fs::path& data) { // the shipped chance is 70 in 100; the test makes it certain, or impossible, in the file
        std::string text = readText(data / "interactions" / "de-escalate.json");
        text.replace(text.find("do calm 15 70"), 13, "do calm 15 100");
        writeText(data / "interactions" / "de-escalate.json", text);
    };
    Studio studio("confront-calm", [](game::Level& level) { level.characters = {placed(level, "goblin", "Grub", 80)}; }, certain);
    REQUIRE(studio.odyssey->enemies().size() == 1);
    const int grub = studio.odyssey->enemies()[0].id;
    studio.odyssey->enemiesMutable()[0].provoke(); // the hero hit it: it winds up to strike back
    REQUIRE(studio.odyssey->enemies()[0].isWindingUp());
    const int before = studio.odyssey->npcPopulation().opinion(grub, sim::NpcPopulation::kHero);
    CHECK(studio.odyssey->attitudeWordOf(grub) == "hostile");

    const std::vector<std::string> menu = studio.confrontMenu(grub); // hostile NPCs can be confronted too
    REQUIRE(menu.size() == 5);
    CHECK(menu[4] == "De-escalate");
    studio.choose(4);
    CHECK_FALSE(studio.odyssey->enemies()[0].isWindingUp());
    CHECK(studio.odyssey->npcPopulation().opinion(grub, sim::NpcPopulation::kHero) == before + 15);
    CHECK(studio.odyssey->run().message().find("calms down") != std::string::npos);

    // When the check fails nothing changes: the file says 0 in 100.
    writeText(studio.data / "interactions" / "de-escalate.json",
              "{\n  \"id\": \"de-escalate\",\n  \"label\": \"De-escalate\",\n  \"actors\": [\"hero\"],\n  \"target\": { \"tags\": [\"npc\"] },\n  \"range\": 8,\n  \"order\": 50,\n  \"menu\": \"confront\",\n"
              "  \"effects\": [ \"do calm 15 0\" ]\n}\n");
    studio.odyssey->update(pressing(luna::engine::Intent::Reload));
    studio.odyssey->enemiesMutable()[0].provoke();
    const int middle = studio.odyssey->npcPopulation().opinion(grub, sim::NpcPopulation::kHero);
    studio.confrontMenu(grub);
    studio.choose(4);
    CHECK(studio.odyssey->enemies()[0].isWindingUp());
    CHECK(studio.odyssey->npcPopulation().opinion(grub, sim::NpcPopulation::kHero) == middle);
    CHECK(studio.odyssey->run().message().find("will not listen") != std::string::npos);
}

TEST_CASE("US-266 Antagonise: a peaceful person picks a fight and becomes an enemy that strikes back; peace warms a little") {
    Studio studio("confront-fight", [](game::Level& level) { level.characters = {placed(level, "wanderer", "Ossa", 60)}; });
    const int ossa = studio.odyssey->level().characters[0].id;
    CHECK(studio.odyssey->enemies().empty());
    const int start = studio.odyssey->npcPopulation().opinion(ossa, sim::NpcPopulation::kHero);
    // Ask for peace first: a small rise.
    studio.confrontMenu(ossa);
    studio.choose(2);
    CHECK(studio.odyssey->npcPopulation().opinion(ossa, sim::NpcPopulation::kHero) == start + 5);
    // Then antagonise: the data says -30, and a fight starts.
    studio.confrontMenu(ossa);
    studio.choose(3);
    CHECK(studio.odyssey->npcPopulation().opinion(ossa, sim::NpcPopulation::kHero) == start + 5 - 30);
    REQUIRE(studio.odyssey->enemies().size() == 1);
    CHECK(studio.odyssey->enemies()[0].id == ossa);
    CHECK(studio.odyssey->enemies()[0].isWindingUp());
    CHECK(studio.odyssey->bystanders().empty());
    // It is still a person of the population with the same opinions.
    CHECK(studio.odyssey->npcPopulation().indexOf(ossa) >= 0);
    // Taunt is a small insult: the data says -10.
    const int before = studio.odyssey->npcPopulation().opinion(ossa, sim::NpcPopulation::kHero);
    studio.confrontMenu(ossa);
    studio.choose(0);
    CHECK(studio.odyssey->npcPopulation().opinion(ossa, sim::NpcPopulation::kHero) == before - 10);
}

TEST_CASE("US-266 Data: the confront files load, say menu confront, and a bad menu word names the file") {
    const fs::path interactions = fs::path(ODYSSEUS_DATA_DIR) / "interactions";
    odysseus::sim::rules::LoadReport report;
    const auto registry = odysseus::sim::rules::InteractionRegistry::load(interactions, report);
    for (const auto& error : report.errors) FAIL(error.text());
    for (const char* id : {"taunt", "insult", "ask-for-peace", "antagonise", "de-escalate"}) {
        const auto* interaction = registry.find(id);
        REQUIRE_MESSAGE(interaction != nullptr, id);
        CHECK(interaction->menu == "confront");
        CHECK(interaction->targetTags == std::vector<std::string>{"npc"});
        // Load, save, load gives the same interaction.
        odysseus::sim::rules::LoadReport again;
        const auto reread = odysseus::sim::rules::InteractionRegistry::parse(odysseus::sim::rules::toJson(*interaction), interaction->file, again, id);
        REQUIRE(reread.has_value());
        CHECK(reread->menu == "confront");
        CHECK(odysseus::sim::rules::toJson(*reread) == odysseus::sim::rules::toJson(*interaction));
    }
    CHECK(registry.find("confront")->menu.empty());
    odysseus::sim::rules::LoadReport bad;
    odysseus::sim::rules::InteractionRegistry::parse(R"({"id":"x","label":"X","actors":["hero"],"target":{"tags":["npc"]},"menu":"secret","effects":[]})", "interactions/x.json", bad, "x");
    REQUIRE_FALSE(bad.errors.empty());
    CHECK(bad.errors[0].message.find("menu") != std::string::npos);
}
