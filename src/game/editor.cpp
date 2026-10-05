#include "game/editor.h"

#include "core/log.h"
#include "sim/data.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace odysseus::game {

using luna::engine::Button;
using luna::engine::Intent;
using luna::engine::Intents;
using luna::engine::Panel;
using luna::engine::PointerButton;
using luna::engine::Rect;
using luna::engine::UiColor;
using luna::engine::UiPainter;

namespace {
constexpr int kStatusTicks = 60;  // a message stays three seconds
constexpr int kPaletteColumns = 3;
constexpr int kPaletteCell = 22;  // a tile button: a 20x20 piece of the tile and a border
constexpr int kToolbarHeight = 18;
} // namespace

const char* toolName(EditorTool tool) {
    switch (tool) {
    case EditorTool::Brush: return "Brush";
    case EditorTool::Rectangle: return "Rectangle";
    case EditorTool::Fill: return "Fill";
    case EditorTool::Eraser: return "Eraser";
    case EditorTool::Place: return "Place";
    case EditorTool::Select: return "Select";
    case EditorTool::Weapon: return "Weapon";
    case EditorTool::Plant: return "Plant";
    case EditorTool::Effect: return "Effect";
    case EditorTool::Light: return "Light";
    case EditorTool::Building: return "Build";
    }
    return "?";
}

namespace {

bool paints(EditorTool tool) {
    return tool == EditorTool::Brush || tool == EditorTool::Rectangle || tool == EditorTool::Fill || tool == EditorTool::Eraser;
}

// "plant monster" -> "Plant monster": the name a newly placed character gets.
std::string capitalised(std::string name) {
    if (!name.empty() && name[0] >= 'a' && name[0] <= 'z') name[0] = static_cast<char>(name[0] - 'a' + 'A');
    return name;
}

constexpr int kKindColumns = 2;
constexpr int kKindWidth = 30;   // a character button: the middle of the figure
constexpr int kKindHeight = 34;
constexpr int kPropertiesWidth = 136;

} // namespace

Editor::Editor(Level& level, const Definitions& definitions, std::filesystem::path levelFile, int viewWidth, int viewHeight)
    : level_(level), definitions_(definitions), levelFile_(std::move(levelFile)), viewWidth_(viewWidth), viewHeight_(viewHeight),
      map_(buildTileMap(level, definitions)), camera_(viewWidth, viewHeight, map_.pixelWidth(), map_.pixelHeight()) {
    buildingEditor_ = std::make_unique<BuildingEditor>(level_, viewWidth, viewHeight, [this](std::unique_ptr<Command> command) { run(std::move(command)); }, [this](const std::string& message) { say(message); });
    buildPanels();
}

void Editor::setTextures(const EditorTextures& textures) {
    textures_ = textures;
    buildingEditor_->setLayer(textures.buildings);
    markerTextures_.clear(); // the pictures belong to the renderer that made them
    buildPanels(); // the palette's buttons show pieces of the tile texture
}

void Editor::buildPanels() {
    toolbar_ = std::make_unique<Panel>(Rect{2, 2, 300, kToolbarHeight});
    int x = 4;
    auto button = [&](const std::string& label, const std::string& hint, auto action) -> Button& {
        const int width = UiPainter::textWidth(label) + 8;
        Button& added = toolbar_->add<Button>(Rect{x, 4, width, kToolbarHeight - 4}, label, action);
        added.hint = hint;
        x += width + 2;
        return added;
    };
    button("Brush", "Paint cells: click or drag", [this] { tool_ = EditorTool::Brush; });
    button("Rect", "Fill a rectangle: drag from corner to corner", [this] { tool_ = EditorTool::Rectangle; });
    button("Fill", "Fill the connected area of the same ground", [this] { tool_ = EditorTool::Fill; });
    button("Erase", "Paint the level's default ground", [this] { tool_ = EditorTool::Eraser; });
    x += 2;
    button("Place", "Place a character: choose one, click the map", [this] { tool_ = EditorTool::Place; });
    button("Arms", "Place a weapon pickup: choose one, click the map", [this] { tool_ = EditorTool::Weapon; });
    button("Plant", "Place a plant: choose one (pages with the arrows), click the map", [this] { tool_ = EditorTool::Plant; });
    button("Fx", "Place a looping effect (fireflies, campfire, portal): choose one, click the map", [this] { tool_ = EditorTool::Effect; });
    button("Light", "Place a light (campfire, torch): choose a kind, click the map; it shines in the game after dark", [this] { tool_ = EditorTool::Light; });
    button("Build", "Place a building (or a blueprint): choose a kind or prefab, R turns it, click the map", [this] { tool_ = EditorTool::Building; });
    button("Prefab", "Compose a building from pieces on a grid and save it as a prefab", [this] { buildingEditor_->showPrefabs(!buildingEditor_->prefabsShown()); });
    button("Select", "Select a character, pickup, plant, effect or light: drag to move, R to turn, Delete to remove", [this] { tool_ = EditorTool::Select; });
    x += 2;
    button("Level", "Level settings: name, size, ground; new and open", [this] { showSettings(!settingsShown_); });
    button("Class", "NPC Classes: create, edit and delete the kinds of people of your world", [this] { showClasses(!classesShown_); });
    button("#", "Grid: show or hide the cell lines (G)", [this] { grid_ = !grid_; });
    button("Sky", "Preview the light of any time of day with the slider (a view only: not saved)", [this] { setPreviewHour(previewHour_ ? std::nullopt : std::optional<double>(12.0)); });
    button("Undo", "Undo (Ctrl+Z)", [this] { undo(); });
    button("Redo", "Redo (Ctrl+Y)", [this] { redo(); });
    button("Save", "Save the level (Ctrl+S)", [this] { save(); });
    toolbar_->bounds.width = x;

    const int count = static_cast<int>(definitions_.tiles.size());
    const int rows = (count + kPaletteColumns - 1) / kPaletteColumns;
    palette_ = std::make_unique<Panel>(Rect{2, kToolbarHeight + 4, kPaletteColumns * kPaletteCell + 4, rows * kPaletteCell + 4});
    for (int i = 0; i < count; ++i) {
        const Rect cell{4 + (i % kPaletteColumns) * kPaletteCell, kToolbarHeight + 6 + (i / kPaletteColumns) * kPaletteCell, kPaletteCell - 2,
                        kPaletteCell - 2};
        Button& tile = palette_->add<Button>(cell, "", [this, i] {
            tile_ = i;
            if (tool_ == EditorTool::Eraser) tool_ = EditorTool::Brush; // choosing a ground means painting with it
        });
        tile.hint = definitions_.tiles[static_cast<std::size_t>(i)].name;
        tile.icon = &textures_.tiles;
        tile.iconSource = {i * kTileSize + 7, 7, kPaletteCell - 4, kPaletteCell - 4};
    }

    buildCharacterPalette();
    // Weapons (US-134): every weapon a pickup may carry, shown by its icon (drawn in render()).
    const int weapons = static_cast<int>(weaponNames_.size());
    const int weaponRows = std::max(1, (weapons + kPaletteColumns - 1) / kPaletteColumns);
    weaponPalette_ = std::make_unique<Panel>(Rect{2, kToolbarHeight + 4, kPaletteColumns * kPaletteCell + 4, weaponRows * kPaletteCell + 4});
    for (int i = 0; i < weapons; ++i) {
        const Rect cell{4 + (i % kPaletteColumns) * kPaletteCell, kToolbarHeight + 6 + (i / kPaletteColumns) * kPaletteCell, kPaletteCell - 2, kPaletteCell - 2};
        Button& weapon = weaponPalette_->add<Button>(cell, "", [this, i] {
            weapon_ = i;
            tool_ = EditorTool::Weapon; // choosing a weapon means placing it
        });
        weapon.hint = weaponNames_[static_cast<std::size_t>(i)];
    }
    buildPlantPalette();
    // Looping effects (US-138): the ones of effects.json that loop, by their first picture (drawn in render()).
    {
        const int effects = static_cast<int>(definitions_.loopingEffects.size());
        const int effectColumns = 4;
        const int effectRows = std::max(1, (effects + effectColumns - 1) / effectColumns);
        effectPalette_ = std::make_unique<Panel>(Rect{2, kToolbarHeight + 4, effectColumns * kPaletteCell + 4, effectRows * kPaletteCell + 4});
        for (int i = 0; i < effects; ++i) {
            const Rect cell{4 + (i % effectColumns) * kPaletteCell, kToolbarHeight + 6 + (i / effectColumns) * kPaletteCell, kPaletteCell - 2, kPaletteCell - 2};
            Button& effectButton = effectPalette_->add<Button>(cell, "", [this, i] {
                effect_ = i;
                tool_ = EditorTool::Effect; // choosing an effect means placing it
            });
            effectButton.hint = definitions_.loopingEffects[static_cast<std::size_t>(i)];
        }
    }
    // Lights (US-247): the kinds of lights.json by name.
    {
        const int kinds = static_cast<int>(definitions_.lightKinds.size());
        int widest = 0;
        for (const std::string& name : definitions_.lightKinds) widest = std::max(widest, UiPainter::textWidth(name));
        constexpr int kRow = 14;
        lightPalette_ = std::make_unique<Panel>(Rect{2, kToolbarHeight + 4, widest + 16, std::max(1, kinds) * kRow + 4});
        for (int i = 0; i < kinds; ++i) {
            const Rect row{4, kToolbarHeight + 6 + i * kRow, widest + 12, kRow - 2};
            Button& lightButton = lightPalette_->add<Button>(row, definitions_.lightKinds[static_cast<std::size_t>(i)], [this, i] {
                light_ = i;
                tool_ = EditorTool::Light; // choosing a kind of light means placing it
            });
            lightButton.hint = definitions_.lightKinds[static_cast<std::size_t>(i)];
        }
    }
    buildProperties();
    buildSettings();
    buildEconomy();
    buildClassPanel();
    buildOpenList();
    buildQuestion();
}

namespace {

std::string joinNames(const std::vector<std::string>& names) {
    std::string out;
    for (const std::string& name : names) out += (out.empty() ? "" : ", ") + name;
    return out;
}

std::vector<std::string> splitNames(const std::string& text) {
    std::vector<std::string> out;
    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t end = text.find(',', start);
        if (end == std::string::npos) end = text.size();
        std::string part = text.substr(start, end - start);
        const auto first = part.find_first_not_of(' ');
        const auto last = part.find_last_not_of(' ');
        if (first != std::string::npos) out.push_back(part.substr(first, last - first + 1));
        start = end + 1;
    }
    return out;
}

// "player=greet.dlg, class:guard=x.dlg" and back (the Talk field of a class and of a kind). nullopt when a part has no "=".
std::string formatDialogues(const std::vector<std::pair<std::string, std::string>>& dialogues) {
    std::string out;
    for (const auto& [type, file] : dialogues) out += (out.empty() ? "" : ", ") + type + "=" + file;
    return out;
}

std::optional<std::vector<std::pair<std::string, std::string>>> parseDialogues(const std::string& text) {
    std::vector<std::pair<std::string, std::string>> parsed;
    for (const std::string& part : splitNames(text)) {
        const auto equals = part.find('=');
        if (equals == std::string::npos) return std::nullopt;
        parsed.emplace_back(part.substr(0, equals), part.substr(equals + 1));
    }
    return parsed;
}

} // namespace

void Editor::showClasses(bool shown) {
    classesShown_ = shown;
    if (shown) {
        classesStale_ = true;
        showSettings(false);
    }
}

void Editor::newClass() {
    classDraft_ = sim::rules::NpcClass{};
    classDraft_.id = "new-class";
    classDraft_.label = "New class";
    classDraft_.colour = 0x808080;
    classDraft_.icon = sim::rules::npcIconNames().front();
    classNew_ = true;
    classSelected_.clear();
    classesStale_ = true;
}

void Editor::selectClass(const std::string& id) {
    if (classBook_ == nullptr) return;
    if (const sim::rules::NpcClass* found = classBook_->catalog().find(id)) {
        classDraft_ = *found;
        classNew_ = false;
        classSelected_ = id;
        classesStale_ = true;
    }
}

bool Editor::saveClass() {
    if (classBook_ == nullptr) return false;
    if (classNew_ && classBook_->catalog().find(classDraft_.id) != nullptr) {
        say(std::format("a class named {} already exists", classDraft_.id));
        return false;
    }
    if (const auto problem = classBook_->save(classDraft_)) {
        say(*problem);
        return false;
    }
    classNew_ = false;
    classSelected_ = classDraft_.id;
    classesStale_ = true;
    say(std::format("saved class {}", classDraft_.id));
    return true;
}

bool Editor::deleteClass() {
    if (classBook_ == nullptr || classNew_ || classSelected_.empty()) return false;
    if (const auto refusal = classBook_->remove(classSelected_, level_)) {
        say(*refusal);
        return false;
    }
    say(std::format("deleted class {}", classSelected_));
    classSelected_.clear();
    classDraft_ = sim::rules::NpcClass{};
    classesStale_ = true;
    return true;
}

