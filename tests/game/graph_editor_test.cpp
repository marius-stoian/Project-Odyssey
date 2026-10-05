// US-171 Dialogue graph editor: open a conversation, edit it, save it with its layout, keep the owner's notes, share one Undo.
#include "game/dialogue_graph.h"
#include "game/editor_history.h"
#include "game/graph_editor.h"
#include "game/level.h"
#include "luna/engine/image_io.h"
#include "luna/engine/ui.h"

#include <doctest/doctest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using namespace odysseus::game;

namespace {

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

void writeFile(const fs::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

// A folder of its own with copies of the shipped conversations, removed again at the end.
struct Folder {
    fs::path path;
    Folder() {
        path = fs::temp_directory_path() / ("odysseus-us171-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(path);
        fs::copy_file(fs::path(ODYSSEUS_DATA_DIR) / "dialogue" / "elder-fire.dlg", path / "elder-fire.dlg");
    }
    ~Folder() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

struct Rig {
    History history;
    Level level;
    std::vector<std::string> said;
    int savedCalls = 0;
    GraphEditor editor;
    explicit Rig(const Folder& folder)
        : editor(960, 540, [this](std::unique_ptr<Command> c) { history.record(std::move(c)); }, [this](const std::string& m) { said.push_back(m); }) {
        editor.setFolders(folder.path, [this] { ++savedCalls; });
    }
    int card(const char* type, const std::string& first) {
        for (const auto& c : editor.graph()->nodes()) {
            if (c.type == type && !c.fields.empty() && c.fields[0] == first) return c.id;
        }
        return 0;
    }
};

} // namespace

TEST_CASE("US-171 Open: the editor lists the folder and opens elder-fire as cards and wires") {
    Folder folder;
    Rig rig(folder);
    CHECK(rig.editor.files() == std::vector<std::string>{"elder-fire"});
    REQUIRE(rig.editor.open("elder-fire"));
    CHECK(rig.editor.openName() == "elder-fire");
    CHECK_FALSE(rig.editor.dirty());
    CHECK(rig.card(dlg_card::kNode, "hunt") != 0);
    CHECK_FALSE(rig.editor.open("nothing-here"));
}

TEST_CASE("US-171 Save: a node added and a choice rewired reach the file in canonical form, with the positions beside it") {
    Folder folder;
    Rig rig(folder);
    REQUIRE(rig.editor.open("elder-fire"));
    const int node = rig.editor.addCard(dlg_card::kNode);
    REQUIRE(node != 0);
    CHECK(rig.editor.setCardField(node, 0, "rest"));
    const int line = rig.editor.addCard(dlg_card::kLine);
    CHECK(rig.editor.setCardField(line, 0, "Elder"));
    CHECK(rig.editor.setCardField(line, 1, "Sleep well."));
    // The wires go through the same graph the canvas edits.
    luna::engine::NodeGraph before = *rig.editor.graph();
    REQUIRE(rig.editor.graph()->connect(node, 0, line, 0));
    REQUIRE(rig.editor.graph()->connect(rig.card(dlg_card::kChoice, "Leave"), 1, node, 0));
    CHECK(rig.editor.dirty());
    REQUIRE(rig.editor.save());
    CHECK(rig.savedCalls == 1);
    CHECK_FALSE(rig.editor.dirty());

    const std::string text = readFile(folder.path / "elder-fire.dlg");
    CHECK(text.find("-> Leave => rest") != std::string::npos);
    CHECK(text.find("=== rest\nElder: Sleep well.\n") != std::string::npos);
    const fs::path sidecar = folder.path / "elder-fire.dlg.layout.json";
    REQUIRE(fs::exists(sidecar));
    const DialogueLayout layout = layoutFromJson(readFile(sidecar));
    CHECK(layout.count("rest") == 1);
    CHECK(layout.count("start") == 1);

    // A new editor opens the saved file with every card where it was left.
    Rig second(folder);
    REQUIRE(second.editor.open("elder-fire"));
    CHECK(layoutOf(DialogueGraph{*second.editor.graph(), second.editor.header()}) == layout);
}

TEST_CASE("US-171 Hand edits: the notes of a file edited in a text editor survive a change in the graph") {
    Folder folder;
    writeFile(folder.path / "notes.dlg",
              "# Top note\n@who elder\n\n# Note on start\n=== start\n# Note on the line\nElder: Hello.\n# Note on the choice\n-> Bye => END\n");
    Rig rig(folder);
    REQUIRE(rig.editor.open("notes"));
    CHECK(rig.editor.setCardField(rig.card(dlg_card::kLine, "Elder"), 1, "Hello again."));
    REQUIRE(rig.editor.save());
    const std::string text = readFile(folder.path / "notes.dlg");
    CHECK(text.find("# Top note") != std::string::npos);
    CHECK(text.find("# Note on start\n=== start") != std::string::npos);
    CHECK(text.find("# Note on the line\nElder: Hello again.") != std::string::npos);
    CHECK(text.find("# Note on the choice\n-> Bye => END") != std::string::npos);
    CHECK(fs::exists(folder.path / "notes.dlg.bak")); // the version before the save
}

TEST_CASE("US-171 Save: a file changed on disk since it was opened asks once before it is overwritten") {
    Folder folder;
    Rig rig(folder);
    REQUIRE(rig.editor.open("elder-fire"));
    const fs::path file = folder.path / "elder-fire.dlg";
    const std::string other = readFile(file) + "# written in Notepad\n";
    writeFile(file, other);
    fs::last_write_time(file, fs::last_write_time(file) + std::chrono::seconds(5));
    CHECK(rig.editor.setCardField(rig.card(dlg_card::kNode, "hunt"), 0, "hunt"));
    rig.editor.addCard(dlg_card::kComment); // makes the graph differ, though the file need not change
    CHECK_FALSE(rig.editor.save());
    CHECK(readFile(file) == other);
    CHECK(rig.editor.save());
    CHECK(rig.said.back().rfind("Saved elder-fire.dlg", 0) == 0);
}

TEST_CASE("US-171 Save: a card the parser refuses is reported and the file stays as it was") {
    Folder folder;
    Rig rig(folder);
    REQUIRE(rig.editor.open("elder-fire"));
    const std::string before = readFile(folder.path / "elder-fire.dlg");
    const int condition = rig.editor.addCard(dlg_card::kCondition);
    CHECK(rig.editor.setCardField(condition, 0, "this is not ( an expression"));
    REQUIRE(rig.editor.graph()->connect(condition, 0, rig.card(dlg_card::kChoice, "Leave"), 1));
    CHECK_FALSE(rig.editor.save());
    CHECK(readFile(folder.path / "elder-fire.dlg") == before);
    REQUIRE_FALSE(rig.editor.problems().empty());
    CHECK(rig.editor.problems().front().rfind("error:", 0) == 0);
    CHECK(rig.said.back().rfind("Not saved:", 0) == 0);
}

TEST_CASE("US-171 Undo: card edits are steps of the one History and Ctrl+Z restores the graph") {
    Folder folder;
    Rig rig(folder);
    REQUIRE(rig.editor.open("elder-fire"));
    const luna::engine::NodeGraph start = *rig.editor.graph();
    const int hunt = rig.card(dlg_card::kNode, "hunt");
    CHECK(rig.editor.setCardField(hunt, 0, "hunting"));
    rig.editor.addCard(dlg_card::kLine);
    CHECK(rig.history.size() == 2);
    CHECK(rig.editor.dirty());
    CHECK(rig.history.undo(rig.level));
    CHECK(rig.history.undo(rig.level));
    CHECK(*rig.editor.graph() == start);
    CHECK_FALSE(rig.editor.dirty());
    CHECK(rig.history.redo(rig.level));
    CHECK(rig.editor.graph()->nodes().size() == start.nodes().size());
}

// The picture for docs/evidence/US-171: the graph editor on elder-fire, drawn with the pixel renderer. Written
// in the temp folder.
TEST_CASE("US-171 Draw: the graph editor shows the open conversation") {
    Folder folder;
    Rig rig(folder);
    REQUIRE(rig.editor.open("elder-fire"));
    rig.editor.show(true);
    luna::engine::ImageRenderer renderer(960, 540);
    renderer.clear({20, 20, 28, 255});
    const luna::engine::Texture sheet = renderer.createTexture(luna::engine::makeUiSheet());
    luna::engine::UiPainter painter(renderer, sheet);
    painter.setScreen({0, 0, 960, 540});
    luna::engine::Intents intents;
    rig.editor.update(intents);
    rig.editor.draw(painter);
    rig.editor.drawOverlay(painter);
    // Something was drawn in the canvas, and the side panel and the bar are there.
    int lit = 0; // pixels in the canvas that are not its background
    const luna::engine::Rect canvas = rig.editor.canvasBounds();
    const luna::engine::Color background = renderer.image().get(canvas.x + 2, canvas.y + 2);
    for (int y = canvas.y; y < canvas.y + canvas.height; ++y) {
        for (int x = canvas.x; x < canvas.x + canvas.width; ++x) lit += renderer.image().get(x, y) != background ? 1 : 0;
    }
    CHECK(lit > 500);
    // For docs/evidence/US-171: the picture is also saved in the temp folder, from where it is copied by hand.
    CHECK(luna::engine::savePng(renderer.image(), fs::temp_directory_path() / "odysseus-us171-dialogue-graph.png"));
}
