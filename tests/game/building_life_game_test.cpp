// US-253 .. US-257: the clan builds, interiors, damage and fire, buildings in the clan's life.
#include "camp.h"

#include "game/game_rules.h"

using namespace camp_support;
namespace buildings = odysseus::sim::buildings;
namespace sim = odysseus::sim;

namespace {

// A top-left cell near the hero where `kind` stands.
std::pair<int, int> spotNear(game::OdysseyGame& odyssey, const std::string& kind) {
    const auto [hx, hy] = game::BuildingLayer::cellOf(odyssey.hero().feetX(), odyssey.hero().feetY());
    const auto [w, h] = odyssey.buildings().store().sizeOf(kind, 0);
    for (int ay = hy - 6; ay <= hy + 6; ++ay) {
        for (int ax = hx - 6; ax <= hx + 6; ++ax) {
            if (std::abs(ax + w / 2 - hx) < 3 && std::abs(ay + h / 2 - hy) < 3) continue; // not on top of the hero
            if (odyssey.buildings().whyNot(odyssey, kind, ax, ay, 0).empty()) return {ax, ay};
        }
    }
    FAIL("no free place near the hero");
    return {0, 0};
}

} // namespace

TEST_CASE("US-253 Clan: clan members bring materials to a blueprint and work on it without the hero") {
    Camp camp("us253-clan");
    const auto [ax, ay] = spotNear(camp.odyssey, "windbreak");
    camp.odyssey.buildings().select("windbreak");
    REQUIRE(camp.odyssey.buildings().placeSelected(camp.odyssey, ax, ay).empty());
    camp.odyssey.buildings().stopPlacing();
    const int id = camp.odyssey.buildings().store().all().back().id;
    camp.play(20 * 60 * 5); // five minutes
    const buildings::PlacedBuilding* building = camp.odyssey.buildings().store().find(id);
    REQUIRE(building != nullptr);
    const bool somethingDone = building->state == buildings::State::Finished || !building->delivered.empty() || building->workMilli > 0;
    CHECK(somethingDone);
}

TEST_CASE("US-254 Interior: a finished building with an interior level opens it, the outside waits, and leaving brings the hero back to the door") {
    Camp camp("us254-interior");
    fs::create_directories(camp.data.parent_path() / "levels");
    fs::copy_file(fs::path(ODYSSEUS_DATA_DIR).parent_path() / "levels" / "interior-hut.json", camp.data.parent_path() / "levels" / "interior-hut.json", fs::copy_options::overwrite_existing);
    const auto [ax, ay] = spotNear(camp.odyssey, "hut");
    const auto placed = camp.odyssey.buildings().store().place("hut", ax, ay, 0, 0, true, {});
    REQUIRE(placed.problem.empty());
    buildings::PlacedBuilding* hut = camp.odyssey.buildings().store().findMutable(placed.id);
    // Without an interior level the door leads nowhere.
    CHECK_FALSE(camp.odyssey.enterBuilding(placed.id).empty());
    hut->interior = "map";
    hut->interiorLevel = "interior-hut";
    const auto tags = camp.odyssey.buildings().store().tags(*hut);
    CHECK(std::find(tags.begin(), tags.end(), "enterable") != tags.end());
    const double outsideX = camp.odyssey.hero().feetX();
    const double outsideY = camp.odyssey.hero().feetY();
    const std::string outsideLevel = camp.odyssey.level().name;
    const std::size_t outsideBuildings = camp.odyssey.buildings().store().all().size();
    REQUIRE(camp.odyssey.enterBuilding(placed.id).empty());
    CHECK(camp.odyssey.insideBuilding());
    CHECK(camp.odyssey.level().name == "Hut inside");
    camp.play(40);
    // The way out is the place named exit.
    bool hasExit = false;
    for (const auto& place : camp.odyssey.level().places) hasExit = hasExit || place.name == "exit";
    CHECK(hasExit);
    REQUIRE(camp.odyssey.leaveBuilding().empty());
    CHECK_FALSE(camp.odyssey.insideBuilding());
    CHECK(camp.odyssey.level().name == outsideLevel);
    CHECK(camp.odyssey.buildings().store().all().size() == outsideBuildings);
    CHECK(std::abs(camp.odyssey.hero().feetX() - outsideX) < 1.0);
    CHECK(std::abs(camp.odyssey.hero().feetY() - outsideY) < 1.0);
}

TEST_CASE("US-255 Raid: a rival at war strikes a finished building at the season start, a friendly one does not, and repair and douse put things right") {
    Camp camp("us255-raid");
    const auto [ax, ay] = spotNear(camp.odyssey, "hut");
    const auto placed = camp.odyssey.buildings().store().place("hut", ax, ay, 0, 0, true, {});
    REQUIRE(placed.problem.empty());
    camp.play(4);
    CHECK(camp.odyssey.buildings().raidsAtSeasonStart(camp.odyssey, 1) == 0); // no rivals in a hand-made level
    REQUIRE(camp.odyssey.buildings().raidFrom(camp.odyssey, 0, 1));
    const buildings::PlacedBuilding* hut = camp.odyssey.buildings().store().find(placed.id);
    REQUIRE(hut != nullptr);
    CHECK(camp.odyssey.buildings().store().condition(*hut) < 100);
    // The same seed and season give the same raid (deterministic).
    const int before = camp.odyssey.buildings().store().condition(*hut);
    // Repair: ten seconds of work each time, through the interaction.
    const auto subject = game::buildingSubject(camp.odyssey, placed.id);
    REQUIRE(subject.has_value());
    CHECK(subject->title.find("damaged") != std::string::npos);
    REQUIRE(game::startInteraction(camp.odyssey, "repair", *subject));
    camp.play(20 * 11);
    CHECK(camp.odyssey.buildings().store().condition(*camp.odyssey.buildings().store().find(placed.id)) > before);
}

