
// US-269 The Editor's Kinds tab and the NPC markers: the defaults of a whole kind are edited with the same form as one NPC and written to assets/data/npcs/<kind>.json;
// under every placed NPC the Editor draws a ring in its class colour with the class icon, and the game never does.
#include "camp.h"

#include "game/npc_class_book.h"
#include "game/npc_marker.h"

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
    int trader = 0;
    int guard = 0;
    int goblin = 0;
    int scaredGoblin = 0;

    explicit Studio(const std::string& name) : data(dataCopy(name)) {
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        const auto place = [&](const std::string& kind, int dx, const std::string& who) {
            game::PlacedCharacter placed{level.nextId++, kind, {level.heroStart.x + dx, level.heroStart.y}, game::Facing::South, who, 60, 4, {}};
            level.characters.push_back(placed);
            return placed.id;
        };
        trader = place("wanderer", 60, "Tala");
        guard = place("wanderer", 100, "Gur");
        goblin = place("goblin", 140, "Gob");
        scaredGoblin = place("goblin", 180, "Boo");
        level.characters[0].classes = {"trader"};
        level.characters[1].classes = {"guard"};
        level.characters[3].attitude = "scared";
        file = data / "kinds-tab-level.json";
        game::saveLevel(level, definitions, file);
        odyssey = std::make_unique<game::OdysseyGame>(data, file);
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
        odyssey->update(pressing(luna::engine::Intent::ModeEditor));
    }
    game::Editor& editor() { return odyssey->editor(); }
    // How many marker pictures one frame draws (found by the texture numbers the Editor made for them).
    int markersDrawn() {
        renderer.clear();
        odyssey->render(renderer, 1.0);
        const std::vector<int> ids = editor().markerTextureIds();
        int count = 0;
        for (const auto& draw : renderer.draws()) {
            if (std::find(ids.begin(), ids.end(), draw.texture) != ids.end()) ++count;
        }
        return count;
    }
};

} // namespace

TEST_CASE("US-269 Kind: setting goblin to neutral and saving makes every goblin without an override neutral in play") {
    Studio studio("kinds-tab-kind");
    game::Editor& editor = studio.editor();
    editor.showKinds(true);
    CHECK(editor.kindsTab());
    const std::vector<std::string> names = editor.kindNames();
    CHECK(std::find(names.begin(), names.end(), "goblin") != names.end());
    CHECK(std::find(names.begin(), names.end(), "hero") == names.end());

    editor.selectKind("goblin");
    REQUIRE(editor.kindDraft().layer.attitude.has_value());
    CHECK(*editor.kindDraft().layer.attitude == "hostile"); // the shipped kind file
    editor.kindDraft().layer.attitude = "neutral";
    CHECK(editor.saveKind());

    // The file on disk says neutral, keeps the monster class, and reads back as the same kind.
    const std::string text = readText(studio.data / "npcs" / "goblin.json");
    CHECK(text.find("\"attitude\": \"neutral\"") != std::string::npos);
    CHECK(text.find("\"monster\"") != std::string::npos);
    sim::rules::LoadReport report;
    const auto reread = sim::rules::NpcKindCatalog::parse(text, "npcs/goblin.json", report, "goblin");
    REQUIRE(reread.has_value());
    CHECK(report.errors.empty());
    CHECK(*reread == editor.kindDraft());
    CHECK(sim::rules::toJson(*reread) == text); // load-save-load gives the same text

    // In play: the goblin that sets nothing is neutral; the one with its own attitude keeps it.
    studio.odyssey->update(pressing(luna::engine::Intent::ModeGame));
    CHECK(studio.odyssey->attitudeWordOf(studio.goblin) == "neutral");
    CHECK(studio.odyssey->attitudeWordOf(studio.scaredGoblin) == "scared");
}