// The NPC Classes panel (US-260): the classes on top, then the form of the draft, then the buttons.
void Editor::buildClassPanel() {
    classesStale_ = false;
    classes_ = std::make_unique<Panel>(Rect{viewWidth_ - 232, kToolbarHeight + 4, 230, kindsTab_ ? 386 : 354});
    classes_->visible = classesShown_;
    const Rect box = classes_->bounds;
    const int left = box.x + 4;
    const int width = box.width - 8;
    int y = box.y + 4;
    // Two tabs: the classes themselves, and the defaults of each kind (US-269).
    Button& classesTab = classes_->add<Button>(Rect{left, y, width / 2 - 1, 11}, "Classes", [this] {
        kindsTab_ = false;
        classesStale_ = true;
    });
    classesTab.selected = !kindsTab_;
    classesTab.hint = "The NPC Classes: create, edit and delete";
    Button& kindsButton = classes_->add<Button>(Rect{left + width / 2 + 1, y, width / 2 - 1, 11}, "Kinds", [this] {
        kindsTab_ = true;
        classesStale_ = true;
    });
    kindsButton.selected = kindsTab_;
    kindsButton.hint = "The defaults of each kind of character: classes, attitude, talk, actions";
    y += 14;
    if (kindsTab_) {
        buildKindForm(box, y);
        return;
    }
    std::vector<std::string> ids = classBook_ != nullptr ? classBook_->catalog().ids() : std::vector<std::string>{};
    auto& list = classes_->add<luna::engine::ListBox>(Rect{left, y, width, 45}, ids, [this, ids](int row) { selectClass(ids[static_cast<std::size_t>(row)]); });
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (ids[i] == classSelected_) {
            list.selected = static_cast<int>(i);
            list.first = std::clamp(static_cast<int>(i) - 2, 0, std::max(0, static_cast<int>(ids.size()) - list.rows()));
        }
    }
    y += 48;
    if (classNew_) {
        classes_->add<luna::engine::TextField>(Rect{left, y, width, 11}, "Id", classDraft_.id, 32, [this](const std::string& v) { classDraft_.id = v; });
    } else {
        classes_->add<Button>(Rect{left, y, width, 11}, "Id: " + classDraft_.id, [] {});
    }
    y += 14;
    classes_->add<luna::engine::TextField>(Rect{left, y, width, 11}, "Label", classDraft_.label, 24, [this](const std::string& v) { classDraft_.label = v; });
    y += 14;
    classes_->add<luna::engine::TextField>(Rect{left, y, width, 11}, "Colour", sim::rules::formatColour(classDraft_.colour), 7, [this](const std::string& v) {
        if (const auto colour = sim::rules::parseColour(v)) classDraft_.colour = *colour;
        else say("colour must be # and six hex digits, like #d9a441");
    });
    y += 14;
    Button& icon = classes_->add<Button>(Rect{left, y, width, 11}, "Icon: " + classDraft_.icon, [this] {
        const auto& icons = sim::rules::npcIconNames();
        const auto at = std::find(icons.begin(), icons.end(), classDraft_.icon);
        classDraft_.icon = icons[(static_cast<std::size_t>(at - icons.begin()) + 1) % icons.size()];
        classesStale_ = true;
    });
    icon.hint = "Click: the next icon of the built-in set";
    y += 14;
    classes_->add<luna::engine::TextField>(Rect{left, y, width, 11}, "Tags", joinNames(classDraft_.tags), 60, [this](const std::string& v) { classDraft_.tags = splitNames(v); });
    y += 14;
    classes_->add<luna::engine::TextField>(Rect{left, y, width, 11}, "Talk", formatDialogues(classDraft_.dialogues), 90, [this](const std::string& v) {
        if (const auto parsed = parseDialogues(v)) classDraft_.dialogues = *parsed;
        else say("talk is partner=file.dlg, for example player=greet.dlg");
    });
    y += 14;
    classes_->add<luna::engine::TextField>(Rect{left, y, width, 11}, "Allow", joinNames(classDraft_.allow), 90, [this](const std::string& v) { classDraft_.allow = splitNames(v); });
    y += 14;
    classes_->add<luna::engine::TextField>(Rect{left, y, width, 11}, "Deny", joinNames(classDraft_.deny), 90, [this](const std::string& v) { classDraft_.deny = splitNames(v); });
    y += 14;
    addTradeRows(*classes_, left, width, y, classDraft_.extras.trade, [this](const std::string& field, const std::string& text) { return setClassTrade(field, text); });
    addScheduleRows(*classes_, left, width, y, classDraft_.extras.schedule, [this](const std::string& field, const std::string& text) { return setClassSchedule(field, text); });
    addDoesRow(*classes_, left, width, y, classDraft_.extras.does, [this](const std::string& text) { return setClassDoes(text); });
    addPartnerRows(*classes_, left, width, y, classDraft_.extras, [this](const std::string& type, const std::string& text) { return setClassPartnerActions(type, text); }, [this] { classesStale_ = true; });
    y += 4;
    classes_->add<Button>(Rect{left, y, 52, 14}, "New", [this] { newClass(); }).hint = "Start a new class";
    classes_->add<Button>(Rect{left + 56, y, 52, 14}, "Save", [this] { saveClass(); }).hint = "Write the class to its file";
    classes_->add<Button>(Rect{left + 112, y, 56, 14}, "Delete", [this] { deleteClass(); }).hint = "Delete the class (refused while NPCs use it)";
    classes_->add<Button>(Rect{left + 172, y, 52, 14}, "Close", [this] { showClasses(false); });
}

// ---- the Kinds tab (US-269)

void Editor::showKinds(bool shown) {
    kindsTab_ = shown;
    showClasses(shown);
}

std::vector<std::string> Editor::kindNames() const {
    std::vector<std::string> names;
    for (const CharacterKindDef& kind : definitions_.characters) {
        if (std::find(kind.tags.begin(), kind.tags.end(), "hero") == kind.tags.end()) names.push_back(kind.name);
    }
    return names;
}

void Editor::selectKind(const std::string& name) {
    if (classBook_ == nullptr) return;
    const std::vector<std::string> names = kindNames();
    if (std::find(names.begin(), names.end(), name) == names.end()) return;
    if (const sim::rules::NpcKind* found = classBook_->kinds().find(name)) {
        kindDraft_ = *found;
    } else {
        kindDraft_ = sim::rules::NpcKind{};
        kindDraft_.kind = name;
        kindDraft_.file = "npcs/" + name + ".json";
    }
    kindSelected_ = name;
    classesStale_ = true;
}

void Editor::toggleKindClass(const std::string& id) {
    if (kindSelected_.empty()) return;
    std::vector<std::string> classes = kindDraft_.layer.classes.value_or(std::vector<std::string>{});
    const auto at = std::find(classes.begin(), classes.end(), id);
    if (at != classes.end()) classes.erase(at);
    else classes.push_back(id);
    kindDraft_.layer.classes = classes.empty() ? std::nullopt : std::optional<std::vector<std::string>>(classes);
    classesStale_ = true;
}

bool Editor::saveKind() {
    if (classBook_ == nullptr || kindSelected_.empty()) return false;
    if (const auto problem = classBook_->saveKind(kindDraft_)) {
        say(*problem);
        return false;
    }
    propertiesStale_ = true; // the NPC panel shows what the kind says now
    classesStale_ = true;
    say(std::format("saved kind {}", kindDraft_.kind));
    return true;
}

// The form of the Kinds tab: the same rows as the NPC panel (classes, attitude, tags, talk, allow, deny), but they edit the kind's draft and Save writes its file.
void Editor::buildKindForm(const Rect& box, int y) {
    const int left = box.x + 4;
    const int width = box.width - 8;
    const std::vector<std::string> names = kindNames();
    std::vector<std::string> rows;
    for (const std::string& name : names) rows.push_back(name + (classBook_ != nullptr && classBook_->kinds().find(name) == nullptr ? " (no file)" : ""));
    auto& list = classes_->add<luna::engine::ListBox>(Rect{left, y, width, 4 * luna::engine::kLineHeight}, rows, [this, names](int row) { selectKind(names[static_cast<std::size_t>(row)]); });
    for (std::size_t i = 0; i < names.size(); ++i) {
        if (names[i] == kindSelected_) {
            list.selected = static_cast<int>(i);
            list.first = std::clamp(static_cast<int>(i) - 1, 0, std::max(0, static_cast<int>(names.size()) - list.rows()));
        }
    }
    y += 4 * luna::engine::kLineHeight + 3;
    if (kindSelected_.empty()) {
        classes_->add<Button>(Rect{left, y, width, 11}, "Pick a kind above", [] {});
    } else {
        classes_->add<Button>(Rect{left, y, width, 11}, "Kind: " + kindSelected_, [] {});
        y += 13;
        const std::vector<std::string> ids = classBook_ != nullptr ? classBook_->catalog().ids() : std::vector<std::string>{};
        const std::vector<std::string> have = kindDraft_.layer.classes.value_or(std::vector<std::string>{});
        std::vector<std::string> classRows;
        for (const std::string& id : ids) classRows.push_back(std::string(std::find(have.begin(), have.end(), id) != have.end() ? "[x] " : "[ ] ") + id);
        classes_->add<luna::engine::ListBox>(Rect{left, y, width, 5 * luna::engine::kLineHeight}, classRows, [this, ids](int row) { toggleKindClass(ids[static_cast<std::size_t>(row)]); });
        y += 5 * luna::engine::kLineHeight + 3;
        // Attitude: a click goes to the next word; after the last comes "none" (the kind says nothing, so NPCs are neutral unless they say otherwise).
        Button& attitude = classes_->add<Button>(Rect{left, y, width, 11}, "Attitude: " + kindDraft_.layer.attitude.value_or("(none)"), [this] {
            const auto& words = sim::rules::attitudeNames();
            if (!kindDraft_.layer.attitude) {
                kindDraft_.layer.attitude = words.front();
            } else {
                const auto at = std::find(words.begin(), words.end(), *kindDraft_.layer.attitude);
                const std::size_t next = static_cast<std::size_t>(at - words.begin()) + 1;
                if (next >= words.size()) kindDraft_.layer.attitude.reset();
                else kindDraft_.layer.attitude = words[next];
            }
            classesStale_ = true;
        });
        attitude.hint = "Click: the next attitude word, then none. NPCs of this kind start like this unless they set their own";
        y += 14;
        classes_->add<luna::engine::TextField>(Rect{left, y, width, 11}, "Tags", joinNames(kindDraft_.layer.tags), 60, [this](const std::string& v) { kindDraft_.layer.tags = splitNames(v); });
        y += 14;
        classes_->add<luna::engine::TextField>(Rect{left, y, width, 11}, "Talk", formatDialogues(kindDraft_.layer.dialogues), 90, [this](const std::string& v) {
            if (const auto parsed = parseDialogues(v)) kindDraft_.layer.dialogues = *parsed;
            else say("talk is partner=file.dlg, for example player=greet.dlg");
        });
        y += 14;
        classes_->add<luna::engine::TextField>(Rect{left, y, width, 11}, "Allow", joinNames(kindDraft_.layer.allow), 90, [this](const std::string& v) { kindDraft_.layer.allow = splitNames(v); });
        y += 14;
        classes_->add<luna::engine::TextField>(Rect{left, y, width, 11}, "Deny", joinNames(kindDraft_.layer.deny), 90, [this](const std::string& v) { kindDraft_.layer.deny = splitNames(v); });
        y += 14;
        addTradeRows(*classes_, left, width, y, kindDraft_.layer.extras.trade, [this](const std::string& field, const std::string& text) { return setKindTrade(field, text); });
        addScheduleRows(*classes_, left, width, y, kindDraft_.layer.extras.schedule, [this](const std::string& field, const std::string& text) { return setKindSchedule(field, text); });
        addDoesRow(*classes_, left, width, y, kindDraft_.layer.extras.does, [this](const std::string& text) { return setKindDoes(text); });
        addPartnerRows(*classes_, left, width, y, kindDraft_.layer.extras, [this](const std::string& type, const std::string& text) { return setKindPartnerActions(type, text); }, [this] { classesStale_ = true; });
    }
    const int bottom = box.y + box.height - 18;
    classes_->add<Button>(Rect{left, bottom, 80, 14}, "Save", [this] { saveKind(); }).hint = "Write the kind's file: every NPC of this kind that sets nothing itself follows it";
    classes_->add<Button>(Rect{left + 84, bottom, 52, 14}, "Close", [this] { showClasses(false); });
}

// The ring and icon under a placed NPC (US-269): one picture per distinct marker, made the first time it is drawn.
const luna::engine::Texture& Editor::markerTexture(luna::engine::Renderer& renderer, const NpcMarker& marker) const {
    std::string key = marker.icon;
    for (const int colour : marker.colours) key += std::format("/{:06x}", colour);
    const auto found = markerTextures_.find(key);
    if (found != markerTextures_.end()) return found->second;
    return markerTextures_.emplace(key, renderer.createTexture(markerImage(marker))).first->second;
}

std::vector<int> Editor::markerTextureIds() const {
    std::vector<int> ids;
    for (const auto& [key, texture] : markerTextures_) ids.push_back(texture.id);
    return ids;
}

int Editor::markerCount() const {
    if (classBook_ == nullptr) return 0;
    int count = 0;
    for (const PlacedCharacter& placed : level_.characters) {
        if (classBook_->kinds().find(placed.kind) != nullptr && markerFor(classBook_->resolve(placed), classBook_->catalog())) ++count;
    }
    return count;
}

void Editor::setPreviewHour(std::optional<double> hour) {
    previewHour_ = hour ? std::optional<double>(std::clamp(*hour, 0.0, 24.0)) : std::nullopt;
    sliderDragging_ = false;
}

// The slider sits right of the tool bar while the preview is on: left end midnight, right end the next midnight.
luna::engine::Rect Editor::timeSliderRect() const {
    return {toolbar_->bounds.x + toolbar_->bounds.width + 6, 4, 144, kToolbarHeight - 4};
}

// One page of the character palette (US-137): 12 kinds, two to a row, then the page arrows. The first page holds
// the twelve characters of characters.json, where they always were; the animals follow on the next pages.
void Editor::buildCharacterPalette() {
    const int kinds = static_cast<int>(definitions_.characters.size());
    const int pages = std::max(1, (kinds + kKindsPerPage - 1) / kKindsPerPage);
    kindPage_ = std::clamp(kindPageWanted_, 0, pages - 1);
    kindPageWanted_ = kindPage_;
    const int rows = kKindsPerPage / kKindColumns;
    characterPalette_ = std::make_unique<Panel>(Rect{2, kToolbarHeight + 4, kKindColumns * kKindWidth + 4, rows * kKindHeight + 4 + 14});
    for (int i = 0; i < kKindsPerPage; ++i) {
        const int index = kindPage_ * kKindsPerPage + i;
        if (index >= kinds) break;
        const CharacterKindDef& def = definitions_.characters[static_cast<std::size_t>(index)];
        const Rect cell{4 + (i % kKindColumns) * kKindWidth, kToolbarHeight + 6 + (i / kKindColumns) * kKindHeight, kKindWidth - 2, kKindHeight - 2};
        Button& kind = characterPalette_->add<Button>(cell, textures_.art == nullptr ? capitalised(def.name).substr(0, 1) : "", [this, index] {
            kind_ = index;
            tool_ = EditorTool::Place; // choosing a character means placing it
        });
        kind.hint = def.animal ? def.name + (def.enemy ? " (enemy)" : " (harmless)") : def.name;
        if (textures_.art != nullptr && !def.animal) {
            const Rect frame = textures_.art->frame(def.frames, def.directions, Facing::South, 0);
            kind.icon = &textures_.characters;
            kind.iconSource = {frame.x + (kCharacterWidth - (kKindWidth - 4)) / 2, frame.y + kCharacterHeight - (kKindHeight - 4) - 2, kKindWidth - 4,
                               kKindHeight - 4};
        }
    }
    const int arrowsTop = kToolbarHeight + 6 + rows * kKindHeight;
    Button& previous = characterPalette_->add<Button>(Rect{4, arrowsTop, 20, 12}, "<", [this] { kindPageWanted_ = kindPage_ - 1; });
    previous.hint = "Previous page of characters";
    Button& next = characterPalette_->add<Button>(Rect{4 + kKindColumns * kKindWidth - 22, arrowsTop, 20, 12}, ">", [this] { kindPageWanted_ = kindPage_ + 1; });
    next.hint = "Next page of characters";
}
// The plants, then the world objects: one list, the objects on the last page(s).
std::vector<std::string> Editor::plantKinds() const {
    std::vector<std::string> kinds = definitions_.plants;
    kinds.insert(kinds.end(), definitions_.objects.begin(), definitions_.objects.end());
    return kinds;
}

