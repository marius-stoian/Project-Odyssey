// US-152: the context menu is made from the interaction files and does what the old menu did.
#include "game/catalogs.h"
#include "game/level.h"
#include "game/odyssey_game.h"
#include "luna/engine/renderer.h"
#include "sim/data.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace game = odysseus::game;

namespace {

std::string readText(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void writeText(const fs::path& file, const std::string& text) { std::ofstream(file, std::ios::binary) << text; }

// A copy of the data folder, so a test may edit the interaction files.
fs::path dataCopy(const std::string& name) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us152" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    fs::copy(ODYSSEUS_DATA_DIR, folder / "data", fs::copy_options::recursive);
    return folder / "data";
}

luna::engine::Intents reloadPressed() {
    luna::engine::Intents intents;
    intents.set(luna::engine::Intent::Reload, true, true);
    return intents;
}

// A camp level: the clan lives here, its fire burns where the hero starts, and the run starts with no growing years (the "Off" preset).
struct Camp {
    luna::engine::RecordingRenderer renderer;
    fs::path data;
    game::OdysseyGame odyssey;

    static fs::path makeLevel(const fs::path& data, const std::string& name, bool wheat = false) {
        const fs::path folder = data.parent_path() / ("level-" + name);
        fs::create_directories(folder);
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.pickups.clear();
        level.characters.clear();
        level.clan = true;
        level.effects.push_back({level.nextId++, "flame", level.heroStart});
        if (wheat) level.plants.push_back({level.nextId++, "wheat", {level.heroStart.x + 32, level.heroStart.y}}); // 1 m east of the hero, ripe
        game::saveLevel(level, definitions, folder / "level.json");
        return folder / "level.json";
    }

    explicit Camp(const std::string& name, const fs::path& dataFolder = {}, bool wheat = false)
        : data(dataFolder.empty() ? dataCopy(name) : dataFolder), odyssey(data, makeLevel(data, name, wheat)) {
        odyssey.start(renderer);
        odyssey.startNewRun({1, 2, 1}, false, false); // preset 2 = "Off": the hero starts grown
        odyssey.run().close();
        REQUIRE(odyssey.life() != nullptr);
        REQUIRE(odyssey.life()->phase() == odysseus::sim::Phase::Free);
    }

    // Right-clicks a point and lists the menu: the label, or "label | reason" for a greyed-out item.
    std::vector<std::string> menuAt(double x, double y) {
        REQUIRE(odyssey.run().openContext(odyssey, x, y));
        std::vector<std::string> lines;
        for (const auto& entry : odyssey.run().contextEntries()) lines.push_back(entry.reason.empty() ? entry.label : entry.label + " | " + entry.reason);
        return lines;
    }
    void choose(std::size_t index) { odyssey.run().press(odyssey, game::RunFlow::kContextBase + static_cast<int>(index)); }

    game::PixelPoint fire() const { return odyssey.campPixels(); }
    // Plays n ticks of nothing, or of the given intents.
    void play(int n, const luna::engine::Intents& intents = {}) {
        for (int i = 0; i < n; ++i) odyssey.update(intents);
    }
    int berries() const { return odyssey.life()->count("berries"); }
    // The hero chooses Gather on the first plant (the menu item runs exactly this; the clan stands around the fire, so a click there may find a person).
    void gather() { REQUIRE(game::startInteraction(odyssey, "gather", game::plantSubject(odyssey, 0))); }
    // A clan member standing where a click finds them.
    int person() const {
        const auto& figures = odyssey.clanView().figures();
        for (std::size_t i = 0; i < figures.size(); ++i) {
            if (figures[i].present && odyssey.personAtWorld(figures[i].x, figures[i].y - 8) == static_cast<int>(i)) return static_cast<int>(i);
        }
        return -1;
    }
};

} // namespace

