// US-280 Currencies per region in the Editor: the Economy panel marks which items are money (and sets market prices and delivery weights); each change is one
// step of Undo; the level file keeps it (level version 5) and a level without currency saves without an economy.
#include "camp.h"

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

    explicit Studio(const std::string& name) : data(dataCopy(name)) {
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        file = data / "economy-level.json";
        game::saveLevel(level, definitions, file);
        odyssey = std::make_unique<game::OdysseyGame>(data, file);
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
        odyssey->update(pressing(luna::engine::Intent::ModeEditor));
    }
    game::Editor& editor() { return odyssey->editor(); }
};

} // namespace

TEST_CASE("US-280 Set: the owner marks shells as currency with value 1, saves, and the level keeps it") {
    Studio studio("economy-set");
    game::Editor& editor = studio.editor();
    REQUIRE_FALSE(editor.level().economy.hasCurrency());
    CHECK(editor.setEconomyCurrencies("shells=1"));
    CHECK(editor.level().economy.isCurrency("shells"));
    CHECK(editor.level().economy.currencyValue("shells") == 1);
    CHECK(editor.setEconomyPrices("flint=4"));
    CHECK(editor.setEconomyResources("berries=5 flint=3"));
    CHECK(editor.save());

    const std::string text = readText(studio.file);
    CHECK(text.find("\"economy\"") != std::string::npos);
    CHECK(text.find("\"shells\": 1") != std::string::npos);
    CHECK(text.find("\"levelVersion\": 7") != std::string::npos);
    const game::Level reread = game::loadLevel(studio.file, studio.odyssey->definitions()).level;
    CHECK(reread.economy == editor.level().economy);
    CHECK(reread.economy.isCurrency("shells"));
    CHECK(reread == editor.level());

    // Playing it: the level the game runs has the money (trades in this level accept shells, US-283).
    studio.odyssey->update(pressing(luna::engine::Intent::ModeGame));
    CHECK(studio.odyssey->level().economy.isCurrency("shells"));
}

TEST_CASE("US-280 Undo: each table is one step of Undo, and the same text again is no step") {
    Studio studio("economy-undo");
    game::Editor& editor = studio.editor();
    const std::size_t before = editor.history().size();
    CHECK(editor.setEconomyCurrencies("shells=1 gold=10"));
    CHECK(editor.history().size() == before + 1);
    CHECK(editor.setEconomyCurrencies("gold=10 shells=1")); // the same table: nothing changes
    CHECK(editor.history().size() == before + 1);
    CHECK(editor.undo());
    CHECK_FALSE(editor.level().economy.hasCurrency());
    CHECK(editor.redo());
    CHECK(editor.level().economy.currencyValue("gold") == 10);
}

TEST_CASE("US-280 Mistake: a text that is not item=number changes nothing and says why") {
    Studio studio("economy-mistake");
    game::Editor& editor = studio.editor();
    CHECK(editor.setEconomyCurrencies("shells=1"));
    CHECK_FALSE(editor.setEconomyCurrencies("shells=free"));
    CHECK(editor.level().economy.currencyValue("shells") == 1); // as before
    CHECK(editor.status().find("currencies") != std::string::npos);
    CHECK_FALSE(editor.setEconomyPrices("Flint=4"));
    CHECK(editor.level().economy.prices.empty());
    CHECK(editor.setEconomyCurrencies("")); // an empty text means no currency: barter only
    CHECK_FALSE(editor.level().economy.hasCurrency());
}

TEST_CASE("US-280 None: a level with no currency is saved without an economy and offers barter only") {
    Studio studio("economy-none");
    game::Editor& editor = studio.editor();
    CHECK(editor.save());
    CHECK(readText(studio.file).find("\"economy\"") == std::string::npos);
    const game::Level reread = game::loadLevel(studio.file, studio.odyssey->definitions()).level;
    CHECK_FALSE(reread.economy.hasCurrency());
    CHECK(reread.economy.empty());
}

TEST_CASE("US-280 Panel: the Economy panel opens from the Level panel and shows the level's tables") {
    Studio studio("economy-panel");
    game::Editor& editor = studio.editor();
    CHECK_FALSE(editor.economyShown());
    editor.showEconomy(true);
    CHECK(editor.economyShown());
    editor.showEconomy(false);
    CHECK_FALSE(editor.economyShown());
}