TEST_CASE("US-269 Kind: classes, tags, talk and actions of the draft; a kind without a file gets one; a mistake is not written") {
    Studio studio("kinds-tab-form");
    game::Editor& editor = studio.editor();
    editor.showKinds(true);
    editor.selectKind("wanderer");
    CHECK(editor.kindDraft().layer.classes == std::optional<std::vector<std::string>>(std::vector<std::string>{"talker"}));
    editor.toggleKindClass("talker");
    editor.toggleKindClass("elder");
    editor.toggleKindClass("trader");
    CHECK(editor.kindDraft().layer.classes == std::optional<std::vector<std::string>>(std::vector<std::string>{"elder", "trader"}));
    editor.toggleKindClass("elder");
    editor.toggleKindClass("trader");
    CHECK_FALSE(editor.kindDraft().layer.classes.has_value()); // no class at all: the field is left out
    editor.toggleKindClass("trader");
    editor.kindDraft().layer.tags = {"market"};
    editor.kindDraft().layer.dialogues = {{"player", "npc-ossa.dlg"}};
    editor.kindDraft().layer.deny = {"barter"};
    CHECK(editor.saveKind());
    const sim::rules::NpcKind* saved = studio.odyssey->npcClasses().kinds().find("wanderer");
    REQUIRE(saved != nullptr);
    CHECK(saved->layer == editor.kindDraft().layer);

    // A mistake (an unknown attitude, here set by hand) is refused and the file stays as it was.
    const std::string before = readText(studio.data / "npcs" / "wanderer.json");
    editor.kindDraft().layer.attitude = "banana";
    CHECK_FALSE(editor.saveKind());
    CHECK(readText(studio.data / "npcs" / "wanderer.json") == before);
    CHECK_FALSE(editor.status().empty());

    // A kind that has no file yet starts blank and gets one when saved.
    fs::remove(studio.data / "npcs" / "goblin.json");
    REQUIRE(studio.odyssey->npcClasses().reload());
    editor.selectKind("goblin");
    CHECK(editor.kindDraft().kind == "goblin");
    CHECK_FALSE(editor.kindDraft().layer.classes.has_value());
    editor.toggleKindClass("monster");
    CHECK(editor.saveKind());
    CHECK(fs::exists(studio.data / "npcs" / "goblin.json"));
}

TEST_CASE("US-269 Markers: a trader, a guard and a goblin each show a ring and icon in the Editor and none in Game mode") {
    Studio studio("kinds-tab-markers");
    game::Editor& editor = studio.editor();
    CHECK(editor.markerCount() == 4); // the second goblin too: every placed NPC with a class has one
    CHECK(studio.markersDrawn() == 4);

    // Game mode: not one marker.
    studio.odyssey->update(pressing(luna::engine::Intent::ModeGame));
    CHECK(studio.markersDrawn() == 0);
    // And back in the Editor they are there again.
    studio.odyssey->update(pressing(luna::engine::Intent::ModeEditor));
    CHECK(studio.markersDrawn() == 4);
}

TEST_CASE("US-269 Markers: the ring is split in equal arcs and the icon is the first class's") {
    sim::rules::NpcClassCatalog catalog;
    sim::rules::LoadReport report;
    const fs::path folder = dataCopy("kinds-tab-image") / "npc-classes";
    catalog = sim::rules::NpcClassCatalog::load(folder, report);
    const sim::rules::NpcClass* trader = catalog.find("trader");
    const sim::rules::NpcClass* guard = catalog.find("guard");
    REQUIRE(trader != nullptr);
    REQUIRE(guard != nullptr);

    sim::rules::ResolvedNpc none;
    CHECK_FALSE(game::markerFor(none, catalog).has_value());
    sim::rules::ResolvedNpc missing;
    missing.classes = {"no-such-class"};
    CHECK_FALSE(game::markerFor(missing, catalog).has_value());

    sim::rules::ResolvedNpc one;
    one.classes = {"trader"};
    const auto single = game::markerFor(one, catalog);
    REQUIRE(single.has_value());
    CHECK(single->colours == std::vector<int>{trader->colour});
    CHECK(single->icon == "coin");

    sim::rules::ResolvedNpc two;
    two.classes = {"trader", "guard"};
    const auto both = game::markerFor(two, catalog);
    REQUIRE(both.has_value());
    CHECK(both->colours == std::vector<int>{trader->colour, guard->colour});
    CHECK(both->icon == trader->icon); // the first class's icon

    const auto colourAt = [](const luna::engine::Image& image, int x, int y) {
        const luna::engine::Color c = image.get(x, y);
        return (c.red << 16) | (c.green << 8) | c.blue;
    };
    const luna::engine::Image ring = game::markerImage(*both);
    CHECK(ring.width() == game::kMarkerSize);
    CHECK(colourAt(ring, 12, 2) == trader->colour); // top right: the first arc, clockwise from the top
    CHECK(colourAt(ring, 5, 2) == guard->colour);   // top left: the second arc
    CHECK(ring.get(0, 0).alpha == 0);               // outside the ring: transparent
    CHECK(ring.get(8, 8).alpha != 0);               // inside: the dark disc or the icon
    CHECK(game::markerImage(*both).data()[0] == ring.data()[0]); // the same marker, the same picture

    // Every icon of the set has a picture of its own: eight rows of eight, at least one lit pixel.
    for (const std::string& name : sim::rules::npcIconNames()) {
        const std::vector<std::string>& bitmap = game::iconBitmap(name);
        REQUIRE(bitmap.size() == 8);
        int lit = 0;
        for (const std::string& row : bitmap) {
            REQUIRE(row.size() == 8);
            lit += static_cast<int>(std::count(row.begin(), row.end(), '#'));
        }
        CHECK(lit > 5);
    }
}
