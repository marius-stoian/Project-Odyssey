// US-282 Reputation in the game: a trader that keeps rare goods back shows them only to a hero it likes, and the Actions pop-up says why; the hero's opinion decides.
#include "camp.h"

#include "game/game_rules.h"
#include "sim/npc_population.h"

#include <memory>

using namespace camp_support;
namespace sim = odysseus::sim;

namespace {

struct Studio {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;
    int wary = 0;
    int friendly = 0;

    explicit Studio(const std::string& name) : data(dataCopy(name)) {
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        const auto make = [&](const std::string& who, int east, const std::string& attitude) {
            game::PlacedCharacter placed{level.nextId++, "wanderer", {level.heroStart.x + east, level.heroStart.y}, game::Facing::South, who, 60, 4, {"trader"}};
            placed.attitude = attitude;
            placed.extras.trade.stock = {{"flint", 3}, {"obsidian", 1}};
            placed.extras.trade.rare = {{"obsidian", "friendly"}};
            return placed;
        };
        game::PlacedCharacter first = make("Harn", 60, "wary");
        game::PlacedCharacter second = make("Brek", 90, "friendly");
        wary = first.id;
        friendly = second.id;
        level.characters = {first, second};
        game::saveLevel(level, definitions, data / "gate-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "gate-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
    }
    std::vector<std::string> popupOf(int id) {
        const auto subject = game::npcSubject(*odyssey, id);
        REQUIRE(subject.has_value());
        REQUIRE(odyssey->run().openActions(*odyssey, *subject));
        std::vector<std::string> out;
        for (const auto& entry : odyssey->run().contextEntries()) out.push_back(entry.reason.empty() ? entry.label : entry.label + " | " + entry.reason);
        return out;
    }
    std::vector<std::string> menuOf(int id) {
        const game::PlacedCharacter* placed = odyssey->placedCharacter(id);
        REQUIRE(placed != nullptr);
        REQUIRE(odyssey->run().openContext(*odyssey, placed->feet.x, placed->feet.y - 20));
        std::vector<std::string> out;
        for (const auto& entry : odyssey->run().contextEntries()) out.push_back(entry.reason.empty() ? entry.label : entry.label + " | " + entry.reason);
        return out;
    }
};

bool has(const std::vector<std::string>& entries, const std::string& prefix) {
    return std::any_of(entries.begin(), entries.end(), [&](const std::string& e) { return e.rfind(prefix, 0) == 0; });
}

} // namespace

TEST_CASE("US-282 Gate: a wary hero is not offered the rare goods and the Actions pop-up says why") {
    Studio studio("trade-gate-wary");
    const sim::TradeMarket& market = studio.odyssey->tradeMarket();
    const int opinion = studio.odyssey->npcPopulation().opinion(studio.wary, sim::NpcPopulation::kHero);
    CHECK(opinion < 10);
    const auto offered = market.offeredGoods(studio.wary, opinion, studio.odyssey->npcOpinions());
    CHECK(std::find(offered.begin(), offered.end(), "flint") != offered.end());
    CHECK(std::find(offered.begin(), offered.end(), "obsidian") == offered.end());
    // The menu hides what the NPC cannot do for a reason of its own; the pop-up lists it with the reason.
    CHECK_FALSE(has(studio.menuOf(studio.wary), "Ask about rare goods"));
    studio.odyssey->run().close();
    const std::vector<std::string> popup = studio.popupOf(studio.wary);
    CHECK(has(popup, "Ask about rare goods | Rare goods are kept for people they like better"));
}

TEST_CASE("US-282 Reputation: a friendly trader shows its rare goods, and a gift that lifts the opinion opens them to the wary one") {
    Studio studio("trade-gate-friendly");
    CHECK(has(studio.menuOf(studio.friendly), "Ask about rare goods"));
    studio.odyssey->run().close();
    const auto subject = game::npcSubject(*studio.odyssey, studio.friendly);
    REQUIRE(subject.has_value());
    REQUIRE(game::startInteraction(*studio.odyssey, "rare-goods", *subject));
    CHECK(studio.odyssey->run().message().find("rare goods: obsidian") != std::string::npos);

    // Harn is wary (-45). Two gifts and some talk lift him past friendly (10).
    sim::NpcPopulation& people = studio.odyssey->npcPopulationMutable();
    people.adjust(studio.wary, sim::NpcPopulation::kHero, 60);
    CHECK(has(studio.popupOf(studio.wary), "Ask about rare goods"));
    CHECK_FALSE(has(studio.popupOf(studio.wary), "Ask about rare goods |"));
    studio.odyssey->run().close();
}

TEST_CASE("US-282 Tags: only a trader with a rare table has the tags that the rare-goods action needs") {
    Studio studio("trade-gate-tags");
    const auto wary = game::npcSubject(*studio.odyssey, studio.wary);
    REQUIRE(wary.has_value());
    const auto count = [&](const game::Subject& subject, const char* tag) { return std::count(subject.info.tags.begin(), subject.info.tags.end(), tag); };
    CHECK(count(*wary, "trader") >= 1);
    CHECK(count(*wary, "has-rare-goods") == 1);
    CHECK(count(*wary, "rare-open") == 0);
    const auto friendly = game::npcSubject(*studio.odyssey, studio.friendly);
    REQUIRE(friendly.has_value());
    CHECK(count(*friendly, "rare-open") == 1);
}