int Editor::plantPageCount() const {
    const int plantPages = (static_cast<int>(definitions_.plants.size()) + kPlantsPerPage - 1) / kPlantsPerPage;
    return std::max(1, plantPages + (definitions_.objects.empty() ? 0 : 1));
}

int Editor::plantPageFirst(int page) const {
    const int plants = static_cast<int>(definitions_.plants.size());
    const int plantPages = (plants + kPlantsPerPage - 1) / kPlantsPerPage;
    return page < plantPages ? page * kPlantsPerPage : plants;
}

int Editor::plantPageSize(int page) const {
    const int plants = static_cast<int>(definitions_.plants.size());
    const int plantPages = (plants + kPlantsPerPage - 1) / kPlantsPerPage;
    if (page < plantPages) return std::min(kPlantsPerPage, plants - page * kPlantsPerPage);
    return std::min(kPlantsPerPage, static_cast<int>(definitions_.objects.size()));
}

// One page of the plant palette: two arrows, then up to 36 plants (or the world objects) by picture (drawn in render()).
void Editor::buildPlantPalette() {
    const std::vector<std::string> kinds = plantKinds();
    const int pages = plantPageCount();
    plantPage_ = std::clamp(plantPageWanted_, 0, pages - 1);
    plantPageWanted_ = plantPage_;
    const int columns = 4;
    const int rows = kPlantsPerPage / columns;
    plantPalette_ = std::make_unique<Panel>(Rect{2, kToolbarHeight + 4, columns * kPaletteCell + 4, rows * kPaletteCell + 4 + 14});
    const int top = kToolbarHeight + 6;
    Button& previous = plantPalette_->add<Button>(Rect{4, top, 20, 12}, "<", [this] { plantPageWanted_ = plantPage_ - 1; });
    previous.hint = "Previous page of plants";
    Button& next = plantPalette_->add<Button>(Rect{4 + columns * kPaletteCell - 22, top, 20, 12}, ">", [this] { plantPageWanted_ = plantPage_ + 1; });
    next.hint = "Next page of plants";
    for (int i = 0; i < plantPageSize(plantPage_); ++i) {
        const int index = plantPageFirst(plantPage_) + i;
        const Rect cell{4 + (i % columns) * kPaletteCell, top + 14 + (i / columns) * kPaletteCell, kPaletteCell - 2, kPaletteCell - 2};
        Button& button = plantPalette_->add<Button>(cell, "", [this, index] {
            plant_ = index;
            tool_ = EditorTool::Plant; // choosing a plant (or an object) means placing it
        });
        button.hint = kinds[static_cast<std::size_t>(index)];
    }
}

const PlacedLight* Editor::findLight(int id) const {
    for (const PlacedLight& light : level_.lights) {
        if (light.id == id) return &light;
    }
    return nullptr;
}

void Editor::changeLights(const std::string& what, std::vector<PlacedLight> after, int nextIdAfter) {
    run(std::make_unique<LightsCommand>(what, level_.lights, std::move(after), level_.nextId, nextIdAfter));
}

// A placed light is picked by a box of 16 x 16 around its spot.
std::optional<int> Editor::lightAt(int screenX, int screenY) const {
    if (screenX < 0 || screenY < 0) return std::nullopt;
    const auto [wx, wy] = toWorld(screenX, screenY);
    for (auto it = level_.lights.rbegin(); it != level_.lights.rend(); ++it) {
        if (wx >= it->at.x - 8 && wx < it->at.x + 8 && wy >= it->at.y - 8 && wy < it->at.y + 8) return it->id;
    }
    return std::nullopt;
}

const PlacedEffect* Editor::findEffect(int id) const {
    for (const PlacedEffect& effect : level_.effects) {
        if (effect.id == id) return &effect;
    }
    return nullptr;
}

void Editor::changeEffects(const std::string& what, std::vector<PlacedEffect> after, int nextIdAfter) {
    run(std::make_unique<EffectsCommand>(what, level_.effects, std::move(after), level_.nextId, nextIdAfter));
}

// A placed effect is picked by a box of 24 x 24 around its middle.
std::optional<int> Editor::effectAt(int screenX, int screenY) const {
    if (screenX < 0 || screenY < 0) return std::nullopt;
    const auto [wx, wy] = toWorld(screenX, screenY);
    for (auto it = level_.effects.rbegin(); it != level_.effects.rend(); ++it) {
        if (wx >= it->at.x - 12 && wx < it->at.x + 12 && wy >= it->at.y - 12 && wy < it->at.y + 12) return it->id;
    }
    return std::nullopt;
}

const PlacedPlant* Editor::findPlant(int id) const {
    for (const PlacedPlant& plant : level_.plants) {
        if (plant.id == id) return &plant;
    }
    return nullptr;
}

void Editor::changePlants(const std::string& what, std::vector<PlacedPlant> after, int nextIdAfter) {
    run(std::make_unique<PlantsCommand>(what, level_.plants, std::move(after), level_.nextId, nextIdAfter));
}

std::optional<int> Editor::plantAt(int screenX, int screenY) const {
    if (screenX < 0 || screenY < 0 || textures_.plants == nullptr) return std::nullopt;
    const auto [wx, wy] = toWorld(screenX, screenY);
    std::optional<int> best;
    int bestY = -1;
    for (const PlacedPlant& plant : level_.plants) {
        const Rect extent = plantExtent(*textures_.plants, plant.kind);
        if (wx >= plant.feet.x - extent.width / 2 && wx < plant.feet.x + extent.width / 2 && wy >= plant.feet.y - extent.height && wy <= plant.feet.y && plant.feet.y >= bestY) {
            best = plant.id; // the one drawn last (lowest on the screen) is on top
            bestY = plant.feet.y;
        }
    }
    return best;
}

void Editor::setWeaponPalette(std::vector<std::string> names) {
    weaponNames_ = std::move(names);
    weapon_ = std::clamp(weapon_, 0, std::max(0, static_cast<int>(weaponNames_.size()) - 1));
    buildPanels();
}

const PlacedPickup* Editor::findPickup(int id) const {
    for (const PlacedPickup& pickup : level_.pickups) {
        if (pickup.id == id) return &pickup;
    }
    return nullptr;
}

void Editor::changePickups(const std::string& what, std::vector<PlacedPickup> after, int nextIdAfter) {
    run(std::make_unique<PickupsCommand>(what, level_.pickups, std::move(after), level_.nextId, nextIdAfter));
}

std::optional<int> Editor::pickupAt(int screenX, int screenY) const {
    if (screenX < 0 || screenY < 0) return std::nullopt;
    const auto [wx, wy] = toWorld(screenX, screenY);
    for (auto it = level_.pickups.rbegin(); it != level_.pickups.rend(); ++it) {
        if (wx >= it->at.x - kPickupSize / 2 && wx < it->at.x + kPickupSize / 2 && wy >= it->at.y - kPickupSize / 2 && wy < it->at.y + kPickupSize / 2) {
            return it->id;
        }
    }
    return std::nullopt;
}

// Delete: the selected character or pickup goes, as one step of Undo.
void Editor::removeSelected() {
    if (!selected_) return;
    if (const PlacedPickup* gone = findPickup(*selected_)) {
        const std::string what = "remove " + gone->weapon;
        auto after = level_.pickups;
        std::erase_if(after, [this](const PlacedPickup& p) { return p.id == *selected_; });
        changePickups(what, std::move(after), level_.nextId);
        select(std::nullopt);
        return;
    }
    if (const PlacedLight* gone = findLight(*selected_)) {
        const std::string what = "remove " + gone->kind + " light";
        auto after = level_.lights;
        std::erase_if(after, [this](const PlacedLight& l) { return l.id == *selected_; });
        changeLights(what, std::move(after), level_.nextId);
        select(std::nullopt);
        return;
    }
    if (const PlacedEffect* gone = findEffect(*selected_)) {
        const std::string what = "remove " + gone->name;
        auto after = level_.effects;
        std::erase_if(after, [this](const PlacedEffect& e) { return e.id == *selected_; });
        changeEffects(what, std::move(after), level_.nextId);
        select(std::nullopt);
        return;
    }
    if (const PlacedPlant* gone = findPlant(*selected_)) {
        const std::string what = "remove " + gone->kind;
        auto after = level_.plants;
        std::erase_if(after, [this](const PlacedPlant& p) { return p.id == *selected_; });
        changePlants(what, std::move(after), level_.nextId);
        select(std::nullopt);
        return;
    }
    const PlacedCharacter* gone = find(*selected_);
    const std::string what = gone == nullptr ? "remove" : "remove " + gone->name;
    auto after = level_.characters;
    std::erase_if(after, [this](const PlacedCharacter& c) { return c.id == *selected_; });
    changeCharacters(what, std::move(after), level_.nextId);
    select(std::nullopt);
}

void Editor::buildProperties() {
    properties_ = std::make_unique<Panel>(Rect{viewWidth_ - kPropertiesWidth - 2, kToolbarHeight + 4, kPropertiesWidth, 62});
    properties_->visible = false;
    npcPanel_ = std::make_unique<Panel>(Rect{0, 0, 0, 0}); // replaced when an NPC is selected
    npcPanel_->visible = false;
    npcTrade_ = std::make_unique<Panel>(Rect{0, 0, 0, 0});
    npcTrade_->visible = false;
    propertiesFor_ = -1;
    propertiesStale_ = false;
    const PlacedCharacter* shown = selected_ ? find(*selected_) : nullptr;
    if (shown == nullptr) return;
    properties_->visible = true;
    propertiesFor_ = shown->id;
    const Rect box = properties_->bounds;
    const int left = box.x + 4;
    const int width = box.width - 8;
    // A label, not a field: the kind decides the picture, and changing it is a new character.
    properties_->add<Button>(Rect{left, box.y + 4, width, 11}, std::format("{} #{}", shown->kind, shown->id), [] {});
    properties_->add<luna::engine::TextField>(Rect{left, box.y + 18, width, 11}, "Name", shown->name, 18,
                                              [this](const std::string& name) { setSelectedName(name); });
    properties_->add<luna::engine::NumberField>(Rect{left, box.y + 32, width, 11}, "HP", shown->hp, 1, 9999, [this](int hp) { setSelectedHp(hp); });
    properties_->add<luna::engine::NumberField>(Rect{left, box.y + 46, width, 11}, "Sword", shown->swordDamage, 0, 999,
                                                [this](int damage) { setSelectedSwordDamage(damage); });
    buildNpcPanel(*shown);
}

// The NPC panel under the properties (US-268): classes (a click ticks or unticks), the starting attitude, the family, one dialogue per partner type and a tick for
// every action of the registry. What it shows is what the NPC is; what it saves is only what differs from its classes and its kind.
void Editor::buildNpcPanel(const PlacedCharacter& shown) {
    constexpr int kWidth = 196;
    constexpr int kRow = 13;
    npcPanel_ = std::make_unique<Panel>(Rect{viewWidth_ - kWidth - 2, kToolbarHeight + 70, kWidth, 230});
    npcPanel_->visible = false;
    if (classBook_ == nullptr || classBook_->kinds().find(shown.kind) == nullptr) return; // a hero or a kind without a kind file has no NPC settings
    npcPanel_->visible = true;
    const sim::rules::ResolvedNpc resolved = classBook_->resolve(shown);
    const Rect box = npcPanel_->bounds;
    const int left = box.x + 4;
    const int width = box.width - 8;
    int y = box.y + 4;
    npcPanel_->add<Button>(Rect{left, y, width, 11}, "NPC: classes, attitude, talk, actions", [] {});
    y += kRow;
    // Classes: the ids with [x] for the ones it has.
    std::vector<std::string> ids = classBook_->catalog().ids();
    std::vector<std::string> rows;
    for (const std::string& id : ids) rows.push_back(std::string(std::find(resolved.classes.begin(), resolved.classes.end(), id) != resolved.classes.end() ? "[x] " : "[ ] ") + id);
    auto& classList = npcPanel_->add<luna::engine::ListBox>(Rect{left, y, width, 5 * luna::engine::kLineHeight}, rows, [this, ids](int row) { toggleSelectedClass(ids[static_cast<std::size_t>(row)]); });
    (void)classList;
    y += 5 * luna::engine::kLineHeight + 3;
    // Attitude: a click goes to the next word.
    Button& attitude = npcPanel_->add<Button>(Rect{left, y, width, 11}, std::string("Attitude: ") + resolved.attitude + (shown.attitude.empty() ? "" : " *"), [this, current = resolved.attitude] {
        const auto& words = sim::rules::attitudeNames();
        const auto at = std::find(words.begin(), words.end(), current);
        setSelectedAttitude(words[(static_cast<std::size_t>(at - words.begin()) + 1) % words.size()]);
    });
    attitude.hint = "Click: the next attitude word. A star means it differs from the kind";
    y += kRow;
    npcPanel_->add<luna::engine::NumberField>(Rect{left, y, width, 11}, "Family", shown.family, 0, 1000000, [this](int family) { setSelectedFamily(family); });
    y += kRow;
    // Dialogue: the partner type (a click goes to the next) and the file.
    const std::vector<std::string> partners = partnerTypes();
    partnerIndex_ = std::clamp(partnerIndex_, 0, static_cast<int>(partners.size()) - 1);
    const std::string partner = partners[static_cast<std::size_t>(partnerIndex_)];
    Button& partnerButton = npcPanel_->add<Button>(Rect{left, y, width, 11}, "Talks with: " + partner, [this, count = partners.size()] {
        partnerIndex_ = (partnerIndex_ + 1) % static_cast<int>(count);
        propertiesStale_ = true;
    });
    partnerButton.hint = "Click: the next partner type (the player, an NPC class, animals, the environment)";
    y += kRow;
    const auto file = resolved.dialogues.find(partner);
    npcPanel_->add<luna::engine::TextField>(Rect{left, y, width, 11}, "Script", file == resolved.dialogues.end() ? std::string() : file->second, 40,
                                            [this, partner](const std::string& value) { setSelectedDialogue(partner, value); });
    y += kRow;
    // Actions: a tick for every interaction of the registry; unticked means denied for this NPC.
    std::vector<std::string> actions;
    for (const std::string& id : actionIds_) actions.push_back(std::string(resolved.denied(id) ? "[ ] " : "[x] ") + id);
    const int listHeight = std::max(3 * luna::engine::kLineHeight, box.y + box.height - y - 20);
    npcPanel_->add<luna::engine::ListBox>(Rect{left, y, width, listHeight}, actions, [this, ids = actionIds_, resolved](int row) {
        const std::string& id = ids[static_cast<std::size_t>(row)];
        setSelectedActionDenied(id, !resolved.denied(id));
    });
    y += listHeight + 3;
    Button& reset = npcPanel_->add<Button>(Rect{left, y, width, 11}, "Reset to defaults", [this] { resetSelectedNpc(); });
    reset.hint = "Forget everything this NPC sets itself: it is what its kind says again";
    buildNpcTradePanel(shown);
}

