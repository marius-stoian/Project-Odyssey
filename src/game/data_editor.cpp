#include "game/data_editor.h"

#include "core/text.h"

#include <algorithm>
#include <cctype>
#include <format>

namespace odysseus::game {

namespace fs = std::filesystem;
namespace form = sim::form;
namespace schema = sim::schema;
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

bool DataEditor::typing() const { return shown_ && panel_ && panel_->typing(); }

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

int DataEditor::visibleRows() const { return std::max(1, (viewHeight_ - kStatus - kBar - 6) / kRow); }

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
        TextField& field = panel.add<TextField>(Rect{x, y, width, kRow - 1}, fitLabel(row.label, row.depth), row.value, 600, [this, path](const std::string& v) { setField(path, v); });
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
    const int right = viewWidth_ - 6 - kActions;
    const int top = kBar + 6;
    const int visible = visibleRows();
    const int first = std::clamp(scroll_, 0, std::max(0, static_cast<int>(rows_.size()) - visible));
    for (int i = first; i < std::min(static_cast<int>(rows_.size()), first + visible); ++i) {
        const form::FormRow& row = rows_[static_cast<std::size_t>(i)];
        addRow(panel, row, left + row.depth * kIndent, top + (i - first) * kRow, right);
    }
    if (rows_.empty()) panel.add<Label>(Rect{left, top, 300, kRow}, openFile_.empty() ? "Choose a file on the left." : "Nothing to show for this entry.");
}

void DataEditor::rebuild() {
    stale_ = false;
    panel_ = std::make_unique<Panel>(Rect{0, 0, viewWidth_, viewHeight_});
    Panel& panel = *panel_;
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
    painter.fill({0, 0, viewWidth_, viewHeight_}, UiColor::Dark);
    panel_->draw(painter);
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
    if (shown_) panel_->drawOverlay(painter);
}

} // namespace odysseus::game
