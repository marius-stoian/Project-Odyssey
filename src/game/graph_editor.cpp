#include "game/graph_editor.h"

#include "game/graph_commands.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace odysseus::game {

namespace fs = std::filesystem;
using luna::engine::Button;
using luna::engine::GraphNode;
using luna::engine::Intent;
using luna::engine::ListBox;
using luna::engine::NodeGraph;
using luna::engine::NodeGraphView;
using luna::engine::NumberField;
using luna::engine::Panel;
using luna::engine::Rect;
using luna::engine::TextField;
using luna::engine::UiColor;
using luna::engine::UiInput;
using luna::engine::UiPainter;

namespace {

constexpr int kBarHeight = 18;
constexpr int kListWidth = 120;
constexpr int kPanelWidth = 176;
constexpr int kStatusHeight = 14;
constexpr int kRow = 16;

std::string readText(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

// Written to a temporary file first and renamed over the real one, so a crash never leaves half a conversation (ADR-010).
bool writeText(const fs::path& path, const std::string& text) {
    const fs::path temp = fs::path(path).concat(".tmp");
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out << text;
        if (!out.good()) return false;
    }
    std::error_code ec;
    fs::rename(temp, path, ec);
    if (ec) {
        fs::remove(temp, ec);
        return false;
    }
    return true;
}

std::string joinWords(const std::vector<std::string>& words) {
    std::string out;
    for (const std::string& word : words) out += (out.empty() ? "" : " ") + word;
    return out;
}

std::vector<std::string> splitWords(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream in(text);
    for (std::string word; in >> word;) words.push_back(word);
    return words;
}

} // namespace

GraphEditor::GraphEditor(int viewWidth, int viewHeight, Record record, Say say)
    : viewWidth_(viewWidth), viewHeight_(viewHeight), record_(std::move(record)), say_(std::move(say)) {
    canvas_ = {kListWidth + 4, kBarHeight + 4, viewWidth - kListWidth - kPanelWidth - 8, viewHeight - kBarHeight - kStatusHeight - 6};
    chrome_ = std::make_unique<Panel>(Rect{0, 0, viewWidth, kBarHeight});
    panel_ = std::make_unique<Panel>(Rect{viewWidth - kPanelWidth - 2, kBarHeight + 4, kPanelWidth, viewHeight - kBarHeight - kStatusHeight - 6});
}

void GraphEditor::setFolders(fs::path dialogueFolder, std::function<void()> saved) {
    folder_ = std::move(dialogueFolder);
    saved_ = std::move(saved);
}

fs::path GraphEditor::fileOf(const std::string& name) const { return folder_ / (name + ".dlg"); }

std::vector<std::string> GraphEditor::files() const {
    std::vector<std::string> names;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(folder_, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".dlg") names.push_back(entry.path().stem().string());
    }
    std::sort(names.begin(), names.end());
    return names;
}

void GraphEditor::show(bool shown) {
    shown_ = shown;
    if (!shown_) return;
    buildChrome();
    if (current_ == nullptr) {
        const std::vector<std::string> names = files();
        if (!names.empty()) open(names.front());
    }
    buildPanel();
}

bool GraphEditor::open(const std::string& name) {
    const auto known = docs_.find(name);
    if (known == docs_.end()) {
        const fs::path file = fileOf(name);
        std::error_code ec;
        if (!fs::is_regular_file(file, ec)) {
            say_("Dialogue: no file " + name + ".dlg");
            return false;
        }
        sim::rules::LoadReport report;
        const std::optional<sim::rules::DlgScript> script = sim::rules::parseDialogue(readText(file), name, "dialogue/" + name + ".dlg", report);
        if (!script) {
            say_("Dialogue: " + (report.errors.empty() ? name : report.errors.front().text()));
            return false;
        }
        const fs::path sidecar = fs::path(file).concat(".layout.json");
        const DialogueLayout layout = fs::is_regular_file(sidecar, ec) ? layoutFromJson(readText(sidecar)) : DialogueLayout{};
        DialogueGraph d = dialogueToGraph(*script, layout);
        Doc& doc = docs_[name];
        doc.name = name;
        *doc.graph = std::move(d.graph);
        doc.header = std::move(d.header);
        doc.saved = *doc.graph;
        doc.savedHeader = headerText(doc.header);
        doc.loadedAt = fs::last_write_time(file, ec);
        doc.existed = true;
        current_ = &doc;
        view_ = std::make_unique<NodeGraphView>(canvas_, *doc.graph, [this](const std::string& what, const NodeGraph& before, const NodeGraph& after) {
            onViewEdit(what, before, after);
        });
        view_->frameAll();
    } else {
        current_ = &known->second;
        view_ = std::make_unique<NodeGraphView>(canvas_, *current_->graph, [this](const std::string& what, const NodeGraph& before, const NodeGraph& after) {
            onViewEdit(what, before, after);
        });
        view_->frameAll();
    }
    problems_.clear();
    overwriteArmed_ = false;
    panelKey_.clear();
    nodeCounter_ = 1;
    if (shown_) {
        buildChrome();
        buildPanel();
    }
    return true;
}