// The Trade section (US-284) of the NPC panel: what this NPC sells, below the NPC panel. It shows the NPC's own values; the hint of the title says what it has in all.
void Editor::buildNpcTradePanel(const PlacedCharacter& shown) {
    constexpr int kWidth = 232;
    const Rect anchor = npcPanel_->bounds;
    npcTrade_ = std::make_unique<Panel>(Rect{viewWidth_ - kWidth - 2, anchor.y + anchor.height + 4, kWidth, 176}); // below the NPC panel: beside it would cover the figure being edited
    npcTrade_->visible = npcPanel_->visible;
    const Rect box = npcTrade_->bounds;
    const int left = box.x + 4;
    const int width = box.width - 8;
    int y = box.y + 4;
    const sim::rules::ResolvedNpc resolved = classBook_->resolve(shown);
    Button& title = npcTrade_->add<Button>(Rect{left, y, width, 11}, resolved.extras.trade.empty() ? "Trade: none (type stock to start)" : "Trade: this NPC trades", [] {});
    title.hint = "In all (class, kind and own): stock " + sim::rules::tradeFieldText(resolved.extras.trade, "stock") + "; wants " + sim::rules::tradeFieldText(resolved.extras.trade, "wants");
    y += 13;
    addTradeRows(*npcTrade_, left, width, y, shown.extras.trade, [this](const std::string& field, const std::string& text) { return setSelectedTrade(field, text); });
    addScheduleRows(*npcTrade_, left, width, y, shown.extras.schedule, [this](const std::string& field, const std::string& text) { return setSelectedSchedule(field, text); });
    addDoesRow(*npcTrade_, left, width, y, shown.extras.does, [this](const std::string& text) { return setSelectedDoes(text); });
    addPartnerRows(*npcTrade_, left, width, y, shown.extras, [this](const std::string& type, const std::string& text) { return setSelectedPartnerActions(type, text); }, [this] { propertiesStale_ = true; });
}

void Editor::addPartnerRows(Panel& panel, int left, int width, int& y, const sim::rules::NpcExtras& shown, const std::function<bool(const std::string&, const std::string&)>& set,
                            const std::function<void()>& refresh) {
    std::vector<std::string> types = partnerTypes();
    types.push_back("class"); // the default for every NPC class (defaults-class.json)
    defaultsIndex_ = std::clamp(defaultsIndex_, 0, static_cast<int>(types.size()) - 1);
    const std::string type = types[static_cast<std::size_t>(defaultsIndex_)];
    Button& button = panel.add<Button>(Rect{left, y, width, 11}, "Defaults with: " + type, [this, count = types.size(), refresh] {
        defaultsIndex_ = (defaultsIndex_ + 1) % static_cast<int>(count);
        refresh();
    });
    button.hint = "Click: the next partner type (the player, animals, the environment, an NPC class, or a type you added to partner-types.json)";
    y += 13;
    panel.add<luna::engine::TextField>(Rect{left, y, width, 11}, "  Does", sim::rules::partnerActionsText(shown, type), 90, [set, type](const std::string& text) { set(type, text); });
    y += 14;
}

bool Editor::setSelectedPartnerActions(const std::string& partnerType, const std::string& text) {
    if (classBook_ == nullptr || !selected_ || find(*selected_) == nullptr) return false;
    sim::rules::NpcExtras own = find(*selected_)->extras;
    std::string problem;
    if (!sim::rules::setPartnerActions(own, partnerType, text, problem)) {
        say("defaults with " + partnerType + ": " + problem);
        propertiesStale_ = true;
        return false;
    }
    changeSelectedNpc("defaults with " + partnerType, [&](PlacedCharacter& placed) { placed.extras.partnerActions = own.partnerActions; });
    return true;
}

bool Editor::setClassPartnerActions(const std::string& partnerType, const std::string& text) {
    std::string problem;
    if (!sim::rules::setPartnerActions(classDraft_.extras, partnerType, text, problem)) {
        say("defaults with " + partnerType + ": " + problem);
        classesStale_ = true;
        return false;
    }
    return true;
}

bool Editor::setKindPartnerActions(const std::string& partnerType, const std::string& text) {
    std::string problem;
    if (!sim::rules::setPartnerActions(kindDraft_.layer.extras, partnerType, text, problem)) {
        say("defaults with " + partnerType + ": " + problem);
        classesStale_ = true;
        return false;
    }
    return true;
}

// The Does line: the actions an NPC does on its own, interaction ids separated by spaces.
void Editor::addDoesRow(Panel& panel, int left, int width, int& y, const std::vector<std::string>& shown, const std::function<bool(const std::string&)>& set) {
    panel.add<luna::engine::TextField>(Rect{left, y, width, 11}, "Does", sim::rules::doesText(shown), 90, [set](const std::string& text) { set(text); });
    y += 14;
}

bool Editor::setSelectedDoes(const std::string& text) {
    if (classBook_ == nullptr || !selected_ || find(*selected_) == nullptr) return false;
    std::vector<std::string> own = find(*selected_)->extras.does;
    std::string problem;
    if (!sim::rules::setDoes(own, text, problem)) {
        say("does: " + problem);
        propertiesStale_ = true;
        return false;
    }
    changeSelectedNpc("does", [&](PlacedCharacter& placed) { placed.extras.does = own; });
    return true;
}

bool Editor::setClassDoes(const std::string& text) {
    std::string problem;
    if (!sim::rules::setDoes(classDraft_.extras.does, text, problem)) {
        say("does: " + problem);
        classesStale_ = true;
        return false;
    }
    return true;
}

bool Editor::setKindDoes(const std::string& text) {
    std::string problem;
    if (!sim::rules::setDoes(kindDraft_.layer.extras.does, text, problem)) {
        say("does: " + problem);
        classesStale_ = true;
        return false;
    }
    return true;
}

// The two lines of the schedule form: Day and Night, "06:00 work market; 21:00 sleep home".
void Editor::addScheduleRows(Panel& panel, int left, int width, int& y, const sim::rules::Schedule& shown, const std::function<bool(const std::string&, const std::string&)>& set) {
    static const char* const kLabels[] = {"Day", "Night"};
    const std::vector<std::string>& fields = sim::rules::scheduleFieldNames();
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const std::string field = fields[i];
        panel.add<luna::engine::TextField>(Rect{left, y, width, 11}, kLabels[i], sim::rules::scheduleFieldText(shown, field), 150, [set, field](const std::string& text) { set(field, text); });
        y += 14;
    }
}

bool Editor::setSelectedSchedule(const std::string& field, const std::string& text) {
    if (classBook_ == nullptr || !selected_ || find(*selected_) == nullptr) return false;
    sim::rules::Schedule own = find(*selected_)->extras.schedule;
    std::string problem;
    if (!sim::rules::setScheduleField(own, field, text, problem)) {
        say("schedule " + field + ": " + problem);
        propertiesStale_ = true;
        return false;
    }
    changeSelectedNpc("schedule " + field, [&](PlacedCharacter& placed) { placed.extras.schedule = own; });
    return true;
}

bool Editor::setClassSchedule(const std::string& field, const std::string& text) {
    std::string problem;
    if (!sim::rules::setScheduleField(classDraft_.extras.schedule, field, text, problem)) {
        say("schedule " + field + ": " + problem);
        classesStale_ = true;
        return false;
    }
    return true;
}

bool Editor::setKindSchedule(const std::string& field, const std::string& text) {
    std::string problem;
    if (!sim::rules::setScheduleField(kindDraft_.layer.extras.schedule, field, text, problem)) {
        say("schedule " + field + ": " + problem);
        classesStale_ = true;
        return false;
    }
    return true;
}

// One text field for each of the six trade fields; `set` is told the field and the text the owner typed.
void Editor::addTradeRows(Panel& panel, int left, int width, int& y, const sim::rules::TradeProfile& shown, const std::function<bool(const std::string&, const std::string&)>& set) {
    // The labels say what each line is: stock and restock are item=number (flint=6), picks a number, weights item=number, wants a list of items, rare item=band (obsidian=friendly).
    static const char* const kLabels[] = {"Stock", "Restock/day", "Picks/day", "Weights", "Wants", "Rare"};
    const std::vector<std::string>& fields = sim::rules::tradeFieldNames();
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const std::string field = fields[i];
        panel.add<luna::engine::TextField>(Rect{left, y, width, 11}, kLabels[i], sim::rules::tradeFieldText(shown, field), 90, [set, field](const std::string& text) { set(field, text); });
        y += 14;
    }
}

bool Editor::setSelectedTrade(const std::string& field, const std::string& text) {
    if (classBook_ == nullptr || !selected_ || find(*selected_) == nullptr) return false;
    sim::rules::TradeProfile own = find(*selected_)->extras.trade;
    std::string problem;
    if (!sim::rules::setTradeField(own, field, text, problem)) {
        say("trade " + field + ": " + problem);
        propertiesStale_ = true; // show the values again
        return false;
    }
    changeSelectedNpc("trade " + field, [&](PlacedCharacter& placed) { placed.extras.trade = own; });
    return true;
}

bool Editor::setClassTrade(const std::string& field, const std::string& text) {
    std::string problem;
    if (!sim::rules::setTradeField(classDraft_.extras.trade, field, text, problem)) {
        say("trade " + field + ": " + problem);
        classesStale_ = true;
        return false;
    }
    return true;
}

bool Editor::setKindTrade(const std::string& field, const std::string& text) {
    std::string problem;
    if (!sim::rules::setTradeField(kindDraft_.layer.extras.trade, field, text, problem)) {
        say("trade " + field + ": " + problem);
        classesStale_ = true;
        return false;
    }
    return true;
}

PlacedCharacter* Editor::find(int id) {
    for (PlacedCharacter& placed : level_.characters) {
        if (placed.id == id) return &placed;
    }
    return nullptr;
}

void Editor::select(std::optional<int> id) {
    if (id && find(*id) == nullptr && findPickup(*id) == nullptr && findPlant(*id) == nullptr && findEffect(*id) == nullptr && findLight(*id) == nullptr) id.reset();
    if (id == selected_ && !propertiesStale_) return;
    selected_ = id;
    buildProperties();
}

void Editor::changeCharacters(const std::string& what, std::vector<PlacedCharacter> after, int nextIdAfter) {
    run(std::make_unique<CharactersCommand>(what, level_.characters, std::move(after), level_.nextId, nextIdAfter));
    propertiesStale_ = true;
}

void Editor::setSelectedName(const std::string& name) {
    if (!selected_ || find(*selected_) == nullptr || name.empty()) return;
    auto after = level_.characters;
    for (PlacedCharacter& placed : after) {
        if (placed.id == *selected_) placed.name = name;
    }
    changeCharacters("rename to " + name, std::move(after), level_.nextId);
}

void Editor::setSelectedHp(int hp) {
    if (!selected_ || find(*selected_) == nullptr) return;
    auto after = level_.characters;
    for (PlacedCharacter& placed : after) {
        if (placed.id == *selected_) placed.hp = std::clamp(hp, 1, 9999);
    }
    changeCharacters(std::format("HP {}", hp), std::move(after), level_.nextId);
}

// ---- the NPC panel (US-268)

bool Editor::selectedIsNpc() const {
    if (classBook_ == nullptr || !selected_) return false;
    for (const PlacedCharacter& placed : level_.characters) {
        if (placed.id == *selected_) return classBook_->kinds().find(placed.kind) != nullptr;
    }
    return false;
}

void Editor::changeSelectedNpc(const std::string& what, const std::function<void(PlacedCharacter&)>& change) {
    if (!selected_ || find(*selected_) == nullptr) return;
    auto after = level_.characters;
    for (PlacedCharacter& placed : after) {
        if (placed.id == *selected_) change(placed);
    }
    if (after == level_.characters) return; // nothing changed: no step of Undo
    changeCharacters(what, std::move(after), level_.nextId);
}

std::vector<std::string> Editor::partnerTypes() const {
    std::vector<std::string> types = sim::rules::partnerTypeNames(); // player, animal, environment and the types the owner added to partner-types.json (US-293)
    if (classBook_ != nullptr) {
        for (const std::string& id : classBook_->catalog().ids()) types.push_back("class:" + id);
    }
    return types;
}

void Editor::setSelectedClasses(std::vector<std::string> classes) {
    if (classBook_ == nullptr) return;
    changeSelectedNpc("set classes", [&](PlacedCharacter& placed) {
        const sim::rules::NpcKind* kind = classBook_->kinds().find(placed.kind);
        // Equal to what the kind says: not kept (the NPC inherits it, and the kind file can change it later).
        const std::vector<std::string> inherited = kind != nullptr && kind->layer.classes ? *kind->layer.classes : std::vector<std::string>{};
        if (classes == inherited) placed.classes.clear();
        else placed.classes = classes;
    });
}

void Editor::toggleSelectedClass(const std::string& id) {
    if (classBook_ == nullptr || !selected_ || find(*selected_) == nullptr) return;
    std::vector<std::string> classes = classBook_->resolve(*find(*selected_)).classes;
    const auto at = std::find(classes.begin(), classes.end(), id);
    if (at != classes.end()) classes.erase(at);
    else classes.push_back(id);
    setSelectedClasses(std::move(classes));
}

