// US-293 Default interactions by partner type in the game and in the Editor: a type added to partner-types.json shows up in the NPC panel; the defaults files give every NPC its
// environment actions; the hunter class hunts the deer it sees.
#include "camp.h"

#include "game/game_rules.h"
#include "game/npc_class_book.h"
#include "sim/npc_director.h"
#include "sim/partner_types.h"

#include <cstdlib>
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
    fs::path file;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;
    int tala = 0;
    int deer = 0;

    // Tala (with the classes given) stands 4 tiles west of the hero; with `withDeer` a deer stands 5 tiles east of her.
    Studio(const std::string& name, std::vector<std::string> classes, bool withDeer = false, const std::string& partnerTypes = {}, bool withGrove = false) : data(dataCopy(name)) {
        if (!partnerTypes.empty()) writeText(data / "sim" / "partner-types.json", partnerTypes);
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        const game::PixelPoint hero = level.heroStart;
        game::PlacedCharacter placed{level.nextId++, "wanderer", {hero.x - 128, hero.y}, game::Facing::South, "Tala", 60, 4, std::move(classes)};
        tala = placed.id;
        level.characters = {placed};
        if (withDeer) {
            game::PlacedCharacter animal{level.nextId++, "deer", {hero.x - 128 + 160, hero.y}, game::Facing::West, "Deer", 80, 0, {}};
            deer = animal.id;
            level.characters.push_back(animal);
        }
        if (withGrove) level.places = {{"grove", {hero.x + 192, hero.y}, {"forage"}}};
        file = data / "defaults-level.json";
        game::saveLevel(level, definitions, file);
        odyssey = std::make_unique<game::OdysseyGame>(data, file);
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
    }
    game::Editor& editor() { return odyssey->editor(); }
    void toEditor() { odyssey->update(pressing(luna::engine::Intent::ModeEditor)); }
    void toGame() { odyssey->update(pressing(luna::engine::Intent::ModeGame)); }
    void play(int ticks) {
        for (int i = 0; i < ticks; ++i) odyssey->update({});
    }
    int index() const { return odyssey->npcPopulation().indexOf(tala); }
};

} // namespace

TEST_CASE("US-293 Extend: a new partner type 'buildings' added to the data file is offered by the NPC panel, and the defaults for it are saved") {
    Studio studio("defaults-extend", {"talker"}, false, R"({ "version": 1, "types": ["player", "animal", "environment", "buildings"] })");
    game::Editor& editor = studio.editor();
    studio.toEditor();
    editor.select(studio.tala);
    REQUIRE(editor.selectedIsNpc());
    const std::vector<std::string> types = editor.partnerTypes();
    CHECK(std::find(types.begin(), types.end(), "buildings") != types.end());
    CHECK(std::find(types.begin(), types.end(), "class:talker") != types.end());

    const std::size_t before = editor.history().size();
    CHECK(editor.setSelectedPartnerActions("buildings", "inspect-wall, repair"));
    CHECK(editor.history().size() == before + 1);
    CHECK(editor.level().characters[0].extras.partnerActions.at("buildings") == std::vector<std::string>{"inspect-wall", "repair"});
    CHECK(editor.setSelectedPartnerActions("buildings", "inspect-wall repair")); // the same list: no step
    CHECK(editor.history().size() == before + 1);
    CHECK_FALSE(editor.setSelectedPartnerActions("robot", "x"));       // not a partner type
    CHECK_FALSE(editor.setSelectedPartnerActions("buildings", "Repair")); // not an interaction id
    CHECK(editor.status().find("defaults with") != std::string::npos);
    REQUIRE(editor.save());
    CHECK(readText(studio.file).find("\"partnerActions\"") != std::string::npos);
    CHECK(game::loadLevel(studio.file, studio.odyssey->definitions()).level == editor.level());
    CHECK(editor.undo());
    CHECK(editor.level().characters[0].extras.partnerActions.empty());

    // A class and a kind can carry them as well.
    editor.showClasses(true);
    editor.selectClass("talker");
    CHECK(editor.setClassPartnerActions("buildings", "repair"));
    CHECK(editor.setClassPartnerActions("class", "npc-chat"));
    REQUIRE(editor.saveClass());
    CHECK(studio.odyssey->npcClasses().resolve(editor.level().characters[0]).extras.partnerActions.at("buildings") == std::vector<std::string>{"repair"});
    editor.showKinds(true);
    editor.selectKind("wanderer");
    CHECK(editor.setKindPartnerActions("animal", "watch"));
    REQUIRE(editor.saveKind());
    CHECK(studio.odyssey->npcClasses().resolve(editor.level().characters[0]).extras.partnerActions.at("animal") == std::vector<std::string>{"watch"});
    sim::rules::setPartnerTypes({}); // the types are global: the next game starts from its own file again
}

TEST_CASE("US-293 Defaults: every NPC has the environment defaults of defaults-environment.json, and the hunter class hunts animals") {
    Studio studio("defaults-shipped", {"hunter"});
    const sim::rules::PartnerDefaults& defaults = studio.odyssey->npcClasses().partnerDefaults();
    REQUIRE(defaults.find("environment") != nullptr);
    CHECK(*defaults.find("environment") == std::vector<std::string>{"forage", "rest-at-shelter", "fish", "pray"});
    const sim::rules::ResolvedNpc resolved = studio.odyssey->npcClasses().resolve(studio.odyssey->level().characters[0]);
    CHECK(resolved.extras.partnerActions.at("environment") == std::vector<std::string>{"forage", "rest-at-shelter", "fish", "pray"});
    CHECK(resolved.extras.partnerActions.at("animal") == std::vector<std::string>{"hunt"}); // the class overrides the empty default
    CHECK((resolved.extras.partnerActions.count("class") == 0 || resolved.extras.partnerActions.at("class").empty())); // the shipped class default is empty
    studio.toGame();
    const sim::NpcProfile* profile = studio.odyssey->npcDirector().profile(studio.index());
    REQUIRE(profile != nullptr);
    CHECK(profile->partnerActions.at("animal") == std::vector<std::string>{"hunt"});
}

TEST_CASE("US-293 Animals: a hunter near a deer hunts it") {
    Studio studio("defaults-hunt", {"hunter"}, true);
    studio.toGame();
    studio.play(110); // the first hour mark
    CHECK(studio.odyssey->npcDirector().lastAction(studio.index()) == "hunt");
    // The hunter set off for the deer (which may have wandered a little): it is no longer at home.
    const sim::NpcPopulation& people = studio.odyssey->npcPopulation();
    const game::PixelPoint home = studio.odyssey->level().characters[0].feet;
    CHECK((people.x(studio.index()) != home.x || people.y(studio.index()) != home.y));
}

TEST_CASE("US-293 Environment: a person at a level with a forage place goes foraging when hungry") {
    Studio studio("defaults-forage", {"talker"}, false, {}, true);
    studio.odyssey->npcPopulationMutable().setNeed(studio.index(), sim::Need::Hunger, 25);
    studio.play(110);
    CHECK(studio.odyssey->npcDirector().lastAction(studio.index()) == "forage");
    const game::PixelPoint grove = studio.odyssey->level().places[0].at;
    CHECK(std::abs(studio.odyssey->npcPopulation().x(studio.index()) - grove.x) <= 48);
}
