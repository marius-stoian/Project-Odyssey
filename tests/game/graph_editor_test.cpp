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
        fs::create_directories(path / "interactions");
        fs::copy_file(fs::path(ODYSSEUS_DATA_DIR) / "interactions" / "gather.json", path / "interactions" / "gather.json");
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
        editor.setFolders(folder.path, folder.path / "interactions", [this] { ++savedCalls; });
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

TEST_CASE("US-172 Open: the Rules side lists the interactions and opens gather as a graph") {
    Folder folder;
    Rig rig(folder);
    rig.editor.showKind(GraphEditor::Kind::Interaction);
    CHECK(rig.editor.files() == std::vector<std::string>{"gather"});
    REQUIRE(rig.editor.open("gather"));
    CHECK(rig.editor.openName() == "gather");
    CHECK(rig.editor.graph()->nodes().size() == 6);
    // The folders of the two kinds do not mix: gather is not a conversation.
    rig.editor.showKind(GraphEditor::Kind::Dialogue);
    CHECK(rig.editor.files() == std::vector<std::string>{"elder-fire"});
}

TEST_CASE("US-172 Save: a changed range reaches the file, the comment on top stays, the layout is beside it") {
    Folder folder;
    Rig rig(folder);
    rig.editor.showKind(GraphEditor::Kind::Interaction);
    REQUIRE(rig.editor.open("gather"));
    int verb = 0;
    for (const auto& c : rig.editor.graph()->nodes()) {
        if (c.type == rule_card::kVerb) verb = c.id;
    }
    REQUIRE(verb != 0);
    CHECK(rig.editor.setCardField(verb, 3, "1.5"));
    CHECK(rig.editor.dirty());
    REQUIRE(rig.editor.save());
    const std::string text = readFile(folder.path / "interactions" / "gather.json");
    CHECK(text.rfind("// Gather from a plant that is ripe", 0) == 0); // the leading comment
    CHECK(text.find("\"range\": 1.5") != std::string::npos);
    CHECK(fs::exists(folder.path / "interactions" / "gather.json.layout.json"));
    CHECK(fs::exists(folder.path / "interactions" / "gather.json.bak"));
    CHECK(rig.savedCalls == 1);
    // The sidecar is not listed as an interaction.
    CHECK(rig.editor.files() == std::vector<std::string>{"gather"});
}

TEST_CASE("US-172 Save: the verb id must stay the name of the file, and Undo walks back through the one History") {
    Folder folder;
    Rig rig(folder);
    rig.editor.showKind(GraphEditor::Kind::Interaction);
    REQUIRE(rig.editor.open("gather"));
    int verb = 0;
    for (const auto& c : rig.editor.graph()->nodes()) {
        if (c.type == rule_card::kVerb) verb = c.id;
    }
    const luna::engine::NodeGraph start = *rig.editor.graph();
    CHECK(rig.editor.setCardField(verb, 0, "pick"));
    CHECK_FALSE(rig.editor.save());
    CHECK(rig.said.back().rfind("Not saved:", 0) == 0);
    CHECK(rig.history.undo(rig.level));
    CHECK(*rig.editor.graph() == start);
}

// The picture for docs/evidence/US-172, saved in the temp folder and copied by hand.
TEST_CASE("US-172 Draw: the graph editor shows an interaction") {
    Folder folder;
    Rig rig(folder);
    rig.editor.showKind(GraphEditor::Kind::Interaction);
    REQUIRE(rig.editor.open("gather"));
    luna::engine::ImageRenderer renderer(960, 540);
    renderer.clear({20, 20, 28, 255});
    const luna::engine::Texture sheet = renderer.createTexture(luna::engine::makeUiSheet());
    luna::engine::UiPainter painter(renderer, sheet);
    painter.setScreen({0, 0, 960, 540});
    luna::engine::Intents intents;
    rig.editor.update(intents);
    rig.editor.draw(painter);
    rig.editor.drawOverlay(painter);
    CHECK(luna::engine::savePng(renderer.image(), fs::temp_directory_path() / "odysseus-us172-interaction-graph.png"));
}

namespace {

// One tick with the left button pressed at a point, as a click on a list row.
luna::engine::Intents clickAt(int x, int y) {
    luna::engine::Intents intents;
    luna::engine::Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    pointer.pressed[0] = true;
    pointer.held[0] = true;
    intents.setPointer(pointer);
    return intents;
}

} // namespace

