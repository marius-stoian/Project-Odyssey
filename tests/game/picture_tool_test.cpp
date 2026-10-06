// US-192 Picture pickers and cutting frames: the atlas frames of a field as pictures to pick from, the frames of an effect playing in the form, and the Cut tool that adds a
// rectangle of a sheet to the cuts file and cuts the atlas again.
#include "core/text.h"
#include "game/art.h"
#include "game/atlas_cuts.h"
#include "game/content_art.h"
#include "game/data_editor.h"
#include "game/picture_tool.h"
#include "luna/engine/image_io.h"
#include "luna/engine/ui.h"
#include "sim/data_document.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <set>
#include <string>

namespace fs = std::filesystem;
namespace game = odysseus::game;
namespace luna_ui = luna::engine;

namespace {

// A small sprites folder: one sheet with a red, a blue and a green square on white, a cuts.json and a content-cuts.json that cut some of them, and the atlas made from them.
struct Sprites {
    fs::path base = fs::temp_directory_path() / ("odysseus-us192-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::path sprites = base / "sprites";

    Sprites() {
        fs::create_directories(sprites);
        luna_ui::Image sheet(96, 64);
        sheet.fillRect(0, 0, 96, 64, {255, 255, 255, 255});
        sheet.fillRect(8, 8, 20, 20, {220, 30, 30, 255});
        sheet.fillRect(40, 10, 24, 24, {30, 30, 220, 255});
        sheet.fillRect(70, 30, 16, 16, {30, 200, 30, 255});
        REQUIRE(luna_ui::savePng(sheet, sprites / "sheet.png"));
        write("cuts.json", R"({
  "tolerance": 12,
  "tileInset": 4,
  "cuts": [
    {"name": "first.0", "kind": "character", "sheet": "sheet.png", "rect": [8, 8, 20, 20]}
  ]
}
)");
        write("content-cuts.json", R"({"contentCutsVersion":1,"pages":{"icons":{"cell":[32,32],"fit":"centre"},"effects":{"cell":[48,48],"fit":"centre"}},
"cuts":[
  {"name":"red gem","page":"icons","sheet":"sheet.png","rect":[4,4,28,28],"key":"flood","tolerance":40},
  {"name":"blue star","page":"effects","sheet":"sheet.png","rect":[4,4,66,40],"key":"flood","tolerance":40,"frameRects":[[4,4,28,28],[36,6,32,32]]}
]}
)");
        REQUIRE_NOTHROW(game::rebuildAtlases(sprites));
    }
    ~Sprites() {
        std::error_code error;
        fs::remove_all(base, error);
    }
    void write(const std::string& name, const std::string& text) const { REQUIRE_FALSE(odysseus::core::writeTextFileSafely(sprites / name, text).has_value()); }
    std::string read(const std::string& name) const { return *odysseus::core::readTextFile(sprites / name); }
};

luna_ui::Intents pointerAt(int x, int y, bool pressed, bool held, bool released) {
    luna_ui::Intents intents;
    luna_ui::Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    pointer.pressed[0] = pressed;
    pointer.held[0] = held;
    pointer.released[0] = released;
    intents.setPointer(pointer);
    return intents;
}

// The Data tab with a data folder and the small sprites folder next to it, drawing with a renderer in memory.
struct Rig {
    Sprites art;
    fs::path folder = art.base / "data";
    luna_ui::ImageRenderer renderer{960, 540};
    std::vector<std::string> said;
    game::DataEditor editor{960, 540, [this](const std::string& message) { said.push_back(message); }};

    Rig() {
        fs::create_directories(folder);
        fs::copy(ODYSSEUS_DATA_DIR, folder, fs::copy_options::recursive);
        editor.setFolder(folder);
        editor.setPictures(art.sprites, [this](const luna_ui::Image& image) { return renderer.createTexture(image); });
        editor.show(true);
    }
    void draw() {
        renderer.clear({20, 20, 28, 255});
        const luna_ui::Texture sheet = renderer.createTexture(luna_ui::makeUiSheet());
        luna_ui::UiPainter painter(renderer, sheet);
        painter.setScreen({0, 0, 960, 540});
        editor.draw(painter);
        editor.drawOverlay(painter);
    }
};