void Editor::setSelectedAttitude(const std::string& word) {
    if (classBook_ == nullptr) return;
    changeSelectedNpc("attitude " + (word.empty() ? std::string("default") : word), [&](PlacedCharacter& placed) {
        const sim::rules::NpcKind* kind = classBook_->kinds().find(placed.kind);
        const std::string inherited = kind != nullptr && kind->layer.attitude ? *kind->layer.attitude : std::string("neutral");
        placed.attitude = word == inherited ? std::string() : word;
    });
}

void Editor::setSelectedFamily(int family) {
    changeSelectedNpc(std::format("family {}", family), [&](PlacedCharacter& placed) { placed.family = std::max(0, family); });
}

void Editor::setSelectedDialogue(const std::string& partner, const std::string& file) {
    if (classBook_ == nullptr) return;
    changeSelectedNpc(std::format("dialogue {} {}", partner, file), [&](PlacedCharacter& placed) {
        // What the partner type would be without the NPC's own entry: the classes and the kind.
        PlacedCharacter bare = placed;
        std::erase_if(bare.dialogues, [&](const auto& entry) { return entry.first == partner; });
        const auto inherited = classBook_->resolve(bare).dialogues;
        const auto found = inherited.find(partner);
        std::erase_if(placed.dialogues, [&](const auto& entry) { return entry.first == partner; });
        if (file.empty() || (found != inherited.end() && found->second == file)) return; // the default: nothing is kept
        placed.dialogues.emplace_back(partner, file);
    });
}

void Editor::setSelectedActionDenied(const std::string& id, bool denied) {
    if (classBook_ == nullptr) return;
    changeSelectedNpc(std::format("{} {}", denied ? "deny" : "allow", id), [&](PlacedCharacter& placed) {
        std::erase(placed.allow, id);
        std::erase(placed.deny, id);
        // What the classes and the kind say about it (with the NPC's own list of classes).
        PlacedCharacter bare = placed;
        bare.allow.clear();
        bare.deny.clear();
        const sim::rules::ActionState inherited = classBook_->resolve(bare).action(id);
        if (denied && inherited != sim::rules::ActionState::Denied) placed.deny.push_back(id);
        if (!denied && inherited == sim::rules::ActionState::Denied) placed.allow.push_back(id);
    });
}

void Editor::resetSelectedNpc() {
    changeSelectedNpc("reset to defaults", [](PlacedCharacter& placed) {
        placed.classes.clear();
        placed.attitude.clear();
        placed.tags.clear();
        placed.dialogues.clear();
        placed.allow.clear();
        placed.deny.clear();
        placed.family = 0;
    });
}

void Editor::setSelectedSwordDamage(int damage) {
    if (!selected_ || find(*selected_) == nullptr) return;
    auto after = level_.characters;
    for (PlacedCharacter& placed : after) {
        if (placed.id == *selected_) placed.swordDamage = std::clamp(damage, 0, 999);
    }
    changeCharacters(std::format("sword damage {}", damage), std::move(after), level_.nextId);
}

std::pair<int, int> Editor::toWorld(int screenX, int screenY) const {
    const Rect view = camera_.view();
    return {view.x + screenX, view.y + screenY};
}

// --- Level settings, and other levels (US-126) ---

void Editor::changeLevel(const std::string& what, Level after) {
    run(std::make_unique<LevelCommand>(what, level_, std::move(after)));
    settingsStale_ = true;
    economyStale_ = true;
    select(selected_); // the selected character may be gone after a resize
}

void Editor::setLevelName(const std::string& name) {
    if (name.empty() || name == level_.name) return;
    Level after = level_;
    after.name = name;
    changeLevel("name: " + name, std::move(after));
}

void Editor::setLevelSize(int width, int height) {
    width = std::clamp(width, kLevelMinSize, kLevelMaxSize);
    height = std::clamp(height, kLevelMinSize, kLevelMaxSize);
    if (width == level_.width && height == level_.height) return;
    changeLevel(std::format("size {}x{}", width, height), resized(level_, width, height));
}

void Editor::setDefaultGround(int tile) {
    if (tile < 0 || tile >= static_cast<int>(definitions_.tiles.size()) || tile == level_.defaultGround) return;
    Level after = level_;
    after.defaultGround = tile;
    changeLevel("default ground: " + definitions_.tiles[static_cast<std::size_t>(tile)].name, std::move(after));
}

void Editor::moveHeroStart(PixelPoint feet) {
    feet = {std::clamp(feet.x, 0, level_.width * kTileSize - 1), std::clamp(feet.y, 0, level_.height * kTileSize - 1)};
    if (feet == level_.heroStart) return;
    Level after = level_;
    after.heroStart = feet;
    changeLevel(std::format("hero start ({}, {})", feet.x, feet.y), std::move(after));
}

void Editor::showSettings(bool shown) {
    settingsShown_ = shown;
    settingsStale_ = true;
}

void Editor::buildSettings() {
    settingsStale_ = false;
    settings_ = std::make_unique<Panel>(Rect{viewWidth_ - 152, kToolbarHeight + 4, 150, 154});
    settings_->visible = settingsShown_;
    const Rect box = settings_->bounds;
    const int left = box.x + 4;
    const int width = box.width - 8;
    settings_->add<Button>(Rect{left, box.y + 4, width, 11}, "Level settings", [] {});
    settings_->add<luna::engine::TextField>(Rect{left, box.y + 18, width, 11}, "Name", level_.name, 20,
                                            [this](const std::string& name) { setLevelName(name); });
    settings_->add<luna::engine::NumberField>(Rect{left, box.y + 32, width, 11}, "Width", level_.width, kLevelMinSize, kLevelMaxSize,
                                              [this](int width) { setLevelSize(width, level_.height); });
    settings_->add<luna::engine::NumberField>(Rect{left, box.y + 46, width, 11}, "Height", level_.height, kLevelMinSize, kLevelMaxSize,
                                              [this](int height) { setLevelSize(level_.width, height); });
    std::vector<std::string> grounds;
    for (const TileKindDef& kind : definitions_.tiles) grounds.push_back(kind.name);
    auto& ground = settings_->add<luna::engine::ListBox>(Rect{left, box.y + 62, width, 45}, grounds, [this](int tile) { setDefaultGround(tile); });
    ground.selected = level_.defaultGround;
    ground.first = std::clamp(level_.defaultGround - 2, 0, std::max(0, static_cast<int>(grounds.size()) - ground.rows()));
    Button& fresh = settings_->add<Button>(Rect{left, box.y + 112, 44, 14}, "New", [this] { requestNew(); });
    fresh.hint = "Start a new level (asks first if this one has unsaved changes)";
    Button& open = settings_->add<Button>(Rect{left + 48, box.y + 112, 44, 14}, "Open", [this] {
        buildOpenList();
        openList_->visible = true;
    });
    open.hint = "Open another level of this folder";
    settings_->add<Button>(Rect{left + 96, box.y + 112, 42, 14}, "Close", [this] { showSettings(false); });
    Button& economy = settings_->add<Button>(Rect{left, box.y + 130, width, 14}, economyShown_ ? "Economy: shown" : "Economy...", [this] { showEconomy(!economyShown_); });
    economy.hint = "Currencies, market prices and delivery weights of this region";
}

void Editor::showEconomy(bool shown) {
    economyShown_ = shown;
    economyStale_ = true;
    settingsStale_ = true; // the button of the Level panel shows whether the Economy panel is open
}

// One table of the economy from the text of its field. A mistake is said and nothing changes.
bool Editor::changeEconomy(const std::string& what, const std::string& text, int minimum, int maximum, sim::ItemCounts sim::RegionEconomy::*table) {
    std::string problem;
    const std::optional<sim::ItemCounts> parsed = sim::parsePairs(text, minimum, maximum, &problem);
    if (!parsed) {
        say(what + ": " + problem);
        economyStale_ = true; // show the values again
        return false;
    }
    if (level_.economy.*table == *parsed) return true; // no change: no step of Undo
    Level after = level_;
    after.economy.*table = *parsed;
    changeLevel(what + ": " + (parsed->empty() ? std::string("none") : sim::formatPairs(*parsed)), std::move(after));
    return true;
}

bool Editor::setEconomyCurrencies(const std::string& text) { return changeEconomy("currencies", text, 1, sim::RegionEconomy::kMaxValue, &sim::RegionEconomy::currencies); }
bool Editor::setEconomyPrices(const std::string& text) { return changeEconomy("prices", text, 1, sim::RegionEconomy::kMaxValue, &sim::RegionEconomy::prices); }
bool Editor::setEconomyResources(const std::string& text) { return changeEconomy("resources", text, 1, 1000, &sim::RegionEconomy::resources); }

bool Editor::setPlaces(const std::string& text) {
    std::string problem;
    const std::optional<std::vector<PlacedPlace>> parsed = parsePlacesText(text, level_, problem);
    if (!parsed) {
        say("places: " + problem);
        economyStale_ = true; // show the values again
        return false;
    }
    if (level_.places == *parsed) return true;
    Level after = level_;
    after.places = *parsed;
    changeLevel("places: " + (parsed->empty() ? std::string("none") : placesText(*parsed)), std::move(after));
    return true;
}

void Editor::buildEconomy() {
    economyStale_ = false;
    economy_ = std::make_unique<Panel>(Rect{viewWidth_ - 152 - 262, kToolbarHeight + 4, 260, 104});
    economy_->visible = economyShown_;
    const Rect box = economy_->bounds;
    const int left = box.x + 4;
    const int width = box.width - 8;
    economy_->add<Button>(Rect{left, box.y + 4, width, 11}, "Region: economy (item=number) and places", [] {});
    luna::engine::TextField& currencies = economy_->add<luna::engine::TextField>(Rect{left, box.y + 18, width, 11}, "Money", sim::formatPairs(level_.economy.currencies), 90,
                                                                                 [this](const std::string& text) { setEconomyCurrencies(text); });
    currencies.label = "Money";
    economy_->add<luna::engine::TextField>(Rect{left, box.y + 32, width, 11}, "Prices", sim::formatPairs(level_.economy.prices), 90, [this](const std::string& text) { setEconomyPrices(text); });
    economy_->add<luna::engine::TextField>(Rect{left, box.y + 46, width, 11}, "Goods", sim::formatPairs(level_.economy.resources), 90, [this](const std::string& text) { setEconomyResources(text); });
    economy_->add<luna::engine::TextField>(Rect{left, box.y + 60, width, 11}, "Places", placesText(level_.places), 120, [this](const std::string& text) { setPlaces(text); });
    economy_->add<Button>(Rect{left, box.y + 78, 52, 14}, "Close", [this] { showEconomy(false); });
}

std::vector<std::filesystem::path> Editor::levelFiles() const {
    std::vector<std::filesystem::path> files;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(levelFile_.parent_path(), error)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end()); // the same order every time
    return files;
}

void Editor::buildOpenList() {
    const auto files = levelFiles();
    std::vector<std::string> names;
    for (const auto& file : files) names.push_back(file.filename().string());
    openList_ = std::make_unique<Panel>(Rect{viewWidth_ / 2 - 80, 60, 160, 118});
    openList_->visible = false;
    openList_->add<Button>(Rect{openList_->bounds.x + 4, 64, 152, 11}, "Open a level", [] {});
    openList_->add<luna::engine::ListBox>(Rect{openList_->bounds.x + 4, 78, 152, 72}, names, [this, files](int row) {
        openList_->visible = false;
        requestOpen(files[static_cast<std::size_t>(row)]);
    });
    openList_->add<Button>(Rect{openList_->bounds.x + 4, 158, 152, 14}, "Cancel", [this] { openList_->visible = false; });
}

void Editor::buildQuestion() {
    question_ = std::make_unique<Panel>(Rect{viewWidth_ / 2 - 90, viewHeight_ / 2 - 26, 180, 52});
    question_->visible = pending_.has_value();
    const Rect box = question_->bounds;
    question_->add<Button>(Rect{box.x + 4, box.y + 4, box.width - 8, 11}, "Save the changes first?", [] {});
    question_->add<Button>(Rect{box.x + 4, box.y + 22, 54, 14}, "Save", [this] { answer(Answer::Save); });
    question_->add<Button>(Rect{box.x + 62, box.y + 22, 54, 14}, "Discard", [this] { answer(Answer::Discard); });
    question_->add<Button>(Rect{box.x + 120, box.y + 22, 56, 14}, "Cancel", [this] { answer(Answer::Cancel); });
}

void Editor::requestOpen(const std::filesystem::path& file) {
    pending_ = file;
    if (unsaved_) {
        buildQuestion(); // ask before anything is lost
        return;
    }
    doPending();
}

void Editor::requestNew() {
    requestOpen({});
}

void Editor::answer(Answer answer) {
    if (!pending_) return;
    if (answer == Answer::Cancel || (answer == Answer::Save && !save())) {
        pending_.reset();
    } else {
        doPending();
    }
    // The question panel hides itself next tick (it must not be rebuilt while its button runs).
}

void Editor::doPending() {
    const std::filesystem::path file = *pending_;
    pending_.reset();
    if (file.empty()) {
        // A new level: 32 x 32 of the current ground, in the first free "level-N.json".
        std::filesystem::path fresh;
        for (int n = 1; fresh.empty() || std::filesystem::exists(fresh); ++n) {
            fresh = levelFile_.parent_path() / std::format("level-{}.json", n);
        }
        Level level = makeLevel("New level", 32, 32, level_.defaultGround);
        replaceLevel(std::move(level), fresh, "new level " + fresh.filename().string());
        save(); // the file exists from the start, so it can be opened again
        return;
    }
    try {
        LoadedLevel loaded = loadLevel(file, definitions_);
        replaceLevel(std::move(loaded.level), file, "opened " + file.filename().string());
    } catch (const sim::DataError& error) {
        say(std::string("Not opened: ") + error.what());
    }
}

void Editor::replaceLevel(Level level, std::filesystem::path file, const std::string& what) {
    level_ = std::move(level);
    levelFile_ = std::move(file);
    history_.clear(); // Undo never crosses into another level
    unsaved_ = false;
    selected_.reset();
    buildProperties();
    settingsStale_ = true;
    economyStale_ = true;
    centreX_ = level_.heroStart.x;
    centreY_ = level_.heroStart.y;
    levelChanged();
    say(what);
}