TEST_CASE("US-152 The same actions are offered as before: a person, the fire and the knapping stone") {
    Camp camp("menu-same");
    // A clan member.
    const int person = camp.person();
    REQUIRE(person >= 0);
    const auto& figure = camp.odyssey.clanView().figures()[static_cast<std::size_t>(person)];
    const bool near = std::hypot(figure.x - camp.odyssey.hero().feetX(), figure.y - camp.odyssey.hero().feetY()) <= 64;
    auto menu = camp.menuAt(figure.x, figure.y - 8);
    CHECK(camp.odyssey.run().contextTitle() == camp.odyssey.clan()->people()[static_cast<std::size_t>(person)].name);
    REQUIRE(menu.size() >= 2);
    CHECK(menu[0] == (near ? "Talk" : "Talk | Too far away"));
    CHECK(menu[1] == (near ? "Give berries | You have no berries" : "Give berries | Too far away"));
    for (std::size_t i = 2; i < menu.size(); ++i) CHECK_MESSAGE(menu[i].rfind("Ask to teach you ", 0) == 0, menu[i]);

    // The clan's fire.
    const game::PixelPoint fire = camp.fire();
    menu = camp.menuAt(fire.x, fire.y - 12);
    CHECK(camp.odyssey.run().contextTitle() == "The clan's fire");
    REQUIRE(menu.size() == 4);
    CHECK(menu[0] == "Craft at the fire");
    CHECK(menu[1] == "Eat berries | You have no berries");
    CHECK(menu[2] == "Tend the fire");
    CHECK(menu[3] == "Tend the sacred fire | No sacred fire yet");

    // The knapping stone, 4 m east and 2 m south of the fire: too far for the hero standing at the fire.
    const game::PixelPoint stone = camp.odyssey.knappingStone();
    menu = camp.menuAt(stone.x, stone.y - 10);
    CHECK(camp.odyssey.run().contextTitle() == "Knapping stone");
    REQUIRE(menu.size() == 1);
    CHECK(menu[0] == "Craft | Too far away");

    // Nothing there: no menu.
    CHECK_FALSE(camp.odyssey.run().openContext(camp.odyssey, fire.x + 600, fire.y + 600));
}

TEST_CASE("US-152 Choosing an item does what it did before") {
    Camp camp("menu-acts");
    const game::PixelPoint fire = camp.fire();
    auto& life = *camp.odyssey.life();

    camp.menuAt(fire.x, fire.y - 12);
    camp.choose(0); // Craft at the fire opens the craft screen
    CHECK(camp.odyssey.run().screen() == game::Screen::Craft);
    camp.odyssey.run().close();

    life.gatherBerries();
    const int berries = life.count("berries");
    REQUIRE(berries >= 2);
    auto menu = camp.menuAt(fire.x, fire.y - 12);
    CHECK(menu[1] == "Eat berries"); // possible now
    camp.choose(1);
    CHECK(life.count("berries") == berries - 1);
    CHECK_FALSE(camp.odyssey.run().message().empty());

    camp.menuAt(fire.x, fire.y - 12);
    camp.odyssey.run().setMessage("");
    camp.choose(2); // Tend the fire
    CHECK_FALSE(camp.odyssey.run().message().empty());

    // A greyed-out item does nothing.
    camp.menuAt(fire.x, fire.y - 12);
    camp.odyssey.run().setMessage("untouched");
    camp.choose(3); // Tend the sacred fire: no sacred fire yet
    CHECK(camp.odyssey.run().message() == "untouched");
    CHECK(camp.odyssey.run().screen() == game::Screen::Context);

    // Talking to a clan member, and giving them a berry.
    const int person = camp.person();
    REQUIRE(person >= 0);
    const auto& figure = camp.odyssey.clanView().figures()[static_cast<std::size_t>(person)];
    camp.menuAt(figure.x, figure.y - 8);
    camp.odyssey.run().setMessage("");
    camp.choose(0); // Talk
    CHECK_FALSE(camp.odyssey.run().message().empty());
    camp.menuAt(figure.x, figure.y - 8);
    camp.choose(1); // Give berries
    CHECK(life.count("berries") == berries - 2);
}

TEST_CASE("US-152 The sacred fire and its menu") {
    Camp camp("menu-sacred");
    auto& life = *camp.odyssey.life();
    const game::PixelPoint spot{camp.fire().x + 96, camp.fire().y};
    life.setSkillPoints(3, 30); // Fire-keeping level 3: the hero may found a sacred fire
    REQUIRE(life.foundFire("Hearth", spot.x / 32, spot.y / 32).ok);
    // At the clan's fire the sacred-fire item is possible now (it has no distance limit).
    auto menu = camp.menuAt(camp.fire().x, camp.fire().y - 12);
    REQUIRE(menu.size() == 4);
    CHECK(menu[3] == "Tend the sacred fire");
    // The sacred fire itself: 3 m from the hero, inside the 4 m reach, so both items are possible.
    menu = camp.menuAt((spot.x / 32) * 32 + 16, (spot.y / 32) * 32 + 16);
    CHECK(camp.odyssey.run().contextTitle() == "Sacred fire Hearth");
    REQUIRE(menu.size() == 2);
    CHECK(menu[0] == "Tend the fire");
    CHECK(menu[1] == "Hold a ritual");
}