template <typename T>
const T* find(const luna_ui::Panel& panel) {
    for (const auto& child : panel.children()) {
        if (const auto* found = dynamic_cast<const T*>(child.get())) return found;
    }
    return nullptr;
}

} // namespace

TEST_CASE("US-192 Cut: a rectangle of a sheet becomes a cut of the cuts file and the atlas is cut again") {
    Sprites art;
    CHECK(game::cutSheets(art.sprites) == std::vector<std::string>{"sheet.png"});
    CHECK(game::cutTargets(art.sprites) == std::vector<std::string>{"character", "tile", "effects", "icons"});
    const std::string before = art.read("cuts.json");
    std::string summary;
    // A character frame goes to cuts.json, in the style of its lines.
    REQUIRE_FALSE(game::addCut(art.sprites, {"sheet.png", {40, 10, 24, 24}, "second.0", "character"}, &summary).has_value());
    CHECK(summary.find("2 character frames") != std::string::npos);
    const std::string after = art.read("cuts.json");
    CHECK(after.find("\"name\": \"second.0\"") != std::string::npos);
    CHECK(after.find("\"rect\": [40, 10, 24, 24]") != std::string::npos);
    CHECK(after.find("\"first.0\"") != std::string::npos); // the line that was there stays
    CHECK(game::loadCuts(art.sprites / "cuts.json").cuts.size() == 2);
    CHECK(art.read("atlas/atlas.json").find("second.0") != std::string::npos); // the atlas was cut again
    // A picture of a content page goes to content-cuts.json and into content.json.
    REQUIRE_FALSE(game::addCut(art.sprites, {"sheet.png", {70, 30, 16, 16}, "green gem", "icons"}, &summary).has_value());
    CHECK(summary.find("content items") != std::string::npos);
    const auto contentCut = [&](const std::string& name) -> const game::ContentCut* {
        static game::ContentCuts cuts;
        cuts = game::loadContentCuts(art.sprites / "content-cuts.json");
        for (const game::ContentCut& cut : cuts.cuts) {
            if (cut.name == name) return &cut;
        }
        return nullptr;
    };
    REQUIRE(contentCut("green gem") != nullptr);
    CHECK(contentCut("green gem")->page == "icons");
    CHECK(contentCut("green gem")->rect.x == 70);
    CHECK(contentCut("green gem")->rect.width == 16);
    CHECK(art.read("content-cuts.json").find("\"red gem\"") != std::string::npos); // the lines that were there stay
    CHECK(art.read("atlas/content.json").find("green gem") != std::string::npos);
    std::string problem;
    CHECK(game::loadContent(art.sprites / "atlas", problem).has_value());
    // What cannot be cut is said and changes nothing.
    const std::string cuts = art.read("cuts.json");
    const std::string content = art.read("content-cuts.json");
    CHECK(game::addCut(art.sprites, {"sheet.png", {0, 0, 10, 10}, "second.0", "character"})->find("already has") != std::string::npos);
    CHECK(game::addCut(art.sprites, {"sheet.png", {90, 60, 20, 20}, "outside", "character"})->find("not inside") != std::string::npos);
    CHECK(game::addCut(art.sprites, {"sheet.png", {0, 0, 2, 2}, "tiny", "character"})->find("too small") != std::string::npos);
    CHECK(game::addCut(art.sprites, {"sheet.png", {0, 0, 10, 10}, "fine", "nowhere"})->find("not a place") != std::string::npos);
    CHECK(game::addCut(art.sprites, {"sheet.png", {0, 0, 10, 10}, "bad\"name", "character"}).has_value());
    CHECK(game::addCut(art.sprites, {"sheet.png", {0, 0, 10, 10}, "", "character"}).has_value());
    CHECK(game::addCut(art.sprites, {"missing.png", {0, 0, 10, 10}, "ok", "character"}).has_value());
    CHECK(game::addCut(art.sprites, {"../sheet.png", {0, 0, 10, 10}, "ok", "character"}).has_value());
    CHECK(art.read("cuts.json") == cuts);
    CHECK(art.read("content-cuts.json") == content);
    (void)before;
}