std::optional<int> Editor::characterAt(int screenX, int screenY) const {
    if (screenX < 0 || screenY < 0) return std::nullopt;
    const auto [wx, wy] = toWorld(screenX, screenY);
    // The last one drawn is on top, so it is the one the owner sees and means.
    for (auto it = level_.characters.rbegin(); it != level_.characters.rend(); ++it) {
        const CharacterKindDef* kind = definitions_.character(it->kind);
        const int half = (kind != nullptr && kind->animal ? kAnimalWidth : kCharacterWidth) / 2;
        if (wx >= it->feet.x - half && wx < it->feet.x + half && wy >= it->feet.y - kCharacterHeight && wy <= it->feet.y) {
            return it->id;
        }
    }
    return std::nullopt;
}

void Editor::levelChanged() {
    map_ = buildTileMap(level_, definitions_);
    camera_ = luna::engine::Camera(viewWidth_, viewHeight_, map_.pixelWidth(), map_.pixelHeight());
    panTo(centreX_, centreY_);
    camera_.centreOn(centreX_, centreY_);
}

void Editor::enter(double centreX, double centreY) {
    centreX_ = centreX;
    centreY_ = centreY;
    levelChanged();
    tile_ = std::clamp(tile_, 0, static_cast<int>(definitions_.tiles.size()) - 1);
}

void Editor::panTo(double x, double y) {
    // The centre stays where a camera can look, so panning back responds at once.
    const double halfW = viewWidth_ / 2.0;
    const double halfH = viewHeight_ / 2.0;
    const double worldW = map_.pixelWidth();
    const double worldH = map_.pixelHeight();
    centreX_ = worldW <= viewWidth_ ? worldW / 2.0 : std::clamp(x, halfW, worldW - halfW);
    centreY_ = worldH <= viewHeight_ ? worldH / 2.0 : std::clamp(y, halfH, worldH - halfH);
}

void Editor::say(std::string message) {
    core::logInfo("Editor: " + message);
    status_ = std::move(message);
    statusTicks_ = kStatusTicks;
}

void Editor::run(std::unique_ptr<Command> command) {
    const std::string what = command->name();
    history_.run(std::move(command), level_);
    unsaved_ = true;
    levelChanged();
    say(what);
}

bool Editor::undo() {
    finishStroke();
    if (!history_.undo(level_)) {
        say("Nothing to undo");
        return false;
    }
    unsaved_ = true;
    levelChanged();
    propertiesStale_ = true; // the selected character may have changed, or be gone
    select(selected_);
    say("Undone");
    return true;
}

bool Editor::redo() {
    if (!history_.redo(level_)) {
        say("Nothing to redo");
        return false;
    }
    unsaved_ = true;
    levelChanged();
    propertiesStale_ = true;
    select(selected_);
    say("Redone");
    return true;
}

bool Editor::save() {
    finishStroke();
    try {
        saveLevel(level_, definitions_, levelFile_);
    } catch (const sim::DataError& error) {
        say(std::string("Not saved: ") + error.what());
        return false;
    }
    unsaved_ = false;
    say("Saved " + levelFile_.filename().string());
    return true;
}

std::optional<std::pair<int, int>> Editor::cellAt(int screenX, int screenY) const {
    if (screenX < 0 || screenY < 0) return std::nullopt;
    const Rect view = camera_.view();
    const int wx = view.x + screenX;
    const int wy = view.y + screenY;
    if (wx < 0 || wy < 0) return std::nullopt;
    const int cx = wx / kTileSize;
    const int cy = wy / kTileSize;
    if (!level_.inside(cx, cy)) return std::nullopt;
    return std::pair{cx, cy};
}

void Editor::paintAt(int x, int y) {
    if (strokeCells_.contains({x, y})) return;
    const int paint = tool_ == EditorTool::Eraser ? level_.defaultGround : tile_;
    strokeCells_.insert({x, y});
    if (level_.at(x, y) == paint) return;
    stroke_.push_back({x, y, level_.at(x, y), paint});
    level_.set(x, y, paint); // painted at once, so the owner sees the stroke as it grows
    map_.set(x, y, paint);
}

void Editor::finishStroke() {
    if (!stroking_) return;
    stroking_ = false;
    strokeCells_.clear();
    if (stroke_.empty()) return;
    auto command = std::make_unique<PaintCommand>(tool_ == EditorTool::Eraser ? "erase" : "paint " + definitions_.tiles[static_cast<std::size_t>(tile_)].name,
                                                  std::move(stroke_));
    stroke_.clear();
    say(command->name());
    history_.record(std::move(command)); // already on the level
    unsaved_ = true;
}

bool Editor::handlePanels(const luna::engine::UiInput& input) {
    if (plantPageWanted_ != plantPage_) buildPlantPalette(); // a page arrow was pressed last tick
    if (kindPageWanted_ != kindPage_) buildCharacterPalette();
    for (auto& button : toolbar_->children()) {
        auto* b = dynamic_cast<Button*>(button.get());
        if (b == nullptr) continue;
        b->selected = (b->label == "Brush" && tool_ == EditorTool::Brush) || (b->label == "Rect" && tool_ == EditorTool::Rectangle) ||
                      (b->label == "Fill" && tool_ == EditorTool::Fill) || (b->label == "Erase" && tool_ == EditorTool::Eraser) ||
                      (b->label == "Place" && tool_ == EditorTool::Place) || (b->label == "Select" && tool_ == EditorTool::Select) || (b->label == "Arms" && tool_ == EditorTool::Weapon) || (b->label == "Plant" && tool_ == EditorTool::Plant) || (b->label == "Level" && settingsShown_) || (b->label == "Class" && classesShown_) ||
                      (b->label == "#" && grid_) || (b->label == "Fx" && tool_ == EditorTool::Effect) || (b->label == "Light" && tool_ == EditorTool::Light) ||
                      (b->label == "Sky" && previewHour_.has_value()) || (b->label == "Build" && tool_ == EditorTool::Building) || (b->label == "Prefab" && buildingEditor_->prefabsShown());
    }
    for (std::size_t i = 0; i < palette_->children().size(); ++i) {
        if (auto* b = dynamic_cast<Button*>(palette_->children()[i].get())) b->selected = static_cast<int>(i) == tile_;
    }
    for (std::size_t i = 0; i + 2 < characterPalette_->children().size(); ++i) {
        if (auto* b = dynamic_cast<Button*>(characterPalette_->children()[i].get())) b->selected = kindPage_ * kKindsPerPage + static_cast<int>(i) == kind_;
    }
    for (std::size_t i = 0; i < weaponPalette_->children().size(); ++i) {
        if (auto* b = dynamic_cast<Button*>(weaponPalette_->children()[i].get())) b->selected = static_cast<int>(i) == weapon_;
    }
    for (std::size_t i = 0; i < effectPalette_->children().size(); ++i) {
        if (auto* b = dynamic_cast<Button*>(effectPalette_->children()[i].get())) b->selected = static_cast<int>(i) == effect_;
    }
    for (std::size_t i = 0; i < lightPalette_->children().size(); ++i) {
        if (auto* b = dynamic_cast<Button*>(lightPalette_->children()[i].get())) b->selected = static_cast<int>(i) == light_;
    }
    for (std::size_t i = 2; i < plantPalette_->children().size(); ++i) {
        if (auto* b = dynamic_cast<Button*>(plantPalette_->children()[i].get())) b->selected = plantPageFirst(plantPage_) + static_cast<int>(i) - 2 == plant_;
    }
    // Painting tools show the ground palette, Weapon the weapons; Place and Select the characters.
    palette_->visible = paints(tool_);
    weaponPalette_->visible = tool_ == EditorTool::Weapon;
    plantPalette_->visible = tool_ == EditorTool::Plant;
    effectPalette_->visible = tool_ == EditorTool::Effect;
    lightPalette_->visible = tool_ == EditorTool::Light;
    characterPalette_->visible = !paints(tool_) && tool_ != EditorTool::Weapon && tool_ != EditorTool::Plant && tool_ != EditorTool::Effect && tool_ != EditorTool::Light && tool_ != EditorTool::Building;
    if (propertiesStale_ && !properties_->typing() && !npcPanel_->typing() && !npcTrade_->typing()) {
        select(selected_); // show the character's values again (after an undo, or a change elsewhere)
    }
    // A question about unsaved changes, or the list of levels, takes all input until answered.
    question_->visible = pending_.has_value();
    if (question_->visible) {
        question_->handle(input);
        return true;
    }
    if (openList_->visible) {
        openList_->handle(input);
        return true;
    }
    if (settingsStale_ && !settings_->typing()) {
        buildSettings(); // show the level's values again (after an undo, a resize, another level)
    }
    if (economyStale_ && !economy_->typing()) buildEconomy();
    economy_->visible = economyShown_;
    if (classesStale_ && !classes_->typing()) buildClassPanel();
    classes_->visible = classesShown_;
    settings_->visible = settingsShown_;
    properties_->visible = propertiesFor_ >= 0 && selected_.has_value() && !settingsShown_;
    npcPanel_->visible = npcPanel_->visible && properties_->visible && !classesShown_;
    npcTrade_->visible = npcPanel_->visible;
    // The time-of-day slider (US-247): press on it and drag; the hour follows the pointer until the button is let go.
    bool onSlider = false;
    if (previewHour_) {
        const Rect slider = timeSliderRect();
        const auto& pointer = input.pointer;
        const bool inside = pointer.x >= slider.x && pointer.x < slider.x + slider.width && pointer.y >= slider.y && pointer.y < slider.y + slider.height;
        if (pointer.wasPressed(PointerButton::Left) && inside) sliderDragging_ = true;
        if (!pointer.isHeld(PointerButton::Left)) sliderDragging_ = false;
        if (sliderDragging_) previewHour_ = std::clamp(24.0 * (pointer.x - slider.x) / (slider.width - 1), 0.0, 24.0);
        onSlider = inside || sliderDragging_;
    }
    const bool onToolbar = toolbar_->handle(input) || onSlider;
    const bool onPalette = palette_->handle(input) || characterPalette_->handle(input) || weaponPalette_->handle(input) || plantPalette_->handle(input) || effectPalette_->handle(input) || lightPalette_->handle(input);
    const bool onProperties = properties_->handle(input) || npcPanel_->handle(input) || npcTrade_->handle(input);
    const bool onSettings = settings_->handle(input);
    const bool onBuildings = buildingEditor_->handle(input, tool_ == EditorTool::Building);
    const bool onClasses = classes_->handle(input);
    const bool onEconomy = economy_->handle(input);
    return onToolbar || onPalette || onProperties || onSettings || onClasses || onEconomy || onBuildings;
}

