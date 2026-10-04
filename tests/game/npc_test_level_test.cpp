// US-270 The NPC test level: assets/levels/npc-test.json holds the seven NPCs of the D-52 cast, loads with no mistake anywhere, reads and writes back as the same text,
// and each NPC is what its row of the walk-through checklist (docs/guides/npc-data.md) says.
#include "camp.h"

#include "game/npc_class_book.h"
#include "sim/dialogue_script.h"

#include <memory>

using namespace camp_support;
namespace sim = odysseus::sim;

namespace {

luna::engine::Intents pressing(luna::engine::Intent intent) {
    luna::engine::Intents intents;
    intents.set(intent, true, true);
    return intents;
}

fs::path shippedLevel() { return fs::path(ODYSSEUS_DATA_DIR).parent_path() / "levels" / "npc-test.json"; }

struct Studio {
    fs::path data;
    fs::path file;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;

    explicit Studio(const std::string& name) : data(dataCopy(name)), file(data / "npc-test.json") {
        fs::copy_file(shippedLevel(), file);
        odyssey = std::make_unique<game::OdysseyGame>(data, file);
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
        odyssey->update(pressing(luna::engine::Intent::ModeGame));
    }
    const game::PlacedCharacter& named(const std::string& name) const {
        for (const game::PlacedCharacter& placed : odyssey->level().characters) {
            if (placed.name == name) return placed;
        }
        FAIL("the test level has no ", name);
        return odyssey->level().characters.front();
    }
};

} // namespace

TEST_CASE("US-270 Loads clean: no class, kind, interaction or dialogue error, and every file the level names exists") {
    Studio studio("npc-test-level-clean");
    CHECK(studio.odyssey->npcClasses().report().errors.empty());
    CHECK(studio.odyssey->interactionReport().errors.empty());
    const auto& characters = studio.odyssey->level().characters;
    REQUIRE(characters.size() == 7);
    for (const game::PlacedCharacter& placed : characters) {
        // The kind has a file, and so does every class the NPC names.
        CHECK(studio.odyssey->npcClasses().kinds().find(placed.kind) != nullptr);
        for (const std::string& id : placed.classes) CHECK_MESSAGE(studio.odyssey->npcClasses().catalog().find(id) != nullptr, placed.name, " names the class ", id);
        // Every dialogue the NPC names is a script of the library.
        for (const auto& [partner, file] : placed.dialogues) {
            CHECK_MESSAGE(studio.odyssey->dialogues().find(file.substr(0, file.size() - 4)) != nullptr, placed.name, " names ", file, " for ", partner);
        }
    }
    for (const char* who : {"Tala", "Ossa", "Harn", "Vell", "Gur"}) CHECK_MESSAGE(studio.odyssey->npcDialogueFor(studio.named(who).id) != nullptr, who, " has something to say");
    CHECK(studio.odyssey->npcDialogueFor(studio.named("Deer").id) == nullptr);
    CHECK(studio.odyssey->npcDialogueFor(studio.named("Goblin").id) == nullptr);
}

TEST_CASE("US-270 Loads clean: load, save and load again gives the same level and the same text") {
    Studio studio("npc-test-level-roundtrip");
    const game::Definitions definitions = game::loadDefinitions(studio.data);
    const game::Level first = game::loadLevel(shippedLevel(), definitions).level;
    const fs::path again = studio.data / "npc-test-again.json";
    game::saveLevel(first, definitions, again);
    CHECK(game::loadLevel(again, definitions).level == first);
    CHECK(readText(again) == readText(shippedLevel()));
}

TEST_CASE("US-270 Walk-through: each NPC is what the checklist says") {
    Studio studio("npc-test-level-walk");
    const game::OdysseyGame& game = *studio.odyssey;
    const auto classesOf = [&](const char* who) { return game.npcClasses().resolve(studio.named(who)).classes; };
    CHECK(classesOf("Tala") == std::vector<std::string>{"trader"});
    CHECK(classesOf("Ossa") == std::vector<std::string>{"talker"}); // from the wanderer kind
    CHECK(classesOf("Harn") == std::vector<std::string>{"hunter"});
    CHECK(classesOf("Vell") == std::vector<std::string>{"elder"});
    CHECK(classesOf("Gur") == std::vector<std::string>{"guard"});
    CHECK(classesOf("Goblin") == std::vector<std::string>{"monster"});
    CHECK(classesOf("Deer") == std::vector<std::string>{"animal"});

    CHECK(game.attitudeWordOf(studio.named("Tala").id) == "neutral");
    CHECK(game.attitudeWordOf(studio.named("Harn").id) == "wary");
    CHECK(game.attitudeWordOf(studio.named("Vell").id) == "friendly");
    CHECK(game.attitudeWordOf(studio.named("Goblin").id) == "hostile");

    // The goblin attacks; nobody else does.
    CHECK(game.fightsHero(studio.named("Goblin")));
    for (const char* who : {"Tala", "Ossa", "Harn", "Vell", "Gur", "Deer"}) CHECK_FALSE(game.fightsHero(studio.named(who)));
}