std::string GraphEditor::headerText(const sim::rules::DlgScript& script) const {
    sim::rules::DlgScript bare = script;
    bare.nodes.clear();
    return sim::rules::writeDialogue(bare);
}

bool GraphEditor::dirty() const {
    return current_ != nullptr && (!(*current_->graph == current_->saved) || headerText(current_->header) != current_->savedHeader);
}

void GraphEditor::onViewEdit(const std::string& name, const NodeGraph& before, const NodeGraph& after) {
    if (current_ == nullptr) return;
    record_(std::make_unique<GraphEditCommand>(name, current_->graph, before, after));
}

void GraphEditor::edit(const std::string& name, const std::function<void(NodeGraph&)>& change) {
    if (current_ == nullptr) return;
    const NodeGraph before = *current_->graph;
    change(*current_->graph);
    if (*current_->graph == before) return;
    record_(std::make_unique<GraphEditCommand>(name, current_->graph, before, *current_->graph));
}

int GraphEditor::addCard(const std::string& type) {
    if (current_ == nullptr || !view_) return 0;
    GraphNode card = newDialogueCard(type);
    if (type == dlg_card::kNode) {
        std::string id;
        do {
            id = "node" + std::to_string(nodeCounter_++);
        } while (std::any_of(current_->graph->nodes().begin(), current_->graph->nodes().end(),
                             [&](const GraphNode& other) { return other.type == dlg_card::kNode && !other.fields.empty() && other.fields[0] == id; }));
        card.fields = {id};
        describeCard(card);
    }
    // Each new card goes a little lower and further right than the last, so a row of them can be told apart.
    const int step = static_cast<int>(current_->graph->nodes().size() % 8) * 10;
    return view_->addNodeAt(card, canvas_.x + canvas_.width / 3 + step, canvas_.y + canvas_.height / 3 + step);
}

bool GraphEditor::setCardField(int card, std::size_t index, const std::string& value) {
    if (current_ == nullptr) return false;
    const GraphNode* found = current_->graph->find(card);
    if (found == nullptr) return false;
    edit("edit " + found->type, [&](NodeGraph& g) {
        GraphNode* node = g.find(card);
        if (node->fields.size() <= index) node->fields.resize(index + 1);
        node->fields[index] = value;
        describeCard(*node);
    });
    return true;
}

bool GraphEditor::addCardField(int card) {
    if (current_ == nullptr) return false;
    const GraphNode* found = current_->graph->find(card);
    if (found == nullptr || (found->type != dlg_card::kEffect && found->type != dlg_card::kComment)) return false;
    edit("add a line", [&](NodeGraph& g) {
        GraphNode* node = g.find(card);
        node->fields.emplace_back();
        describeCard(*node);
    });
    return true;
}

bool GraphEditor::removeCardField(int card) {
    if (current_ == nullptr) return false;
    const GraphNode* found = current_->graph->find(card);
    if (found == nullptr || (found->type != dlg_card::kEffect && found->type != dlg_card::kComment) || found->fields.size() <= 1) return false;
    edit("remove a line", [&](NodeGraph& g) {
        GraphNode* node = g.find(card);
        node->fields.pop_back();
        describeCard(*node);
    });
    return true;
}

