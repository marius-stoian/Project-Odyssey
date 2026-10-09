// US-207 Play the edited region: a new game on a world file, Play here from the Region view and Esc back, the run save that names its world, and saves from before region
// editing.
#include "camp.h"

#include "game/play_world.h"
#include "game/region_view.h"
#include "luna/engine/renderer.h"
#include "sim/hero_life.h"
#include "sim/rivals.h"
#include "sim/world.h"
#include "sim/world_file.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <string>

using namespace camp_support;
namespace sim = odysseus::sim;
namespace eng = luna::engine;

namespace {

sim::RegionConfig config(const fs::path& data) { return sim::loadRegionConfig(data / "sim" / "region.json"); }

// The game on a copy of the data folder, with its own saves; the worlds folder is next to the data folder, so every test has its own.
struct Studio {
    eng::RecordingRenderer renderer;
    fs::path data;
    fs::path saves;
    std::unique_ptr<game::OdysseyGame> odyssey;

    explicit Studio(const std::string& name, const fs::path& reuse = {}) : data(reuse.empty() ? dataCopy(name) : reuse), saves(data.parent_path() / "saves") {
        fs::create_directories(saves);
        odyssey = std::make_unique<game::OdysseyGame>(data, Camp::makeLevel(data, name + (reuse.empty() ? "" : "-again")));
        odyssey->setSaveDirectory(saves);
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
    }
};

eng::Intents pressed(eng::Intent intent) {
    eng::Intents intents;
    intents.set(intent, true, true);
    return intents;
}

// A world made in the Region view the way the owner makes one: a painted lake, a person, a rival camp and a setup for the clans.
struct MadeWorld {
    sim::Tile lake{-1, -1};
    sim::Tile person{-1, -1};
    sim::Tile camp{-1, -1};
    sim::Tile far{-1, -1}; // a steppe tile that is not painted
    std::string name;
};

MadeWorld makeWorld(const fs::path& data, const std::string& name) {
    MadeWorld made;
    made.name = name;
    std::vector<std::string> said;
    game::RegionView view(960, 540, [&said](const std::string& text) { said.push_back(text); });
    view.setWorldsFolder(game::worldsFolder(data), name);
    view.open(1, config(data));
    sim::Region& land = *view.region();
    for (int y = 40; y < 200 && made.person.x < 0; ++y) {
        for (int x = 40; x < 200 && made.person.x < 0; ++x) {
            bool open = true;
            for (int d = 0; d < 8; ++d) open = open && land.biomeAt(x + d, y) == sim::Biome::Steppe && land.biomeAt(x + d, y + 1) == sim::Biome::Steppe;
            if (open && std::max(std::abs(x - land.start().x), std::abs(y - land.start().y)) > 12) made.person = {x, y};
        }
    }
    REQUIRE(made.person.x >= 0);
    made.lake = {made.person.x + 5, made.person.y};
    made.far = {made.person.x + 7, made.person.y + 1};
    view.beginStroke();
    view.setPaintBiome(sim::Biome::Water);
    view.paintBrush(made.lake.x, made.lake.y);
    view.endStroke();

    view.setPlaceGroup(sim::EditGroup::Person);
    view.setPlaceKind("goblin");
    view.setPlaceName("Tough Gob");
    REQUIRE_FALSE(view.placeEntry(made.person.x, made.person.y).empty());

    for (int y = 30; y < 226 && made.camp.x < 0; y += 3) {
        for (int x = 30; x < 226 && made.camp.x < 0; x += 3) {
            if (std::max(std::abs(x - land.start().x), std::abs(y - land.start().y)) >= 60 && land.biomeAt(x, y) == sim::Biome::Steppe && land.goodSite({x, y})) made.camp = {x, y};
        }
    }
    REQUIRE(made.camp.x >= 0);
    view.setPlaceGroup(sim::EditGroup::Camp);
    view.setPlaceKind("rival");
    view.setPlaceName("the Crow Clan");
    REQUIRE_FALSE(view.placeEntry(made.camp.x, made.camp.y).empty());

    REQUIRE(view.setClanField("player", "store", "food=80"));
    REQUIRE(view.setClanField("the Crow Clan", "store", "food=77"));
    REQUIRE(view.setPersonField("0", "opinions", "1=-50"));
    REQUIRE(view.setPersonField("0", "grudges", "1:30:stole the last flint"));
    REQUIRE(view.saveWorld());
    return made;
}

} // namespace

