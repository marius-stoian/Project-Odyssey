#include "game/materials.h"
#include "game/odyssey_game.h"
#include "game/spear_range.h"
#include "game/test_map.h"
#include "luna/engine/physics_view.h"
#include "sim/data.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using luna::engine::Intent;
using luna::engine::Intents;
using luna::engine::toDouble;
using luna::physics::Vec3;
using odysseus::game::HitKind;
using odysseus::game::OdysseyGame;
using odysseus::game::SpearState;
namespace game = odysseus::game;

namespace {

Intents pressing(Intent intent) {
    Intents intents;
    intents.set(intent, true, true);
    return intents;
}

Intents holding(Intent intent) {
    Intents intents;
    intents.set(intent, true, false);
    return intents;
}

bool sameRect(const odysseus::core::Rect& a, const odysseus::core::Rect& b) {
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

// Throws one spear of `kind` from the hero's start west towards the open target; returns the hit.
game::SpearHit throwAtOpenTarget(const std::string& kind) {
    game::SpearRange range(game::makeTestMap(), game::loadMaterials(ODYSSEUS_DATA_DIR));
    range.addTarget(OdysseyGame::openTargetBase());
    const Vec3 heroFeet{luna::physics::Fixed::fromRatio(65, 2), luna::physics::Fixed::fromRatio(131, 4), {}};
    range.throwSpear(heroFeet, game::Facing::West, range.materials().spear(kind));
    for (int tick = 0; tick < 100; ++tick) {
        const auto hits = range.update();
        if (!hits.empty()) {
            return hits.front();
        }
    }
    FAIL("the spear never hit anything");
    return {};
}

} // namespace

TEST_CASE("US-029 Throw") {
    // The straw target stands 8 tiles west of the hero's start; one step left faces it.
    OdysseyGame odyssey(ODYSSEUS_DATA_DIR, ODYSSEUS_DEMO_LEVEL);
    luna::engine::RecordingRenderer renderer;
    odyssey.start(renderer);
    odyssey.update(holding(Intent::MoveLeft));
    REQUIRE(odyssey.hero().facing() == game::Facing::West);
    const double distance = toDouble(luna::engine::metresFromPixels(odyssey.hero().feetX()) - OdysseyGame::openTargetBase().x);
    MESSAGE("target distance: ", distance, " m");
    CHECK(distance > 7.8);
    odyssey.update(pressing(Intent::Interact));
    REQUIRE(odyssey.range().spears().size() == 1);

    std::vector<double> heights;
    int shadowsBelowSpear = 0;
    for (int tick = 0; tick < 60 && odyssey.range().allHits().empty(); ++tick) {
        const auto& spear = odyssey.range().spears().front();
        heights.push_back(toDouble(spear.body.position.z));
        renderer.clear();
        odyssey.render(renderer, 1.0);
        // The spear sprite is lifted by its height; its shadow stays on the ground below it.
        const auto& draws = renderer.draws();
        // Textures are numbered in creation order: characters 0, tiles 1, props 2.
        constexpr int kProps = 2;
        const auto shadow = std::find_if(draws.begin(), draws.end(), [](const auto& d) {
            return d.texture == kProps && sameRect(d.source, game::kShadowFrame);
        });
        const auto sprite = std::find_if(draws.begin(), draws.end(), [](const auto& d) {
            return d.texture == kProps && d.source.height == game::kSpearFrameSize;
        });
        REQUIRE(shadow != draws.end());
        REQUIRE(sprite != draws.end());
        const double shadowCentreY = shadow->at.y + game::kShadowFrame.height / 2.0;
        const double spriteCentreY = sprite->at.y + game::kSpearFrameSize / 2.0;
        if (std::abs((shadowCentreY - spriteCentreY) - heights.back() * 32.0) <= 2.0) {
            ++shadowsBelowSpear;
        }
        odyssey.update(Intents{});
    }

    // It hit the target...
    REQUIRE(odyssey.range().allHits().size() == 1);
    const game::SpearHit& hit = odyssey.range().allHits().front();
    CHECK(hit.kind == HitKind::Target);
    CHECK(hit.target == 0);
    CHECK(hit.damage > luna::physics::kFixedZero);
    CHECK(odyssey.range().targets()[0].hits == 1);
    // ...after flying in an arc: up from the hand, over the top, and down again.
    REQUIRE(heights.size() > 4);
    const double top = *std::max_element(heights.begin(), heights.end());
    MESSAGE("flight: ", heights.size(), " ticks, hand ", heights.front(), " m, top ", top, " m, hit at ",
            toDouble(hit.point.z), " m, ", toDouble(luna::physics::length(hit.velocity)), " m/s");
    CHECK(top > heights.front() + 0.1);
    CHECK(heights.back() < top - 0.1);
    // Every frame drew the shadow exactly the spear's height (x 32 px) below the spear.
    CHECK(shadowsBelowSpear == static_cast<int>(heights.size()));
}

TEST_CASE("US-029 Material") {
    const game::SpearHit flint = throwAtOpenTarget("flint");
    const game::SpearHit wooden = throwAtOpenTarget("wooden");
    REQUIRE(flint.kind == HitKind::Target);
    REQUIRE(wooden.kind == HitKind::Target);

    // The configured formula, computed here with the numbers in materials.json:
    // mass = 1.2 kg shaft + 0.00006 m^3 x tip density; damage = 1/2 m v^2 x hardness / 10 x sharpness.
    auto expected = [](double density, double hardness, double sharpness, const game::SpearHit& hit) {
        const double mass = 1.2 + 0.00006 * density;
        const double speed = toDouble(luna::physics::length(hit.velocity));
        return 0.5 * mass * speed * speed * hardness / 10.0 * sharpness;
    };
    const double flintExpected = expected(2600, 7.0, 0.9, flint);
    const double woodenExpected = expected(700, 3.0, 0.4, wooden);
    MESSAGE("flint spear: ", toDouble(flint.damage), " damage (formula ", flintExpected, "); wooden spear: ",
            toDouble(wooden.damage), " (formula ", woodenExpected, ")");
    CHECK(std::abs(toDouble(flint.damage) - flintExpected) / flintExpected < 1e-6);
    CHECK(std::abs(toDouble(wooden.damage) - woodenExpected) / woodenExpected < 1e-6);
    CHECK(toDouble(flint.damage) > 4 * toDouble(wooden.damage)); // harder, denser, sharper
}

TEST_CASE("US-029 Materials are validated") {
    // A broken copy of materials.json: the error names the file and the field (ARC-08).
    const std::filesystem::path folder = std::filesystem::temp_directory_path() / "odysseus-us029-materials";
    std::filesystem::create_directories(folder);
    std::ifstream in(std::filesystem::path(ODYSSEUS_DATA_DIR) / "materials.json");
    std::stringstream text;
    text << in.rdbuf();
    std::string broken = text.str();
    broken.replace(broken.find("\"hardness\": 7.0"), 15, "\"hardness\": 12.0");
    std::ofstream(folder / "materials.json") << broken;
    try {
        (void)game::loadMaterials(folder);
        FAIL("the broken file was accepted");
    } catch (const odysseus::sim::DataError& error) {
        const std::string message = error.what();
        MESSAGE(message);
        CHECK(message.find("materials.json") != std::string::npos);
        CHECK(message.find("materials.flint.hardness") != std::string::npos);
    }
    std::filesystem::remove_all(folder);
}

TEST_CASE("US-029 Blocked") {
    // The second target stands 8 tiles north, right behind a boulder.
    OdysseyGame odyssey(ODYSSEUS_DATA_DIR, ODYSSEUS_DEMO_LEVEL);
    odyssey.update(holding(Intent::MoveUp)); // turn to face North (one step)
    REQUIRE(odyssey.hero().facing() == game::Facing::North);
    odyssey.update(pressing(Intent::Interact));
    for (int tick = 0; tick < 60 && odyssey.range().allHits().empty(); ++tick) {
        odyssey.update(Intents{});
    }
    REQUIRE(odyssey.range().allHits().size() == 1);
    const game::SpearHit& hit = odyssey.range().allHits().front();
    MESSAGE("hit ", std::string(game::hitKindName(hit.kind)), " at (", toDouble(hit.point.x), ", ", toDouble(hit.point.y), ", ",
            toDouble(hit.point.z), ")");
    CHECK(hit.kind == HitKind::Rock);
    CHECK(std::abs(toDouble(hit.point.y) - 26.0) < 0.001); // the boulder's south face (tile row 25 spans y 25..26)
    // The spear stopped there, and the target is unharmed.
    const auto& spear = odyssey.range().spears().front();
    CHECK(spear.state == SpearState::Stuck);
    CHECK(spear.body.velocity == Vec3{});
    CHECK(odyssey.range().targets()[1].hits == 0);
    CHECK(odyssey.range().targets()[1].damageTaken == luna::physics::kFixedZero);
}