bool GraphEditor::save() {
    if (current_ == nullptr) return false;
    Doc& doc = *current_;
    DialogueGraph d{*doc.graph, doc.header};
    problems_.clear();
    const std::optional<sim::rules::DlgScript> script = graphToDialogue(d, problems_);
    if (!script) {
        const auto error = std::find_if(problems_.begin(), problems_.end(), [](const std::string& p) { return p.rfind("error:", 0) == 0; });
        say_("Not saved: " + (error != problems_.end() ? *error : std::string("the conversation cannot be written")));
        return false;
    }
    const fs::path file = fileOf(doc.name);
    std::error_code ec;
    if (doc.existed && fs::is_regular_file(file, ec) && fs::last_write_time(file, ec) != doc.loadedAt && !overwriteArmed_) {
        overwriteArmed_ = true;
        say_(doc.name + ".dlg changed on disk since it was opened: press Save again to overwrite it");
        return false;
    }
    if (fs::is_regular_file(file, ec)) fs::copy_file(file, fs::path(file).concat(".bak"), fs::copy_options::overwrite_existing, ec);
    if (!writeText(file, sim::rules::writeDialogue(*script)) || !writeText(fs::path(file).concat(".layout.json"), layoutToJson(layoutOf(d)))) {
        say_("Not saved: " + doc.name + ".dlg cannot be written");
        return false;
    }
    doc.saved = *doc.graph;
    doc.savedHeader = headerText(doc.header);
    doc.loadedAt = fs::last_write_time(file, ec);
    doc.existed = true;
    overwriteArmed_ = false;
    int warnings = 0;
    for (const std::string& p : problems_) warnings += p.rfind("warning:", 0) == 0 ? 1 : 0;
    say_("Saved " + doc.name + ".dlg" + (warnings > 0 ? " (" + std::to_string(warnings) + " card(s) not connected)" : ""));
    if (saved_) saved_();
    return true;
}

int GraphEditor::selectedCard() const {
    if (!view_ || view_->selection().size() != 1) return 0;
    return *view_->selection().begin();
}

void GraphEditor::refresh() {
    if (!view_ || current_ == nullptr) return;
    // A card may be gone after an Undo.
    std::vector<int> gone;
    for (const int id : view_->selection()) {
        if (current_->graph->find(id) == nullptr) gone.push_back(id);
    }
    if (!gone.empty()) view_->clearSelection();
    panelKey_.clear();
}

bool GraphEditor::typing() const { return shown_ && ((panel_ && panel_->typing()) || (chrome_ && chrome_->typing())); }

void GraphEditor::buildChrome() {
    chrome_ = std::make_unique<Panel>(Rect{0, 0, viewWidth_, kBarHeight}); // only the bar is the panel: the file list and the canvas are not under it
    int x = 2;
    auto button = [&](const std::string& label, const std::string& hint, auto action) {
        const int width = UiPainter::textWidth(label) + 8;
        Button& added = chrome_->add<Button>(Rect{x, 2, width, kBarHeight - 4}, label, action);
        added.hint = hint;
        x += width + 2;
    };
    button("Close", "Back to the map (Esc)", [this] { show(false); });
    button("Save", "Write the conversation and its layout (Ctrl+S)", [this] { save(); });
    button("Frame", "Show every card", [this] {
        if (view_) view_->frameAll();
    });
    x += 6;
    button("+Node", "A new node: a stop in the conversation", [this] { addCard(dlg_card::kNode); });
    button("+Line", "A line someone says", [this] { addCard(dlg_card::kLine); });
    button("+Choice", "A choice for the player", [this] { addCard(dlg_card::kChoice); });
    button("+If", "A condition: wire it to the If port of a line or choice", [this] { addCard(dlg_card::kCondition); });
    button("+Do", "Effects: wire them to the Do port of a choice", [this] { addCard(dlg_card::kEffect); });
    button("+Goto", "A named jump to a node, for a wire that would be too long", [this] { addCard(dlg_card::kGoto); });
    button("+Note", "A note kept in the file: wire it to what it is about", [this] { addCard(dlg_card::kComment); });
    const std::vector<std::string> names = files();
    ListBox& list = chrome_->add<ListBox>(Rect{2, kBarHeight + 4, kListWidth, viewHeight_ - kBarHeight - kStatusHeight - 6}, names, [this, names](int index) {
        if (index >= 0 && index < static_cast<int>(names.size())) open(names[static_cast<std::size_t>(index)]);
    });
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (current_ != nullptr && names[i] == current_->name) list.selected = static_cast<int>(i);
    }
}