void Editor::usePlaceOrSelect(const luna::engine::Pointer& pointer, bool pressed, bool held, bool released) {
    const auto [wx, wy] = toWorld(pointer.x, pointer.y);
    auto insideLevel = [this](int x, int y) {
        return std::pair{std::clamp(x, 0, level_.width * kTileSize - 1), std::clamp(y, 0, level_.height * kTileSize - 1)};
    };
    if (tool_ == EditorTool::Place) {
        if (pressed && hover_) {
            const CharacterKindDef& kind = definitions_.characters[static_cast<std::size_t>(kind_)];
            auto after = level_.characters;
            const auto [x, y] = insideLevel(wx, wy);
            after.push_back({level_.nextId, kind.name, {x, y}, Facing::South, capitalised(kind.name), kind.hp, kind.swordDamage});
            const int id = level_.nextId;
            changeCharacters(std::format("place {} #{}", kind.name, id), std::move(after), level_.nextId + 1);
            select(id);
        }
        return;
    }
    if (tool_ == EditorTool::Building) {
        if (pressed && hover_) buildingEditor_->placeAt(map_, hover_->first, hover_->second);
        return;
    }
    if (tool_ == EditorTool::Weapon) {
        if (pressed && hover_ && !weaponNames_.empty()) {
            const std::string& weapon = weaponNames_[static_cast<std::size_t>(weapon_)];
            auto after = level_.pickups;
            const auto [x, y] = insideLevel(wx, wy);
            const int id = level_.nextId;
            after.push_back({id, weapon, {x, y}});
            changePickups(std::format("place {} #{}", weapon, id), std::move(after), level_.nextId + 1);
            select(id);
        }
        return;
    }
    if (tool_ == EditorTool::Effect) {
        if (pressed && hover_ && !definitions_.loopingEffects.empty()) {
            const std::string& name = definitions_.loopingEffects[static_cast<std::size_t>(effect_)];
            const auto [x, y] = insideLevel(wx, wy);
            auto after = level_.effects;
            const int id = level_.nextId;
            after.push_back({id, name, {x, y}});
            changeEffects(std::format("place {} #{}", name, id), std::move(after), level_.nextId + 1);
            select(id);
        }
        return;
    }
    if (tool_ == EditorTool::Light) {
        if (pressed && hover_ && !definitions_.lightKinds.empty()) {
            const std::string& kind = definitions_.lightKinds[static_cast<std::size_t>(light_)];
            const auto [x, y] = insideLevel(wx, wy);
            auto after = level_.lights;
            const int id = level_.nextId;
            after.push_back({id, kind, {x, y}});
            changeLights(std::format("place {} light #{}", kind, id), std::move(after), level_.nextId + 1);
            select(id);
        }
        return;
    }
    if (tool_ == EditorTool::Plant) {
        if (pressed && hover_ && !plantKinds().empty()) {
            const std::string kind = plantKinds()[static_cast<std::size_t>(plant_)];
            const PixelPoint feet{hover_->first * kTileSize + kTileSize / 2, hover_->second * kTileSize + kTileSize - 4};
            for (const PlacedPlant& there : level_.plants) {
                if (plantCell(there.feet) == plantCell(feet)) {
                    say(std::format("a {} already grows there", there.kind));
                    return;
                }
            }
            auto after = level_.plants;
            const int id = level_.nextId;
            after.push_back({id, kind, feet});
            changePlants(std::format("place {} #{}", kind, id), std::move(after), level_.nextId + 1);
            select(id);
        }
        return;
    }
    // Select, first the hero start marker: drag it to where the hero should begin.
    const PixelPoint start = level_.heroStart;
    const bool onStart = wx >= start.x - kCharacterWidth / 2 && wx < start.x + kCharacterWidth / 2 && wy >= start.y - kCharacterHeight && wy <= start.y;
    if (pressed && onStart) {
        select(std::nullopt);
        movingStart_ = true;
        startBefore_ = start;
        grabX_ = start.x - wx;
        grabY_ = start.y - wy;
        return;
    }
    if (movingStart_) {
        if (held && pointer.inside()) {
            const auto [x, y] = insideLevel(wx + grabX_, wy + grabY_);
            level_.heroStart = {x, y}; // follows the pointer; one step of Undo when let go
        }
        if (released || !held) {
            movingStart_ = false;
            const PixelPoint moved = level_.heroStart;
            level_.heroStart = startBefore_;
            moveHeroStart(moved);
        }
        return;
    }
    // Then characters: click one to select it and drag it to move it (one step of Undo).
    if (pressed) {
        std::optional<int> hit = characterAt(pointer.x, pointer.y);
        const bool character = hit.has_value();
        if (!hit) hit = pickupAt(pointer.x, pointer.y); // pickups lie on the ground, under the characters
        const bool pickup = hit.has_value() && !character;
        if (!hit) hit = lightAt(pointer.x, pointer.y);   // light markers hang above everything but characters and pickups
        const bool light = hit.has_value() && !character && !pickup;
        if (!hit) hit = effectAt(pointer.x, pointer.y);  // effects float above the plants
        const bool effect = hit.has_value() && !character && !pickup && !light;
        if (!hit) hit = plantAt(pointer.x, pointer.y);   // plants stand under everything
        const bool onBuilding = !hit && buildingEditor_->selectAt(wx, wy); // buildings are the biggest things: picked when nothing smaller is there
        if (!onBuilding) buildingEditor_->clearSelection();
        select(hit);
        if (hit && light) {
            const PlacedLight& grabbed = *findLight(*hit);
            movingLight_ = true;
            movingLightsBefore_ = level_.lights;
            grabX_ = grabbed.at.x - wx;
            grabY_ = grabbed.at.y - wy;
        }
        if (hit && effect) {
            const PlacedEffect& grabbed = *findEffect(*hit);
            movingEffect_ = true;
            movingEffectsBefore_ = level_.effects;
            grabX_ = grabbed.at.x - wx;
            grabY_ = grabbed.at.y - wy;
        }
        if (hit && !character && !pickup && !effect && !light) {
            const PlacedPlant& grabbed = *findPlant(*hit);
            movingPlant_ = true;
            movingPlantsBefore_ = level_.plants;
            grabX_ = grabbed.feet.x - wx;
            grabY_ = grabbed.feet.y - wy;
        }
        if (hit && pickup) {
            const PlacedPickup& grabbed = *findPickup(*hit);
            movingPickup_ = true;
            movingPickupsBefore_ = level_.pickups;
            grabX_ = grabbed.at.x - wx;
            grabY_ = grabbed.at.y - wy;
        }
        if (hit && character) {
            const PlacedCharacter& grabbed = *find(*hit);
            moving_ = true;
            movingBefore_ = level_.characters;
            grabX_ = grabbed.feet.x - wx;
            grabY_ = grabbed.feet.y - wy;
        }
    }
    if (moving_ && held && pointer.inside() && selected_) {
        if (PlacedCharacter* dragged = find(*selected_)) {
            const auto [x, y] = insideLevel(wx + grabX_, wy + grabY_);
            dragged->feet = {x, y}; // moved at once, so the owner sees it follow the pointer
        }
    }
    if (movingLight_ && held && pointer.inside() && selected_) {
        for (PlacedLight& light : level_.lights) {
            if (light.id == *selected_) {
                const auto [x, y] = insideLevel(wx + grabX_, wy + grabY_);
                light.at = {x, y};
            }
        }
    }
    if (movingLight_ && (released || !held)) {
        movingLight_ = false;
        if (level_.lights != movingLightsBefore_) {
            const PlacedLight* moved = selected_ ? findLight(*selected_) : nullptr;
            const std::string what = moved == nullptr ? "move" : std::format("move {} light to ({}, {})", moved->kind, moved->at.x, moved->at.y);
            history_.record(std::make_unique<LightsCommand>(what, movingLightsBefore_, level_.lights, level_.nextId, level_.nextId));
            unsaved_ = true;
            say(what);
        }
    }
    if (movingEffect_ && held && pointer.inside() && selected_) {
        for (PlacedEffect& effect : level_.effects) {
            if (effect.id == *selected_) {
                const auto [x, y] = insideLevel(wx + grabX_, wy + grabY_);
                effect.at = {x, y};
            }
        }
    }
    if (movingEffect_ && (released || !held)) {
        movingEffect_ = false;
        if (level_.effects != movingEffectsBefore_) {
            const PlacedEffect* moved = selected_ ? findEffect(*selected_) : nullptr;
            const std::string what = moved == nullptr ? "move" : std::format("move {} to ({}, {})", moved->name, moved->at.x, moved->at.y);
            history_.record(std::make_unique<EffectsCommand>(what, movingEffectsBefore_, level_.effects, level_.nextId, level_.nextId));
            unsaved_ = true;
            say(what);
        }
    }
    if (movingPlant_ && held && pointer.inside() && selected_) {
        for (PlacedPlant& plant : level_.plants) {
            if (plant.id == *selected_) {
                // A plant keeps to the grid: feet in the middle of the bottom edge of the cell under the pointer.
                const int cellX = std::clamp((wx + grabX_) / kTileSize, 0, level_.width - 1);
                const int cellY = std::clamp((wy + grabY_ - 1) / kTileSize, 0, level_.height - 1);
                plant.feet = {cellX * kTileSize + kTileSize / 2, cellY * kTileSize + kTileSize - 4};
            }
        }
    }
    if (movingPlant_ && (released || !held)) {
        movingPlant_ = false;
        if (level_.plants != movingPlantsBefore_) {
            const PlacedPlant* moved = selected_ ? findPlant(*selected_) : nullptr;
            const std::string what = moved == nullptr ? "move" : std::format("move {} to ({}, {})", moved->kind, moved->feet.x, moved->feet.y);
            history_.record(std::make_unique<PlantsCommand>(what, movingPlantsBefore_, level_.plants, level_.nextId, level_.nextId));
            unsaved_ = true;
            say(what);
        }
    }
    if (movingPickup_ && held && pointer.inside() && selected_) {
        for (PlacedPickup& pickup : level_.pickups) {
            if (pickup.id == *selected_) {
                const auto [x, y] = insideLevel(wx + grabX_, wy + grabY_);
                pickup.at = {x, y};
            }
        }
    }
    if (movingPickup_ && (released || !held)) {
        movingPickup_ = false;
        if (level_.pickups != movingPickupsBefore_) {
            const PlacedPickup* moved = selected_ ? findPickup(*selected_) : nullptr;
            const std::string what = moved == nullptr ? "move" : std::format("move {} to ({}, {})", moved->weapon, moved->at.x, moved->at.y);
            history_.record(std::make_unique<PickupsCommand>(what, movingPickupsBefore_, level_.pickups, level_.nextId, level_.nextId));
            unsaved_ = true;
            say(what);
        }
    }
    if (moving_ && (released || !held)) {
        moving_ = false;
        if (level_.characters != movingBefore_) {
            const PlacedCharacter* moved = selected_ ? find(*selected_) : nullptr;
            const std::string what = moved == nullptr ? "move" : std::format("move {} to ({}, {})", moved->name, moved->feet.x, moved->feet.y);
            history_.record(std::make_unique<CharactersCommand>(what, movingBefore_, level_.characters, level_.nextId, level_.nextId));
            unsaved_ = true;
            say(what);
        }
    }
}

void Editor::useTool(const luna::engine::Pointer& pointer, bool overPanel) {
    hover_ = overPanel ? std::nullopt : cellAt(pointer.x, pointer.y);
    const bool pressed = pointer.wasPressed(PointerButton::Left) && !overPanel;
    const bool held = pointer.isHeld(PointerButton::Left);
    const bool released = pointer.wasReleased(PointerButton::Left);
    if (!paints(tool_)) {
        usePlaceOrSelect(pointer, pressed, held, released);
        return;
    }

    if (tool_ == EditorTool::Brush || tool_ == EditorTool::Eraser) {
        if (pressed && hover_) {
            stroking_ = true;
            lastCell_ = *hover_;
            paintAt(hover_->first, hover_->second);
        }
        if (stroking_ && held && hover_) {
            for (const auto& [x, y] : lineCells(lastCell_.first, lastCell_.second, hover_->first, hover_->second)) {
                if (level_.inside(x, y)) paintAt(x, y);
            }
            lastCell_ = *hover_;
        }
        if (stroking_ && (released || !held)) finishStroke();
    } else if (tool_ == EditorTool::Rectangle) {
        if (pressed && hover_) {
            rectangleStart_ = *hover_;
            rectangleEnd_ = *hover_;
        }
        if (rectangleStart_ && hover_) rectangleEnd_ = *hover_;
        if (rectangleStart_ && (released || !held)) {
            auto changes = rectangleFill(level_, rectangleStart_->first, rectangleStart_->second, rectangleEnd_.first, rectangleEnd_.second, tile_);
            rectangleStart_.reset();
            if (!changes.empty()) run(std::make_unique<PaintCommand>("rectangle of " + definitions_.tiles[static_cast<std::size_t>(tile_)].name, std::move(changes)));
        }
    } else if (tool_ == EditorTool::Fill) {
        if (pressed && hover_) {
            auto changes = floodFill(level_, hover_->first, hover_->second, tile_);
            if (!changes.empty()) run(std::make_unique<PaintCommand>("fill with " + definitions_.tiles[static_cast<std::size_t>(tile_)].name, std::move(changes)));
        }
    }
}

void Editor::update(const Intents& intents) {
    if (statusTicks_ > 0 && --statusTicks_ == 0) status_.clear();
    const luna::engine::UiInput input = luna::engine::UiInput::from(intents);
    const bool overPanel = handlePanels(input);
    const bool typing = buildingEditor_->typing() || toolbar_->typing() || palette_->typing() || properties_->typing() || npcPanel_->typing() || npcTrade_->typing() || settings_->typing() || classes_->typing() || economy_->typing();
    if (!typing) {
        if (intents.pressed(Intent::Undo)) undo();
        if (intents.pressed(Intent::Redo)) redo();
        if (intents.pressed(Intent::Save)) save();
        if (intents.pressed(Intent::ToggleGrid)) grid_ = !grid_;
        if (buildingEditor_->hasSelection() && intents.pressed(Intent::Delete)) buildingEditor_->removeSelected();
        else if (selected_ && intents.pressed(Intent::Delete)) removeSelected();
        if (intents.pressed(Intent::Rotate) && buildingEditor_->hasSelection()) buildingEditor_->turnSelected();
        else if (intents.pressed(Intent::Rotate) && tool_ == EditorTool::Building) buildingEditor_->turnPlaced();
        else if (selected_ && find(*selected_) != nullptr && intents.pressed(Intent::Rotate)) {
            auto after = level_.characters;
            for (PlacedCharacter& c : after) {
                if (c.id == *selected_) c.facing = static_cast<Facing>((static_cast<int>(c.facing) + 1) % static_cast<int>(Facing::Count)); // clockwise
            }
            changeCharacters("turn", std::move(after), level_.nextId);
        }
    }

    double x = centreX_ + (typing ? 0 : intents.moveX() * kPanPerTick);
    double y = centreY_ + (typing ? 0 : intents.moveY() * kPanPerTick);
    // Right-button drag: the world moves with the pointer, like sliding a map on a table.
    const auto& pointer = intents.pointer();
    if (pointer.isHeld(PointerButton::Right) && pointer.inside()) {
        if (dragging_) {
            x -= pointer.x - dragX_;
            y -= pointer.y - dragY_;
        }
        dragging_ = true;
        dragX_ = pointer.x;
        dragY_ = pointer.y;
    } else {
        dragging_ = false;
    }
    panTo(x, y);
    camera_.follow(centreX_, centreY_, 1.0);
    useTool(pointer, overPanel);
}