TEST_CASE("US-192 Cut: a cut that makes the atlas fail is taken out again") {
    Sprites art;
    // The cuts file already names a sheet that is not there: every rebuild fails.
    art.write("cuts.json", R"({"tolerance":12,"tileInset":4,"cuts":[{"name":"ghost","kind":"character","sheet":"missing.png","rect":[0,0,10,10]}]})");
    const std::string content = art.read("content-cuts.json");
    const std::optional<std::string> problem = game::addCut(art.sprites, {"sheet.png", {70, 30, 16, 16}, "green gem", "icons"});
    REQUIRE(problem.has_value());
    CHECK(problem->find("taken out again") != std::string::npos);
    CHECK(art.read("content-cuts.json") == content);
}

TEST_CASE("US-192 Pick: a field that holds an atlas frame opens its frames as pictures and the choice is saved") {
    Rig rig;
    REQUIRE(rig.editor.open("weapons.json"));
    REQUIRE(rig.editor.selectEntry("weapons[0]"));
    REQUIRE(rig.editor.setField("weapons[0].frame", "blue star.0"));
    luna_ui::Intents idle;
    rig.editor.update(idle);
    REQUIRE(rig.editor.openPicker("weapons[0].frame"));
    REQUIRE(rig.editor.pictureTool() != nullptr);
    rig.editor.update(idle); // the picker is built, on the page of the frame the field holds now
    const game::PictureGrid* first = find<game::PictureGrid>(*rig.editor.pictureTool()->panel());
    REQUIRE(first != nullptr);
    CHECK(first->cells()[0].name == "blue star.0");
    // The list of pages on the left: choose "icons".
    const luna_ui::ListBox* pages = find<luna_ui::ListBox>(*rig.editor.pictureTool()->panel());
    REQUIRE(pages != nullptr);
    REQUIRE(pages->items.size() == 2);
    rig.editor.update(pointerAt(pages->bounds.x + 4, pages->bounds.y + luna_ui::kLineHeight + 3, true, true, true));
    rig.editor.update(idle);
    const game::PictureGrid* grid = find<game::PictureGrid>(*rig.editor.pictureTool()->panel());
    REQUIRE(grid != nullptr);
    REQUIRE_FALSE(grid->cells().empty());
    CHECK(grid->cells()[0].name == "red gem"); // the page of icons, as a picture
    CHECK(grid->cells()[0].texture.id >= 0);
    rig.draw(); // the grid draws
    const std::optional<luna_ui::Rect> cell = grid->cellRect(0);
    REQUIRE(cell.has_value());
    rig.editor.update(pointerAt(cell->x + 5, cell->y + 5, true, true, false));
    rig.editor.update(idle); // the tool closes
    CHECK(rig.editor.pictureTool() == nullptr);
    REQUIRE(rig.editor.document()->find("weapons[0].frame") != nullptr);
    CHECK(rig.editor.document()->find("weapons[0].frame")->get<std::string>() == "red gem");
    CHECK(rig.editor.dirty());
    // Esc leaves the picker without choosing.
    REQUIRE(rig.editor.openPicker("weapons[0].frame"));
    luna_ui::Intents escape;
    escape.set(luna_ui::Intent::OpenMenu, true, true);
    rig.editor.update(idle);
    rig.editor.update(escape);
    CHECK(rig.editor.pictureTool() == nullptr);
    CHECK(rig.editor.shown()); // the Data tab itself stays
    CHECK(rig.editor.document()->find("weapons[0].frame")->get<std::string>() == "red gem");
}

