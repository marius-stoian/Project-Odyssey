// US-191 Live catalogs (CI-007, CI-021): weapons, animals, effects and weather are read again while the game runs. What the play state holds of a definition is held by
// name and pointed at the new definition: a shot in the air, the starter weapons, the weather under way; a thing whose kind is gone is dropped or marked.
#include "camp.h"

#include "game/data_reload.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

using namespace camp_support;

namespace {

using luna::engine::Intent;
using luna::engine::Intents;
using luna::engine::Pointer;

// The text of the line of weapons.json that holds a weapon, replaced (or taken out when `with` is empty).
std::string replaceLine(std::string text, const std::string& name, const std::string& with) {
    const std::size_t at = text.find("{\"name\":\"" + name + "\"");
    REQUIRE_MESSAGE(at != std::string::npos, name);
    const std::size_t begin = text.rfind('\n', at) + 1;
    const std::size_t end = text.find('\n', at) + 1;
    return text.replace(begin, end - begin, with.empty() ? std::string() : with + "\n");
}

struct Studio {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;

    explicit Studio(const std::string& name) : data(dataCopy(name)) {
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        const game::PlacedCharacter first = level.characters.at(0);
        level.characters.clear();
        level.pickups.clear();
        game::PlacedCharacter goblin = first;
        goblin.id = 1;
        goblin.feet = {level.heroStart.x, level.heroStart.y + 4 * 32};
        goblin.hp = 100;
        goblin.swordDamage = 4;
        level.characters.push_back(goblin);
        game::saveLevel(level, definitions, data / "live-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "live-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
        tick(30); // the camera settles on the hero
    }
    void tick(int count = 1, Intents intents = {}) {
        for (int i = 0; i < count; ++i) odyssey->update(intents);
    }
    void hold(const std::string& weapon) {
        REQUIRE(odyssey->pickUp(weapon));
        odyssey->selectSlot(static_cast<int>(std::find(odyssey->hotbar().begin(), odyssey->hotbar().end(), weapon) - odyssey->hotbar().begin()));
    }
    void shootAtGoblin() {
        const game::Enemy& enemy = odyssey->enemies().at(0);
        Pointer pointer;
        const auto view = odyssey->cameraView();
        pointer.x = static_cast<int>(std::lround(enemy.feetX())) - view.x;
        pointer.y = static_cast<int>(std::lround(enemy.feetY())) - view.y;
        Intents intents;
        intents.set(Intent::Attack, true, true);
        intents.setPointer(pointer);
        tick(1, intents);
    }
    // What an Editor save or the watcher does: the file changed, the sets that watch it read it again.
    std::vector<game::ReloadOutcome> changed(const std::string& relative) {
        odyssey->fileChanged(data / relative);
        std::vector<game::ReloadOutcome> outcomes;
        for (const std::string& set : odyssey->dataReload().setsFor(data / relative)) {
            if (const game::ReloadOutcome* last = odyssey->dataReload().last(set)) outcomes.push_back(*last);
        }
        return outcomes;
    }
    std::string read(const std::string& relative) const { return readText(data / relative); }
    void write(const std::string& relative, const std::string& text) { writeText(data / relative, text); }
};

} // namespace

TEST_CASE("US-191 A weapon's damage changes while the game runs") {
    Studio studio("live-damage");
    studio.hold("iron sword");
    REQUIRE(studio.odyssey->heldWeapon() != nullptr);
    CHECK(studio.odyssey->heldWeapon()->damage == 5);
    std::string text = studio.read("weapons.json");
    const std::size_t at = text.find("\"damage\":5");
    REQUIRE(at != std::string::npos);
    text.replace(at, 10, "\"damage\":9");
    studio.write("weapons.json", text);
    const std::vector<game::ReloadOutcome> outcomes = studio.changed("weapons.json");
    REQUIRE(outcomes.size() == 1);
    CHECK(outcomes.front().set == "catalog");
    CHECK(outcomes.front().result.ok);
    CHECK_FALSE(outcomes.front().result.atNextStart);
    REQUIRE(studio.odyssey->heldWeapon() != nullptr); // the hotbar holds the name: the new definition is found at once
    CHECK(studio.odyssey->heldWeapon()->damage == 9);
    CHECK(studio.odyssey->catalogs().weapon("iron sword")->damage == 9);
}

TEST_CASE("US-191 A shot in the air follows the new definition") {
    Studio studio("live-shot");
    studio.hold("wooden longbow");
    studio.shootAtGoblin();
    REQUIRE_FALSE(studio.odyssey->arcShots().empty());
    const int before = studio.odyssey->catalogs().weapon("wooden longbow")->damage;
    std::string text = studio.read("weapons.json");
    const std::size_t line = text.find("{\"name\":\"wooden longbow\"");
    REQUIRE(line != std::string::npos);
    const std::size_t at = text.find("\"damage\":", line);
    const std::size_t end = text.find_first_of(",}", at);
    text.replace(at, end - at, "\"damage\":" + std::to_string(before + 7));
    studio.write("weapons.json", text);
    REQUIRE(studio.changed("weapons.json").front().result.ok);
    REQUIRE_FALSE(studio.odyssey->arcShots().empty());
    CHECK(studio.odyssey->arcShots().front().weapon == studio.odyssey->catalogs().weapon("wooden longbow")); // pointed at the new definition by its name
    CHECK(studio.odyssey->arcShots().front().weapon->damage == before + 7);
    studio.tick(40);
    CHECK(studio.odyssey->enemies().at(0).hp() <= 100 - (before + 7)); // the hit used the new number
}

TEST_CASE("US-191 A weapon that is gone takes its shots with it") {
    Studio studio("live-gone");
    studio.hold("throwing knives");
    studio.shootAtGoblin();
    REQUIRE_FALSE(studio.odyssey->arcShots().empty());
    studio.write("weapons.json", replaceLine(studio.read("weapons.json"), "throwing knives", ""));
    const std::vector<game::ReloadOutcome> outcomes = studio.changed("weapons.json");
    REQUIRE(outcomes.size() == 1);
    CHECK(outcomes.front().result.ok);
    CHECK(studio.odyssey->arcShots().empty());                 // no shot holds a definition that is gone
    CHECK(studio.odyssey->catalogs().weapon("throwing knives") == nullptr);
    CHECK(studio.odyssey->heldWeapon() == nullptr);              // the hand is empty-handed: the name is not a weapon any more
    studio.tick(60);                                             // and the game goes on without a crash
}

TEST_CASE("US-191 A new weapon is offered at once") {
    Studio studio("live-new");
    const std::vector<std::string> before = studio.odyssey->editor().weaponPalette();
    CHECK(std::find(before.begin(), before.end(), "test blade") == before.end());
    std::string text = studio.read("weapons.json");
    const std::string line = "    {\"name\":\"iron sword\",";
    const std::size_t at = text.find(line);
    REQUIRE(at != std::string::npos);
    const std::size_t end = text.find('\n', at) + 1;
    std::string copy = text.substr(at, end - at);
    copy.replace(copy.find("iron sword\",\"frame\""), 10, "test blade"); // the name only: the picture is the sword's
    text.insert(end, copy);
    studio.write("weapons.json", text);
    REQUIRE(studio.changed("weapons.json").front().result.ok);
    const game::WeaponDef* made = studio.odyssey->catalogs().weapon("test blade");
    REQUIRE(made != nullptr);
    const std::vector<std::string> after = studio.odyssey->editor().weaponPalette();
    CHECK(std::find(after.begin(), after.end(), "test blade") != after.end()); // a starter: the Editor's palette lists it
    CHECK(studio.odyssey->definitions().hasWeapon("test blade"));              // a level may place it
    studio.hold("test blade");
    CHECK(studio.odyssey->heldWeapon() == made);
}

TEST_CASE("US-191 A mistake keeps the old catalog") {
    Studio studio("live-mistake");
    std::string text = studio.read("weapons.json");
    const std::size_t at = text.find("\"damage\":5");
    REQUIRE(at != std::string::npos);
    text.replace(at, 10, "\"damage\":-5"); // out of range
    studio.write("weapons.json", text);
    const std::vector<game::ReloadOutcome> outcomes = studio.changed("weapons.json");
    REQUIRE(outcomes.size() == 1);
    CHECK_FALSE(outcomes.front().result.ok);
    CHECK(studio.odyssey->catalogs().weapon("iron sword")->damage == 5); // all or nothing: nothing was half applied
}

TEST_CASE("US-191 The weather goes on under the same name") {
    Studio studio("live-weather");
    const auto& weathers = studio.odyssey->catalogs().weather;
    REQUIRE(weathers.size() > 2);
    const std::string name = weathers.back().name;
    // Make the last weather the current one, then change a number of weather.json (a new weight for the first weather).
    REQUIRE(studio.odyssey->setWeatherNamed(name));
    CHECK(studio.odyssey->catalogs().weather[static_cast<std::size_t>(studio.odyssey->weather().current())].name == name);
    std::string text = studio.read("weather.json");
    const std::size_t at = text.find("\"weight\":");
    REQUIRE(at != std::string::npos);
    const std::size_t end = text.find_first_of(",}", at);
    text.replace(at, end - at, "\"weight\":" + std::to_string(40));
    studio.write("weather.json", text);
    REQUIRE(studio.changed("weather.json").front().result.ok);
    CHECK(studio.odyssey->catalogs().weather[static_cast<std::size_t>(studio.odyssey->weather().current())].name == name);
}

TEST_CASE("US-193 A copied plant is a plant of the game at once") {
    // The Data tab of the running game: copy the wheat, change its tags, save. The copy is in the catalog, in the Editor's list of plants, and its interactions are found.
    Studio studio("live-copy");
    game::DataEditor& tab = studio.odyssey->editor().data();
    tab.show(true);
    REQUIRE(tab.open("plants.json"));
    const auto& entries = tab.entries();
    const auto wheat = std::find_if(entries.begin(), entries.end(), [](const auto& entry) { return entry.label == "wheat"; });
    REQUIRE(wheat != entries.end());
    REQUIRE(tab.selectEntry(wheat->path));
    const std::vector<std::string> original = tab.interactionsOfEntry();
    CHECK(std::find(original.begin(), original.end(), "gather") != original.end()); // wheat is edible and a plant: it can be gathered
    REQUIRE(tab.copyEntry());
    REQUIRE(tab.setField(tab.entryPath() + ".inspect", "A copy of the wheat."));
    REQUIRE(tab.save());
    const game::PlantDef* made = studio.odyssey->catalogs().plant("wheat-copy");
    REQUIRE(made != nullptr);
    CHECK(made->inspect == "A copy of the wheat.");
    CHECK(studio.odyssey->definitions().hasPlant("wheat-copy")); // a level may place it
    const std::vector<std::string> copied = tab.interactionsOfEntry();
    CHECK(copied == original); // the copy offers the same interactions: the same tags
}
