// US-150: the game reads the interaction files when it starts.
#include "game/odyssey_game.h"

#include <doctest/doctest.h>

#include <string>

namespace game = odysseus::game;
namespace rules = odysseus::sim::rules;

namespace {

// Just enough world for conditions: ripe plants in summer.
class RipeSummer : public rules::RuleContext {
public:
    rules::Value path(const std::string& dotted) const override {
        if (dotted == "season") return rules::Value::ofText("summer");
        if (dotted == "target.state") return rules::Value::ofText("ripe");
        return rules::Value::ofNumber(0);
    }
    rules::Value call(const std::string&, const std::vector<rules::Value>&) const override { return rules::Value::ofNumber(0); }
};

} // namespace

TEST_CASE("US-150 The game registers gather at start and offers it on edible plants") {
    game::OdysseyGame odyssey(ODYSSEUS_DATA_DIR, ODYSSEUS_DEMO_LEVEL);
    for (const rules::Diagnostic& d : odyssey.interactionReport().errors) MESSAGE(d.text());
    CHECK(odyssey.interactionReport().errors.empty());
    CHECK(odyssey.interactionReport().loaded >= 1);
    REQUIRE(odyssey.interactions().find("gather") != nullptr);

    const rules::ThingInfo hero{"hero", {"hero", "person"}};
    const RipeSummer world;
    int edible = 0;
    for (const game::PlantDef& plant : odyssey.catalogs().plants) {
        // US-151 gives every plant its tags; until then a plant is tagged here the way its catalog flags say.
        rules::ThingInfo thing{plant.name, {"plant"}};
        if (plant.edible) thing.tags.push_back("edible");
        const auto offers = odyssey.interactions().offered(hero, thing, 1000, world);
        if (plant.edible) {
            ++edible;
            REQUIRE_MESSAGE(!offers.empty(), plant.name);
            CHECK(offers[0].interaction->id == "gather");
            CHECK(offers[0].enabled);
        } else {
            CHECK_MESSAGE(offers.empty(), plant.name);
        }
    }
    CHECK(edible == 31);
}
