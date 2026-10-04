// US-283 The trade screen for any trader: a placed NPC with a trade profile is traded with through one screen (barter and the region's money, the live balance bar,
// Haggle); coins become a balance when it opens and come back as coins when it closes; the rival camps keep their own barter.
#include "camp.h"

#include "game/game_rules.h"
#include "game/run_flow.h"

#include <format>
#include <memory>

using namespace camp_support;
namespace sim = odysseus::sim;

namespace {

struct Fair {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;
    int tala = 0;

    Fair(const std::string& name, const std::string& attitude, bool currency, bool wantsBerries = false) : data(dataCopy(name)) {
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.pickups.clear();
        level.characters.clear();
        level.clan = true;
        level.effects.push_back({level.nextId++, "flame", level.heroStart});
        if (currency) level.economy.currencies = {{"shells", 1}};
        game::PlacedCharacter trader{level.nextId++, "wanderer", {level.heroStart.x + 60, level.heroStart.y}, game::Facing::South, "Tala", 60, 4, {"trader"}};
        trader.attitude = attitude;
        trader.extras.trade.stock = {{"flint", 6}, {"fur", 4}};
        if (wantsBerries) trader.extras.trade.wants = {"berries"};
        tala = trader.id;
        level.characters = {trader};
        game::saveLevel(level, definitions, data / "fair-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "fair-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
        odyssey->startNewRun({1, 2, 1}, false, false); // preset 2 = "Off": the hero starts grown
        odyssey->run().close();
        REQUIRE(odyssey->life() != nullptr);
    }
    game::RunFlow& run() { return odyssey->run(); }
    sim::HeroLife& hero() { return *odyssey->life(); }
    void openTrade() {
        const auto subject = game::npcSubject(*odyssey, tala);
        REQUIRE(subject.has_value());
        REQUIRE(game::startInteraction(*odyssey, "trade", *subject));
        REQUIRE(run().screen() == game::Screen::Barter);
        run().press(*odyssey, 0); // build the screen (and take the coins into the balance)
    }
    // The widget of the screen whose label starts with the text, among the ids from low to high; -1 when there is none.
    int widget(const std::string& prefix, int low, int high) {
        run().press(*odyssey, 0);
        for (const auto& w : run().widgets()) {
            if (w.id >= low && w.id < high && w.label.rfind(prefix, 0) == 0) return w.id;
        }
        return -1;
    }
    bool pressLabel(const std::string& prefix, int low, int high, int times = 1) {
        for (int i = 0; i < times; ++i) {
            const int id = widget(prefix, low, high);
            if (id < 0 || !run().press(*odyssey, id)) return false;
        }
        return true;
    }
    bool shows(const std::string& text) {
        run().press(*odyssey, 0);
        for (const std::string& line : run().shownText()) {
            if (line.find(text) != std::string::npos) return true;
        }
        return false;
    }
};

} // namespace

TEST_CASE("US-283 Trade: berries for flint with a friendly trader; the goods move and the balance bar shows the deal") {
    Fair fair("trade-screen-trade", "friendly", false);
    const int berries = fair.hero().count("berries");
    fair.hero().give("berries", 10);
    const int flint = fair.hero().count("flint");
    fair.openTrade();
    CHECK(std::holds_alternative<game::NpcTrader>(fair.run().currentTrader()));
    CHECK(fair.shows("TRADE with Tala (friendly)"));
    CHECK(fair.shows("barter only"));
    // 10 berries (0.55 each) against 2 flint (2.7 each).
    REQUIRE(fair.pressLabel("Berries", game::RunFlow::kTradeGive, game::RunFlow::kTradeGiveLess, 9));
    REQUIRE(fair.pressLabel("Flint", game::RunFlow::kTradeGet, game::RunFlow::kTradeGetLess, 2));
    CHECK(fair.shows("They receive 4.95   They give 5.4"));
    CHECK(fair.run().message().empty()); // nothing was said yet
    int deal = fair.widget("Deal", game::RunFlow::kTradeDeal, game::RunFlow::kTradeDeal + 1);
    REQUIRE(deal == game::RunFlow::kTradeDeal);
    bool enabled = false;
    for (const auto& w : fair.run().widgets()) {
        if (w.id == game::RunFlow::kTradeDeal) enabled = w.enabled;
    }
    CHECK_FALSE(enabled); // nine berries are not enough
    REQUIRE(fair.pressLabel("Berries", game::RunFlow::kTradeGive, game::RunFlow::kTradeGiveLess, 1));
    CHECK(fair.shows("They accept"));
    REQUIRE(fair.run().press(*fair.odyssey, game::RunFlow::kTradeDeal));
    CHECK(fair.hero().count("berries") == berries);
    CHECK(fair.hero().count("flint") == flint + 2);
    CHECK(fair.odyssey->tradeMarket().stock(fair.tala, "flint") == 4);
    CHECK(fair.odyssey->tradeMarket().stock(fair.tala, "berries") == 10);
    CHECK(fair.run().message().find("It is a deal") != std::string::npos);
    CHECK(fair.run().deal().heroGives.empty()); // the table is cleared
    // Trading makes friends (D-52 Q-05).
    CHECK(fair.odyssey->npcPopulation().opinion(fair.tala, sim::NpcPopulation::kHero) == 30);
}

TEST_CASE("US-283 Currency: shells in the bag become a balance, buy a fur at the shown price, and the change comes back as shells") {
    Fair fair("trade-screen-currency", "neutral", true);
    fair.hero().give("shells", 20);
    const int fur = fair.hero().count("fur");
    fair.openTrade();
    CHECK(fair.hero().count("shells") == 0); // the coins are in the balance now
    CHECK(fair.odyssey->tradeMarket().heroBalance() == 20);
    CHECK(fair.shows("Your balance: 20"));
    REQUIRE(fair.pressLabel("Fur", game::RunFlow::kTradeGet, game::RunFlow::kTradeGetLess, 1));
    CHECK(fair.widget("Fur*", game::RunFlow::kTradeGet, game::RunFlow::kTradeGetLess) == -1); // the trader does not want it: no star
    // The fur is 5 at a neutral trader: pay 5 from the balance.
    REQUIRE(fair.run().press(*fair.odyssey, game::RunFlow::kTradePayMore5));
    CHECK(fair.shows("They receive 5   They give 5"));
    REQUIRE(fair.run().press(*fair.odyssey, game::RunFlow::kTradeDeal));
    CHECK(fair.hero().count("fur") == fur + 1);
    CHECK(fair.odyssey->tradeMarket().heroBalance() == 15);
    // Closing returns the rest as coin.
    REQUIRE(fair.run().press(*fair.odyssey, game::RunFlow::kClose));
    CHECK(fair.run().screen() == game::Screen::None);
    CHECK(fair.hero().count("shells") == 15);
    CHECK(fair.odyssey->tradeMarket().heroBalance() == 0);
}

TEST_CASE("US-283 None: a level without currency offers barter only: no balance to pay from") {
    Fair fair("trade-screen-none", "friendly", false);
    fair.openTrade();
    CHECK(fair.shows("This region has no money: barter only."));
    CHECK(fair.widget("Pay from balance", 0, 100000) == -1);
    CHECK(fair.widget("+5", game::RunFlow::kTradePayMore5, game::RunFlow::kTradePayMore5 + 1) == -1);
}

TEST_CASE("US-283 Hostile: a hostile trader has no Trade action and the screen says it will not trade") {
    Fair fair("trade-screen-hostile", "hostile", false);
    const auto subject = game::npcSubject(*fair.odyssey, fair.tala);
    REQUIRE(subject.has_value());
    REQUIRE(fair.run().openActions(*fair.odyssey, *subject));
    bool listed = false;
    for (const auto& entry : fair.run().contextEntries()) {
        if (entry.label == "Trade") {
            listed = true;
            CHECK(entry.reason == "They will not trade with you: they are hostile");
        }
    }
    CHECK(listed);
    fair.run().close();
    fair.run().openBarter(game::NpcTrader{fair.tala});
    CHECK(fair.shows("will not trade with you"));
}

TEST_CASE("US-283 Haggle: one try a day from the screen; the roll and the chance are shown") {
    Fair fair("trade-screen-haggle", "friendly", false);
    fair.openTrade();
    const int haggle = fair.widget("Haggle", game::RunFlow::kTradeHaggle, game::RunFlow::kTradeHaggle + 1);
    REQUIRE(haggle == game::RunFlow::kTradeHaggle);
    REQUIRE(fair.run().press(*fair.odyssey, haggle));
    CHECK(fair.run().message().find("% chance, rolled") != std::string::npos);
    // Used up for today: the button is greyed out and pressing it does nothing.
    bool enabled = true;
    fair.run().press(*fair.odyssey, 0);
    for (const auto& w : fair.run().widgets()) {
        if (w.id == game::RunFlow::kTradeHaggle) enabled = w.enabled;
    }
    CHECK_FALSE(enabled);
    CHECK_FALSE(fair.run().press(*fair.odyssey, game::RunFlow::kTradeHaggle));
}

TEST_CASE("US-283 Wants: a trader's wants are starred and paid at full value") {
    Fair fair("trade-screen-wants", "neutral", false, true);
    fair.hero().give("berries", 4);
    const int have = fair.hero().count("berries");
    fair.openTrade();
    CHECK(fair.widget(std::format("Berries 0/{} @1", have), game::RunFlow::kTradeGive, game::RunFlow::kTradeGiveLess) >= 0); // a want: the full value 1, not the half
    const sim::TradeMarket& market = fair.odyssey->tradeMarket();
    CHECK(market.wants(fair.tala, "berries"));
}

TEST_CASE("US-283 Rivals unchanged: a rival camp still has its own barter with its counter-offer and pay-later") {
    Fair fair("trade-screen-rival", "neutral", false);
    fair.hero().setRivals({{"Ash clan", 12}});
    fair.run().openBarter(0); // the old call with the number of the rival
    CHECK(std::holds_alternative<game::RivalTrader>(fair.run().currentTrader()));
    CHECK(fair.run().screen() == game::Screen::Barter);
    fair.run().press(*fair.odyssey, 0);
    const std::vector<std::string> text = fair.run().shownText();
    REQUIRE_FALSE(text.empty());
    CHECK(text[0].rfind("BARTER with Ash clan", 0) == 0);
    bool propose = false;
    bool payLater = false;
    for (const auto& w : fair.run().widgets()) {
        propose = propose || w.label == "Propose";
        payLater = payLater || w.label.rfind("Pay later", 0) == 0;
    }
    CHECK(propose);
    CHECK(payLater);
    // The currency item is not offered by the rival's list.
    for (const auto& w : fair.run().widgets()) CHECK(w.label.rfind("Shells", 0) != 0);
}