void GraphEditor::buildPanel() {
    panel_ = std::make_unique<Panel>(Rect{viewWidth_ - kPanelWidth - 2, kBarHeight + 4, kPanelWidth, viewHeight_ - kBarHeight - kStatusHeight - 6});
    panelTitle_.clear();
    if (current_ == nullptr) return;
    const int left = panel_->bounds.x + 3;
    const int width = kPanelWidth - 6;
    int y = panel_->bounds.y + 14;
    const int id = selectedCard();
    const GraphNode* card = id != 0 ? current_->graph->find(id) : nullptr;
    auto field = [&](const std::string& label, const std::string& value, std::function<void(const std::string&)> set) {
        panel_->add<TextField>(Rect{left, y, width, kRow - 2}, label, value, 200, std::move(set));
        y += kRow;
    };
    if (card == nullptr) {
        panelTitle_ = current_->name + ".dlg";
        sim::rules::DlgScript& h = current_->header;
        field("who: ", joinWords(h.who), [&h](const std::string& v) { h.who = splitWords(v); });
        field("when: ", h.whenSource, [&h](const std::string& v) { h.whenSource = v; });
        panel_->add<NumberField>(Rect{left, y, width, kRow - 2}, "priority: ", h.priority, -1000, 1000, [&h](int v) { h.priority = v; });
        y += kRow;
        field("bark: ", h.bark, [&h](const std::string& v) { h.bark = v; });
        field("pair: ", joinWords(h.pair), [&h](const std::string& v) { h.pair = splitWords(v); });
        return;
    }
    panelTitle_ = card->type;
    const int cardId = card->id;
    const auto set = [this, cardId](std::size_t index) { return [this, cardId, index](const std::string& v) { setCardField(cardId, index, v); }; };
    const auto at = [&](std::size_t i) { return i < card->fields.size() ? card->fields[i] : std::string(); };
    if (card->type == dlg_card::kNode) {
        field("id: ", at(0), set(0));
    } else if (card->type == dlg_card::kLine) {
        field("who: ", at(0), set(0));
        field("says: ", at(1), set(1));
    } else if (card->type == dlg_card::kChoice) {
        field("text: ", at(0), set(0));
        field("else: ", at(1), set(1));
    } else if (card->type == dlg_card::kCondition) {
        field("if: ", at(0), set(0));
    } else if (card->type == dlg_card::kGoto) {
        field("to: ", at(0), set(0));
    } else {
        for (std::size_t i = 0; i < card->fields.size(); ++i) field(card->type == dlg_card::kEffect ? "do: " : "note: ", at(i), set(i));
        panel_->add<Button>(Rect{left, y, 40, kRow - 2}, "+ line", [this, cardId] { addCardField(cardId); });
        panel_->add<Button>(Rect{left + 44, y, 40, kRow - 2}, "- line", [this, cardId] { removeCardField(cardId); });
        y += kRow;
    }
    panel_->add<Button>(Rect{left, y + 4, 60, kRow - 2}, "Delete", [this] {
        if (view_) view_->deleteSelected();
    });
}

void GraphEditor::update(const luna::engine::Intents& intents) {
    if (!shown_) return;
    const UiInput input = UiInput::from(intents);
    // The side panel follows the chosen card and what is in it; it is rebuilt when either changed, but never under a field being typed in.
    std::string key;
    if (current_ != nullptr) {
        const int id = selectedCard();
        key = std::to_string(id);
        if (const GraphNode* card = id != 0 ? current_->graph->find(id) : nullptr) {
            for (const std::string& f : card->fields) key += "|" + f;
        }
    }
    if (key != panelKey_ && !panel_->typing()) {
        panelKey_ = key;
        buildPanel();
    }
    const bool onChrome = chrome_->handle(input);
    const bool onPanel = panel_->handle(input);
    if (!onChrome && !onPanel && view_) view_->handle(input);
    if (!typing() && view_ && intents.pressed(Intent::Delete)) view_->deleteSelected();
}

void GraphEditor::draw(UiPainter& painter) const {
    if (!shown_) return;
    painter.fill({0, 0, viewWidth_, viewHeight_}, UiColor::Dark);
    if (view_) view_->draw(painter);
    chrome_->draw(painter);
    panel_->draw(painter);
    painter.text(panel_->bounds.x + 4, panel_->bounds.y + 3, panelTitle_, UiColor::Gold);
    const Rect bar{0, viewHeight_ - kStatusHeight, viewWidth_, kStatusHeight};
    painter.fill(bar, UiColor::Shade);
    std::string line = current_ != nullptr ? "Dialogue: " + current_->name + ".dlg" + (dirty() ? "  *unsaved" : "") : "Dialogue: no file";
    if (view_) line += "  zoom " + std::to_string(view_->zoomPercent()) + "%";
    if (!problems_.empty()) line += "  " + problems_.front();
    painter.text(4, bar.y + 3, line.substr(0, static_cast<std::size_t>((viewWidth_ - 8) / luna::engine::kTextAdvance)), UiColor::Text);
}

void GraphEditor::drawOverlay(UiPainter& painter) const {
    if (!shown_) return;
    chrome_->drawOverlay(painter);
    panel_->drawOverlay(painter);
}

} // namespace odysseus::game
