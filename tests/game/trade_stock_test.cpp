// US-281 Trader stock in the game: a placed NPC with a trade profile (its own, its kind's or its class's) is a trader; its stock is limited, restocks once a day wherever
// the hero is, wants are known, and the stock is saved with the placed people and kept in the level file.
#include "camp.h"

#include "game/game_rules.h"
#include "game/npc_class_book.h"

#include <memory>

using namespace camp_support;
namespace sim = odysseus::sim;

namespace {

struct Market {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;
    int tala = 0;
    int ossa = 0;

    explicit Market(const std::string& name, bool clan = false) : data(dataCopy(name)) {
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        level.clan = clan;
        const game::PixelPoint hero = level.heroStart;
        game::PlacedCharacter trader{level.nextId++, "wanderer", {hero.x + 600, hero.y}, game::Facing::South, "Tala", 60, 4, {}};
        trader.classes = {"trader"};
        trader.extras.trade.stock = {{"fur", 3}, {"flint", 0}};
        trader.extras.trade.restockPerDay = {{"flint", 1}};
        trader.extras.trade.wants = {"berries"};
        tala = trader.id;
        game::PlacedCharacter talker{level.nextId++, "wanderer", {hero.x + 700, hero.y}, game::Facing::South, "Ossa", 60, 4, {}};
        ossa = talker.id;
        level.characters = {trader, talker};
        game::saveLevel(level, definitions, data / "market-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "market-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
    }
    void tick(int count) {
        for (int i = 0; i < count; ++i) odyssey->update({});
    }
};

} // namespace

TEST_CASE("US-281 Trader: an NPC with a trade profile is a trader and one without is not") {
    Market market("trade-stock-who");
    const sim::TradeMarket& stock = market.odyssey->tradeMarket();
    CHECK(stock.isTrader(market.tala));
    CHECK_FALSE(stock.isTrader(market.ossa));
    CHECK(stock.traderCount() == 1);
    CHECK(stock.stock(market.tala, "fur") == 3);
    CHECK(stock.stock(market.tala, "flint") == 0);
    CHECK(stock.wants(market.tala, "berries"));
    const auto subject = game::npcSubject(*market.odyssey, market.tala);
    REQUIRE(subject.has_value());
    CHECK(std::find(subject->info.tags.begin(), subject->info.tags.end(), "trader") != subject->info.tags.end());
    const auto talker = game::npcSubject(*market.odyssey, market.ossa);
    REQUIRE(talker.has_value());
    CHECK(std::find(talker->info.tags.begin(), talker->info.tags.end(), "trader") == talker->info.tags.end());
}

TEST_CASE("US-281 Restock: a day passes in play and the trader has one more flint, far from the hero") {
    Market market("trade-stock-restock");
    const sim::TradeMarket& stock = market.odyssey->tradeMarket();
    market.tick(2400 - 5);
    CHECK(stock.stock(market.tala, "flint") == 0); // the day has not ended yet
    market.tick(10);
    CHECK(stock.stock(market.tala, "flint") == 1);
    market.tick(2400);
    CHECK(stock.stock(market.tala, "flint") == 2);
    CHECK(stock.stock(market.tala, "fur") == 3);
}

TEST_CASE("US-281 Saved: the stock after a trade is kept by the autosave and read back") {
    Market market("trade-stock-save", true);
    const sim::TradeMarket& stock = market.odyssey->tradeMarket();
    market.tick(2400 + 100);
    REQUIRE(stock.stock(market.tala, "flint") == 1);
    market.odyssey->tradeMarketMutable().removeStock(market.tala, "fur", 2); // a trade happened
    const std::uint64_t hash = stock.hash();
    REQUIRE(market.odyssey->autosave());
    CHECK(fs::exists(market.odyssey->saveDirectory() / "trade.json"));
    market.odyssey->tradeMarketMutable().addStock(market.tala, "fur", 5); // change the live state, then load
    REQUIRE(market.odyssey->loadAutosave());
    CHECK(stock.stock(market.tala, "fur") == 1);
    CHECK(stock.stock(market.tala, "flint") == 1);
    CHECK(stock.hash() == hash);
}

TEST_CASE("US-281 Level: the trade block of a placed NPC is written to the level file and read back; a level without one still loads") {
    Market market("trade-stock-file");
    const fs::path file = market.data / "market-copy.json";
    game::saveLevel(market.odyssey->level(), market.odyssey->definitions(), file);
    const std::string text = readText(file);
    CHECK(text.find("\"trade\"") != std::string::npos);
    CHECK(text.find("\"restockPerDay\"") != std::string::npos);
    const game::Level reread = game::loadLevel(file, market.odyssey->definitions()).level;
    CHECK(reread == market.odyssey->level());
    CHECK(reread.characters[0].extras.trade.wants == std::vector<std::string>{"berries"});
    CHECK(reread.characters[1].extras.empty());

    // A mistake in the block names the file and the field.
    std::string broken = text;
    broken.replace(broken.find("\"flint\": 1"), 10, "\"Flint\": 1");
    writeText(market.data / "broken.json", broken);
    try {
        game::readLevelFile(market.data / "broken.json", market.odyssey->definitions());
        FAIL("a bad item id must be refused");
    } catch (const sim::DataError& error) {
        const std::string message = error.what();
        CHECK(message.find("broken.json") != std::string::npos);
        CHECK(message.find("characters[0]") != std::string::npos);
        CHECK(message.find("item id") != std::string::npos);
    }
}

TEST_CASE("US-281 Class: the Trader class can carry a default profile that every NPC of the class takes") {
    Market market("trade-stock-class");
    sim::rules::NpcClass trader = *market.odyssey->npcClasses().catalog().find("trader");
    trader.extras.trade.stock = {{"berries", 9}};
    trader.extras.trade.wants = {"fur"};
    REQUIRE_FALSE(market.odyssey->npcClasses().save(trader).has_value());
    // Ossa gets the class: now she is a trader too, with the class's stock and her own nothing.
    game::Level level = market.odyssey->level();
    level.characters[1].classes = {"trader"};
    const sim::rules::ResolvedNpc resolved = market.odyssey->npcClasses().resolve(level.characters[1]);
    CHECK(resolved.extras.trade.stock.at("berries") == 9);
    CHECK(resolved.extras.trade.wants == std::vector<std::string>{"fur"});
    CHECK(std::find(resolved.tags.begin(), resolved.tags.end(), "trader") != resolved.tags.end());
}