TEST_CASE("US-192 Preview: an effect plays its frames in the form") {
    Rig rig;
    REQUIRE(rig.editor.open("effects.json"));
    std::size_t index = 0;
    const odysseus::sim::DataDocument* document = rig.editor.document();
    REQUIRE(document != nullptr);
    const odysseus::sim::OrderedJson* list = document->find("effects");
    REQUIRE(list != nullptr);
    for (std::size_t i = 0; i < list->size(); ++i) {
        if ((*list)[i]["name"] == "blue star") index = i;
    }
    REQUIRE(rig.editor.selectEntry("effects[" + std::to_string(index) + "]"));
    luna_ui::Intents idle;
    rig.editor.update(idle);
    REQUIRE(rig.editor.previewFrames().size() == 2); // the two frames of "blue star" in the small atlas
    CHECK(rig.editor.previewFrames()[0].name == "blue star.0");
    CHECK(rig.editor.previewFrames()[1].name == "blue star.1");
    // The first frame is red, the second blue: the middle of the box changes colour as the frames play (ticksPerFrame ticks each).
    const auto middle = [&] {
        rig.draw();
        return rig.renderer.image().get(960 - 6 - game::DataEditor::kPreviewWidth + 4 + (game::DataEditor::kPreviewWidth - 4) / 2, 22 + 8 + 50);
    };
    std::set<int> reds;
    for (int tick = 0; tick < 12; ++tick) {
        rig.editor.update(idle);
        reds.insert(middle().red > middle().blue ? 1 : 0);
    }
    CHECK(reds.size() == 2); // both colours were seen: it plays
    // An entry with no picture shows none.
    REQUIRE(rig.editor.open("hero/items.json"));
    rig.editor.update(idle);
    CHECK(rig.editor.previewFrames().empty());
    // A weapon shows the frame it names.
    REQUIRE(rig.editor.open("weapons.json"));
    REQUIRE(rig.editor.selectEntry("weapons[0]"));
    REQUIRE(rig.editor.setField("weapons[0].frame", "red gem"));
    rig.editor.update(idle);
    REQUIRE(rig.editor.previewFrames().size() == 1);
    CHECK(rig.editor.previewFrames()[0].name == "red gem");
}

TEST_CASE("US-192 Cut tool: drag a rectangle on a sheet, name it, and the cut is added and the atlas cut again") {
    Rig rig;
    REQUIRE(rig.editor.openCutTool());
    luna_ui::Intents idle;
    rig.editor.update(idle);
    game::PictureTool* tool = rig.editor.pictureToolMutable();
    REQUIRE(tool != nullptr);
    tool->chooseSheet("sheet.png");
    rig.editor.update(idle); // the sheet is shown
    const game::SheetView* view = find<game::SheetView>(*tool->panel());
    REQUIRE(view != nullptr);
    rig.draw();
    // Drag from the corner of the green square to its other corner: (70, 30) to (85, 45) of the sheet.
    const int x = view->bounds.x;
    const int y = view->bounds.y;
    rig.editor.update(pointerAt(x + 70, y + 30, true, true, false));
    rig.editor.update(pointerAt(x + 85, y + 45, false, true, false));
    rig.editor.update(pointerAt(x + 85, y + 45, false, false, true));
    REQUIRE(tool->rect().has_value());
    CHECK(tool->rect()->x == 70);
    CHECK(tool->rect()->y == 30);
    CHECK(tool->rect()->width == 16);
    CHECK(tool->rect()->height == 16);
    // Without a name the cut is refused; with one, and a place for it, it is added.
    CHECK_FALSE(tool->cut());
    tool->setName("lime gem");
    tool->chooseTarget("icons");
    REQUIRE(tool->cut());
    CHECK(tool->message().find("content-cuts.json") != std::string::npos);
    CHECK(rig.art.read("content-cuts.json").find("\"lime gem\"") != std::string::npos);
    const game::ContentCuts cuts = game::loadContentCuts(rig.art.sprites / "content-cuts.json");
    const auto lime = std::find_if(cuts.cuts.begin(), cuts.cuts.end(), [](const game::ContentCut& cut) { return cut.name == "lime gem"; });
    REQUIRE(lime != cuts.cuts.end());
    CHECK(lime->rect.x == 70);
    CHECK(lime->rect.y == 30);
    CHECK(rig.art.read("atlas/content.json").find("lime gem") != std::string::npos);
    CHECK_FALSE(rig.said.empty());
    // The picker offers the new picture at once.
    luna_ui::Intents escape;
    escape.set(luna_ui::Intent::OpenMenu, true, true);
    rig.editor.update(escape);
    CHECK(rig.editor.pictureTool() == nullptr);
    REQUIRE(rig.editor.open("weapons.json"));
    REQUIRE(rig.editor.selectEntry("weapons[0]"));
    REQUIRE(rig.editor.setField("weapons[0].frame", "red gem")); // a frame of the page of icons: the picker starts there
    rig.editor.update(idle);
    REQUIRE(rig.editor.openPicker("weapons[0].frame"));
    rig.editor.update(idle);
    const game::PictureGrid* grid = find<game::PictureGrid>(*rig.editor.pictureTool()->panel());
    REQUIRE(grid != nullptr);
    CHECK(std::any_of(grid->cells().begin(), grid->cells().end(), [](const game::PictureGrid::Cell& cell) { return cell.name == "lime gem"; }));
}
