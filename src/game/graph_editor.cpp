#include "game/graph_editor.h"

#include "game/graph_commands.h"

#include <algorithm>
#include <cctype>
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
constexpr int kProblemsHeight = 56; // the list of findings under the canvas

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
    canvas_ = {kListWidth + 4, kBarHeight + 4, viewWidth - kListWidth - kPanelWidth - 8, viewHeight - kBarHeight - kStatusHeight - 6 - kProblemsHeight};
    problemList_ = std::make_unique<ListBox>(Rect{canvas_.x, canvas_.y + canvas_.height + 4, canvas_.width, kProblemsHeight - 4}, std::vector<std::string>{}, [this](int index) {
        if (index >= 0) pickProblem(static_cast<std::size_t>(index));
    });
    chrome_ = std::make_unique<Panel>(Rect{0, 0, viewWidth, kBarHeight});
    panel_ = std::make_unique<Panel>(Rect{viewWidth - kPanelWidth - 2, kBarHeight + 4, kPanelWidth, viewHeight - kBarHeight - kStatusHeight - 6});
}

void GraphEditor::setFolders(fs::path dialogueFolder, fs::path interactionFolder, std::function<void()> saved) {
    folder_ = std::move(dialogueFolder);
    interactionFolder_ = std::move(interactionFolder);
    saved_ = std::move(saved);
}

fs::path GraphEditor::fileOf(const std::string& name) const {
    return kind_ == Kind::Dialogue ? folder_ / (name + ".dlg") : interactionFolder_ / (name + ".json");
}

