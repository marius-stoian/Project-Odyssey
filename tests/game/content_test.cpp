// US-130 Content catalogs from the new sheets.
#include "game/catalogs.h"
#include "game/content_art.h"

#include "luna/engine/image_ops.h"
#include "sim/data.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
namespace game = odysseus::game;
using luna::engine::Color;
using luna::engine::Image;

namespace {

fs::path assets() { return fs::path(ODYSSEUS_DATA_DIR).parent_path(); }

std::string problemWith(const fs::path& data) {
    try {
        game::loadCatalogs(data);
    } catch (const odysseus::sim::DataError& error) {
        return error.what();
    }
    return "";
}

} // namespace

TEST_CASE("US-130 Cut") {
    // The committed atlas holds every item listed in content-cuts.json, and every catalog
    // frame is in it.
    const auto cuts = game::loadContentCuts(assets() / "sprites" / "content-cuts.json");
    CHECK(cuts.cuts.size() == 653);
    std::string problem;
    const auto atlas = game::loadContent(assets() / "sprites" / "atlas", problem);
    REQUIRE_MESSAGE(atlas, problem);
    CHECK(atlas->frameCounts.size() == cuts.cuts.size());
    for (const auto& cut : cuts.cuts) {
        CHECK_MESSAGE(atlas->rect(atlas->frameName(cut.name, 0)).has_value(), cut.name);
    }
    const auto catalogs = game::loadCatalogs(ODYSSEUS_DATA_DIR, &*atlas);
    CHECK(catalogs.weapons.size() == 150);
    CHECK(catalogs.plants.size() == 153);
    CHECK(catalogs.animals.size() == 50);
    CHECK(catalogs.effects.size() == 200);
    CHECK(catalogs.weather.size() == 101); // the 100 of the sheet and "clear"

    // The starter set (D-21): one plain and one elemental weapon for each of the 8 classes.
    std::map<game::WeaponClass, int> plain;
    std::map<game::WeaponClass, int> elemental;
    std::set<game::Element> elements;
    for (const auto& weapon : catalogs.weapons) {
        if (!weapon.starter) continue;
        (weapon.element == game::Element::None ? plain : elemental)[weapon.weaponClass]++;
        elements.insert(weapon.element);
    }
    CHECK(plain.size() == 8);
    CHECK(elemental.size() == 8);
    for (const auto& [weaponClass, count] : plain) CHECK_MESSAGE(count == 1, game::weaponClassName(weaponClass));
    for (const auto& [weaponClass, count] : elemental) CHECK_MESSAGE(count == 1, game::weaponClassName(weaponClass));
    CHECK(elements.size() == 6); // none and all five elements

    // Enemies among the animals: predators and boars (D-21), 20 of them.
    int enemies = 0;
    for (const auto& animal : catalogs.animals) enemies += animal.enemy ? 1 : 0;
    CHECK(enemies == 20);
    CHECK(catalogs.animal("grey wolf")->enemy);
    CHECK_FALSE(catalogs.animal("deer")->enemy);
    // Clear sky is about a third of all weather.
    int total = 0;
    for (const auto& weather : catalogs.weather) total += weather.weight;
    CHECK(catalogs.weather.front().name == "clear");
    CHECK(catalogs.weather.front().weight * 3 == doctest::Approx(total).epsilon(0.05));
}

