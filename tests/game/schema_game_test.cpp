#include "core/text.h"
#include "game/catalogs.h"
#include "game/celestial.h"
#include "game/level.h"
#include "game/lighting.h"
#include "game/materials.h"
#include "game/sky.h"
#include "game/tutorial.h"
#include "sim/data.h"
#include "sim/schema.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <memory>
#include <string>

namespace fs = std::filesystem;
namespace schema = odysseus::sim::schema;
namespace game = odysseus::game;

namespace {

fs::path dataCopy(const std::string& name) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us190-game" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    fs::copy(ODYSSEUS_DATA_DIR, folder, fs::copy_options::recursive);
    return folder;
}

void installFrom(const fs::path& data) { schema::install(std::make_shared<schema::SchemaSet>(schema::SchemaSet::load(data / "schemas")), data); }

} // namespace

TEST_CASE("US-190 Game loaders") {
    // The game's own readers, with the schemas installed, still load every shipped file.
    const fs::path data = ODYSSEUS_DATA_DIR;
    installFrom(data);
    CHECK_NOTHROW((void)game::loadCatalogs(data));
    CHECK_NOTHROW((void)game::loadDefinitions(data));
    CHECK_NOTHROW((void)game::loadLighting(data / "light" / "lights.json"));
    CHECK_NOTHROW((void)game::loadSky(data / "light" / "sky.json", data / "sim" / "calendar.json"));
    CHECK_NOTHROW((void)game::loadCelestialEvents(data / "light" / "celestial-events.json"));
    CHECK_NOTHROW((void)game::loadMaterials(data));
    CHECK_NOTHROW((void)game::loadTutorial(data / "hero" / "tutorial.json"));
    schema::uninstall();
}

TEST_CASE("US-190 Game errors") {
    // A weapon's damage out of range stops the load, and the message names file, line, field and the allowed range.
    const fs::path data = dataCopy("errors");
    const fs::path weapons = data / "weapons.json";
    std::string text = *odysseus::core::readTextFile(weapons);
    const std::size_t at = text.find("\"damage\":5");
    REQUIRE(at != std::string::npos);
    text.replace(at, 10, "\"damage\":-4");
    odysseus::core::writeTextFileSafely(weapons, text);
    installFrom(data);
    std::string message;
    try {
        (void)game::loadCatalogs(data);
    } catch (const odysseus::sim::DataError& error) {
        message = error.what();
    }
    schema::uninstall();
    INFO(message);
    CHECK(message.find("weapons.json:") != std::string::npos);
    CHECK(message.find("damage") != std::string::npos);
    CHECK(message.find("must be between 0 and 1000 (is -4)") != std::string::npos);
}