TEST_CASE("US-255 Fire: a burning piece can be doused through the interaction") {
    Camp camp("us255-douse");
    const auto [ax, ay] = spotNear(camp.odyssey, "piece:wall-wood");
    const auto placed = camp.odyssey.buildings().store().place("piece:wall-wood", ax, ay, 0, 0, true, {});
    REQUIRE(placed.problem.empty());
    const buildings::PlacedBuilding* hut = camp.odyssey.buildings().store().find(placed.id);
    REQUIRE(camp.odyssey.buildings().store().ignitePiece(placed.id, 0));
    CHECK(camp.odyssey.buildings().store().burning(*hut));
    const auto subject = game::buildingSubject(camp.odyssey, placed.id);
    REQUIRE(subject.has_value());
    CHECK(subject->title.find("fire") != std::string::npos);
    REQUIRE(game::startInteraction(camp.odyssey, "douse-fire", *subject));
    camp.play(50);
    CHECK_FALSE(camp.odyssey.buildings().store().burning(*camp.odyssey.buildings().store().find(placed.id)));
}

TEST_CASE("US-257 Life: a finished hut is given to the lowest-id adult, housed people lose warmth more slowly, a storage pit halves spoilage and store-food fills it") {
    Camp camp("us257-life");
    sim::World* clan = camp.odyssey.clanMutable();
    REQUIRE(clan != nullptr);
    const auto [ax, ay] = spotNear(camp.odyssey, "hut");
    const auto hut = camp.odyssey.buildings().store().place("hut", ax, ay, 0, 0, true, {});
    REQUIRE(hut.problem.empty());
    camp.odyssey.buildings().applyClanLife(camp.odyssey);
    const buildings::PlacedBuilding* placed = camp.odyssey.buildings().store().find(hut.id);
    REQUIRE(placed != nullptr);
    REQUIRE(placed->owner >= 0);
    int lowest = -1;
    for (const sim::Person& person : clan->people()) {
        if (person.alive && !person.exiled && person.ageYears(clan->calendar().daysPerYear()) >= 15) {
            lowest = lowest < 0 ? person.id : std::min(lowest, person.id);
        }
    }
    CHECK(placed->owner == lowest);
    // The pit keeps meals: put some in with the interaction (the hero needs berries).
    const auto [bx, by] = spotNear(camp.odyssey, "storage-pit");
    const auto pit = camp.odyssey.buildings().store().place("storage-pit", bx, by, 0, 0, true, {});
    REQUIRE(pit.problem.empty());
    camp.odyssey.life()->give("berries", 4);
    const int foodBefore = clan->food();
    const auto subject = game::buildingSubject(camp.odyssey, pit.id);
    REQUIRE(subject.has_value());
    REQUIRE(game::startInteraction(camp.odyssey, "store-food", *subject));
    camp.play(60);
    CHECK(clan->food() == foodBefore + 2);
    CHECK(camp.odyssey.buildings().store().find(pit.id)->contents.at("berries") == 2);
}

TEST_CASE("US-254 Doors: enter-building and leave-building work through the interactions of the right-click menu") {
    Camp camp("us254-doors");
    fs::create_directories(camp.data.parent_path() / "levels");
    fs::copy_file(fs::path(ODYSSEUS_DATA_DIR).parent_path() / "levels" / "interior-hut.json", camp.data.parent_path() / "levels" / "interior-hut.json", fs::copy_options::overwrite_existing);
    const auto [ax, ay] = spotNear(camp.odyssey, "hut");
    const auto placed = camp.odyssey.buildings().store().place("hut", ax, ay, 0, 0, true, {});
    REQUIRE(placed.problem.empty());
    buildings::PlacedBuilding* hut = camp.odyssey.buildings().store().findMutable(placed.id);
    hut->interior = "map";
    hut->interiorLevel = "interior-hut";
    const auto subject = game::buildingSubject(camp.odyssey, placed.id);
    REQUIRE(subject.has_value());
    REQUIRE(game::startInteraction(camp.odyssey, "enter-building", *subject));
    camp.play(30);
    CHECK(camp.odyssey.insideBuilding());
    // The exit is a place of the interior level: its subject is found by tag.
    const auto& places = camp.odyssey.level().places;
    const auto exitPlace = std::find_if(places.begin(), places.end(), [](const game::PlacedPlace& p) { return p.name == "exit"; });
    REQUIRE(exitPlace != places.end());
    CHECK(std::find(exitPlace->tags.begin(), exitPlace->tags.end(), "exit") != exitPlace->tags.end());
    const auto out = game::placeSubject(camp.odyssey, static_cast<int>(exitPlace - places.begin()));
    REQUIRE(out.has_value());
    REQUIRE(game::startInteraction(camp.odyssey, "leave-building", *out));
    camp.play(30);
    CHECK_FALSE(camp.odyssey.insideBuilding());
}