TEST_CASE("US-152 The owner renames Tend the fire in tend-fire.json and F5 shows it") {
    const fs::path data = dataCopy("menu-rename");
    Camp camp("menu-rename", data);
    const game::PixelPoint fire = camp.fire();
    auto menu = camp.menuAt(fire.x, fire.y - 12);
    REQUIRE(menu.size() == 4);
    CHECK(menu[2] == "Tend the fire");

    const fs::path file = data / "interactions" / "tend-fire.json";
    std::string text = readText(file);
    const std::string from = "\"label\": \"Tend the fire\"";
    const std::size_t at = text.find(from);
    REQUIRE(at != std::string::npos);
    text.replace(at, from.size(), "\"label\": \"Feed the flames\"");
    writeText(file, text);
    camp.odyssey.update(reloadPressed());

    menu = camp.menuAt(fire.x, fire.y - 12);
    REQUIRE(menu.size() == 4);
    CHECK(menu[2] == "Feed the flames");
    CHECK(menu[0] == "Craft at the fire"); // everything else is as it was
}

TEST_CASE("US-152 A do naming a built-in action the game does not have is an error") {
    const fs::path data = dataCopy("menu-bad-do");
    writeText(data / "interactions" / "oops.json",
              "{ \"id\": \"oops\", \"label\": \"Oops\", \"actors\": [\"hero\"], \"target\": { \"tags\": [\"person\"] },\n \"effects\": [\"do talks\"] }");
    game::OdysseyGame odyssey(data, ODYSSEUS_DEMO_LEVEL);
    REQUIRE(odyssey.interactionReport().errors.size() == 1);
    CHECK(odyssey.interactionReport().errors[0].text().find("interactions/oops.json:2: do names \"talks\", which the game does not know (it knows: ") == 0);
    CHECK(odyssey.interactions().find("oops") == nullptr);
    CHECK(odyssey.interactions().find("talk") != nullptr); // the rest loaded
}

TEST_CASE("US-152 Every action the old menu had is now a file") {
    game::OdysseyGame odyssey(ODYSSEUS_DATA_DIR, ODYSSEUS_DEMO_LEVEL);
    REQUIRE(odyssey.interactionReport().errors.empty());
    REQUIRE(odyssey.interactionReport().warnings.empty());
    for (const char* id : {"talk", "give-berries", "ask-to-teach-hunter", "ask-to-teach-gatherer", "ask-to-teach-flint-knapper", "ask-to-teach-fire-keeper",
                           "ask-to-teach-shaman-healer", "craft-at-fire", "eat-berries", "tend-fire", "tend-sacred-fire-from-camp", "craft-at-stone",
                           "tend-sacred-fire", "hold-ritual", "barter", "gather", "knap", "pick-flint", "chop", "inspect"}) {
        CHECK_MESSAGE(odyssey.interactions().find(id) != nullptr, id);
    }
    // Every built-in action is used by some file (and every `do` names one the game has: checked at load).
    std::set<std::string> used;
    for (const auto& interaction : odyssey.interactions().all()) {
        for (const auto& effect : interaction.effects) {
            if (effect.verb == "do") used.insert(effect.args[0]->text);
        }
    }
    for (const std::string& name : game::builtInActionNames()) CHECK_MESSAGE(used.count(name) == 1, name);
}

// ---- US-153: timed actions and world state

TEST_CASE("US-153 Gather takes three seconds under a ring, then gives berries and hides the plant until it is ripe again") {
    Camp camp("timed-gather", {}, true);
    REQUIRE(camp.odyssey.plants().size() == 1);
    const game::WorldPlant& plant = camp.odyssey.plants()[0];
    REQUIRE(plant.state == "ripe");
    const int before = camp.berries();

    CHECK(camp.odyssey.plantOffers(0)[0].interaction->id == "gather");
    camp.gather(); // what choosing Gather in the menu does
    REQUIRE(camp.odyssey.actions().running(0) != nullptr);
    CHECK(camp.berries() == before); // nothing yet

    // The ring is on screen while it fills.
    camp.renderer.clear();
    camp.odyssey.render(camp.renderer, 0.0);
    const std::size_t withRing = camp.renderer.draws().size();

    camp.play(30); // 1.5 s
    CHECK(camp.odyssey.actions().progress(0, camp.odyssey.actionClock()) == 50);
    CHECK(camp.berries() == before);
    camp.play(29);
    CHECK(camp.berries() == before); // one tick short
    camp.play(1);
    CHECK(camp.berries() == before + 2); // after 3 s
    CHECK(camp.odyssey.actions().running(0) == nullptr);
    CHECK(plant.state == "picked");
    CHECK_FALSE(plant.present()); // hidden
    CHECK(camp.odyssey.plantAtWorld(plant.feet.x, plant.feet.y - 8) == -1);

    camp.renderer.clear();
    camp.odyssey.render(camp.renderer, 0.0);
    CHECK(camp.renderer.draws().size() < withRing); // the ring and the plant are gone

    // Greyed out while it waits, even though it cannot be clicked: asked directly.
    const auto offers = camp.odyssey.plantOffers(0);
    CHECK_FALSE(offers[0].enabled);
    CHECK(offers[0].reason == "Nothing to pick yet");

    camp.play(299);
    CHECK(plant.state == "picked");
    camp.play(1); // 15 s after the pick
    CHECK(plant.state == "ripe");
    CHECK(plant.present());
    CHECK(camp.odyssey.plantAtWorld(plant.feet.x, plant.feet.y - 8) == 0); // back in the same spot
}

