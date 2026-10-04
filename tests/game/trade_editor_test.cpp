// US-284 The Editor's trade section: the NPC panel, the Class panel and the Kinds tab set a trader's stock, restock per day, weighted deliveries, wants and rare goods; each
// change to an NPC is one step of Undo; play shows it; the test level's trader and wary hunter are set up.
#include "camp.h"

#include "game/game_rules.h"
#include "game/npc_class_book.h"
#include "sim/trade_market.h"

#include <memory>

using namespace camp_support;
namespace sim = odysseus::sim;

namespace {

luna::engine::Intents pressing(luna::engine::Intent intent) {
    luna::engine::Intents intents;
    intents.set(intent, true, true);
    return intents;
}

fs::path shippedLevel() { return fs::path(ODYSSEUS_DATA_DIR).parent_path() / "levels" / "npc-test.json"; }

struct Studio {
    fs::path data;
    fs::path file;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;
    int tala = 0;

    explicit Studio(const std::string& name, const fs::path& levelCopy = {}) : data(dataCopy(name)) {
        if (!levelCopy.empty()) {
            file = data / "npc-test.json";
            fs::copy_file(levelCopy, file);
        } else {
            const game::Definitions definitions = game::loadDefinitions(data);
            game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
            level.characters.clear();
            level.pickups.clear();
            game::PlacedCharacter trader{level.nextId++, "wanderer", {level.heroStart.x + 600, level.heroStart.y}, game::Facing::South, "Tala", 60, 4, {"trader"}};
            tala = trader.id;
            level.characters = {trader};
            file = data / "trade-editor-level.json";
            game::saveLevel(level, definitions, file);
        }
        odyssey = std::make_unique<game::OdysseyGame>(data, file);
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
        odyssey->update(pressing(luna::engine::Intent::ModeEditor));
        if (tala != 0) odyssey->editor().select(tala);
    }
    game::Editor& editor() { return odyssey->editor(); }
    const game::PlacedCharacter& named(const std::string& name) const {
        for (const game::PlacedCharacter& placed : odyssey->level().characters) {
            if (placed.name == name) return placed;
        }
        FAIL("the level has no ", name);
        return odyssey->level().characters.front();
    }
    void play(int ticks) {
        for (int i = 0; i < ticks; ++i) odyssey->update({});
    }
};

} // namespace

TEST_CASE("US-284 Edit: the owner gives the trader 3 furs, a restock of 1 flint a day and the want berries, saves, and play shows them and restocks one a day") {
    Studio studio("trade-editor-edit");
    game::Editor& editor = studio.editor();
    REQUIRE(editor.selectedIsNpc());
    CHECK(editor.setSelectedTrade("stock", "fur=3 flint=0"));
    CHECK(editor.setSelectedTrade("restock", "flint=1"));
    CHECK(editor.setSelectedTrade("wants", "berries"));
    REQUIRE(editor.save());
    const std::string text = readText(studio.file);
    CHECK(text.find("\"trade\"") != std::string::npos);
    CHECK(text.find("\"restockPerDay\"") != std::string::npos);
    CHECK(text.find("\"wants\"") != std::string::npos);
    const game::Level reread = game::loadLevel(studio.file, studio.odyssey->definitions()).level;
    CHECK(reread == editor.level());
    CHECK(reread.characters[0].extras.trade.stock == sim::ItemCounts{{"flint", 0}, {"fur", 3}});

    // Play: the furs are there, and a flint comes each day.
    studio.odyssey->update(pressing(luna::engine::Intent::ModeGame));
    const sim::TradeMarket& market = studio.odyssey->tradeMarket();
    CHECK(market.isTrader(studio.tala));
    CHECK(market.stock(studio.tala, "fur") == 3);
    CHECK(market.stock(studio.tala, "flint") == 0);
    CHECK(market.wants(studio.tala, "berries"));
    studio.play(2400 + 10);
    CHECK(market.stock(studio.tala, "flint") == 1);
    studio.play(2400);
    CHECK(market.stock(studio.tala, "flint") == 2);
}

TEST_CASE("US-284 Undo: every change to the NPC is one step of Undo, and a mistake changes nothing") {
    Studio studio("trade-editor-undo");
    game::Editor& editor = studio.editor();
    const std::size_t before = editor.history().size();
    CHECK(editor.setSelectedTrade("stock", "fur=3"));
    CHECK(editor.history().size() == before + 1);
    CHECK(editor.setSelectedTrade("picks", "2"));
    CHECK(editor.history().size() == before + 2);
    CHECK(editor.setSelectedTrade("rare", "obsidian=friendly"));
    CHECK(editor.history().size() == before + 3);
    CHECK(editor.level().characters[0].extras.trade.deliveries == 2);
    CHECK(editor.level().characters[0].extras.trade.rare.at("obsidian") == "friendly");
    CHECK(editor.undo());
    CHECK(editor.level().characters[0].extras.trade.rare.empty());
    CHECK(editor.undo());
    CHECK(editor.level().characters[0].extras.trade.deliveries == -1);
    CHECK(editor.redo());
    CHECK(editor.level().characters[0].extras.trade.deliveries == 2);

    const std::size_t steps = editor.history().size();
    CHECK_FALSE(editor.setSelectedTrade("stock", "fur=lots"));
    CHECK_FALSE(editor.setSelectedTrade("wants", "Berries"));
    CHECK_FALSE(editor.setSelectedTrade("rare", "obsidian=nice"));
    CHECK_FALSE(editor.setSelectedTrade("picks", "99"));
    CHECK_FALSE(editor.setSelectedTrade("price", "3"));
    CHECK(editor.history().size() == steps);
    CHECK(editor.level().characters[0].extras.trade.stock == sim::ItemCounts{{"fur", 3}});
    CHECK(editor.status().find("trade") != std::string::npos);
    // The same text again changes nothing and is no step.
    CHECK(editor.setSelectedTrade("stock", "fur=3"));
    CHECK(editor.history().size() == steps);
}