TEST_CASE("US-207 New game: a run on a world file has every edit of it, its setup and its fingerprint") {
    Studio studio("us207-newgame");
    const MadeWorld made = makeWorld(studio.data, "test");
    game::OdysseyGame& odyssey = *studio.odyssey;
    const fs::path file = game::worldFilePath(studio.data, "test");

    REQUIRE(odyssey.startNewRun({99, 2, 1, "", "test"}, true, false)); // the seed asked for is the file's to replace
    odyssey.run().close();
    REQUIRE(odyssey.region() != nullptr);
    CHECK(odyssey.region()->seed() == 1);
    CHECK(odyssey.worldName() == "test");
    CHECK(odyssey.worldHash() == sim::worldFileHash(file));

    // The land: the painted lake, the person put on it, the rival camp.
    CHECK(odyssey.region()->biomeAt(made.lake.x, made.lake.y) == sim::Biome::Water);
    const auto& characters = odyssey.level().characters;
    CHECK(std::any_of(characters.begin(), characters.end(), [](const game::PlacedCharacter& c) { return c.name == "Tough Gob"; }));
    REQUIRE(odyssey.rivals() != nullptr);
    const auto& rivals = odyssey.rivals()->clans();
    const auto crow = std::find_if(rivals.begin(), rivals.end(), [](const sim::RivalClan& clan) { return clan.name == "the Crow Clan"; });
    REQUIRE(crow != rivals.end());
    CHECK(crow->camp.x == made.camp.x);
    CHECK(crow->camp.y == made.camp.y);

    // The setup: the clan's meals, a rival's meals, an opinion as written and a grudge the chronicle tells.
    REQUIRE(odyssey.clan() != nullptr);
    CHECK(odyssey.clan()->food() == 80);
    CHECK(crow->world->food() == 77);
    CHECK(odyssey.clan()->opinion(0, 1) == -50);
    const sim::Grudge* grudge = sim::heaviestGrudge(odyssey.clan()->people()[0], 1);
    REQUIRE(grudge != nullptr);
    const sim::ChronicleEntry* told = odyssey.clan()->chronicle().find(grudge->event);
    REQUIRE(told != nullptr);
    CHECK(told->text.find("stole the last flint") != std::string::npos);

    // The run knows the world it began on.
    REQUIRE(odyssey.life() != nullptr);
    CHECK(odyssey.life()->game().world == "test");
    CHECK(odyssey.life()->game().worldHash == sim::worldFileHash(file));
    CHECK(odyssey.life()->game().seed == 1);

    // The New Game screen offers the world, and its Start button starts the run on it.
    Studio screen("us207-screen");
    makeWorld(screen.data, "test");
    screen.odyssey->run().openNewGame();
    screen.odyssey->update({}); // builds the screen; the first New Game screen asks about statistics before it shows
    if (screen.odyssey->run().screen() == game::Screen::Privacy) {
        screen.odyssey->run().press(*screen.odyssey, 6); // No
        screen.odyssey->update({});
    }
    REQUIRE(screen.odyssey->run().screen() == game::Screen::NewGame);
    bool offered = false;
    int testId = 0;
    for (const auto& widget : screen.odyssey->run().widgets()) {
        if (widget.label == "test") {
            offered = true;
            testId = widget.id;
        }
    }
    REQUIRE(offered);
    CHECK(screen.odyssey->run().press(*screen.odyssey, testId));
    CHECK(screen.odyssey->run().pickedWorld() == "test");
    CHECK(screen.odyssey->run().press(*screen.odyssey, 1)); // Start
    CHECK(screen.odyssey->worldName() == "test");
    CHECK(screen.odyssey->clan()->food() == 80);
}

TEST_CASE("US-207 New game: a world that cannot be played changes nothing and says why") {
    Studio studio("us207-broken");
    fs::create_directories(game::worldsFolder(studio.data));
    writeText(game::worldFilePath(studio.data, "broken"), "{ \"version\": 99 }");
    game::OdysseyGame& odyssey = *studio.odyssey;
    const std::string levelBefore = odyssey.level().name;
    CHECK_FALSE(odyssey.startNewRun({1, 2, 1, "", "broken"}, true, false));
    CHECK(odyssey.region() == nullptr);
    CHECK(odyssey.level().name == levelBefore);
    CHECK(odyssey.message().find("broken") != std::string::npos);
    CHECK_FALSE(odyssey.loadWorld("nothing-here"));
}

