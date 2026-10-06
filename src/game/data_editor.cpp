#include "game/data_editor.h"
#include "game/atlas_cuts.h"
#include "game/timeline_view.h"

#include "core/text.h"
#include "sim/json_text.h"
#include "sim/npc_schedule.h"

#include <algorithm>
#include <cctype>
#include <format>

namespace odysseus::game {

namespace fs = std::filesystem;
namespace form = sim::form;
namespace schema = sim::schema;
namespace refs = sim::refs;
using luna::engine::Button;
using luna::engine::Label;
using luna::engine::ListBox;
using luna::engine::Panel;
using luna::engine::Rect;
using luna::engine::TextField;
using luna::engine::Toggle;
using luna::engine::UiColor;
using luna::engine::UiInput;
using luna::engine::UiPainter;

namespace {

constexpr int kBar = 18;         // the top bar
constexpr int kStatus = 14;      // the status line at the bottom
constexpr int kRow = 13;         // one row of the form
constexpr int kIndent = 8;       // pixels a nested group moves to the right
constexpr int kLabelChars = 24;  // letters of a field's label before its box, so the boxes of a form line up
constexpr int kActions = 40;     // room at the right of a row for its Remove and arrow buttons
constexpr int kFoldersWidth = 92;
constexpr int kFilesWidth = 132;
constexpr int kEntriesWidth = 168;
constexpr int kProblemTicks = 100; // five seconds
constexpr int kTimelineRows = 2;     // a routine's bar takes this many rows of the form (22 pixels and a gap)
constexpr int kRulesHeadingRows = 2; // the heading and the summary above the form of a rules file
constexpr int kQuickYears = 20;    // the Quick check runs the clan this long
constexpr int kQuickSliceDays = 30; // and this many days at every update

std::string lowered(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

// A label of exactly kLabelChars letters (cut with ".." or padded), indented by the depth, so every box of a form starts at the same place.
std::string fitLabel(const std::string& text, int depth) {
    std::string label = std::string(static_cast<std::size_t>(std::min(depth, 6)) * 2, ' ') + text;
    if (static_cast<int>(label.size()) > kLabelChars) label = label.substr(0, kLabelChars - 2) + "..";
    label.resize(kLabelChars, ' ');
    return label;
}

bool isMapEntry(const form::FormRow& row) {
    const std::optional<sim::DocPath> parsed = sim::parsePath(row.path);
    return row.kind == form::RowKind::ItemHeader && parsed && !parsed->empty() && !parsed->back().index;
}

std::string folderOf(const std::string& relative) {
    const std::size_t slash = relative.rfind('/');
    return slash == std::string::npos ? std::string() : relative.substr(0, slash);
}

// ---- Help made from the schemas

std::string rangeWords(const schema::Node& node) {
    if (!node.choices.empty()) {
        std::string words = "one of: ";
        for (std::size_t i = 0; i < node.choices.size(); ++i) words += (i != 0 ? ", " : "") + (node.choices[i].is_string() ? node.choices[i].get<std::string>() : node.choices[i].dump());
        return words;
    }
    const auto number = [](double value) { return std::format("{}", value); };
    if (node.minimum && node.maximum) return number(*node.minimum) + " to " + number(*node.maximum);
    if (node.minimum) return "at least " + number(*node.minimum);
    if (node.maximum) return "at most " + number(*node.maximum);
    if (node.format == "colour") return "# and six hex digits";
    if (node.maxLength) return std::format("up to {} letters", *node.maxLength);
    return {};
}

void helpWalk(const schema::Node& node, const std::string& schemaName, const std::string& path, std::map<std::string, EditorHelp::Entry>& out) {
    for (const schema::Property& property : node.properties) {
        const schema::Node& child = *property.node;
        EditorHelp::Entry entry;
        entry.purpose = child.description;
        entry.range = rangeWords(child);
        entry.example = child.example;
        if (!child.choices.empty()) {
            entry.suggest = "values:";
            for (std::size_t i = 0; i < child.choices.size(); ++i) entry.suggest += (i != 0 ? "|" : "") + (child.choices[i].is_string() ? child.choices[i].get<std::string>() : child.choices[i].dump());
        } else if (child.ref.starts_with("catalog:")) {
            entry.suggest = child.ref;
        } else {
            entry.suggest = "none";
        }
        const std::string at = path + "." + property.name;
        out["data." + schemaName + at] = entry;
        helpWalk(child, schemaName, at, out);
    }
    if (node.items) helpWalk(*node.items, schemaName, path, out);
    if (node.additional) {
        const std::string at = path + ".*";
        EditorHelp::Entry entry;
        entry.purpose = node.additional->description;
        entry.range = rangeWords(*node.additional);
        entry.example = node.additional->example;
        entry.suggest = "none";
        out["data." + schemaName + at] = entry;
        helpWalk(*node.additional, schemaName, at, out);
    }
}

} // namespace

DataEditor::DataEditor(int viewWidth, int viewHeight, Say say) : viewWidth_(viewWidth), viewHeight_(viewHeight), say_(std::move(say)) {
    panel_ = std::make_unique<Panel>(Rect{0, 0, viewWidth, viewHeight});
}

std::map<std::string, EditorHelp::Entry> DataEditor::helpFromSchemas(const schema::SchemaSet& set) {
    std::map<std::string, EditorHelp::Entry> out;
    for (const schema::Schema& entry : set.schemas()) helpWalk(*entry.root, entry.name, std::string(), out);
    return out;
}

void DataEditor::setFolder(fs::path dataFolder, std::function<void(const fs::path&)> saved) {
    folder_ = std::move(dataFolder);
    saved_ = std::move(saved);
    set_.reset();
    installHelp();
    markStale();
}

// The help entries made from the schemas go to EditorHelp once both the folder and the help are known (the game gives them in either order).
void DataEditor::installHelp() {
    if (help_ != nullptr && !folder_.empty() && loadSchemas()) help_->setGenerated(helpFromSchemas(*set_));
}

void DataEditor::setHelp(EditorHelp* help) {
    help_ = help;
    installHelp();
}

bool DataEditor::loadSchemas() {
    if (set_) return true;
    try {
        set_ = std::make_shared<schema::SchemaSet>(schema::SchemaSet::load(folder_ / "schemas"));
    } catch (const std::exception& error) {
        say_(std::string("Data: ") + error.what());
        return false;
    }
    return true;
}

void DataEditor::refreshIndex() {
    if (set_) index_ = schema::buildIndex(folder_, *set_);
}

std::vector<std::string> DataEditor::catalogNames(const std::string& catalog) const {
    const auto found = index_.catalogs.find(catalog);
    if (found == index_.catalogs.end()) return {};
    return std::vector<std::string>(found->second.begin(), found->second.end());
}

void DataEditor::show(bool shown) {
    shown_ = shown;
    if (!shown_) return;
    if (!loadSchemas()) {
        shown_ = false;
        return;
    }
    refreshIndex();
    if (openFile_.empty()) {
        const std::vector<std::string> all = files();
        if (!all.empty()) open(all.front());
    }
    markStale();
}

bool DataEditor::typing() const { return shown_ && ((tool_ && tool_->open() && tool_->typing()) || (panel_ && panel_->typing())); }

// ---- Pictures (US-192)

void DataEditor::setPictures(fs::path sprites, PictureTool::MakeTexture makeTexture) {
    sprites_ = std::move(sprites);
    makeTexture_ = std::move(makeTexture);
    tool_.reset();
    content_.reset();
    pageTextures_.clear();
    previewKey_.clear();
}

void DataEditor::ensureTool() {
    if (!tool_) tool_ = std::make_unique<PictureTool>(viewWidth_, viewHeight_, sprites_.empty() ? folder_.parent_path() / "sprites" : sprites_, makeTexture_, [this](const std::string& text) { say_("Data: " + text); });
}

bool DataEditor::openCutTool() {
    ensureTool();
    tool_->openCutter([this] { // the atlas was cut again: the previews read it again; the game reads it at its next start
        content_.reset();
        pageTextures_.clear();
        previewKey_.clear();
    });
    return true;
}

bool DataEditor::openPicker(const std::string& path) {
    const form::FormRow* row = rowAt(path);
    if (row == nullptr) return false;
    ensureTool();
    tool_->openPicker(row->label, row->value, [this, path](const std::string& name) { setField(path, name); });
    return true;
}

// The frames the chosen entry plays: the picture its `frame` names, or for an effect or weather the animation of its own name. Read from the atlas the game cut (assets/sprites/atlas).
void DataEditor::refreshPreview() {
    std::string frame;
    std::string name;
    int ticks = 3;
    if (document_ && !entryPath_.empty()) {
        if (const sim::OrderedJson* value = document_->find(entryPath_ + ".frame"); value != nullptr && value->is_string()) frame = value->get<std::string>();
        if (const sim::OrderedJson* value = document_->find(entryPath_ + ".name"); value != nullptr && value->is_string()) name = value->get<std::string>();
        if (const sim::OrderedJson* value = document_->find(entryPath_ + ".ticksPerFrame"); value != nullptr && value->is_number_integer()) ticks = std::clamp(value->get<int>(), 1, 60);
    }
    const std::string item = !frame.empty() ? frame : (openFile_ == "effects.json" || openFile_ == "weather.json" ? name : std::string());
    const std::string key = openFile_ + "|" + item + "|" + std::to_string(ticks);
    if (key == previewKey_) return;
    previewKey_ = key;
    previewTicksPerFrame_ = ticks;
    const bool had = !preview_.empty();
    preview_.clear();
    if (!item.empty()) {
        if (!content_) {
            std::string problem;
            content_ = loadContent((sprites_.empty() ? folder_.parent_path() / "sprites" : sprites_) / "atlas", problem);
        }
        if (content_) {
            const auto counted = content_->frameCounts.find(item);
            const int count = counted != content_->frameCounts.end() ? counted->second : 1;
            for (int i = 0; i < count; ++i) {
                const std::string frameName = count > 1 || counted != content_->frameCounts.end() ? content_->frameName(item, i) : item;
                const auto place = content_->frames.find(frameName);
                const std::optional<core::Rect> rect = content_->rect(frameName);
                if (place == content_->frames.end() || !rect) continue;
                luna::engine::Texture& texture = pageTextures_[place->second.page];
                if (texture.id < 0 && makeTexture_) texture = makeTexture_(content_->pictures.at(place->second.page));
                preview_.push_back({frameName, texture, {rect->x, rect->y, rect->width, rect->height}});
            }
        }
    }
    if (had != !preview_.empty()) markStale(); // the form is narrower while a picture plays beside it
}

std::vector<std::string> DataEditor::files() const {
    std::vector<std::string> found;
    if (!set_) return found;
    std::error_code error;
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(folder_, error)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;
        const std::string relative = entry.path().lexically_relative(folder_).generic_string();
        if (relative.starts_with("schemas/") || relative.find(".bak") != std::string::npos) continue;
        if (set_->schemaFor(relative) != nullptr) found.push_back(relative);
    }
    std::sort(found.begin(), found.end());
    return found;
}

void DataEditor::refuse(const std::string& message) {
    problem_ = message;
    problemTicks_ = kProblemTicks;
    say_("Data: " + message);
}

bool DataEditor::open(const std::string& relative) {
    if (!loadSchemas()) return false;
    if (dirty() && relative != openFile_) {
        refuse("unsaved changes in " + openFile_ + ": save (Ctrl+S) or undo them first");
        return false;
    }
    const schema::Schema* found = set_->schemaFor(relative);
    if (found == nullptr) {
        refuse(relative + " has no schema");
        return false;
    }
    std::string reason;
    std::optional<sim::DataDocument> opened = sim::DataDocument::open(folder_ / relative, reason);
    if (!opened) {
        refuse(reason);
        return false;
    }
    document_ = std::move(opened);
    schema_ = found;
    openFile_ = relative;
    folderChosen_ = folderOf(relative);
    collapsed_.clear();
    search_.clear();
    scroll_ = 0;
    problem_.clear();
    discardWarned_ = false;
    rowsFor_ = static_cast<std::size_t>(-1);
    refreshRows();
    entryPath_ = entries_.size() > 1 ? entries_[1].path : std::string(); // a catalog opens on its first entry, any other file on its settings
    refreshRows();
    markStale();
    return true;
}

void DataEditor::refreshRows() {
    if (!document_ || schema_ == nullptr) {
        rows_.clear();
        entries_.clear();
        return;
    }
    entries_ = form::entriesOf(document_->root(), *schema_);
    const bool known = std::any_of(entries_.begin(), entries_.end(), [&](const form::Entry& entry) { return entry.path == entryPath_; });
    if (!known) entryPath_ = entries_.empty() ? std::string() : entries_.front().path;
    issues_ = schema::check(*schema_->root, document_->root()).issues;
    rows_ = form::buildRows(document_->root(), *schema_, entryPath_, collapsed_, issues_);
    rowsFor_ = document_->version();
    const int most = std::max(0, static_cast<int>(rows_.size()) - visibleRows());
    scroll_ = std::clamp(scroll_, 0, most);
}

int DataEditor::visibleRows() const {
    return std::max(1, (viewHeight_ - kStatus - kBar - 6) / kRow - (gameRulesPage() ? kRulesHeadingRows : 0) - static_cast<int>(routinesShown().size()) * kTimelineRows);
}

// The routines the chosen entry shows as a timeline (US-196): a `day` or `night` list of schedule blocks (the US-290 format) on the entry itself (a profession) or in its
// `schedule` group (an NPC class or kind). The bar and the form below it edit the same document, so there is one writer and one format.
std::vector<DataEditor::Routine> DataEditor::routinesShown() const {
    std::vector<Routine> found;
    if (!document_) return found;
    for (const std::string& holder : {entryPath_, entryPath_.empty() ? std::string("schedule") : entryPath_ + ".schedule"}) {
        for (const char* part : {"day", "night"}) {
            const std::string path = holder.empty() ? std::string(part) : holder + "." + part;
            const sim::OrderedJson* list = document_->find(path);
            if (list == nullptr || !list->is_array() || list->empty()) continue;
            Routine routine{path, std::string(part), {}};
            bool shape = true;
            for (const sim::OrderedJson& block : *list) {
                const std::optional<int> minute = block.is_object() && block.contains("from") && block["from"].is_string() ? sim::rules::parseClock(block["from"].get<std::string>()) : std::nullopt;
                if (!minute) {
                    shape = false;
                    break;
                }
                routine.blocks.push_back({*minute, block.contains("do") && block["do"].is_string() ? block["do"].get<std::string>() : std::string("idle"),
                                          block.contains("at") && block["at"].is_string() ? block["at"].get<std::string>() : std::string("home")});
            }
            if (shape) found.push_back(std::move(routine));
        }
    }
    return found;
}

// A block dragged on a timeline: the same edit as typing the time into its field.
bool DataEditor::moveRoutineBlock(const std::string& listPath, std::size_t index, int minute) {
    return setField(std::format("{}[{}].from", listPath, index), sim::rules::formatClock(minute));
}

// The Game Rules page (US-195) is the Data tab opened on a file of rules/: a heading and one line that says what the saved file switches on and off.
bool DataEditor::gameRulesPage() const { return openFile_.starts_with("rules/") && document_.has_value(); }

std::string DataEditor::gameRulesSummary() const {
    if (!gameRulesPage()) return {};
    try {
        return sim::describeRules(sim::loadPlayRules(folder_, fs::path(openFile_).stem().string()));
    } catch (const std::exception& problem) {
        return std::string("the saved file cannot be read: ") + problem.what();
    }
}

const form::FormRow* DataEditor::rowAt(const std::string& path) const {
    for (const form::FormRow& row : rows_) {
        if (row.path == path) return &row;
    }
    return nullptr;
}

bool DataEditor::selectEntry(const std::string& path) {
    const bool known = std::any_of(entries_.begin(), entries_.end(), [&](const form::Entry& entry) { return entry.path == path; });
    if (!known) return false;
    entryPath_ = path;
    scroll_ = 0;
    collapsed_.clear();
    refreshRows();
    markStale();
    return true;
}

bool DataEditor::setField(const std::string& path, const std::string& text) {
    if (!document_) return false;
    if (rowsFor_ != document_->version()) refreshRows();
    const form::FormRow* row = rowAt(path);
    if (row == nullptr) {
        refuse("no field " + path);
        return false;
    }
    std::string reason;
    const form::FormRow copy = *row; // the rows are made again by the edit
    if (!form::applyText(*document_, copy, text, reason)) {
        refuse(copy.label + ": " + reason);
        markStale(); // the field shows what the file holds again
        return false;
    }
    problem_.clear();
    refreshRows();
    markStale();
    return true;
}

bool DataEditor::toggleFold(const std::string& path) {
    if (collapsed_.erase(path) == 0) collapsed_.insert(path);
    refreshRows();
    markStale();
    return true;
}

bool DataEditor::add(const std::string& path) {
    if (!document_) return false;
    const form::FormRow* row = rowAt(path);
    if (row == nullptr || row->node == nullptr) return false;
    const schema::Node& node = *row->node;
    std::string reason;
    bool done = false;
    if (row->kind == form::RowKind::MapHeader && node.additional) {
        std::set<std::string> taken;
        const sim::OrderedJson* map = document_->find(path);
        if (map != nullptr && map->is_object()) {
            for (const auto& item : map->items()) taken.insert(item.key());
        }
        done = document_->addMember(path, form::uniqueName("new-entry", taken), form::defaultValue(*node.additional), reason);
    } else if (row->kind == form::RowKind::ListHeader && node.items) {
        const sim::OrderedJson* list = document_->find(path);
        sim::OrderedJson element = form::defaultValue(*node.items);
        if (element.is_object() && list != nullptr && list->is_array()) {
            for (const char* key : {"name", "id", "kind"}) { // a new entry gets a name of its own
                if (!element.contains(key) || !element[key].is_string()) continue;
                std::set<std::string> taken;
                for (const sim::OrderedJson& other : *list) {
                    if (other.is_object() && other.contains(key) && other[key].is_string()) taken.insert(other[key].get<std::string>());
                }
                element[key] = form::uniqueName("new-entry", taken);
                break;
            }
        }
        done = document_->insertElement(path, list != nullptr ? list->size() : 0, std::move(element), reason);
    } else if (row->kind == form::RowKind::Heading && !row->present) {
        done = document_->set(path, form::defaultValue(node), reason);
    } else {
        return false;
    }
    if (!done) {
        refuse(reason);
        return false;
    }
    collapsed_.erase(path);
    refreshRows();
    markStale();
    return true;
}

bool DataEditor::remove(const std::string& path) {
    if (!document_) return false;
    const std::optional<sim::DocPath> parsed = sim::parsePath(path);
    if (!parsed || parsed->empty()) return false;
    sim::DocPath parent = *parsed;
    const sim::PathSegment last = parent.back();
    parent.pop_back();
    std::string reason;
    const bool done = last.index ? document_->removeElement(sim::formatPath(parent), last.position, reason) : document_->removeMember(sim::formatPath(parent), last.key, reason);
    if (!done) {
        refuse(reason);
        return false;
    }
    refreshRows();
    markStale();
    return true;
}

bool DataEditor::move(const std::string& path, int steps) {
    if (!document_) return false;
    const std::optional<sim::DocPath> parsed = sim::parsePath(path);
    if (!parsed || parsed->empty() || !parsed->back().index) return false;
    sim::DocPath parent = *parsed;
    const sim::PathSegment last = parent.back();
    parent.pop_back();
    const long long target = static_cast<long long>(last.position) + steps;
    if (target < 0) return false;
    std::string reason;
    if (!document_->moveElement(sim::formatPath(parent), last.position, static_cast<std::size_t>(target), reason)) {
        refuse(reason);
        return false;
    }
    refreshRows();
    markStale();
    return true;
}

bool DataEditor::undo() {
    if (!document_ || !document_->undo()) return false;
    refreshRows();
    markStale();
    say_("Data: undo " + document_->lastEdit());
    return true;
}

bool DataEditor::redo() {
    if (!document_ || !document_->redo()) return false;
    refreshRows();
    markStale();
    return true;
}

bool DataEditor::save() {
    if (!document_) return false;
    if (!document_->dirty()) {
        say_("Data: " + openFile_ + " has no changes to save");
        return true;
    }
    if (const std::optional<std::string> reason = document_->save()) {
        refuse("not saved: " + *reason);
        return false;
    }
    problem_.clear();
    discardWarned_ = false;
    say_("Saved " + openFile_);
    refreshIndex();
    if (saved_) saved_(folder_ / openFile_);
    markStale();
    return true;
}

// ---- Entity actions (US-193)

DataEditor::EntryInfo DataEditor::describeEntry() const {
    EntryInfo info;
    if (!document_ || schema_ == nullptr) return info;
    const form::Entry* current = nullptr;
    for (const form::Entry& entry : entries_) {
        if (entry.path == entryPath_) current = &entry;
    }
    if (current == nullptr || !current->element) return info;
    const std::optional<sim::DocPath> parsed = sim::parsePath(current->path);
    if (!parsed || parsed->size() != 2 || (*parsed)[0].index || !(*parsed)[1].index) return info;
    info.group = (*parsed)[0].key;
    info.index = (*parsed)[1].position;
    // What names the entry: the member that a "provides" of the schema takes the catalog's names from ("weapons[].name"); the entry is in each catalog that does.
    const std::string prefix = info.group + "[].";
    for (const schema::Provide& provide : schema_->provides) {
        if (!provide.at.starts_with(prefix)) continue;
        const std::string field = provide.at.substr(prefix.size());
        if (field.empty() || field.find_first_of(".[]*") != std::string::npos) continue; // "tags[]" are tags, not the name
        if (info.field.empty()) info.field = field;
        if (field == info.field) info.catalogs.insert(provide.catalog);
    }
    const sim::OrderedJson* name = info.field.empty() ? nullptr : document_->find(current->path + "." + info.field);
    if (name == nullptr || !name->is_string()) return info;
    info.name = name->get<std::string>();
    info.valid = !info.catalogs.empty();
    return info;
}

bool DataEditor::copyEntry() {
    const EntryInfo info = describeEntry();
    if (!info.valid) {
        refuse("choose an entry of a list first");
        return false;
    }
    const sim::OrderedJson* list = document_->find(info.group);
    sim::OrderedJson copy = (*list)[info.index];
    std::set<std::string> taken;
    for (const sim::OrderedJson& other : *list) {
        if (other.is_object() && other.contains(info.field) && other[info.field].is_string()) taken.insert(other[info.field].get<std::string>());
    }
    const std::string name = form::uniqueName(info.name + (info.name.find(' ') != std::string::npos ? " copy" : "-copy"), taken);
    copy[info.field] = name;
    std::string reason;
    if (!document_->insertElement(info.group, info.index + 1, std::move(copy), reason)) {
        refuse(reason);
        return false;
    }
    entryPath_ = sim::formatPath({{false, 0, info.group}, {true, info.index + 1, {}}});
    scroll_ = 0;
    refreshRows();
    markStale();
    say_("Data: copied " + info.name + " as " + name);
    return true;
}

namespace {

std::vector<std::string> useLines(const std::vector<refs::Use>& uses) {
    std::vector<std::string> lines;
    for (const refs::Use& use : uses) lines.push_back(std::format("{}:{}: {}{}", use.file, use.line, use.where, use.kind == "entry" ? " (the entry itself)" : ""));
    return lines;
}

} // namespace

bool DataEditor::beginRename(const std::string& newName) {
    const EntryInfo info = describeEntry();
    if (!info.valid) {
        refuse("choose an entry of a list first");
        return false;
    }
    if (dirty()) {
        refuse("unsaved changes in " + openFile_ + ": save (Ctrl+S) or undo them first, a rename writes the files");
        return false;
    }
    plan_ = refs::planRename(roots(), *set_, info.catalogs, info.name, newName);
    if (!plan_.problem.empty()) {
        refuse(plan_.problem);
        return false;
    }
    std::set<std::string> files;
    for (const refs::Use& use : plan_.uses) files.insert(use.file);
    question_ = {Question::Kind::Rename, std::format("Rename {} to {}: {} places in {} files", info.name, newName, plan_.uses.size(), files.size()), useLines(plan_.uses), newName};
    markStale();
    return true;
}

bool DataEditor::beginDelete() {
    const EntryInfo info = describeEntry();
    if (!info.valid) {
        refuse("choose an entry of a list first");
        return false;
    }
    const std::vector<refs::Use> uses = refs::usesOf(roots(), *set_, info.catalogs, info.name);
    question_ = {Question::Kind::Delete, std::format("Delete {}: still used in {} places", info.name, uses.size()), useLines(uses), std::string()};
    if (uses.empty()) return confirm(); // nothing names it: no need to ask
    markStale();
    return true;
}

bool DataEditor::confirm() {
    const EntryInfo info = describeEntry();
    if (question_.kind == Question::Kind::None || !info.valid) return false;
    if (question_.kind == Question::Kind::Delete) {
        std::string reason;
        if (!document_->removeElement(info.group, info.index, reason)) {
            refuse(reason);
            return false;
        }
        const std::size_t left = document_->find(info.group) != nullptr ? document_->find(info.group)->size() : 0;
        entryPath_ = left == 0 ? std::string() : sim::formatPath({{false, 0, info.group}, {true, std::min(info.index, left - 1), {}}});
        question_ = {};
        scroll_ = 0;
        refreshRows();
        markStale();
        say_("Data: deleted " + info.name + " (Ctrl+Z brings it back)");
        return true;
    }
    // A rename: the plan for the name in the dialog (made now when the dialog was opened without one), written to every file it names.
    if (plan_.texts.empty() && plan_.uses.empty()) plan_ = refs::planRename(roots(), *set_, info.catalogs, info.name, question_.name);
    if (!plan_.problem.empty()) {
        refuse(plan_.problem);
        return false;
    }
    if (dirty()) {
        refuse("unsaved changes in " + openFile_ + ": save (Ctrl+S) or undo them first");
        return false;
    }
    if (const std::optional<std::string> problem = refs::applyPlan(plan_)) {
        refuse("not renamed: " + *problem);
        return false;
    }
    const std::size_t places = plan_.uses.size();
    std::set<std::string> files;
    for (const refs::Use& use : plan_.uses) files.insert(use.file);
    std::string reason;
    document_->reloadFromDisk(reason);
    for (const auto& [path, text] : plan_.texts) {
        (void)text;
        if (saved_ && path.generic_string().starts_with(folder_.generic_string())) saved_(path); // the game reads the sets that watch the file again
    }
    say_(std::format("Data: renamed {} to {}: {} places in {} files", info.name, question_.name, places, files.size()));
    plan_ = {};
    question_ = {};
    refreshIndex();
    refreshRows();
    markStale();
    return true;
}

bool DataEditor::startQuickCheck() {
    if (dirty()) say_("Data: the unsaved changes to " + openFile_ + " are not in the check, save them first (Ctrl+S)");
    check_.emplace(folder_, quickSeed_ ? quickSeed_() : 42, kQuickYears);
    question_ = {Question::Kind::Quick, std::string(), {}, std::string()};
    stepQuickCheck();
    return true;
}

void DataEditor::stepQuickCheck() {
    if (!check_) return;
    check_->step(kQuickSliceDays);
    if (!check_->done()) {
        question_.title = std::format("Quick check: running, day {} of {} (Esc stops it)", check_->daysDone(), check_->daysTotal());
    } else if (!check_->error().empty()) {
        question_.title = "Quick check: the data cannot be read";
        question_.lines = {check_->error()};
        check_.reset();
    } else {
        const sim::QuickSummary now = check_->summary();
        question_.title = std::format("Quick check: {} years on seed {}", now.years, now.seed);
        question_.lines = sim::formatQuickSummary(now, lastCheck_ ? &*lastCheck_ : nullptr);
        lastCheck_ = now;
        check_.reset();
    }
    markStale();
}

void DataEditor::cancel() {
    check_.reset(); // Esc stops a Quick check that is still running
    question_ = {};
    plan_ = {};
    markStale();
}

std::vector<std::string> DataEditor::interactionsOfEntry() const {
    std::vector<std::string> found;
    const EntryInfo info = describeEntry();
    if (!info.valid) return found;
    std::set<std::string> tags; // the entry's own tags, when the file writes them
    const form::Entry* current = nullptr;
    for (const form::Entry& entry : entries_) {
        if (entry.path == entryPath_) current = &entry;
    }
    if (const sim::OrderedJson* list = current != nullptr ? document_->find(current->path + ".tags") : nullptr; list != nullptr && list->is_array()) {
        for (const sim::OrderedJson& tag : *list) {
            if (tag.is_string()) tags.insert(tag.get<std::string>());
        }
    }
    if (tagsOf_) {
        for (const std::string& tag : tagsOf_(info.name)) tags.insert(tag);
    }
    std::error_code error;
    for (const fs::directory_entry& file : fs::directory_iterator(folder_ / "interactions", error)) {
        const std::string name = file.path().filename().string();
        if (!file.is_regular_file() || file.path().extension() != ".json" || name.starts_with("defaults-") || name.find(".layout") != std::string::npos) continue;
        const std::optional<std::string> text = core::readTextFile(file.path());
        if (!text) continue;
        const nlohmann::json data = nlohmann::json::parse(*text, nullptr, false, true);
        if (data.is_discarded() || !data.contains("target") || !data.at("target").is_object()) continue;
        const nlohmann::json& target = data.at("target");
        bool matches = false;
        if (target.contains("kinds") && target.at("kinds").is_array()) {
            for (const nlohmann::json& kind : target.at("kinds")) matches = matches || (kind.is_string() && kind.get<std::string>() == info.name);
        }
        if (!matches && target.contains("tags") && target.at("tags").is_array() && !target.at("tags").empty() && !tags.empty()) {
            matches = true;
            for (const nlohmann::json& tag : target.at("tags")) matches = matches && tag.is_string() && tags.count(tag.get<std::string>()) != 0;
        }
        if (matches) found.push_back(file.path().stem().string());
    }
    std::sort(found.begin(), found.end());
    return found;
}

bool DataEditor::changedOnDisk(const fs::path& file) {
    if (!document_) return false;
    std::error_code error;
    if (!fs::equivalent(file, folder_ / openFile_, error) || error) return false;
    if (dirty()) {
        say_("Data: " + openFile_ + " changed on disk (your unsaved changes are kept)");
        return false;
    }
    std::string reason;
    if (!document_->reloadFromDisk(reason)) {
        refuse(reason);
        return false;
    }
    refreshRows();
    markStale();
    return true;
}

bool DataEditor::newEntry() {
    if (!document_ || schema_ == nullptr) return false;
    const std::vector<std::string> keys = [&] {
        std::vector<std::string> found;
        for (const form::Entry& entry : entries_) {
            if (entry.element && std::find(found.begin(), found.end(), entry.group) == found.end()) found.push_back(entry.group);
        }
        return found;
    }();
    if (keys.empty()) {
        refuse("this file has no list of entries to add to");
        return false;
    }
    const form::Entry* current = nullptr;
    for (const form::Entry& entry : entries_) {
        if (entry.path == entryPath_) current = &entry;
    }
    const std::string group = current != nullptr && current->element ? current->group : keys.front();
    const schema::Node* list = schema_->root->property(group);
    if (list == nullptr || !list->items) return false;
    const std::string listRows = sim::formatPath({{false, 0, group}});
    std::set<std::string> taken;
    const sim::OrderedJson* existing = document_->find(listRows);
    sim::OrderedJson element = form::defaultValue(*list->items);
    std::string label;
    if (element.is_object()) {
        for (const char* key : {"name", "id", "kind"}) {
            if (!element.contains(key) || !element[key].is_string()) continue;
            if (existing != nullptr) {
                for (const sim::OrderedJson& other : *existing) {
                    if (other.is_object() && other.contains(key) && other[key].is_string()) taken.insert(other[key].get<std::string>());
                }
            }
            label = form::uniqueName("new-entry", taken);
            element[key] = label;
            break;
        }
    }
    std::string reason;
    const std::size_t index = existing != nullptr ? existing->size() : 0;
    if (!document_->insertElement(listRows, index, std::move(element), reason)) {
        refuse(reason);
        return false;
    }
    entryPath_ = sim::formatPath({{false, 0, group}, {true, index, {}}});
    scroll_ = 0;
    refreshRows();
    markStale();
    return true;
}

std::string DataEditor::tipFor(const form::FormRow& row) const {
    std::string text;
    const EditorHelp::Entry* entry = help_ != nullptr ? help_->find(row.helpId) : nullptr;
    if (entry != nullptr) {
        text = entry->purpose;
        if (!entry->range.empty()) text += "\nRange: " + entry->range;
        if (!entry->example.empty()) text += "\nExample: " + entry->example;
    } else if (row.node != nullptr) {
        text = row.node->description;
        if (!row.node->example.empty()) text += "\nExample: " + row.node->example;
    }
    if (!row.error.empty()) text += (text.empty() ? "" : "\n") + std::string("Problem: ") + row.error;
    return text;
}

void DataEditor::addRow(Panel& panel, const form::FormRow& row, int x, int y, int right) {
    const std::string path = row.path;
    const int width = right - x;
    if (help_ != nullptr) help_->ask(row.helpId);
    const auto actions = [&] {
        int at = right + 2;
        if (row.canRemove) {
            Button& button = panel.add<Button>(Rect{at, y, 11, kRow - 1}, "x", [this, path] { remove(path); });
            button.hint = row.kind == form::RowKind::Heading || row.present ? "Remove this (a field goes back to its default)" : "";
            at += 12;
        }
        if (row.canMoveUp) {
            panel.add<Button>(Rect{at, y, 11, kRow - 1}, "^", [this, path] { move(path, -1); }).hint = "Move up";
            at += 12;
        }
        if (row.canMoveDown) panel.add<Button>(Rect{at, y, 11, kRow - 1}, "v", [this, path] { move(path, 1); }).hint = "Move down";
    };
    switch (row.kind) {
    case form::RowKind::Heading:
    case form::RowKind::ListHeader:
    case form::RowKind::MapHeader:
    case form::RowKind::ItemHeader: {
        if (row.kind == form::RowKind::ItemHeader && isMapEntry(row)) {
            // The key of a map entry is a name the owner can change.
            sim::DocPath parent = *sim::parsePath(path);
            const std::string key = parent.back().key;
            parent.pop_back();
            const std::string parentPath = sim::formatPath(parent);
            TextField& field = panel.add<TextField>(Rect{x, y, width, kRow - 1}, fitLabel("name", row.depth), key, 64, [this, parentPath, key](const std::string& v) {
                std::string reason;
                if (v.empty() || (document_ && document_->renameMember(parentPath, key, v, reason))) {
                    refreshRows();
                } else if (!reason.empty()) {
                    refuse(reason);
                }
                markStale();
            });
            field.helpId = row.helpId;
            field.tip.text = tipFor(row);
            actions();
            break;
        }
        const std::string label = std::string(row.collapsed ? "[+] " : "[-] ") + row.label + (row.kind == form::RowKind::ItemHeader ? "" : (row.present ? "" : " (not set)"));
        Button& heading = panel.add<Button>(Rect{x, y, std::max(30, width - (row.canAdd ? 30 : 0)), kRow - 1}, label, [this, path] { toggleFold(path); });
        heading.hint = row.node != nullptr ? row.node->description : std::string();
        if (row.canAdd) panel.add<Button>(Rect{right - 28, y, 28, kRow - 1}, "add", [this, path] { add(path); }).hint = "Add an entry";
        actions();
        break;
    }
    case form::RowKind::Flag: {
        Toggle& toggle = panel.add<Toggle>(Rect{x, y, width, kRow - 1}, fitLabel(row.label, row.depth), row.value == "true", [this, path](bool value) { setField(path, value ? "true" : "false"); });
        toggle.helpId = row.helpId;
        toggle.tip.text = tipFor(row);
        toggle.invalid = !row.error.empty();
        actions();
        break;
    }
    default: {
        const bool frame = row.node != nullptr && row.node->format == "frame"; // a picture of the atlas: a button opens the picker (US-192)
        if (frame) panel.add<Button>(Rect{x + width - 13, y, 13, kRow - 1}, "...", [this, path] { openPicker(path); }).hint = "Pick the picture from the atlas";
        TextField& field = panel.add<TextField>(Rect{x, y, frame ? width - 15 : width, kRow - 1}, fitLabel(row.label, row.depth), row.value, 600, [this, path](const std::string& v) { setField(path, v); });
        field.helpId = row.helpId;
        field.tip.text = tipFor(row);
        field.invalid = !row.error.empty();
        if (row.kind == form::RowKind::Choice) {
            field.suggest = [choices = row.choices](const std::string&) { return choices; };
        } else if (!row.catalog.empty()) {
            field.suggest = [this, catalog = row.catalog](const std::string&) { return catalogNames(catalog); };
            field.listItems = row.kind == form::RowKind::Words;
        }
        actions();
        break;
    }
    }
}

void DataEditor::buildForm(Panel& panel) {
    const int left = kFoldersWidth + kFilesWidth + kEntriesWidth + 14;
    const int right = viewWidth_ - 6 - kActions - previewWidth();
    int top = kBar + 6;
    if (gameRulesPage()) {
        panel.add<Label>(Rect{left, top, viewWidth_ - left - 6, kRow}, "GAME RULES: one page for the New Game choices, the thresholds of victory and the switches of whole systems", UiColor::Gold);
        const std::string summary = gameRulesSummary(); // the saved file: Ctrl+S, then the line follows the form
        panel.add<Label>(Rect{left, top + kRow, viewWidth_ - left - 6, kRow}, summary.substr(0, static_cast<std::size_t>((viewWidth_ - left - 6) / luna::engine::kTextAdvance)), UiColor::Dim);
        top += kRulesHeadingRows * kRow;
    }
    for (const Routine& routine : routinesShown()) { // the timeline of a routine above its form (US-196)
        const std::string list = routine.path;
        panel.add<TimelineView>(Rect{left, top, right - left, TimelineView::kHeight}, routine.title, routine.blocks, [this, list](std::size_t index, int minute) { moveRoutineBlock(list, index, minute); });
        top += kTimelineRows * kRow;
    }
    const int visible = visibleRows();
    const int first = std::clamp(scroll_, 0, std::max(0, static_cast<int>(rows_.size()) - visible));
    for (int i = first; i < std::min(static_cast<int>(rows_.size()), first + visible); ++i) {
        const form::FormRow& row = rows_[static_cast<std::size_t>(i)];
        addRow(panel, row, left + row.depth * kIndent, top + (i - first) * kRow, right);
    }
    if (rows_.empty()) panel.add<Label>(Rect{left, top, 300, kRow}, openFile_.empty() ? "Choose a file on the left." : "Nothing to show for this entry.");
}

// The dialog of a rename opens with the places that use the entry listed and the new name to type; Rename writes them all (confirm).
void DataEditor::openRenameDialog() {
    const EntryInfo info = describeEntry();
    if (!info.valid) {
        refuse("choose an entry of a list first");
        return;
    }
    if (dirty()) {
        refuse("unsaved changes in " + openFile_ + ": save (Ctrl+S) or undo them first, a rename writes the files");
        return;
    }
    const std::vector<refs::Use> uses = refs::usesOf(roots(), *set_, info.catalogs, info.name);
    plan_ = {};
    question_ = {Question::Kind::Rename, std::format("Rename {}: used in {} places (type the new name)", info.name, uses.size()), useLines(uses), info.name};
    markStale();
}

void DataEditor::buildQuestion(Panel& panel) {
    const int width = std::min(viewWidth_ - 40, 640);
    const int left = (viewWidth_ - width) / 2;
    const int top = 30;
    panel.add<Label>(Rect{left, top, width, 12}, question_.title, UiColor::Gold);
    int y = top + 18;
    if (question_.kind == Question::Kind::Rename) {
        panel.add<TextField>(Rect{left, y, width, 12}, "new name: ", question_.name, 48, [this](const std::string& v) {
            question_.name = v;
            plan_ = {}; // another name: the plan is made again when it is confirmed
        });
        y += 18;
    }
    const std::vector<std::string> lines = question_.lines.empty() ? std::vector<std::string>{question_.kind == Question::Kind::Quick ? "" : "(nothing uses it)"} : question_.lines;
    const int rows = std::max(4, (viewHeight_ - y - 60) / luna::engine::kLineHeight);
    panel.add<ListBox>(Rect{left, y, width, rows * luna::engine::kLineHeight}, lines, [](int) {});
    y += rows * luna::engine::kLineHeight + 8;
    if (question_.kind == Question::Kind::Quick) {
        panel.add<Button>(Rect{left, y, 60, 14}, check_ ? "Stop" : "Close", [this] { cancel(); });
        return;
    }
    const std::string yes = question_.kind == Question::Kind::Rename ? "Rename" : "Delete anyway";
    panel.add<Button>(Rect{left, y, UiPainter::textWidth(yes) + 16, 14}, yes, [this] { confirm(); }).hint = "Do it (Esc leaves everything as it was)";
    panel.add<Button>(Rect{left + UiPainter::textWidth(yes) + 24, y, 60, 14}, "Cancel", [this] { cancel(); });
}

void DataEditor::rebuild() {
    stale_ = false;
    panel_ = std::make_unique<Panel>(Rect{0, 0, viewWidth_, viewHeight_});
    Panel& panel = *panel_;
    if (question_.kind != Question::Kind::None) { // a question has the whole screen until it is answered
        buildQuestion(panel);
        return;
    }
    int x = 2;
    const auto button = [&](const std::string& label, const std::string& hint, std::function<void()> action) {
        const int width = UiPainter::textWidth(label) + 8;
        panel.add<Button>(Rect{x, 2, width, kBar - 4}, label, std::move(action)).hint = hint;
        x += width + 2;
    };
    button("Close", "Back to the map (Esc)", [this] { closeRequested_ = true; });
    button("Save", "Write the file: only what you changed changes (Ctrl+S)", [this] { save(); });
    button("Undo", "Undo the last edit of this file (Ctrl+Z)", [this] { undo(); });
    button("Redo", "Redo it (Ctrl+Y)", [this] { redo(); });
    button("New entry", "Add a new entry to the list of this file", [this] { newEntry(); });
    button("Copy", "Copy the chosen entry as a new one beside it", [this] { copyEntry(); });
    button("Rename", "Rename the chosen entry everywhere it is used (data files, levels, rules, dialogues): the places are listed first", [this] { openRenameDialog(); });
    button("Delete", "Delete the chosen entry; when it is still used the places are listed and you confirm (Ctrl+Z brings it back)", [this] { beginDelete(); });
    button("Cut tool", "Take a new picture from one of your sheets: drag a rectangle, name it, and the atlas is cut again", [this] { openCutTool(); });
    button("Quick check", "Run the clan for 20 years with the saved data and the game's seed and compare it with the run before", [this] { startQuickCheck(); });
    button("Interactions", "Open the interactions that target the chosen entry in the interaction graph", [this] {
        const std::vector<std::string> ids = interactionsOfEntry();
        if (ids.empty()) say_("Data: no interaction names this entry as its target kind; the list of interactions opens");
        if (openInteraction_) openInteraction_(ids.empty() ? std::string() : ids.front());
    });

    const int listTop = kBar + 6;
    const int listHeight = viewHeight_ - kStatus - listTop - 4;

    // Folders and files.
    const std::vector<std::string> all = files();
    std::vector<std::string> folders;
    for (const std::string& file : all) {
        const std::string folder = folderOf(file);
        if (std::find(folders.begin(), folders.end(), folder) == folders.end()) folders.push_back(folder);
    }
    std::sort(folders.begin(), folders.end(), [](const std::string& a, const std::string& b) { return a.empty() != b.empty() ? a.empty() : a < b; });
    std::vector<std::string> folderLabels;
    for (const std::string& folder : folders) folderLabels.push_back(folder.empty() ? "(top)" : folder);
    ListBox& folderList = panel.add<ListBox>(Rect{2, listTop, kFoldersWidth, listHeight}, folderLabels, [this, folders](int index) {
        if (index >= 0 && index < static_cast<int>(folders.size())) {
            folderChosen_ = folders[static_cast<std::size_t>(index)];
            markStale();
        }
    });
    for (std::size_t i = 0; i < folders.size(); ++i) {
        if (folders[i] == folderChosen_) folderList.selected = static_cast<int>(i);
    }
    std::vector<std::string> inFolder;
    std::vector<std::string> names;
    for (const std::string& file : all) {
        if (folderOf(file) != folderChosen_) continue;
        inFolder.push_back(file);
        names.push_back(file.substr(file.rfind('/') == std::string::npos ? 0 : file.rfind('/') + 1));
    }
    ListBox& fileList = panel.add<ListBox>(Rect{kFoldersWidth + 6, listTop, kFilesWidth, listHeight}, names, [this, inFolder](int index) {
        if (index >= 0 && index < static_cast<int>(inFolder.size()) && inFolder[static_cast<std::size_t>(index)] != openFile_) open(inFolder[static_cast<std::size_t>(index)]);
    });
    for (std::size_t i = 0; i < inFolder.size(); ++i) {
        if (inFolder[i] == openFile_) fileList.selected = static_cast<int>(i);
    }

    // The entries of the open file, found by what is typed in the search box.
    const int entriesLeft = kFoldersWidth + kFilesWidth + 10;
    panel.add<TextField>(Rect{entriesLeft, listTop, kEntriesWidth, kRow - 1}, "find ", search_, 40, [this](const std::string& v) {
        search_ = v;
        markStale();
    });
    std::vector<std::string> labels;
    std::vector<std::string> paths;
    int selected = -1;
    for (const form::Entry& entry : entries_) {
        if (!search_.empty() && lowered(entry.label).find(lowered(search_)) == std::string::npos) continue;
        if (entry.path == entryPath_) selected = static_cast<int>(labels.size());
        labels.push_back(entry.label);
        paths.push_back(entry.path);
    }
    ListBox& entryList = panel.add<ListBox>(Rect{entriesLeft, listTop + kRow + 2, kEntriesWidth, listHeight - kRow - 2}, labels, [this, paths](int index) {
        if (index >= 0 && index < static_cast<int>(paths.size()) && paths[static_cast<std::size_t>(index)] != entryPath_) selectEntry(paths[static_cast<std::size_t>(index)]);
    });
    entryList.selected = selected;

    buildForm(panel);
}

void DataEditor::update(const luna::engine::Intents& intents) {
    if (!shown_) return;
    if (problemTicks_ > 0 && --problemTicks_ == 0) problem_.clear();
    if (document_ && rowsFor_ != document_->version()) refreshRows();
    if (tool_ && tool_->open()) { // a picture tool has the whole screen until it is closed
        if (!tool_->update(intents)) {
            tool_.reset();
            markStale();
        }
        return;
    }
    refreshPreview();
    ++previewTick_;
    stepQuickCheck();
    if (stale_ && !typing()) rebuild();
    UiInput input = UiInput::from(intents);
    const int formLeft = kFoldersWidth + kFilesWidth + kEntriesWidth + 14;
    if (!typing() && input.pointer.wheel != 0 && input.pointer.x >= formLeft) { // the wheel over the form scrolls it
        const int most = std::max(0, static_cast<int>(rows_.size()) - visibleRows());
        scroll_ = std::clamp(scroll_ - input.pointer.wheel * 3, 0, most);
        input.pointer.wheel = 0;
        markStale();
    }
    panel_->handle(input);
    if (typing()) return;
    if (question_.kind != Question::Kind::None) { // a question waits for its answer: only Esc (no) works besides the buttons
        if (intents.pressed(luna::engine::Intent::OpenMenu)) cancel();
        return;
    }
    if (intents.pressed(luna::engine::Intent::Save)) save();
    if (intents.pressed(luna::engine::Intent::Undo)) undo();
    if (intents.pressed(luna::engine::Intent::Redo)) redo();
    if (intents.pressed(luna::engine::Intent::OpenMenu)) closeRequested_ = true;
    if (closeRequested_) {
        closeRequested_ = false;
        if (dirty() && !discardWarned_) {
            discardWarned_ = true;
            refuse(openFile_ + " has unsaved changes: Ctrl+S saves them, Esc again throws them away");
        } else {
            if (dirty()) say_("Data: unsaved changes to " + openFile_ + " were thrown away");
            document_.reset();
            openFile_.clear();
            schema_ = nullptr;
            rows_.clear();
            entries_.clear();
            discardWarned_ = false;
            show(false);
        }
    }
}

void DataEditor::draw(UiPainter& painter) const {
    if (!shown_) return;
    if (tool_ && tool_->open()) {
        tool_->draw(painter);
        return;
    }
    painter.fill({0, 0, viewWidth_, viewHeight_}, UiColor::Dark);
    panel_->draw(painter);
    if (!preview_.empty()) { // the frames of the chosen entry play beside its form
        const Rect box{viewWidth_ - 6 - kPreviewWidth + 4, kBar + 6, kPreviewWidth - 4, 104};
        painter.fill(box, UiColor::Panel);
        painter.outline(box, UiColor::Border);
        const PictureGrid::Cell& cell = preview_[static_cast<std::size_t>(previewTick_ / previewTicksPerFrame_) % preview_.size()];
        if (cell.texture.id >= 0) painter.image(cell.texture, cell.source, {box.x + (box.width - cell.source.width) / 2, box.y + (96 - cell.source.height) / 2 + 2});
        painter.text(box.x + 3, box.y + 94, std::format("{}/{}", static_cast<std::size_t>(previewTick_ / previewTicksPerFrame_) % preview_.size() + 1, preview_.size()), UiColor::Dim);
    }
    const Rect bar{0, viewHeight_ - kStatus, viewWidth_, kStatus};
    painter.fill(bar, UiColor::Shade);
    std::string line = openFile_.empty() ? std::string("Data: no file open") : "Data: " + openFile_ + (entryPath_.empty() ? std::string() : "  " + entryPath_) + (dirty() ? "  *unsaved" : "");
    painter.text(4, bar.y + 3, line, UiColor::Text);
    if (!problem_.empty()) {
        const int width = UiPainter::textWidth(problem_);
        painter.text(std::max(UiPainter::textWidth(line) + 24, viewWidth_ - width - 6), bar.y + 3, problem_, UiColor::Red);
    }
}

void DataEditor::drawOverlay(UiPainter& painter) const {
    if (!shown_) return;
    if (tool_ && tool_->open()) {
        tool_->drawOverlay(painter);
        return;
    }
    panel_->drawOverlay(painter);
}

} // namespace odysseus::game
