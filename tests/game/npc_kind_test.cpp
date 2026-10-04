// US-261 Kind defaults and placed-NPC overrides in the game: resolving a placed NPC, a level without the new fields, and F5.
#include "camp.h"

#include "game/npc_class_book.h"
#include "sim/npc_kind.h"

#include <functional>
#include <memory>

using namespace camp_support;
namespace rules = odysseus::sim::rules;

namespace {

struct Studio {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;

    explicit Studio(const std::string& name, const std::function<void(game::Level&)>& changeLevel = {}) : data(dataCopy(name)) {
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        if (changeLevel) changeLevel(level);
        game::saveLevel(level, definitions, data / "kinds-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "kinds-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
    }
};

game::PlacedCharacter npc(int id, const std::string& kind, const std::string& name) {
    return {id, kind, {200 + id * 40, 200}, game::Facing::South, name, 60, 4, {}};
}

} // namespace

TEST_CASE("US-261 Precedence in the game: trade is denied for the placed NPC that denies it, and only for it") {
    Studio studio("npc-kind-precedence", [](game::Level& level) {
        game::PlacedCharacter denier = npc(level.nextId++, "wanderer", "Denier");
        denier.classes = {"trader"};
        denier.deny = {"barter"};
        game::PlacedCharacter plain = npc(level.nextId++, "wanderer", "Plain");
        plain.classes = {"trader"};
        level.characters = {denier, plain};
    });
    // The shipped trader class allows nothing yet: give it barter, as the owner would in the Class panel.
    rules::NpcClass trader = *studio.odyssey->npcClasses().catalog().find("trader");
    trader.allow = {"barter"};
    REQUIRE_FALSE(studio.odyssey->npcClasses().save(trader).has_value());
    const auto& characters = studio.odyssey->level().characters;
    const rules::ResolvedNpc denier = studio.odyssey->npcClasses().resolve(characters[0]);
    const rules::ResolvedNpc plain = studio.odyssey->npcClasses().resolve(characters[1]);
    CHECK(denier.denied("barter"));
    CHECK(plain.action("barter") == rules::ActionState::Allowed);
    CHECK(plain.tags == std::vector<std::string>{"trader"});
    CHECK(plain.attitude == "neutral"); // the wanderer kind file
}

TEST_CASE("US-261 Kinds keep today's behaviour: enemies are hostile monsters, wanderers neutral") {
    Studio studio("npc-kind-today", [](game::Level& level) {
        level.characters = {npc(level.nextId++, "goblin", "G"), npc(level.nextId++, "wanderer", "W"), npc(level.nextId++, "deer", "D")};
    });
    const auto& characters = studio.odyssey->level().characters;
    const rules::ResolvedNpc goblin = studio.odyssey->npcClasses().resolve(characters[0]);
    CHECK(goblin.attitude == "hostile");
    CHECK(goblin.classes == std::vector<std::string>{"monster"});
    CHECK(goblin.tags == std::vector<std::string>{"hostile"}); // the monster class tag
    const rules::ResolvedNpc wanderer = studio.odyssey->npcClasses().resolve(characters[1]);
    CHECK(wanderer.attitude == "neutral");
    CHECK(wanderer.classes == std::vector<std::string>{"talker"});
    CHECK(studio.odyssey->npcClasses().resolve(characters[2]).classes == std::vector<std::string>{"animal"});
}

TEST_CASE("US-261 Old level: a level saved before this story is loaded and saved byte for byte the same") {
    const fs::path data = dataCopy("npc-kind-old");
    const game::Definitions definitions = game::loadDefinitions(data);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    game::saveLevel(level, definitions, data / "old.json"); // as the game saved it before US-261: no NPC field set
    const std::string before = readText(data / "old.json");
    const game::Level loaded = game::readLevelFile(data / "old.json", definitions);
    game::saveLevel(loaded, definitions, data / "again.json");
    CHECK(readText(data / "again.json") == before);
    CHECK(before.find("\"attitude\"") == std::string::npos);
    CHECK(before.find("\"actions\"") == std::string::npos);

    // An NPC that sets things writes only those, and reads them back.
    game::PlacedCharacter set = npc(level.nextId++, "wanderer", "Ossa");
    set.classes = {"trader"};
    set.attitude = "friendly";
    set.dialogues = {{"player", "ossa.dlg"}};
    set.deny = {"barter"};
    level.characters.push_back(set);
    game::saveLevel(level, definitions, data / "set.json");
    const std::string text = readText(data / "set.json");
    CHECK(text.find("\"attitude\": \"friendly\"") != std::string::npos);
    CHECK(text.find("\"deny\"") != std::string::npos);
    CHECK(text.find("\"allow\"") == std::string::npos);
    CHECK(game::readLevelFile(data / "set.json", definitions) == level);
    // Mistakes are named.
    const auto problem = [&](const std::string& from, const std::string& to) {
        std::string broken = text;
        const auto at = broken.find(from);
        REQUIRE_MESSAGE(at != std::string::npos, from);
        broken.replace(at, from.size(), to);
        writeText(data / "broken.json", broken);
        try {
            game::readLevelFile(data / "broken.json", definitions);
        } catch (const odysseus::sim::DataError& error) {
            return std::string(error.what());
        }
        return std::string();
    };
    CHECK(problem("\"friendly\"", "\"grumpy\"").find(".attitude") != std::string::npos);
    CHECK(problem("\"player\"", "\"robot\"").find(".dialogues.robot") != std::string::npos);
    CHECK(problem("\"ossa.dlg\"", "\"ossa.txt\"").find(".dialogues.player") != std::string::npos);
}

TEST_CASE("US-261 Reload: a kind file changed on disk applies after F5 to every NPC of that kind without an override") {
    Studio studio("npc-kind-reload", [](game::Level& level) {
        game::PlacedCharacter own = npc(level.nextId++, "goblin", "Own");
        own.attitude = "scared";
        level.characters = {npc(level.nextId++, "goblin", "Plain"), own};
    });
    const auto& characters = studio.odyssey->level().characters;
    CHECK(studio.odyssey->npcClasses().resolve(characters[0]).attitude == "hostile");
    writeText(studio.data / "npcs" / "goblin.json", "{\n  \"kind\": \"goblin\",\n  \"classes\": [\"monster\"],\n  \"attitude\": \"wary\"\n}\n");
    CHECK(studio.odyssey->npcClasses().resolve(characters[0]).attitude == "hostile"); // not before F5
    studio.odyssey->update(reloadPressed());
    CHECK(studio.odyssey->npcClasses().resolve(characters[0]).attitude == "wary");
    CHECK(studio.odyssey->npcClasses().resolve(characters[1]).attitude == "scared"); // the override stays
    // A mistake keeps the last good data.
    writeText(studio.data / "npcs" / "goblin.json", "{\n  \"kind\": \"goblin\",\n  \"attitude\": \"grumpy\"\n}\n");
    studio.odyssey->update(reloadPressed());
    CHECK(studio.odyssey->npcClasses().resolve(characters[0]).attitude == "wary");
    CHECK(studio.odyssey->npcClasses().report().errors.size() == 1);
}