TEST_CASE("US-207 Play here: the game starts on the edited region at the chosen tile, Esc returns with the Editor's level and the edits as they were, and nothing is saved") {
    Studio studio("us207-playhere");
    const MadeWorld made = makeWorld(studio.data, "default");
    game::OdysseyGame& odyssey = *studio.odyssey;
    const std::string levelName = odyssey.level().name;
    const std::size_t characters = odyssey.level().characters.size();
    odyssey.update(pressed(eng::Intent::ModeEditor));
    REQUIRE(odyssey.mode() == game::Mode::Editor);
    odyssey.editor().regionView().show(true);
    REQUIRE(odyssey.editor().regionView().shown());

    REQUIRE(odyssey.playWorldHere("default", made.far.x, made.far.y));
    CHECK(odyssey.playingWorld());
    CHECK(odyssey.mode() == game::Mode::Game);
    REQUIRE(odyssey.region() != nullptr);
    CHECK(odyssey.region()->biomeAt(made.lake.x, made.lake.y) == sim::Biome::Water); // the edits are there
    CHECK(static_cast<int>(odyssey.hero().feetX()) / 32 == made.far.x);               // and the hero stands at the cursor
    CHECK(static_cast<int>(odyssey.hero().feetY()) / 32 == made.far.y);
    REQUIRE(odyssey.life() != nullptr);
    CHECK(odyssey.life()->phase() == sim::Phase::Free);
    CHECK_FALSE(odyssey.run().modal());
    CHECK(odyssey.clan()->food() == 80);

    // A test run touches no save.
    for (int i = 0; i < 3; ++i) odyssey.update({});
    CHECK(odyssey.autosave());
    CHECK_FALSE(fs::exists(studio.saves / "hero.json"));
    CHECK_FALSE(fs::exists(studio.saves / "world.json"));
    CHECK_FALSE(fs::exists(studio.saves / "clan.json"));

    odyssey.update(pressed(eng::Intent::OpenMenu)); // Esc
    CHECK_FALSE(odyssey.playingWorld());
    CHECK(odyssey.mode() == game::Mode::Editor);
    CHECK(odyssey.region() == nullptr);
    CHECK(odyssey.life() == nullptr);
    CHECK(odyssey.worldName().empty());
    CHECK(odyssey.level().name == levelName);
    CHECK(odyssey.level().characters.size() == characters);
    CHECK(odyssey.editor().regionView().shown()); // the Region view with its edits
    CHECK(odyssey.editor().regionView().region()->biomeAt(made.lake.x, made.lake.y) == sim::Biome::Water);
    CHECK_FALSE(odyssey.editor().regionView().edits().placed.empty());

    // F2 does the same as Esc, and a second Play here works after the first.
    REQUIRE(odyssey.playWorldHere("default", made.far.x, made.far.y));
    odyssey.update(pressed(eng::Intent::ModeEditor));
    CHECK_FALSE(odyssey.playingWorld());
    CHECK(odyssey.mode() == game::Mode::Editor);
}

