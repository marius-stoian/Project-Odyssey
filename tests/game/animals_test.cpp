// US-137 Animals in the Editor: placed like characters, predators and boars fight, the others are bystanders.
#include "game/editor.h"
#include "game/level.h"
#include "game/odyssey_game.h"

#include "luna/engine/renderer.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;
namespace game = odysseus::game;
using luna::engine::Intent;
using luna::engine::Intents;
using luna::engine::Pointer;
using luna::engine::PointerButton;

namespace {

Intents pressing(Intent intent) {
    Intents intents;
    intents.set(intent, true, true);
    return intents;
}

Intents mouse(int x, int y, bool press, bool hold, bool release) {
    Intents intents;
    Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    const auto left = static_cast<std::size_t>(PointerButton::Left);
    pointer.pressed[left] = press;
    pointer.held[left] = hold;
    pointer.released[left] = release;
    intents.setPointer(pointer);
    return intents;
}

Intents attackToward(int x, int y) {
    Intents intents;
    Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    intents.setPointer(pointer);
    intents.set(Intent::Attack, true, true);
    return intents;
}

int kindIndex(const game::Definitions& definitions, const std::string& name) {
    for (std::size_t i = 0; i < definitions.characters.size(); ++i) {
        if (definitions.characters[i].name == name) return static_cast<int>(i);
    }
    FAIL("no such kind: " << name);
    return -1;
}

// The demo level with only these characters: (kind, tiles east of the hero start, tiles south).
fs::path levelWith(const std::string& name, const std::vector<std::tuple<std::string, int, int>>& animals) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us137" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    level.pickups.clear();
    level.characters.clear();
    for (const auto& [kind, east, south] : animals) {
        const game::CharacterKindDef& def = *definitions.character(kind);
        level.characters.push_back({level.nextId++, kind, {level.heroStart.x + east * 32, level.heroStart.y + south * 32}, game::Facing::West, kind, def.hp, def.swordDamage});
    }
    game::saveLevel(level, definitions, folder / "level.json");
    return folder / "level.json";
}

struct Play {
    game::OdysseyGame odyssey;
    luna::engine::RecordingRenderer renderer;
    explicit Play(const fs::path& level) : odyssey(ODYSSEUS_DATA_DIR, level) { odyssey.start(renderer); }
    void tick(int count = 1, Intents intents = {}) {
        for (int i = 0; i < count; ++i) odyssey.update(intents);
    }
    void hold(const std::string& weapon) {
        REQUIRE(odyssey.pickUp(weapon));
        odyssey.selectSlot(static_cast<int>(std::find(odyssey.hotbar().begin(), odyssey.hotbar().end(), weapon) - odyssey.hotbar().begin()));
    }
};

} // namespace

TEST_CASE("US-137 The fifty animals are character kinds") {
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    const game::Catalogs catalogs = game::loadCatalogs(ODYSSEUS_DATA_DIR);
    REQUIRE(catalogs.animals.size() == 50);
    int enemies = 0;
    for (const game::AnimalDef& animal : catalogs.animals) {
        CAPTURE(animal.name);
        const game::CharacterKindDef* kind = definitions.character(animal.name);
        REQUIRE(kind != nullptr);
        CHECK(kind->animal);
        CHECK(kind->hp == animal.hp);
        CHECK(kind->swordDamage == animal.strikeDamage);
        CHECK(kind->enemy == animal.enemy);
        if (animal.enemy) ++enemies;
    }
    CHECK(enemies == 20); // predators and boars (D-21)
    // The twelve characters of characters.json keep their places at the front.
    CHECK(definitions.characters[0].name == "hero");
    CHECK_FALSE(definitions.characters[0].animal);
    CHECK(definitions.characters.size() == 62);
}