TEST_CASE("US-130 Keys") {
    SUBCASE("alpha: faint pixels go, the rest becomes solid") {
        Image picture(2, 1);
        picture.set(0, 0, Color{10, 20, 30, 100});
        picture.set(1, 0, Color{10, 20, 30, 200});
        luna::engine::keyAlpha(picture, 128, true);
        CHECK(picture.get(0, 0).alpha == 0);
        CHECK(picture.get(1, 0).alpha == 255);
    }
    SUBCASE("brightness: brighter than the background is more opaque") {
        Image picture(3, 1);
        picture.set(0, 0, Color{20, 20, 20});
        picture.set(1, 0, Color{60, 30, 20});
        picture.set(2, 0, Color{250, 250, 250});
        luna::engine::keyBrightness(picture, Color{20, 20, 20}, 3);
        CHECK(picture.get(0, 0).alpha == 0);
        CHECK(picture.get(1, 0).alpha == 120);
        CHECK(picture.get(2, 0).alpha == 255);
    }
    SUBCASE("colour: the background goes everywhere, holes too") {
        Image picture(3, 3);
        picture.fillRect(0, 0, 3, 3, Color{24, 26, 30});
        picture.set(1, 1, Color{200, 0, 0});
        picture.set(1, 0, Color{25, 26, 30});
        luna::engine::removeColour(picture, Color{24, 26, 30}, 10);
        CHECK(picture.get(1, 1).alpha == 255);
        CHECK(picture.get(1, 0).alpha == 0);
        CHECK(picture.get(2, 2).alpha == 0);
    }
    SUBCASE("main figure: a neighbour reaching in is dropped, a loose part kept") {
        Image picture(20, 10);
        picture.fillRect(8, 3, 6, 6, Color{200, 100, 50});  // the figure
        picture.fillRect(10, 0, 2, 2, Color{200, 100, 50}); // its loose antler, above it
        picture.fillRect(0, 4, 3, 3, Color{50, 50, 200});   // a neighbour's horn at the edge
        luna::engine::keepMainFigure(picture, {2, 0, 16, 10});
        CHECK(picture.get(10, 5).alpha == 255);
        CHECK(picture.get(10, 0).alpha == 255);
        CHECK(picture.get(1, 5).alpha == 0);
    }
}

TEST_CASE("US-130 Valid") {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us130";
    fs::remove_all(folder);
    fs::create_directories(folder);
    for (const char* name : {"weapons.json", "plants.json", "animals.json", "effects.json", "weather.json"}) {
        fs::copy_file(fs::path(ODYSSEUS_DATA_DIR) / name, folder / name);
    }
    CHECK(problemWith(folder).empty());
    // A weapon of a class that does not exist: the file and the field are named.
    std::ofstream(folder / "weapons.json", std::ios::trunc)
        << R"({"weapons": [{"name": "stick", "frame": "iron sword", "class": "club", "element": "none", "era": "fantasy", "starter": false, "damage": 1, "speed": 1, "range": 1}]})";
    const std::string problem = problemWith(folder);
    CHECK(problem.find("weapons.json") != std::string::npos);
    CHECK(problem.find("weapons[0].class") != std::string::npos);
    // A catalog frame the atlas does not have.
    std::ofstream(folder / "weapons.json", std::ios::trunc)
        << R"({"weapons": [{"name": "stick", "frame": "no such frame", "class": "sword", "element": "none", "era": "fantasy", "starter": false, "damage": 1, "speed": 1, "range": 1}]})";
    std::string atlasProblem;
    const auto atlas = game::loadContent(assets() / "sprites" / "atlas", atlasProblem);
    REQUIRE(atlas);
    CHECK_THROWS_WITH_AS(game::loadCatalogs(folder, &*atlas), doctest::Contains("weapons[0].frame"), odysseus::sim::DataError);
}

TEST_CASE("US-130 Review") {
    // The numbered sheets for the owner, and their name lists, match the cut list.
    const auto cuts = game::loadContentCuts(assets() / "sprites" / "content-cuts.json");
    const fs::path evidence = assets().parent_path() / "docs" / "evidence" / "US-130";
    for (const auto& page : cuts.pages) {
        CHECK_MESSAGE(fs::exists(evidence / (page.name + ".png")), page.name);
        std::ifstream list(evidence / (page.name + ".md"));
        REQUIRE_MESSAGE(list.good(), page.name);
        std::string line;
        std::size_t numbered = 0;
        while (std::getline(list, line)) numbered += !line.empty() && std::isdigit(static_cast<unsigned char>(line[0])) ? 1 : 0;
        std::size_t onPage = 0;
        for (const auto& cut : cuts.cuts) onPage += cut.page == page.name ? 1 : 0;
        CHECK_MESSAGE(numbered == onPage, page.name);
    }
}