TEST_CASE("US-175 Unreachable: the editor lists a node no choice leads to, and clicking the finding selects that node") {
    Folder folder;
    writeFile(folder.path / "orphan.dlg", "=== start\nElder: Hi.\n-> Bye => END\n\n=== lost\nElder: Lonely.\n-> Back => start\n");
    Rig rig(folder);
    REQUIRE(rig.editor.open("orphan"));
    rig.editor.show(true);
    luna::engine::Intents idle;
    rig.editor.update(idle);
    REQUIRE(rig.editor.findings().size() == 1);
    CHECK_FALSE(rig.editor.findings()[0].error);
    CHECK(rig.editor.findings()[0].text.find("\"lost\" cannot be reached") != std::string::npos);
    // A click on the first row of the list.
    const luna::engine::Rect list = rig.editor.problemListBounds();
    rig.editor.update(clickAt(list.x + 10, list.y + 3));
    const int lost = rig.card(dlg_card::kNode, "lost");
    REQUIRE(lost != 0);
    CHECK(rig.editor.view()->selection().count(lost) == 1);
    CHECK(rig.editor.view()->selection().size() == 1);
}

TEST_CASE("US-175 Dead end: a node with no choice and no END is listed as an error") {
    Folder folder;
    writeFile(folder.path / "stuck.dlg", "=== start\nElder: Hi.\n-> On => stuck\n\n=== stuck\nElder: ...\n");
    Rig rig(folder);
    REQUIRE(rig.editor.open("stuck"));
    rig.editor.recheck();
    REQUIRE(rig.editor.findings().size() == 1);
    CHECK(rig.editor.findings()[0].error);
    CHECK(rig.editor.findings()[0].text.find("dead end") != std::string::npos);
    CHECK(rig.editor.pickProblem(0));
    CHECK(rig.editor.view()->selection().count(rig.card(dlg_card::kNode, "stuck")) == 1);
}

TEST_CASE("US-175 Unknown: an effect giving an unknown item names the item and the node; saving is allowed with a warning") {
    Folder folder;
    writeFile(folder.path / "gift.dlg", "=== start\nElder: Hi.\n-> Share {give hero glowstone 1} => END\n");
    Rig rig(folder);
    odysseus::sim::rules::GraphCatalog catalog;
    catalog.items = {"berries"};
    rig.editor.setCatalog(catalog);
    REQUIRE(rig.editor.open("gift"));
    rig.editor.recheck();
    REQUIRE(rig.editor.findings().size() == 1);
    CHECK(rig.editor.findings()[0].text == "start, choice 1: unknown item \"glowstone\"");
    CHECK(rig.editor.pickProblem(0));
    // The file can still be saved (D-56 Q18); the note says it will not ship.
    CHECK(rig.editor.setCardField(rig.card(dlg_card::kChoice, "Share"), 0, "Share it"));
    REQUIRE(rig.editor.save());
    CHECK(rig.said.back().find("1 error(s) block shipping") != std::string::npos);
    // Fixing the name clears the finding at once.
    int effect = 0;
    for (const auto& c : rig.editor.graph()->nodes()) {
        if (c.type == dlg_card::kEffect) effect = c.id;
    }
    REQUIRE(effect != 0);
    CHECK(rig.editor.setCardField(effect, 0, "give hero berries 1"));
    rig.editor.recheck();
    CHECK(rig.editor.findings().empty());
}

TEST_CASE("US-175 Unknown: the interaction graph is checked too") {
    Folder folder;
    Rig rig(folder);
    rig.editor.showKind(GraphEditor::Kind::Interaction);
    odysseus::sim::rules::GraphCatalog catalog;
    catalog.items = {"stick"};
    catalog.tags = {"plant"};
    rig.editor.setCatalog(catalog);
    REQUIRE(rig.editor.open("gather"));
    rig.editor.recheck();
    // gather.json gives berries and aims at the tag "edible", neither in this small catalog.
    bool target = false;
    for (const auto& p : rig.editor.findings()) target = target || p.key == "target";
    CHECK(target);
    CHECK(rig.editor.pickProblem(0));
}

namespace {

const char* kTestScript =
    "=== start\n"
    "Elder: Hello.\n"
    "-> Ask for help [if opinion(npc, hero) >= 20] [else Not yet] => help\n"
    "-> Leave => END\n"
    "\n"
    "=== help\n"
    "Elder: Take this.\n"
    "-> Thanks {give hero stone 1} => END\n";

} // namespace

TEST_CASE("US-174 Branch: with the opinion set to 25 the choice that needs 20 is offered") {
    Folder folder;
    writeFile(folder.path / "help.dlg", kTestScript);
    Rig rig(folder);
    REQUIRE(rig.editor.open("help"));
    rig.editor.setTestWords("");
    REQUIRE(rig.editor.startTest(false));
    CHECK_FALSE(rig.editor.testPlay()->view().choices[0].enabled);
    rig.editor.setTestWords("opinion=25");
    REQUIRE(rig.editor.startTest(false));
    CHECK(rig.editor.testPlay()->view().choices[0].enabled);
    CHECK(rig.editor.testChoose(0));
    CHECK(rig.editor.testPlay()->nodeId() == "help");
}

