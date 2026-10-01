// Shared by the menu and NPC tests (US-152..US-154): a copy of the data folder, and a camp level with a clan, a run and a hero.
#pragma once
#include "game/catalogs.h"
#include "game/editor.h"
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

namespace camp_support {

inline std::string readText(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

inline void writeText(const fs::path& file, const std::string& text) { std::ofstream(file, std::ios::binary) << text; }

// A copy of the data folder, so a test may edit the interaction files.
inline fs::path dataCopy(const std::string& name) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us152" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    fs::copy(ODYSSEUS_DATA_DIR, folder / "data", fs::copy_options::recursive);
    return folder / "data";
}

inline luna::engine::Intents reloadPressed() {
    luna::engine::Intents intents;
    intents.set(luna::engine::Intent::Reload, true, true);
    return intents;
}

// Things to put in a camp level, in tiles east and south of the hero's start (and so of the clan's fire).
struct Spec {
    struct At {
        std::string kind;
        int east = 0;
        int south = 0;
    };
    std::vector<At> plants;
    std::vector<At> characters; // animals and monsters of characters.json and animals.json
};

// A camp level: the clan lives here, its fire burns where the hero starts, and the run starts with no growing years (the "Off" preset).
struct Camp {
    luna::engine::RecordingRenderer renderer;
    fs::path data;
    game::OdysseyGame odyssey;

    static fs::path makeLevel(const fs::path& data, const std::string& name, bool wheat = false, const std::string& object = {}, const Spec& spec = {}) {
        const fs::path folder = data.parent_path() / ("level-" + name);
        fs::create_directories(folder);
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.pickups.clear();
        level.characters.clear();
        level.clan = true;
        level.effects.push_back({level.nextId++, "flame", level.heroStart});
        if (wheat) level.plants.push_back({level.nextId++, "wheat", {level.heroStart.x + 32, level.heroStart.y}}); // 1 m east of the hero, ripe
        if (!object.empty()) level.plants.push_back({level.nextId++, object, {level.heroStart.x + 32, level.heroStart.y}}); // a world object 1 m east of the hero
        for (const Spec::At& at : spec.plants) level.plants.push_back({level.nextId++, at.kind, {level.heroStart.x + 32 * at.east, level.heroStart.y + 32 * at.south}});
        for (const Spec::At& at : spec.characters) {
            const game::CharacterKindDef* def = definitions.character(at.kind);
            REQUIRE_MESSAGE(def != nullptr, at.kind);
            level.characters.push_back({level.nextId++, at.kind, {level.heroStart.x + 32 * at.east, level.heroStart.y + 32 * at.south}, game::Facing::West, at.kind, def->hp, def->swordDamage});
        }
        game::saveLevel(level, definitions, folder / "level.json");
        return folder / "level.json";
    }

    explicit Camp(const std::string& name, const fs::path& dataFolder = {}, bool wheat = false, const std::string& object = {}, const Spec& spec = {})
        : data(dataFolder.empty() ? dataCopy(name) : dataFolder), odyssey(data, makeLevel(data, name, wheat, object, spec)) {
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

} // namespace camp_support
