// US-201 Generator settings with live preview: every setting of region.json in a form, a Preview map beside the old one, Apply that writes the file.
#include "core/text.h"
#include "game/generator_panel.h"
#include "game/region_view.h"
#include "luna/engine/renderer.h"
#include "luna/engine/ui.h"
#include "sim/region.h"
#include "sim/region_edits.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace game = odysseus::game;
namespace sim = odysseus::sim;
namespace eng = luna::engine;

namespace {

struct Rig {
    fs::path base;
    fs::path file; // a private copy of region.json, so Apply never touches the shipped one
    std::vector<fs::path> saved;
    std::vector<std::string> said;
    game::RegionView view;

    Rig()
        : base(fs::temp_directory_path() / ("odysseus-us201-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))),
          file(base / "region.json"), view(960, 540, [this](const std::string& message) { said.push_back(message); }) {
        fs::create_directories(base);
        fs::copy_file(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "region.json", file);
        view.setSource(file, [] { return std::uint64_t{1}; }, [this](const fs::path& written) { saved.push_back(written); });
        view.show(true);
        view.settings().show(true);
    }
    ~Rig() {
        std::error_code error;
        fs::remove_all(base, error);
    }
    std::string text() const { return *odysseus::core::readTextFile(file); }
    void tick(int count = 1) {
        for (int i = 0; i < count; ++i) view.update(eng::Intents{});
    }
    // Ticks until the preview map is painted; returns how many it took.
    int ticksToPreview() {
        int ticks = 0;
        while (!view.settings().previewComplete() && ticks < 400) {
            tick();
            ++ticks;
        }
        return ticks;
    }
};

std::vector<std::string> lines(const std::string& text) {
    std::vector<std::string> out;
    std::istringstream stream(text);
    for (std::string line; std::getline(stream, line);) out.push_back(line);
    return out;
}

} // namespace

TEST_CASE("US-201 Settings: every setting of region.json except the size is a field, with the file's ranges") {
    Rig rig;
    const auto& settings = game::generatorSettings();
    CHECK(settings.size() == 14);
    sim::RegionConfig shipped = sim::loadRegionConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "region.json");
    for (const game::GeneratorSetting& setting : settings) {
        CAPTURE(setting.key);
        CHECK(rig.view.settings().draft().*setting.member == shipped.*setting.member); // the fields start from the land on screen
        // The panel's range is inside what the file accepts (the cross-setting rules are in problems()).
        sim::RegionConfig low = shipped;
        low.*setting.member = setting.minimum;
        sim::RegionConfig high = shipped;
        high.*setting.member = setting.maximum;
        for (const sim::RegionConfig* config : {&low, &high}) {
            for (const std::string& problem : sim::regionConfigProblems(*config)) {
                const std::string key = setting.key;
                if (key == "mountainLevel" || key == "herdMaximum") continue; // their low end depends on another setting: problems() says so
                CHECK(problem.rfind(setting.key, 0) != 0); // the setting itself is in range
            }
        }
    }
}

TEST_CASE("US-201 Preview: a new map shows within a few ticks beside the old one, and nothing else changes") {
    Rig rig;
    const std::string before = rig.text();
    rig.tick(8); // the map on screen is painted
    REQUIRE(rig.view.settings().nowMix().valid);

    rig.view.settings().setDraft("lakeLevel", 300);
    rig.view.settings().setDraft("mountainLevel", 900);
    REQUIRE(rig.view.settings().preview());
    CHECK(rig.view.settings().previewStarted());
    CHECK_FALSE(rig.view.settings().previewComplete());
    const int ticks = rig.ticksToPreview();
    CHECK(ticks <= 20); // a second at 20 ticks a second; the Codex asks for "a few seconds"
    REQUIRE(rig.view.settings().previewComplete());
    REQUIRE(rig.view.settings().previewMix().valid);

    // More lake level, more water: the mix of land changes, and the old map is still the old land.
    CHECK(rig.view.settings().previewMix().percent[static_cast<std::size_t>(sim::Biome::Water)] > rig.view.settings().nowMix().percent[static_cast<std::size_t>(sim::Biome::Water)]);
    CHECK(rig.view.region()->config().lakeLevel == 140);
    CHECK(rig.view.settings().previewRegion()->config().lakeLevel == 300);
    CHECK(rig.text() == before); // Preview writes nothing
    CHECK(rig.saved.empty());

    // Drawn: the old map and the new map are both on screen.
    eng::RecordingRenderer renderer;
    const eng::Texture sheet = renderer.createTexture(eng::makeUiSheet());
    eng::UiPainter painter(renderer, sheet);
    painter.setScreen({0, 0, 960, 540});
    renderer.clear();
    rig.view.render(renderer, painter);
    int thumbnails = 0;
    for (const auto& draw : renderer.draws()) {
        if (draw.styled && draw.source.width == game::GeneratorPanel::kThumbCells && draw.destination.width == game::GeneratorPanel::kThumbSize) ++thumbnails;
    }
    CHECK(thumbnails == 2);
}

TEST_CASE("US-201 Preview and Apply refuse a draft that breaks a rule, and say which") {
    Rig rig;
    rig.view.settings().setDraft("mountainLevel", 150); // must be at least 100 above the lake level (140)
    CHECK_FALSE(rig.view.settings().problems().empty());
    CHECK_FALSE(rig.view.settings().preview());
    CHECK_FALSE(rig.view.settings().previewStarted());
    const std::string before = rig.text();
    CHECK_FALSE(rig.view.settings().apply());
    CHECK(rig.text() == before);
    CHECK(rig.saved.empty());
    CHECK_FALSE(rig.said.empty());
    rig.view.settings().revert();
    CHECK(rig.view.settings().problems().empty());
    CHECK(rig.view.settings().draft().mountainLevel == 800);
}

TEST_CASE("US-201 Apply: region.json and the region use the new settings, and only the changed lines of the file change") {
    Rig rig;
    const std::string before = rig.text();
    sim::Region old(1, rig.view.region()->config());
    const std::uint64_t oldPrint = old.fingerprint();
    rig.view.setZoom(3);
    rig.view.centreOn(90.5, 70.5);

    rig.view.settings().setDraft("lakeLevel", 260);
    rig.view.settings().setDraft("woodPerMille", 80);
    REQUIRE(rig.view.settings().apply());

    CHECK(rig.saved == std::vector<fs::path>{rig.file});
    const sim::RegionConfig written = sim::loadRegionConfig(rig.file); // round trip: the file reads back as the draft
    CHECK(written.lakeLevel == 260);
    CHECK(written.woodPerMille == 80);
    CHECK(written.mountainLevel == 800);
    CHECK(rig.view.region()->config().lakeLevel == 260);
    CHECK(rig.view.region()->config().woodPerMille == 80);
    CHECK(rig.view.region()->fingerprint() != oldPrint);
    CHECK(rig.view.region()->seed() == 1);
    CHECK(rig.view.zoom() == 3); // the same view over the new land
    CHECK(rig.view.centreX() == doctest::Approx(90.5));
    CHECK(rig.view.centreY() == doctest::Approx(70.5));

    // Only the two settings that changed are different in the file, line for line.
    const auto a = lines(before);
    const auto b = lines(rig.text());
    REQUIRE(a.size() == b.size());
    int different = 0;
    for (std::size_t i = 0; i < a.size(); ++i) different += a[i] != b[i] ? 1 : 0;
    CHECK(different == 2);
    // The fields now start from the new land.
    CHECK(rig.view.settings().draft().lakeLevel == 260);
    rig.view.settings().setDraft("lakeLevel", 140);
    rig.view.settings().revert();
    CHECK(rig.view.settings().draft().lakeLevel == 260);
}

TEST_CASE("US-201 Keep edits: changing the settings leaves the hand edits where they are and lists the ones the new land does not suit") {
    Rig rig;
    sim::RegionConfig wetter = rig.view.region()->config();
    wetter.lakeLevel = 300;
    wetter.mountainLevel = 900;
    sim::Region before(1, rig.view.region()->config());
    sim::Region after(1, wetter);
    sim::Tile flooded{-1, -1};
    sim::Tile dry{-1, -1};
    for (int y = 30; y < 226; ++y) {
        for (int x = 30; x < 226; ++x) {
            if (flooded.x < 0 && sim::walkable(before.biomeAt(x, y)) && after.biomeAt(x, y) == sim::Biome::Water) flooded = {x, y};
            if (dry.x < 0 && sim::walkable(before.biomeAt(x, y)) && sim::walkable(after.biomeAt(x, y))) dry = {x, y};
        }
    }
    REQUIRE(flooded.x >= 0);
    REQUIRE(dry.x >= 0);
    rig.view.edits().placed = {
        {"t-0001", sim::EditGroup::Thing, "boulder", flooded.x, flooded.y, false, false},
        {"t-0002", sim::EditGroup::Thing, "boulder", dry.x, dry.y, false, false},
    };
    const sim::RegionEdits kept = rig.view.edits();

    // The Preview already says what would not fit.
    rig.view.settings().setDraft("lakeLevel", 300);
    rig.view.settings().setDraft("mountainLevel", 900);
    REQUIRE(rig.view.settings().preview());
    REQUIRE(rig.view.settings().conflicts().size() == 1);
    CHECK(rig.view.settings().conflicts().front().id == "t-0001");

    REQUIRE(rig.view.settings().apply());
    CHECK(rig.view.edits() == kept); // not moved, not dropped
    REQUIRE(rig.view.settings().conflicts().size() == 1);
    CHECK(rig.view.settings().conflicts().front().id == "t-0001");
    CHECK(rig.view.settings().conflicts().front().reason.find("water") != std::string::npos);
    CHECK(rig.view.region()->config().lakeLevel == 300);
}

TEST_CASE("US-201 A number typed into a field becomes the draft") {
    Rig rig;
    rig.tick();
    const eng::Rect area = rig.view.settings().bounds();
    // The first field is the lake level: click it, type 250, press Enter.
    const int labelWidth = eng::UiPainter::textWidth(game::generatorSettings().front().label);
    const int x = area.x + 4 + labelWidth + 4 + 8;
    const int y = area.y + 16 + 7;
    eng::Intents click;
    eng::Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    pointer.held[static_cast<std::size_t>(eng::PointerButton::Left)] = true;
    pointer.pressed[static_cast<std::size_t>(eng::PointerButton::Left)] = true;
    click.setPointer(pointer);
    rig.view.update(click);
    CHECK(rig.view.typing());
    eng::Intents typed;
    typed.setPointer(pointer);
    typed.setText("250");
    rig.view.update(typed);
    eng::Intents enter;
    enter.set(eng::Intent::Confirm, true, true);
    rig.view.update(enter);
    CHECK_FALSE(rig.view.typing());
    CHECK(rig.view.settings().draft().lakeLevel == 250);
    CHECK(rig.view.region()->config().lakeLevel == 140); // the land on screen waits for Apply
}