TEST_CASE("US-137 Place") {
    const fs::path file = levelWith("place", {});
    Play play(file);
    play.odyssey.update(pressing(Intent::ModeEditor));
    game::Editor& editor = play.odyssey.editor();
    const game::Definitions& definitions = play.odyssey.definitions();
    const game::Catalogs catalogs = game::loadCatalogs(ODYSSEUS_DATA_DIR);
    // Every animal, one after the other, each in its own cell.
    editor.setTool(game::EditorTool::Place);
    int column = 0;
    for (const game::AnimalDef& animal : catalogs.animals) {
        editor.setTool(game::EditorTool::Place);
        editor.setKind(kindIndex(definitions, animal.name));
        const int screenX = 120 + (column % 8) * 26; // inside the map area: right of the palette, left of the properties panel
        const int screenY = 60 + (column / 8) * 25;
        play.odyssey.update(mouse(screenX, screenY, true, true, false));
        play.odyssey.update(mouse(screenX, screenY, false, false, true));
        ++column;
    }
    REQUIRE(editor.level().characters.size() == 50);
    for (std::size_t i = 0; i < catalogs.animals.size(); ++i) {
        const game::PlacedCharacter& placed = editor.level().characters[i];
        CHECK(placed.kind == catalogs.animals[i].name);
        CHECK(placed.hp == catalogs.animals[i].hp);
        CHECK(placed.swordDamage == catalogs.animals[i].strikeDamage);
        CHECK_FALSE(placed.name.empty());
    }
    // Saved and read back the same.
    CHECK(editor.save());
    CHECK(game::loadLevel(file, definitions).level.characters == editor.level().characters);
    // In the game they all stand in the world: the enemies as enemies, the others as bystanders.
    play.odyssey.update(pressing(Intent::ModeGame));
    CHECK(play.odyssey.enemies().size() + play.odyssey.bystanders().size() == 50);
    CHECK(play.odyssey.enemies().size() == 20);
    // The palette has pages, and the animals are on the later ones.
    play.odyssey.update(pressing(Intent::ModeEditor));
    editor.setTool(game::EditorTool::Place);
    play.odyssey.update({});
    REQUIRE(editor.kindPage() == 0);
    play.odyssey.update(mouse(50, 234, true, true, false)); // the ">" arrow under the first page
    play.odyssey.update(mouse(50, 234, false, false, true));
    play.odyssey.update({});
    CHECK(editor.kindPage() == 1);
}

TEST_CASE("US-137 Enemies") {
    // A grey wolf one metre east: hit, it strikes back half a second later, and it dies at 0 HP.
    Play play(levelWith("enemy", {{"grey wolf", 1, 0}}));
    play.tick(30);
    REQUIRE(play.odyssey.enemies().size() == 1);
    const game::Enemy& wolf = play.odyssey.enemies()[0];
    CHECK(wolf.animal);
    play.hold("iron sword");
    const int damage = play.odyssey.catalogs().weapon("iron sword")->damage;
    play.tick(1, attackToward(540, 270));
    CHECK(wolf.hp() == wolf.maxHp() - damage);
    CHECK(wolf.isWindingUp());
    play.tick(10);
    CHECK(play.odyssey.heroHp() == 100 - wolf.swordDamage); // it struck back like a goblin
    // Keep hitting: it falls.
    for (int i = 0; i < 400 && wolf.isAlive(); ++i) play.tick(1, attackToward(540, 270));
    CHECK_FALSE(wolf.isAlive());
    // It is drawn while it lives.
    Play drawn(levelWith("enemy-drawn", {{"grey wolf", 1, 0}}));
    drawn.tick(2);
    drawn.renderer.clear();
    drawn.odyssey.render(drawn.renderer, 1.0);
    const std::size_t with = drawn.renderer.draws().size();
    Play bare(levelWith("enemy-bare", {}));
    bare.tick(2);
    bare.renderer.clear();
    bare.odyssey.render(bare.renderer, 1.0);
    CHECK(with > bare.renderer.draws().size());
}

TEST_CASE("US-137 Bystanders") {
    // A deer, a cow and a rabbit next to the hero: a swing over all of them hurts nobody.
    Play play(levelWith("bystanders", {{"deer", 1, 0}, {"cow", 1, 1}, {"rabbit", 0, 1}}));
    play.tick(30);
    CHECK(play.odyssey.enemies().empty());
    REQUIRE(play.odyssey.bystanders().size() == 3);
    play.hold("iron sword");
    const auto before = play.odyssey.bystanders();
    play.tick(1, attackToward(540, 270));
    play.tick(20, attackToward(480, 335));
    play.tick(20, attackToward(420, 270));
    CHECK(play.odyssey.bystanders() == before); // nothing changed: no HP lost, none removed
    CHECK(play.odyssey.heroHp() == 100);
    // They are drawn all the same.
    play.renderer.clear();
    play.odyssey.render(play.renderer, 1.0);
    Play bare(levelWith("bystanders-bare", {}));
    bare.tick(2);
    bare.renderer.clear();
    bare.odyssey.render(bare.renderer, 1.0);
    CHECK(play.renderer.draws().size() >= bare.renderer.draws().size() + 3);
}