TEST_CASE("US-153 Moving or attacking stops a timed action and it gives nothing") {
    for (const luna::engine::Intent stopper : {luna::engine::Intent::MoveRight, luna::engine::Intent::Attack}) {
        Camp camp("timed-interrupt", {}, true);
        const game::WorldPlant& plant = camp.odyssey.plants()[0];
        const int before = camp.berries();
        camp.gather();
        camp.play(20);
        REQUIRE(camp.odyssey.actions().running(0) != nullptr);
        luna::engine::Intents stop;
        stop.set(stopper, true, true);
        camp.play(1, stop);
        CHECK(camp.odyssey.actions().running(0) == nullptr);
        camp.play(200);
        CHECK(camp.berries() == before);
        CHECK(plant.state == "ripe");
        CHECK(plant.present());
    }
}

TEST_CASE("US-153 Standing still lets the action finish; opening a screen pauses it") {
    Camp camp("timed-pause", {}, true);
    const int before = camp.berries();
    camp.gather();
    camp.play(20);
    camp.odyssey.run().openMenu(); // the world waits while a screen is open
    camp.play(100);
    CHECK(camp.berries() == before);
    CHECK(camp.odyssey.actions().running(0) != nullptr);
    camp.odyssey.run().close();
    camp.play(40);
    CHECK(camp.berries() == before + 2);
}

TEST_CASE("US-153 A picked bush and a lit fire are still there after a save and a load") {
    const fs::path data = dataCopy("timed-save");
    Camp camp("timed-save", data, true);
    auto& life = *camp.odyssey.life();
    life.setSkillPoints(3, 30);
    const game::PixelPoint spot{camp.fire().x + 96, camp.fire().y};
    REQUIRE(life.foundFire("Hearth", spot.x / 32, spot.y / 32).ok);
    REQUIRE(life.fire().lit);

    const game::WorldPlant& plant = camp.odyssey.plants()[0];
    camp.gather();
    camp.play(60);
    REQUIRE(plant.state == "picked");
    camp.play(100); // 5 s into the ripening: 10 s are left
    REQUIRE(camp.odyssey.autosave());
    const int berries = camp.berries();

    // A new game over the same folder, as after closing and starting the game again.
    luna::engine::RecordingRenderer renderer;
    game::OdysseyGame again(data, Camp::makeLevel(data, "timed-save", true));
    again.start(renderer);
    REQUIRE(again.loadAutosave());
    REQUIRE(again.plants().size() == 1);
    CHECK(again.plants()[0].state == "picked"); // still picked
    CHECK_FALSE(again.plants()[0].present());
    REQUIRE(again.life() != nullptr);
    CHECK(again.life()->fire().founded); // the fire is still lit
    CHECK(again.life()->fire().lit);
    CHECK(again.life()->count("berries") == berries);

    for (int i = 0; i < 199; ++i) again.update({});
    CHECK(again.plants()[0].state == "picked");
    again.update({}); // 10 s after loading: the 15 s are over
    CHECK(again.plants()[0].state == "ripe");
}

TEST_CASE("US-153 Things that never changed save nothing, and a damaged things file is reported") {
    const fs::path data = dataCopy("timed-clean");
    Camp camp("timed-clean", data, true);
    REQUIRE(camp.odyssey.autosave());
    const std::string text = readText(data.parent_path() / "saves" / "things.json");
    CHECK(text.find("\"plants\": {}") != std::string::npos); // a ripe plant is the default: not written
    CHECK(text.find("\"pending\": []") != std::string::npos);

    writeText(data.parent_path() / "saves" / "things.json", "{ not json");
    luna::engine::RecordingRenderer renderer;
    game::OdysseyGame again(data, Camp::makeLevel(data, "timed-clean", true));
    again.start(renderer);
    CHECK(again.loadAutosave()); // the clan and the hero still load
    CHECK(again.message().find("things.json could not be read") != std::string::npos);
}