TEST_CASE("US-207 Play here: the Region view asks for it with P or the button, refuses water, and saves the world first") {
    Studio studio("us207-ask");
    const fs::path folder = game::worldsFolder(studio.data);
    std::vector<std::string> said;
    game::RegionView view(960, 540, [&said](const std::string& text) { said.push_back(text); });
    view.setWorldsFolder(folder, "asked");
    view.open(1, config(studio.data));
    sim::Region& land = *view.region();
    sim::Tile steppe{-1, -1};
    sim::Tile water{-1, -1};
    for (int y = 40; y < 200; ++y) {
        for (int x = 40; x < 200; ++x) {
            if (steppe.x < 0 && land.biomeAt(x, y) == sim::Biome::Steppe) steppe = {x, y};
            if (water.x < 0 && land.biomeAt(x, y) == sim::Biome::Water) water = {x, y};
        }
    }
    REQUIRE(steppe.x >= 0);
    REQUIRE(water.x >= 0);
    view.setZoom(game::RegionView::kMaxZoom);

    const auto press = [&view](const sim::Tile& tile) {
        view.centreOn(tile.x + 0.5, tile.y + 0.5);
        eng::Intents intents;
        eng::Pointer pointer;
        pointer.x = 480;
        pointer.y = 270;
        intents.setPointer(pointer);
        intents.set(eng::Intent::PlayHere, true, true);
        view.update(intents);
    };
    press(steppe);
    const std::optional<sim::Tile> asked = view.takePlayRequest();
    REQUIRE(asked.has_value());
    CHECK(asked->x == steppe.x);
    CHECK(asked->y == steppe.y);
    CHECK_FALSE(view.takePlayRequest().has_value()); // taken once

    said.clear();
    press(water);
    CHECK_FALSE(view.takePlayRequest().has_value());
    REQUIRE_FALSE(said.empty());
    CHECK(said.back().find("land") != std::string::npos);

    // The world file does not exist yet: saving before the play creates it; once saved and unchanged it is left alone.
    CHECK_FALSE(fs::exists(view.worldFile()));
    REQUIRE(view.saveBeforePlay());
    CHECK(fs::exists(view.worldFile()));
    const std::string hash = sim::worldFileHash(view.worldFile());
    REQUIRE(view.saveBeforePlay());
    CHECK(sim::worldFileHash(view.worldFile()) == hash);
}

TEST_CASE("US-207 World saves: a run save names its world, a changed file is a warning, and the run keeps the world it began in") {
    fs::path data;
    MadeWorld made;
    std::string startHash;
    {
        Studio first("us207-save");
        data = first.data;
        made = makeWorld(first.data, "test");
        REQUIRE(first.odyssey->startNewRun({1, 2, 1, "", "test"}, true, false));
        first.odyssey->run().close();
        CHECK(first.odyssey->clan()->food() == 80);
        REQUIRE(first.odyssey->autosave());
        startHash = first.odyssey->worldHash();
        const sim::HeroLife::SavedWorld saved = sim::HeroLife::savedWorld(first.saves / "hero.json");
        CHECK(saved.name == "test");
        CHECK(saved.hash == startHash);
        CHECK(fs::exists(first.saves / "world.json")); // the copy the run keeps
    }

    // The owner paints the lake dry and saves the world again, then loads the run.
    sim::WorldFile world = sim::loadWorld(game::worldFilePath(data, "test"), config(data));
    world.edits.tiles.erase(std::remove_if(world.edits.tiles.begin(), world.edits.tiles.end(), [&made](const sim::TileEdit& edit) { return edit.x == made.lake.x && edit.y == made.lake.y; }),
                            world.edits.tiles.end());
    world.setup.clans["player"].food = 5;
    sim::saveWorld(world, game::worldFilePath(data, "test"), config(data));
    REQUIRE(sim::worldFileHash(game::worldFilePath(data, "test")) != startHash);

    Studio second("us207-save", data);
    REQUIRE(second.odyssey->loadAutosave());
    CHECK(second.odyssey->message().find("was changed") != std::string::npos);
    CHECK(second.odyssey->worldName() == "test");
    CHECK(second.odyssey->worldHash() == startHash); // the run's own fingerprint, not the file's now
    REQUIRE(second.odyssey->region() != nullptr);
    CHECK(second.odyssey->region()->biomeAt(made.lake.x, made.lake.y) == sim::Biome::Water); // the lake is still there
    REQUIRE(second.odyssey->life() != nullptr);
    CHECK(second.odyssey->life()->game().world == "test");
    CHECK(second.odyssey->clan()->food() == 80); // from the save, not the changed file
    const auto& characters = second.odyssey->level().characters;
    CHECK(std::any_of(characters.begin(), characters.end(), [](const game::PlacedCharacter& c) { return c.name == "Tough Gob"; }));
    const auto& rivals = second.odyssey->rivals()->clans();
    CHECK(std::any_of(rivals.begin(), rivals.end(), [](const sim::RivalClan& clan) { return clan.name == "the Crow Clan" && clan.world->food() == 77; }));
}