TEST_CASE("US-174 Start anywhere: Play from here starts at the node of the selected card, with edits not yet saved") {
    Folder folder;
    writeFile(folder.path / "help.dlg", kTestScript);
    Rig rig(folder);
    REQUIRE(rig.editor.open("help"));
    CHECK_FALSE(rig.editor.startTest(true)); // nothing selected
    CHECK(rig.said.back().rfind("Test-play: select a card", 0) == 0);
    const int line = rig.card(dlg_card::kLine, "Elder");
    REQUIRE(line != 0);
    // The first Elder line belongs to "start": select the line of "help" instead.
    int helpLine = 0;
    for (const auto& c : rig.editor.graph()->nodes()) {
        if (c.type == dlg_card::kLine && c.fields.size() > 1 && c.fields[1] == "Take this.") helpLine = c.id;
    }
    REQUIRE(helpLine != 0);
    CHECK(rig.editor.setCardField(helpLine, 1, "Take this, friend.")); // not saved
    rig.editor.view()->select(helpLine);
    REQUIRE(rig.editor.startTest(true));
    CHECK(rig.editor.testPlay()->nodeId() == "help");
    CHECK(rig.editor.testPlay()->view().lines[0].text == "Take this, friend.");
}

TEST_CASE("US-174 No side effects: a test-play that gives items leaves the file, the undo steps and the saved state as they were") {
    Folder folder;
    writeFile(folder.path / "help.dlg", kTestScript);
    Rig rig(folder);
    REQUIRE(rig.editor.open("help"));
    const std::string before = readFile(folder.path / "help.dlg");
    const std::size_t steps = rig.history.size();
    rig.editor.setTestWords("opinion=30");
    REQUIRE(rig.editor.startTest(false));
    REQUIRE(rig.editor.testChoose(0));
    REQUIRE(rig.editor.testChoose(0)); // Thanks: gives a stone
    CHECK(rig.editor.testPlay()->state().items.at("stone") == 1);
    CHECK(rig.editor.testPlay()->finished());
    rig.editor.stopTest();
    CHECK_FALSE(rig.editor.testing());
    CHECK(readFile(folder.path / "help.dlg") == before);
    CHECK(rig.history.size() == steps);
    CHECK_FALSE(rig.editor.dirty());
    CHECK(rig.savedCalls == 0);
    CHECK(fs::directory_iterator(folder.path) != fs::directory_iterator()); // the folder holds what it did: the .dlg and the interactions folder
    int files = 0;
    for (const auto& entry : fs::directory_iterator(folder.path)) files += entry.is_regular_file() ? 1 : 0;
    CHECK(files == 2); // elder-fire.dlg and help.dlg: no sidecar, no backup
}

TEST_CASE("US-174 State: a mistake in the values is said and nothing starts; a conversation that cannot be written is not played") {
    Folder folder;
    Rig rig(folder);
    REQUIRE(rig.editor.open("elder-fire"));
    rig.editor.setTestWords("opinion=999");
    CHECK_FALSE(rig.editor.startTest(false));
    CHECK(rig.said.back().rfind("Test-play: ", 0) == 0);
    CHECK_FALSE(rig.editor.testing());
    rig.editor.setTestWords("");
    const int condition = rig.editor.addCard(dlg_card::kCondition);
    CHECK(rig.editor.setCardField(condition, 0, "this is not ( an expression"));
    REQUIRE(rig.editor.graph()->connect(condition, 0, rig.card(dlg_card::kChoice, "Leave"), 1));
    CHECK_FALSE(rig.editor.startTest(false));
    CHECK_FALSE(rig.editor.testing());
}

TEST_CASE("US-174 Draw: the Test-play card shows the lines, the numbered choices and the log") {
    Folder folder;
    writeFile(folder.path / "help.dlg", kTestScript);
    Rig rig(folder);
    REQUIRE(rig.editor.open("help"));
    rig.editor.show(true);
    rig.editor.showTest(true);
    rig.editor.setTestWords("opinion=25");
    REQUIRE(rig.editor.startTest(false));
    luna::engine::ImageRenderer renderer(960, 540);
    renderer.clear({20, 20, 28, 255});
    const luna::engine::Texture sheet = renderer.createTexture(luna::engine::makeUiSheet());
    luna::engine::UiPainter painter(renderer, sheet);
    painter.setScreen({0, 0, 960, 540});
    luna::engine::Intents idle;
    rig.editor.update(idle);
    rig.editor.draw(painter);
    rig.editor.drawOverlay(painter);
    CHECK(luna::engine::savePng(renderer.image(), fs::temp_directory_path() / "odysseus-us174-test-play.png"));
    // A click on the first choice button of the card chooses it.
    const luna::engine::Rect canvas = rig.editor.canvasBounds();
    luna::engine::Intents click;
    luna::engine::Pointer pointer;
    pointer.x = canvas.x + 20;
    pointer.y = canvas.y + canvas.height - 216 + 45 + 4 * luna::engine::kLineHeight + 2 + 4; // the first choice row
    pointer.pressed[0] = true;
    pointer.held[0] = true;
    pointer.released[0] = true; // a Button reacts when the pointer is let go over it
    click.setPointer(pointer);
    rig.editor.update(click);
    CHECK(rig.editor.testPlay()->nodeId() == "help");
}