TEST_CASE("US-284 Class: the Class panel gives a class a default profile that every NPC of the class takes") {
    Studio studio("trade-editor-class");
    game::Editor& editor = studio.editor();
    editor.showClasses(true);
    editor.selectClass("trader");
    CHECK(editor.setClassTrade("stock", "flint=4 berries=6"));
    CHECK(editor.setClassTrade("picks", "2"));
    CHECK(editor.setClassTrade("weights", "fur=3"));
    CHECK(editor.setClassTrade("rare", "obsidian=friendly"));
    CHECK_FALSE(editor.setClassTrade("stock", "flint"));
    REQUIRE(editor.saveClass());
    const sim::rules::NpcClass* saved = studio.odyssey->npcClasses().catalog().find("trader");
    REQUIRE(saved != nullptr);
    CHECK(saved->extras.trade.stock == sim::ItemCounts{{"berries", 6}, {"flint", 4}});
    CHECK(saved->extras.trade.deliveries == 2);
    CHECK(readText(studio.data / "npc-classes" / "trader.json").find("\"trade\"") != std::string::npos);
    // Tala has none of her own, so she is a trader with the class's stock.
    const sim::rules::ResolvedNpc resolved = studio.odyssey->npcClasses().resolve(studio.named("Tala"));
    CHECK(resolved.extras.trade.stock.at("flint") == 4);
    CHECK(resolved.extras.trade.rare.at("obsidian") == "friendly");
    // F1: the class's profile is a stock in play.
    studio.odyssey->update(pressing(luna::engine::Intent::ModeGame));
    CHECK(studio.odyssey->tradeMarket().stock(studio.tala, "flint") == 4);
}

TEST_CASE("US-284 Kind: the Kinds tab gives a whole kind its wants and stock") {
    Studio studio("trade-editor-kind");
    game::Editor& editor = studio.editor();
    editor.showKinds(true);
    editor.selectKind("wanderer");
    CHECK(editor.setKindTrade("wants", "berries fur"));
    CHECK(editor.setKindTrade("stock", "fur=2"));
    REQUIRE(editor.saveKind());
    const sim::rules::ResolvedNpc resolved = studio.odyssey->npcClasses().resolve(studio.named("Tala"));
    CHECK(resolved.extras.trade.wants == std::vector<std::string>{"berries", "fur"});
    CHECK(resolved.extras.trade.stock.at("fur") == 2);
    CHECK(readText(studio.data / "npcs" / "wanderer.json").find("\"trade\"") != std::string::npos);
}

TEST_CASE("US-284 Test level: Tala trades, Harn keeps a spearhead for people he likes, and the region has shells") {
    Studio studio("trade-editor-level", shippedLevel());
    const game::PlacedCharacter& tala = studio.named("Tala");
    const game::PlacedCharacter& harn = studio.named("Harn");
    CHECK(studio.odyssey->level().economy.isCurrency("shells"));
    CHECK(studio.odyssey->level().economy.resources.at("berries") == 3);

    const sim::TradeMarket& market = studio.odyssey->tradeMarket();
    REQUIRE(market.isTrader(tala.id));
    REQUIRE(market.isTrader(harn.id));
    CHECK(market.stock(tala.id, "flint") == 6);
    CHECK(market.stock(tala.id, "fur") == 2);
    CHECK(market.wants(tala.id, "berries"));
    CHECK(market.stock(harn.id, "fur") == 4);
    CHECK(market.wants(harn.id, "flint"));
    CHECK(market.isRare(harn.id, "spearhead"));

    // Harn is wary (-45): the spearhead is kept back; a friendly hero (10 and over) is offered it.
    const sim::OpinionConfig& opinions = studio.odyssey->npcOpinions();
    const int opinion = studio.odyssey->npcPopulation().opinion(harn.id, sim::NpcPopulation::kHero);
    CHECK(opinion < 10);
    const auto offered = market.offeredGoods(harn.id, opinion, opinions);
    CHECK(std::find(offered.begin(), offered.end(), "spearhead") == offered.end());
    CHECK(std::find(offered.begin(), offered.end(), "fur") != offered.end());
    const auto friendly = market.offeredGoods(harn.id, 25, opinions);
    CHECK(std::find(friendly.begin(), friendly.end(), "spearhead") != friendly.end());
    const auto harnSubject = game::npcSubject(*studio.odyssey, harn.id);
    REQUIRE(harnSubject.has_value());
    CHECK(std::count(harnSubject->info.tags.begin(), harnSubject->info.tags.end(), "has-rare-goods") == 1);
    CHECK(std::count(harnSubject->info.tags.begin(), harnSubject->info.tags.end(), "rare-open") == 0);
    const auto talaSubject = game::npcSubject(*studio.odyssey, tala.id);
    REQUIRE(talaSubject.has_value());
    CHECK(std::count(talaSubject->info.tags.begin(), talaSubject->info.tags.end(), "trades") == 1);

    // Tala's restock: a flint a day, and a random delivery from her goods and the region's (berries and flint).
    studio.odyssey->update(pressing(luna::engine::Intent::ModeGame));
    studio.play(2400 + 10);
    CHECK(market.stock(tala.id, "flint") >= 7);
}