TEST_CASE("US-207 Old saves: a run saved before region editing still loads, whether it is a version 2 or a version 1 hero file") {
    for (const int version : {2, 1}) {
        fs::path data;
        {
            Studio first("us207-old" + std::to_string(version));
            data = first.data;
            REQUIRE(first.odyssey->startNewRun({3, 2, 1}, true, false)); // generated land, seed 3
            first.odyssey->run().close();
            REQUIRE(first.odyssey->autosave());
            CHECK(first.odyssey->worldName().empty());
            const fs::path heroFile = first.saves / "hero.json";
            nlohmann::json hero = nlohmann::json::parse(readText(heroFile));
            CHECK(hero.at("world").get<std::string>().empty());
            hero["version"] = version;
            hero.erase("world");
            hero.erase("worldHash");
            if (version == 1) hero.erase("rules");
            writeText(heroFile, hero.dump(1));
            CHECK_FALSE(fs::exists(first.saves / "world.json")); // a run on generated land keeps no copy of a world
        }
        Studio second("us207-old" + std::to_string(version), data);
        REQUIRE(second.odyssey->loadAutosave());
        CHECK(second.odyssey->worldName().empty());
        REQUIRE(second.odyssey->region() != nullptr);
        CHECK(second.odyssey->region()->seed() == 3);
        REQUIRE(second.odyssey->life() != nullptr);
        CHECK(second.odyssey->life()->game().seed == 3);
        CHECK(second.odyssey->life()->game().world.empty());
        CHECK(second.odyssey->clan()->population() > 0);
    }
}

// ---- X-M12: the exit demonstration

TEST_CASE("X-M12 Exit: an edited region (terrain, a river, a camp moved, a clan's store and a grudge set) is saved as a small world file and a new game starts on it") {
    Studio studio("xm12-exit");
    const MadeWorld made = makeWorld(studio.data, "default"); // a lake, a person, a rival camp, the player's store and a grudge, saved
    const fs::path file = game::worldFilePath(studio.data, "default");

    // The owner opens the region again (the file is read back), draws a river, moves the camp and changes the Crow Clan's store.
    std::vector<std::string> said;
    game::RegionView view(960, 540, [&said](const std::string& text) { said.push_back(text); });
    view.setWorldsFolder(game::worldsFolder(studio.data), "default");
    view.open(1, config(studio.data));
    REQUIRE_FALSE(view.edits().placed.empty()); // the person and the camp came back from the file
    sim::Region& land = *view.region();
    const sim::Tile from{made.person.x - 3, made.person.y - 2};
    CHECK(view.paintRiver(from, {from.x, from.y + 6}, 1, 0) > 0);
    view.setTool(game::RegionTool::Move);
    REQUIRE(view.selectAt(made.camp.x, made.camp.y));
    int step = 1;
    while (step < 12 && !(land.biomeAt(made.camp.x + step, made.camp.y) == sim::Biome::Steppe && land.goodSite({made.camp.x + step, made.camp.y}))) ++step;
    REQUIRE(view.moveSelectedTo(made.camp.x + step, made.camp.y));
    REQUIRE(view.setClanField("the Crow Clan", "store", "food=90"));
    REQUIRE(view.saveWorld());

    // The file is small: differences from the seed only.
    CHECK(fs::file_size(file) < 20000);
    CHECK(fs::file_size(file) < 5000 + 200 * view.edits().tiles.size());

    // A new game on it has the river, the camp where it was moved, the stores and the grudge.
    game::OdysseyGame& odyssey = *studio.odyssey;
    REQUIRE(odyssey.startNewRun({5, 2, 1, "", "default"}, true, false));
    odyssey.run().close();
    CHECK(odyssey.region()->biomeAt(from.x, from.y + 3) == sim::Biome::Water);
    CHECK(odyssey.region()->biomeAt(made.lake.x, made.lake.y) == sim::Biome::Water);
    const auto& rivals = odyssey.rivals()->clans();
    const auto crow = std::find_if(rivals.begin(), rivals.end(), [](const sim::RivalClan& clan) { return clan.name == "the Crow Clan"; });
    REQUIRE(crow != rivals.end());
    CHECK(crow->camp.x == made.camp.x + step);
    CHECK(crow->world->food() == 90);
    CHECK(odyssey.clan()->food() == 80);
    const sim::Grudge* grudge = sim::heaviestGrudge(odyssey.clan()->people()[0], 1);
    REQUIRE(grudge != nullptr);
    CHECK(odyssey.clan()->chronicle().find(grudge->event)->text.find("stole the last flint") != std::string::npos);
    const auto& characters = odyssey.level().characters;
    CHECK(std::any_of(characters.begin(), characters.end(), [](const game::PlacedCharacter& c) { return c.name == "Tough Gob"; }));
}