void Editor::render(luna::engine::Renderer& renderer, double alpha) const {
    // The time-of-day preview (US-247): the ground and everything standing on it are lit as at that hour; the marks, panels and text are not.
    luna::engine::LightFrame preview;
    const bool previewing = previewHour_.has_value() && static_cast<bool>(lightPreview_);
    if (previewing) {
        preview = lightPreview_(*previewHour_, camera_.view(alpha));
        renderer.setLighting(&preview);
    }
    map_.draw(renderer, textures_.tiles, camera_, alpha);
    const Rect view = camera_.view(alpha);
    auto screen = [&view](int worldX, int worldY) { return luna::engine::Point{worldX - view.x, worldY - view.y}; };
    UiPainter painter(renderer, textures_.ui);
    painter.setScreen({0, 0, viewWidth_, viewHeight_});

    if (grid_) {
        for (int gx = (view.x / kTileSize) * kTileSize; gx < view.x + view.width; gx += kTileSize) {
            painter.fill({gx - view.x, 0, 1, view.height}, UiColor::Grid);
        }
        for (int gy = (view.y / kTileSize) * kTileSize; gy < view.y + view.height; gy += kTileSize) {
            painter.fill({0, gy - view.y, view.width, 1}, UiColor::Grid);
        }
    }
    buildingEditor_->drawWorld(renderer, painter, view, map_, tool_ == EditorTool::Building, hover_);
    for (const PixelPoint& target : level_.targets) {
        renderer.draw(textures_.props, kTargetFrame, screen(target.x - kTargetFrame.width / 2, target.y - kTargetFrame.height));
    }
    // Pickups lie on the ground: a shadow, then the weapon.
    for (const PlacedPickup& pickup : level_.pickups) {
        renderer.draw(textures_.props, kShadowFrame, screen(pickup.at.x - kShadowFrame.width / 2, pickup.at.y + 4));
        if (textures_.weapons != nullptr) {
            const auto corner = screen(pickup.at.x - kPickupSize / 2, pickup.at.y - kPickupSize / 2);
            drawWeaponIcon(renderer, painter, *textures_.weapons, pickup.weapon, {corner.x, corner.y, kPickupSize, kPickupSize});
        }
    }
    if (textures_.plants != nullptr) {
        std::vector<const PlacedPlant*> order;
        for (const PlacedPlant& plant : level_.plants) order.push_back(&plant);
        std::sort(order.begin(), order.end(), [](const PlacedPlant* a, const PlacedPlant* b) { return a->feet.y != b->feet.y ? a->feet.y < b->feet.y : a->id < b->id; });
        for (const PlacedPlant* plant : order) {
            const auto feet = screen(plant->feet.x, plant->feet.y);
            drawPlant(renderer, *textures_.plants, plant->kind, feet.x, feet.y);
        }
    }
    for (const PlacedCharacter& placed : level_.characters) {
        const CharacterKindDef* kind = definitions_.character(placed.kind);
        if (kind != nullptr && kind->animal && textures_.animals != nullptr) {
            const auto feet = screen(placed.feet.x, placed.feet.y);
            drawAnimal(renderer, *textures_.animals, kind->name, feet.x, feet.y, placed.facing, false);
            continue;
        }
        if (kind == nullptr || textures_.art == nullptr) continue;
        renderer.draw(textures_.characters, textures_.art->frame(kind->frames, kind->directions, placed.facing, 0),
                      screen(placed.feet.x - kCharacterWidth / 2, placed.feet.y - kCharacterHeight));
    }
    if (previewing) renderer.setLighting(nullptr);
    // Where the hero will begin: the hero, framed in gold, with a label.
    const auto start = screen(level_.heroStart.x - kCharacterWidth / 2, level_.heroStart.y - kCharacterHeight);
    renderer.draw(textures_.heroSheet, {0, 0, kCharacterWidth, kCharacterHeight}, start);
    painter.outline({start.x - 1, start.y - 1, kCharacterWidth + 2, kCharacterHeight + 2}, UiColor::Gold);
    painter.text(start.x + (kCharacterWidth - UiPainter::textWidth("START")) / 2, start.y - 9, "START", UiColor::Gold);

    // What the tool would do: the rectangle being dragged, or the cell under the pointer.
    if (rectangleStart_) {
        const int x0 = std::min(rectangleStart_->first, rectangleEnd_.first) * kTileSize;
        const int y0 = std::min(rectangleStart_->second, rectangleEnd_.second) * kTileSize;
        const int x1 = (std::max(rectangleStart_->first, rectangleEnd_.first) + 1) * kTileSize;
        const int y1 = (std::max(rectangleStart_->second, rectangleEnd_.second) + 1) * kTileSize;
        const auto corner = screen(x0, y0);
        painter.outline({corner.x, corner.y, x1 - x0, y1 - y0}, UiColor::Gold);
    } else if (hover_) {
        const auto corner = screen(hover_->first * kTileSize, hover_->second * kTileSize);
        painter.outline({corner.x, corner.y, kTileSize, kTileSize}, UiColor::Text);
    }

    // NPC markers (US-269): under every placed NPC a ring in its class colour with the class icon. Only here, never in play.
    if (classBook_ != nullptr) {
        for (const PlacedCharacter& placed : level_.characters) {
            if (classBook_->kinds().find(placed.kind) == nullptr) continue;
            const auto marker = markerFor(classBook_->resolve(placed), classBook_->catalog());
            if (!marker) continue;
            const luna::engine::Texture& picture = markerTexture(renderer, *marker);
            const auto corner = screen(placed.feet.x - kMarkerSize / 2, placed.feet.y + 1);
            renderer.draw(picture, {0, 0, kMarkerSize, kMarkerSize}, {corner.x, corner.y});
        }
    }
    // The selected character: framed in gold, with its name above.
    if (selected_) {
        for (const PlacedCharacter& placed : level_.characters) {
            if (placed.id != *selected_) continue;
            const auto corner = screen(placed.feet.x - kCharacterWidth / 2, placed.feet.y - kCharacterHeight);
            painter.outline({corner.x - 1, corner.y - 1, kCharacterWidth + 2, kCharacterHeight + 2}, UiColor::Gold);
            painter.text(corner.x + (kCharacterWidth - UiPainter::textWidth(placed.name)) / 2, corner.y - 9, placed.name, UiColor::Gold);
        }
    }

    if (selected_) {
        if (const PlacedPickup* pickup = findPickup(*selected_)) {
            const auto corner = screen(pickup->at.x - kPickupSize / 2, pickup->at.y - kPickupSize / 2);
            painter.outline({corner.x - 1, corner.y - 1, kPickupSize + 2, kPickupSize + 2}, UiColor::Gold);
            painter.text(corner.x + (kPickupSize - UiPainter::textWidth(pickup->weapon)) / 2, corner.y - 10, pickup->weapon, UiColor::Gold);
        }
    }

    if (textures_.effects != nullptr) {
        for (const PlacedEffect& placed : level_.effects) {
            const auto corner = screen(placed.at.x - 24, placed.at.y - 24);
            drawEffectPicture(renderer, *textures_.effects, placed.name, {corner.x, corner.y, 48, 48});
        }
        if (selected_) {
            if (const PlacedEffect* effect = findEffect(*selected_)) {
                const auto corner = screen(effect->at.x - 12, effect->at.y - 12);
                painter.outline({corner.x, corner.y, 24, 24}, UiColor::Gold);
                painter.text(corner.x + (24 - UiPainter::textWidth(effect->name)) / 2, corner.y - 10, effect->name, UiColor::Gold);
            }
        }
    }
    // Placed lights (US-247): a small gold sun with the reach of the light as a frame (the kind's radius, from lights.json when the game gave it).
    for (const PlacedLight& placed : level_.lights) {
        const auto spot = screen(placed.at.x, placed.at.y);
        painter.fill({spot.x - 3, spot.y - 3, 7, 7}, UiColor::Gold);
        painter.outline({spot.x - 4, spot.y - 4, 9, 9}, UiColor::Dark);
        if (selected_ && *selected_ == placed.id) {
            painter.outline({spot.x - 9, spot.y - 9, 19, 19}, UiColor::Gold);
            painter.text(spot.x - UiPainter::textWidth(placed.kind) / 2, spot.y - 20, placed.kind, UiColor::Gold);
        }
    }
    if (selected_ && textures_.plants != nullptr) {
        if (const PlacedPlant* plant = findPlant(*selected_)) {
            const Rect extent = plantExtent(*textures_.plants, plant->kind);
            const auto corner = screen(plant->feet.x - extent.width / 2, plant->feet.y - extent.height);
            painter.outline({corner.x - 1, corner.y - 1, extent.width + 2, extent.height + 2}, UiColor::Gold);
            painter.text(corner.x + (extent.width - UiPainter::textWidth(plant->kind)) / 2, corner.y - 10, plant->kind, UiColor::Gold);
        }
    }

    toolbar_->draw(painter);
    if (previewHour_) { // the time-of-day slider: a track, a gold marker at the hour, and the time of day
        const Rect slider = timeSliderRect();
        painter.fill(slider, UiColor::Panel);
        painter.outline(slider, UiColor::Border);
        const int marker = slider.x + static_cast<int>(std::lround(*previewHour_ / 24.0 * (slider.width - 3)));
        painter.fill({marker, slider.y + 1, 3, slider.height - 2}, UiColor::Gold);
        const int minutes = static_cast<int>(std::lround(*previewHour_ * 60.0)) % (24 * 60);
        painter.text(slider.x + slider.width + 4, slider.y + 2, std::format("{:02}:{:02}", minutes / 60, minutes % 60), UiColor::Text);
    }
    palette_->draw(painter);
    characterPalette_->draw(painter);
    if (characterPalette_->visible) {
        const int kinds = static_cast<int>(definitions_.characters.size());
        const int pages = std::max(1, (kinds + kKindsPerPage - 1) / kKindsPerPage);
        const std::string label = std::format("{}/{}", kindPage_ + 1, pages);
        const Rect& panel = characterPalette_->bounds;
        painter.text(panel.x + (panel.width - UiPainter::textWidth(label)) / 2, panel.y + panel.height - 11, label, UiColor::Text);
        if (textures_.animals != nullptr) {
            for (std::size_t i = 0; i + 2 < characterPalette_->children().size(); ++i) {
                const std::size_t index = static_cast<std::size_t>(kindPage_ * kKindsPerPage) + i;
                if (index >= definitions_.characters.size() || !definitions_.characters[index].animal) continue;
                const Rect& cell = characterPalette_->children()[i]->bounds;
                drawAnimalIcon(renderer, *textures_.animals, definitions_.characters[index].name, {cell.x + 1, cell.y + 1, cell.width - 2, cell.height - 2});
            }
        }
    }    effectPalette_->draw(painter);
    if (effectPalette_->visible && textures_.effects != nullptr) {
        for (std::size_t i = 0; i < effectPalette_->children().size() && i < definitions_.loopingEffects.size(); ++i) {
            const Rect& cell = effectPalette_->children()[i]->bounds;
            drawEffectPicture(renderer, *textures_.effects, definitions_.loopingEffects[i], {cell.x + 1, cell.y + 1, cell.width - 2, cell.height - 2});
        }
    }
    plantPalette_->draw(painter);    if (plantPalette_->visible && textures_.plants != nullptr) {
        const std::vector<std::string> kinds = plantKinds();
        const int pages = plantPageCount();
        const std::string label = std::format("{}/{}{}", plantPage_ + 1, pages, onObjectPage(plantPage_) ? " objects" : "");
        const Rect& panel = plantPalette_->bounds;
        painter.text(panel.x + (panel.width - UiPainter::textWidth(label)) / 2, panel.y + 4, label, UiColor::Text);
        for (std::size_t i = 0; i < effectPalette_->children().size(); ++i) {
        if (auto* b = dynamic_cast<Button*>(effectPalette_->children()[i].get())) b->selected = static_cast<int>(i) == effect_;
    }
    for (std::size_t i = 0; i < lightPalette_->children().size(); ++i) {
        if (auto* b = dynamic_cast<Button*>(lightPalette_->children()[i].get())) b->selected = static_cast<int>(i) == light_;
    }
    for (std::size_t i = 2; i < plantPalette_->children().size(); ++i) {
            const Rect& cell = plantPalette_->children()[i]->bounds;
            const std::size_t index = static_cast<std::size_t>(plantPageFirst(plantPage_)) + i - 2;
            if (index < kinds.size()) {
                drawPlantIcon(renderer, *textures_.plants, kinds[index], {cell.x + 1, cell.y + 1, cell.width - 2, cell.height - 2});
            }
        }
    }
    lightPalette_->draw(painter);
    buildingEditor_->drawPanels(painter, renderer, tool_ == EditorTool::Building);
    weaponPalette_->draw(painter);
    if (weaponPalette_->visible && textures_.weapons != nullptr) {
        for (std::size_t i = 0; i < weaponPalette_->children().size() && i < weaponNames_.size(); ++i) {
            const Rect& cell = weaponPalette_->children()[i]->bounds;
            drawWeaponIcon(renderer, painter, *textures_.weapons, weaponNames_[i], {cell.x + 1, cell.y + 1, cell.width - 2, cell.height - 2});
        }
    }
    properties_->draw(painter);
    npcPanel_->draw(painter);
    npcTrade_->draw(painter);
    settings_->draw(painter);
    economy_->draw(painter);
    classes_->draw(painter);
    // The status line: tool, what it uses, cell, and the last thing done.
    std::string what;
    if (tool_ == EditorTool::Eraser) {
        what = definitions_.tiles[static_cast<std::size_t>(level_.defaultGround)].name;
    } else if (paints(tool_)) {
        what = definitions_.tiles[static_cast<std::size_t>(tile_)].name;
    } else if (tool_ == EditorTool::Place) {
        what = definitions_.characters[static_cast<std::size_t>(kind_)].name;
    } else if (tool_ == EditorTool::Weapon) {
        what = weaponNames_.empty() ? "no weapons" : weaponNames_[static_cast<std::size_t>(weapon_)];
    } else if (tool_ == EditorTool::Effect) {
        what = definitions_.loopingEffects.empty() ? "no effects" : definitions_.loopingEffects[static_cast<std::size_t>(effect_)];
    } else if (tool_ == EditorTool::Building) {
        what = buildingEditor_->currentKind().empty() ? "no buildings" : buildingEditor_->currentKind();
    } else if (tool_ == EditorTool::Light) {
        what = definitions_.lightKinds.empty() ? "no lights" : definitions_.lightKinds[static_cast<std::size_t>(light_)];
    } else if (selected_ && findLight(*selected_) != nullptr) {
        what = findLight(*selected_)->kind + " light";
    } else if (selected_ && findEffect(*selected_) != nullptr) {
        what = findEffect(*selected_)->name;
    } else if (tool_ == EditorTool::Plant) {
        what = plantKinds().empty() ? "no plants" : plantKinds()[static_cast<std::size_t>(plant_)];
    } else if (selected_ && findPlant(*selected_) != nullptr) {
        what = findPlant(*selected_)->kind;
    } else if (selected_ && findPickup(*selected_) != nullptr) {
        what = findPickup(*selected_)->weapon;
    } else {
        const auto shown = std::find_if(level_.characters.begin(), level_.characters.end(), [this](const PlacedCharacter& c) { return selected_ && c.id == *selected_; });
        what = shown == level_.characters.end() ? "nothing selected" : shown->name;
    }
    std::string line = std::format("{}  {}", toolName(tool_), what);
    if (hover_) line += std::format("  ({}, {})", hover_->first, hover_->second);
    if (unsaved_) line += "  *unsaved";
    if (!status_.empty()) line += "  " + status_;
    const Rect bar{0, viewHeight_ - luna::engine::kGlyphHeight - 5, viewWidth_, luna::engine::kGlyphHeight + 5};
    painter.fill(bar, UiColor::Shade);
    // The mode label ("EDITOR  F1: PLAY") takes the right end of the line.
    painter.text(4, bar.y + 3, line.substr(0, static_cast<std::size_t>((viewWidth_ - 110) / luna::engine::kTextAdvance)), UiColor::Text);
    const bool dialog = openList_->visible || question_->visible;
    if (!dialog) { // a dialog takes the pointer: hints behind it would be stale
        toolbar_->drawOverlay(painter);
        palette_->drawOverlay(painter);
        characterPalette_->drawOverlay(painter);
        weaponPalette_->drawOverlay(painter);
        plantPalette_->drawOverlay(painter);
        effectPalette_->drawOverlay(painter);
        lightPalette_->drawOverlay(painter);
        buildingEditor_->drawOverlay(painter, tool_ == EditorTool::Building);
        properties_->drawOverlay(painter);
        npcPanel_->drawOverlay(painter);
        npcTrade_->drawOverlay(painter);
        settings_->drawOverlay(painter);
        economy_->drawOverlay(painter);
        classes_->drawOverlay(painter);
    }
    // Dialogs last, over everything, with the world dimmed behind them.
    if (dialog) {
        painter.fill({0, 0, viewWidth_, viewHeight_}, UiColor::Shade);
        openList_->draw(painter);
        question_->draw(painter);
        openList_->drawOverlay(painter);
        question_->drawOverlay(painter);
    }
}

} // namespace odysseus::game