std::vector<std::string> GraphEditor::files() const {
    std::vector<std::string> names;
    std::error_code ec;
    const std::string extension = kind_ == Kind::Dialogue ? ".dlg" : ".json";
    for (const auto& entry : fs::directory_iterator(kind_ == Kind::Dialogue ? folder_ : interactionFolder_, ec)) {
        if (!entry.is_regular_file() || entry.path().extension() != extension) continue;
        const std::string stem = entry.path().stem().string();
        if (stem.size() > 7 && stem.compare(stem.size() - 7, 7, ".layout") == 0) continue; // `<name>.dlg.layout.json` and `<id>.json.layout.json` are sidecars
        names.push_back(stem);
    }
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::string> GraphEditor::dialogueNames() const {
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
    if (current_ == nullptr || current_->kind != kind_) {
        current_ = nullptr;
        view_.reset();
        const std::vector<std::string> names = files();
        if (!names.empty()) open(names.front());
    }
    rebuild_ = true;
}

void GraphEditor::showKind(Kind kind) {
    kind_ = kind;
    current_ = nullptr;
    view_.reset();
    show(true);
}

void GraphEditor::bindView() {
    view_ = std::make_unique<NodeGraphView>(canvas_, *current_->graph, [this](const std::string& what, const NodeGraph& before, const NodeGraph& after) {
        onViewEdit(what, before, after);
    });
    checked_.reset();
    view_->frameAll();
    problems_.clear();
    overwriteArmed_ = false;
    panelKey_.clear();
    nodeCounter_ = 1;
    rebuild_ = true;
}

bool GraphEditor::open(const std::string& name) {
    const auto known = docs_.find(docKey(kind_, name));
    if (known != docs_.end()) {
        current_ = &known->second;
        bindView();
        return true;
    }
    return kind_ == Kind::Dialogue ? openDialogue(name) : openInteraction(name);
}

bool GraphEditor::openDialogue(const std::string& name) {
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
    Doc& doc = docs_[docKey(Kind::Dialogue, name)];
    doc.name = name;
    doc.kind = Kind::Dialogue;
    *doc.graph = std::move(d.graph);
    doc.header = std::move(d.header);
    doc.saved = *doc.graph;
    doc.savedHeader = headerText(doc.header);
    doc.loadedAt = fs::last_write_time(file, ec);
    doc.existed = true;
    current_ = &doc;
    bindView();
    return true;
}

bool GraphEditor::openInteraction(const std::string& name) {
    const fs::path file = fileOf(name);
    std::error_code ec;
    if (!fs::is_regular_file(file, ec)) {
        say_("Interaction: no file " + name + ".json");
        return false;
    }
    const std::string text = readText(file);
    sim::rules::LoadReport report;
    const std::optional<sim::rules::Interaction> interaction = sim::rules::InteractionRegistry::parse(text, "interactions/" + name + ".json", report, name);
    if (!interaction) {
        say_("Interaction: " + (report.errors.empty() ? name : report.errors.front().text()));
        return false;
    }
    const fs::path sidecar = fs::path(file).concat(".layout.json");
    const DialogueLayout layout = fs::is_regular_file(sidecar, ec) ? layoutFromJson(readText(sidecar)) : DialogueLayout{};
    Doc& doc = docs_[docKey(Kind::Interaction, name)];
    doc.name = name;
    doc.kind = Kind::Interaction;
    *doc.graph = interactionToGraph(*interaction, layout);
    doc.saved = *doc.graph;
    doc.leading = leadingComments(text);
    doc.loadedAt = fs::last_write_time(file, ec);
    doc.existed = true;
    current_ = &doc;
    bindView();
    return true;
}

bool GraphEditor::createNew(const std::string& name) {
    const bool valid = !name.empty() && name.size() <= 40 &&
                       std::all_of(name.begin(), name.end(), [](unsigned char c) { return std::islower(c) != 0 || std::isdigit(c) != 0 || c == '-'; });
    if (!valid) {
        say_("New: a name is lower-case letters, digits and hyphens (up to 40)");
        return false;
    }
    std::error_code ec;
    if (fs::exists(fileOf(name), ec) || docs_.count(docKey(kind_, name)) != 0) {
        say_("New: " + name + " exists already");
        return false;
    }
    sim::rules::LoadReport report;
    Doc doc;
    doc.name = name;
    doc.kind = kind_;
    if (kind_ == Kind::Dialogue) {
        const auto script = sim::rules::parseDialogue("=== start\nElder: Hello, {hero}.\n-> Leave => END\n", name, "dialogue/" + name + ".dlg", report);
        if (!script) {
            say_("New: the starting conversation could not be made");
            return false;
        }
        DialogueGraph d = dialogueToGraph(*script, {});
        *doc.graph = std::move(d.graph);
        doc.header = std::move(d.header);
        doc.savedHeader = headerText(doc.header);
    } else {
        const std::string text = "{ \"id\": \"" + name + "\", \"label\": \"New action\", \"actors\": [\"hero\"], \"target\": { \"tags\": [\"edible\"] }, \"range\": 1.5, "
                                 "\"duration\": 0, \"order\": 100, \"requires\": [], \"effects\": [ \"say \\\"Nothing to do yet\\\"\" ] }";
        const auto interaction = sim::rules::InteractionRegistry::parse(text, "interactions/" + name + ".json", report, name);
        if (!interaction) {
            say_("New: the starting interaction could not be made" + (report.errors.empty() ? std::string() : ": " + report.errors.front().text()));
            return false;
        }
        *doc.graph = interactionToGraph(*interaction, {});
    }
    // `saved` stays empty: a file nobody has written is unsaved by definition.
    doc.existed = false;
    Doc& kept = docs_[docKey(kind_, name)] = std::move(doc);
    current_ = &kept;
    bindView();
    say_("New " + name + (kind_ == Kind::Dialogue ? ".dlg" : ".json") + ": not written until you press Save");
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

// What a card shows is made from its fields, by the kind of graph it is in.
static void describeAny(GraphEditor::Kind kind, GraphNode& card) {
    if (kind == GraphEditor::Kind::Dialogue) describeCard(card);
    else describeRuleCard(card);
}

int GraphEditor::addCard(const std::string& type) {
    if (current_ == nullptr || !view_) return 0;
    GraphNode card = kind_ == Kind::Dialogue ? newDialogueCard(type) : newRuleCard(type);
    if (kind_ == Kind::Dialogue && type == dlg_card::kNode) {
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

void GraphEditor::tidy() {
    const Kind kind = kind_;
    edit("tidy", [kind](NodeGraph& g) {
        if (kind == Kind::Dialogue) arrangeDialogue(g);
        else arrangeInteraction(g);
    });
    if (view_) view_->frameAll();
}

bool GraphEditor::setCardField(int card, std::size_t index, const std::string& value) {
    if (current_ == nullptr) return false;
    const GraphNode* found = current_->graph->find(card);
    if (found == nullptr) return false;
    const Kind kind = kind_;
    edit("edit " + found->type, [&](NodeGraph& g) {
        GraphNode* node = g.find(card);
        if (node->fields.size() <= index) node->fields.resize(index + 1);
        node->fields[index] = value;
        describeAny(kind, *node);
    });
    return true;
}

// The cards that hold a list of lines: effects and notes.
static bool isListCard(const GraphNode& card) {
    return card.type == dlg_card::kEffect || card.type == dlg_card::kComment || card.type == rule_card::kEffects;
}

bool GraphEditor::addCardField(int card) {
    if (current_ == nullptr) return false;
    const GraphNode* found = current_->graph->find(card);
    if (found == nullptr || !isListCard(*found)) return false;
    const Kind kind = kind_;
    edit("add a line", [&](NodeGraph& g) {
        GraphNode* node = g.find(card);
        node->fields.emplace_back();
        describeAny(kind, *node);
    });
    return true;
}

bool GraphEditor::removeCardField(int card) {
    if (current_ == nullptr) return false;
    const GraphNode* found = current_->graph->find(card);
    if (found == nullptr || !isListCard(*found) || found->fields.size() <= 1) return false;
    const Kind kind = kind_;
    edit("remove a line", [&](NodeGraph& g) {
        GraphNode* node = g.find(card);
        node->fields.pop_back();
        describeAny(kind, *node);
    });
    return true;
}

bool GraphEditor::save() {
    if (current_ == nullptr) return false;
    return current_->kind == Kind::Dialogue ? saveDialogue() : saveInteraction();
}

// The checks before a file is touched, the same for both kinds: the file changed on disk since it was opened asks once.
static bool changedOnDisk(const fs::path& file, const std::filesystem::file_time_type& loadedAt, bool existed) {
    std::error_code ec;
    return existed && fs::is_regular_file(file, ec) && fs::last_write_time(file, ec) != loadedAt;
}

bool GraphEditor::saveDialogue() {
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
    if (changedOnDisk(file, doc.loadedAt, doc.existed) && !overwriteArmed_) {
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
    recheck();
    say_("Saved " + doc.name + ".dlg" + (warnings > 0 ? " (" + std::to_string(warnings) + " card(s) not connected)" : "") + errorsNote());
    if (saved_) saved_();
    return true;
}

bool GraphEditor::saveInteraction() {
    Doc& doc = *current_;
    problems_.clear();
    std::string json;
    const std::optional<sim::rules::Interaction> interaction = graphToInteraction(*doc.graph, problems_, json);
    if (!interaction) {
        const auto error = std::find_if(problems_.begin(), problems_.end(), [](const std::string& p) { return p.rfind("error:", 0) == 0; });
        say_("Not saved: " + (error != problems_.end() ? *error : std::string("the interaction cannot be written")));
        return false;
    }
    if (interaction->id != doc.name) { // one file per interaction, named by its id: renaming means a new file, which this editor does not make
        problems_.push_back("error: the verb id \"" + interaction->id + "\" must stay \"" + doc.name + "\", the name of the file");
        say_("Not saved: " + problems_.back());
        return false;
    }
    const fs::path file = fileOf(doc.name);
    std::error_code ec;
    if (changedOnDisk(file, doc.loadedAt, doc.existed) && !overwriteArmed_) {
        overwriteArmed_ = true;
        say_(doc.name + ".json changed on disk since it was opened: press Save again to overwrite it");
        return false;
    }
    if (fs::is_regular_file(file, ec)) fs::copy_file(file, fs::path(file).concat(".bak"), fs::copy_options::overwrite_existing, ec);
    if (!writeText(file, doc.leading + json) || !writeText(fs::path(file).concat(".layout.json"), layoutToJson(interactionLayoutOf(*doc.graph)))) {
        say_("Not saved: " + doc.name + ".json cannot be written");
        return false;
    }
    doc.saved = *doc.graph;
    doc.loadedAt = fs::last_write_time(file, ec);
    doc.existed = true;
    overwriteArmed_ = false;
    int warnings = 0;
    for (const std::string& p : problems_) warnings += p.rfind("warning:", 0) == 0 ? 1 : 0;
    recheck();
    say_("Saved " + doc.name + ".json" + (warnings > 0 ? " (" + std::to_string(warnings) + " card(s) not connected)" : "") + errorsNote());
    if (saved_) saved_();
    return true;
}

void GraphEditor::recheck() {
    findings_.clear();
    if (current_ == nullptr) {
        checked_.reset();
        problemList_->items.clear();
        return;
    }
    checked_ = *current_->graph;
    std::vector<std::string> structural;
    if (current_->kind == Kind::Dialogue) {
        const DialogueGraph d{*current_->graph, current_->header};
        const std::optional<sim::rules::DlgScript> script = graphToDialogue(d, structural);
        for (const std::string& s : structural) findings_.push_back({s.rfind("error:", 0) == 0, std::string(), s});
        if (script) {
            for (const sim::rules::GraphFinding& f : sim::rules::checkDialogue(*script, catalog_)) findings_.push_back({f.error, f.key, f.message});
        }
    } else {
        std::string json;
        const std::optional<sim::rules::Interaction> interaction = graphToInteraction(*current_->graph, structural, json);
        for (const std::string& s : structural) findings_.push_back({s.rfind("error:", 0) == 0, std::string(), s});
        if (interaction) {
            for (const sim::rules::GraphFinding& f : sim::rules::checkInteraction(*interaction, catalog_)) findings_.push_back({f.error, f.key, f.message});
        }
    }
    problemList_->items.clear();
    for (const Problem& p : findings_) problemList_->items.push_back((p.error ? "! " : "? ") + p.text);
    problemList_->selected = -1;
}

bool GraphEditor::pickProblem(std::size_t index) {
    if (current_ == nullptr || !view_ || index >= findings_.size() || findings_[index].key.empty()) return false;
    const std::map<std::string, int> keys = current_->kind == Kind::Dialogue ? cardKeys(DialogueGraph{*current_->graph, current_->header})
                                                                              : interactionCardKeys(*current_->graph);
    const auto found = keys.find(findings_[index].key);
    if (found == keys.end()) return false;
    const GraphNode* card = current_->graph->find(found->second);
    if (card == nullptr) return false;
    view_->select(card->id);
    view_->centerOn(card->x + luna::engine::kGraphNodeWidth / 2, card->y + card->height() / 2);
    return true;
}

// "; 2 error(s) block shipping: see the list" after a save that went through with mistakes the check found (D-56 Q18: saving is allowed, shipping is not).
std::string GraphEditor::errorsNote() const {
    int errors = 0;
    for (const Problem& p : findings_) errors += p.error ? 1 : 0;
    return errors == 0 ? std::string() : "; " + std::to_string(errors) + " error(s) block shipping: see the list";
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

bool GraphEditor::typing() const { return shown_ && ((panel_ && panel_->typing()) || (chrome_ && chrome_->typing()) || (testShown_ && testPanel_ && testPanel_->typing())); }

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
    button("Save", "Write the file and its layout (Ctrl+S)", [this] { save(); });
    button("Frame", "Show every card", [this] {
        if (view_) view_->frameAll();
    });
    button("Tidy", "Put the cards in rows again", [this] { tidy(); });
    x += 6;
    {
        const bool talk = kind_ == Kind::Dialogue;
        button(talk ? "[Talk]" : "Talk", "Conversations (.dlg)", [this] { if (kind_ != Kind::Dialogue) showKind(Kind::Dialogue); });
        button(talk ? "Rules" : "[Rules]", "Interactions (.json): who may do what to what", [this] { if (kind_ != Kind::Interaction) showKind(Kind::Interaction); });
    }
    x += 6;
    if (kind_ == Kind::Dialogue) button("Test", "Play this conversation with values you choose; nothing is saved", [this] { showTest(!testShown_); });
    if (kind_ == Kind::Dialogue) {
        button("+Node", "A new node: a stop in the conversation", [this] { addCard(dlg_card::kNode); });
        button("+Line", "A line someone says", [this] { addCard(dlg_card::kLine); });
        button("+Choice", "A choice for the player", [this] { addCard(dlg_card::kChoice); });
        button("+If", "A condition: wire it to the If port of a line or choice", [this] { addCard(dlg_card::kCondition); });
        button("+Do", "Effects: wire them to the Do port of a choice", [this] { addCard(dlg_card::kEffect); });
        button("+Goto", "A named jump to a node, for a wire that would be too long", [this] { addCard(dlg_card::kGoto); });
        button("+Note", "A note kept in the file: wire it to what it is about", [this] { addCard(dlg_card::kComment); });
    } else {
        button("+Needs", "A requirement: wire it to the Requires port of the verb", [this] { addCard(rule_card::kRequirement); });
        button("+Effects", "Effects: wire them to the Do port of the verb", [this] { addCard(rule_card::kEffects); });
        button("+NPC", "The rule that makes clan members do this on their own", [this] { addCard(rule_card::kNpcRule); });
        button("+Chronicle", "A line for the chronicle when this happens", [this] { addCard(rule_card::kChronicle); });
    }
    x += 6;
    chrome_->add<TextField>(Rect{x, 2, 110, kBarHeight - 4}, "name: ", newName_, 40, [this](const std::string& v) { newName_ = v; });
    x += 114;
    button("New", "Make a new file of this name in the shown kind; it is written when you press Save", [this] { createNew(newName_); });
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
        if (current_->kind == Kind::Interaction) {
            panelTitle_ = current_->name + ".json";
            return; // the verb card holds the file's own values
        }
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
    } else if (card->type == rule_card::kActor) {
        field("actors: ", at(0), set(0));
    } else if (card->type == rule_card::kVerb) {
        field("id: ", at(0), set(0)); // stays the name of the file; Save refuses another
        field("label: ", at(1), set(1));
        field("note: ", at(2), set(2));
        field("range m: ", at(3), set(3));
        field("secs: ", at(4), set(4));
        field("order: ", at(5), set(5));
        field("menu: ", at(6), set(6));
    } else if (card->type == rule_card::kTarget) {
        field("tags: ", at(0), set(0));
        field("kinds: ", at(1), set(1));
    } else if (card->type == rule_card::kRequirement) {
        field("if: ", at(0), set(0));
        field("else: ", at(1), set(1));
    } else if (card->type == rule_card::kNpcRule) {
        field("score: ", at(0), set(0));
        field("cooldown: ", at(1), set(1));
    } else if (card->type == rule_card::kChronicle) {
        field("text: ", at(0), set(0));
    } else {
        for (std::size_t i = 0; i < card->fields.size(); ++i) field(card->type == dlg_card::kComment ? "note: " : "do: ", at(i), set(i));
        panel_->add<Button>(Rect{left, y, 40, kRow - 2}, "+ line", [this, cardId] { addCardField(cardId); });
        panel_->add<Button>(Rect{left + 44, y, 40, kRow - 2}, "- line", [this, cardId] { removeCardField(cardId); });
        y += kRow;
    }
    panel_->add<Button>(Rect{left, y + 4, 60, kRow - 2}, "Delete", [this] {
        if (view_) view_->deleteSelected();
    });
}

// ---- Test-play (US-174)

std::string GraphEditor::nodeOfSelection() const {
    if (current_ == nullptr || current_->kind != Kind::Dialogue) return {};
    const int id = selectedCard();
    if (id == 0) return {};
    for (const auto& [key, card] : cardKeys(DialogueGraph{*current_->graph, current_->header})) {
        if (card == id) return key.substr(0, key.find('/')); // "start/choice1" belongs to the node "start"
    }
    return {};
}

bool GraphEditor::startTest(bool fromSelected) {
    test_.reset();
    if (current_ == nullptr || current_->kind != Kind::Dialogue) {
        say_("Test-play: open a conversation first");
        return false;
    }
    sim::rules::TestState state;
    std::string problem;
    if (!sim::rules::applyTestState(state, testWords_, problem)) {
        say_("Test-play: " + problem);
        testRebuild_ = true;
        return false;
    }
    std::string startNode;
    if (fromSelected) {
        startNode = nodeOfSelection();
        if (startNode.empty()) {
            say_("Test-play: select a card of the node to play from");
            return false;
        }
    }
    std::vector<std::string> conversion;
    const std::optional<sim::rules::DlgScript> script = graphToDialogue(DialogueGraph{*current_->graph, current_->header}, conversion);
    if (!script) {
        const auto error = std::find_if(conversion.begin(), conversion.end(), [](const std::string& p) { return p.rfind("error:", 0) == 0; });
        say_("Test-play: " + (error != conversion.end() ? *error : std::string("the conversation cannot be written yet")));
        return false;
    }
    auto play = std::make_unique<sim::rules::TestPlay>(*script, state, startNode);
    if (!play->started()) {
        say_("Test-play: there is no node \"" + startNode + "\"");
        return false;
    }
    test_ = std::move(play);
    testShown_ = true;
    testRebuild_ = true;
    return true;
}

void GraphEditor::stopTest() {
    test_.reset();
    testRebuild_ = true;
}

void GraphEditor::showTest(bool shown) {
    testShown_ = shown && kind_ == Kind::Dialogue;
    if (!testShown_) test_.reset();
    testRebuild_ = true;
}

bool GraphEditor::testChoose(int visibleIndex) {
    if (!test_) return false;
    const bool chosen = test_->choose(visibleIndex);
    testRebuild_ = true;
    return chosen;
}

void GraphEditor::buildTestPanel() {
    testRebuild_ = false;
    testLines_.clear();
    testLog_.clear();
    const int width = std::min(440, canvas_.width - 16);
    const Rect box{canvas_.x + 8, canvas_.y + canvas_.height - 216, width, 208};
    testPanel_ = std::make_unique<Panel>(box);
    testPanel_->visible = testShown_;
    if (!testShown_) return;
    const int left = box.x + 4;
    const int inner = box.width - 8;
    testPanel_->add<TextField>(Rect{left, box.y + 13, inner, kRow - 2}, "state: ", testWords_, 160, [this](const std::string& v) { testWords_ = v; });
    int x = left;
    auto button = [&](const std::string& label, const std::string& hint, auto action) {
        const int w = UiPainter::textWidth(label) + 8;
        testPanel_->add<Button>(Rect{x, box.y + 29, w, 12}, label, action).hint = hint;
        x += w + 3;
    };
    button("Play", "Start at the beginning with these values", [this] { startTest(false); });
    button("From here", "Start at the node of the selected card", [this] { startTest(true); });
    button("Leave", "Walk away from the talk", [this] {
        if (test_) test_->leave();
        testRebuild_ = true;
    });
    button("Stop", "End the test-play and forget it", [this] { stopTest(); });
    button("Close", "Hide this card", [this] { showTest(false); });
    if (!test_) return;
    // What the NPC says now, wrapped to the card.
    const std::size_t chars = static_cast<std::size_t>(inner / luna::engine::kTextAdvance);
    const sim::rules::ConversationView view = test_->view();
    for (const sim::rules::ConversationLine& line : view.lines) {
        std::string text = line.speaker + ": " + line.text;
        while (!text.empty() && testLines_.size() < 4) {
            testLines_.push_back(text.substr(0, chars));
            text = text.size() > chars ? text.substr(chars) : std::string();
        }
    }
    int y = box.y + 45 + 4 * luna::engine::kLineHeight + 2;
    if (test_->finished()) {
        testLog_.push_back("-- the talk is over --");
    } else {
        for (const sim::rules::ConversationChoice& choice : view.choices) {
            const std::string label = std::to_string(choice.index + 1) + ". " + choice.text + (choice.enabled ? "" : "  (" + choice.reason + ")");
            Button& b = testPanel_->add<Button>(Rect{left, y, inner, 11}, label.substr(0, chars - 1), [this, index = static_cast<int>(&choice - view.choices.data())] { testChoose(index); });
            if (!choice.enabled) b.hint = "Greyed out: " + choice.reason;
            y += 12;
        }
    }
    for (const std::string& line : test_->log()) testLog_.push_back(line);
    if (testLog_.size() > 6) testLog_.erase(testLog_.begin(), testLog_.end() - 6);
}

void GraphEditor::update(const luna::engine::Intents& intents) {
    if (!shown_) return;
    if (rebuild_) {
        rebuild_ = false;
        buildChrome();
        buildPanel();
    }
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
    if (current_ != nullptr && (!checked_ || !(*checked_ == *current_->graph))) recheck();
    if (testRebuild_ || !testPanel_) buildTestPanel();
    const bool onChrome = chrome_->handle(input);
    const bool onTest = testShown_ && testPanel_->handle(input);
    const bool onPanel = panel_->handle(input) || problemList_->handle(input) || onTest;
    if (!onChrome && !onPanel && view_) view_->handle(input);
    if (!typing() && view_ && intents.pressed(Intent::Delete)) view_->deleteSelected();
}

void GraphEditor::draw(UiPainter& painter) const {
    if (!shown_) return;
    painter.fill({0, 0, viewWidth_, viewHeight_}, UiColor::Dark);
    if (view_) view_->draw(painter);
    chrome_->draw(painter);
    painter.fill({problemList_->bounds.x, problemList_->bounds.y - 1, problemList_->bounds.width, problemList_->bounds.height + 2}, UiColor::Panel);
    problemList_->draw(painter);
    panel_->draw(painter);
    painter.text(panel_->bounds.x + 4, panel_->bounds.y + 3, panelTitle_, UiColor::Gold);
    if (testShown_ && testPanel_) {
        testPanel_->draw(painter);
        const Rect box = testPanel_->bounds;
        painter.text(box.x + 4, box.y + 3, test_ ? "Test-play: " + test_->nodeId() : std::string("Test-play: type values, then Play"), UiColor::Gold);
        int y = box.y + 45;
        for (const std::string& line : testLines_) {
            painter.text(box.x + 4, y, line, UiColor::Text);
            y += luna::engine::kLineHeight;
        }
        int logY = box.y + box.height - static_cast<int>(testLog_.size()) * luna::engine::kLineHeight - 2;
        for (const std::string& line : testLog_) {
            painter.text(box.x + 4, logY, line.substr(0, static_cast<std::size_t>((box.width - 8) / luna::engine::kTextAdvance)), UiColor::Dim);
            logY += luna::engine::kLineHeight;
        }
    }
    const Rect bar{0, viewHeight_ - kStatusHeight, viewWidth_, kStatusHeight};
    painter.fill(bar, UiColor::Shade);
    const bool talk = kind_ == Kind::Dialogue;
    std::string line = current_ != nullptr ? std::string(talk ? "Dialogue: " : "Interaction: ") + current_->name + (talk ? ".dlg" : ".json") + (dirty() ? "  *unsaved" : "") : std::string(talk ? "Dialogue" : "Interaction") + ": no file";
    if (view_) line += "  zoom " + std::to_string(view_->zoomPercent()) + "%";
    int errors = 0;
    for (const Problem& p : findings_) errors += p.error ? 1 : 0;
    if (!findings_.empty()) line += "  " + std::to_string(errors) + " error(s), " + std::to_string(findings_.size() - static_cast<std::size_t>(errors)) + " warning(s)";
    if (!problems_.empty()) line += "  " + problems_.front();
    painter.text(4, bar.y + 3, line.substr(0, static_cast<std::size_t>((viewWidth_ - 8) / luna::engine::kTextAdvance)), UiColor::Text);
}

void GraphEditor::drawOverlay(UiPainter& painter) const {
    if (!shown_) return;
    if (testShown_ && testPanel_) testPanel_->drawOverlay(painter);
    chrome_->drawOverlay(painter);
    panel_->drawOverlay(painter);
}

} // namespace odysseus::game
