#include "game/building_editor.h"

#include "sim/economy.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>
#include <fstream>
#include <system_error>

namespace odysseus::game {

namespace {

using luna::engine::Button;
using luna::engine::NumberField;
using luna::engine::Panel;
using luna::engine::Rect;
using luna::engine::TextField;
using luna::engine::UiColor;
using luna::engine::UiPainter;
using sim::buildings::KindDef;

constexpr int kToolbarHeight = 18; // as the Editor's toolbar
constexpr int kRow = 14;
constexpr int kPanelWidth = 140;
constexpr int kPrefabWidth = 304;
constexpr int kCanvasCell = 12;

bool validId(const std::string& id) {
    if (id.empty() || id.size() > 40) return false;
    return std::all_of(id.begin(), id.end(), [](unsigned char c) { return std::islower(c) || std::isdigit(c) || c == '-'; });
}

} // namespace

BuildingEditor::BuildingEditor(Level& level, int viewWidth, int viewHeight, std::function<void(std::unique_ptr<Command>)> run, std::function<void(const std::string&)> say)
    : level_(level), viewWidth_(viewWidth), viewHeight_(viewHeight), run_(std::move(run)), say_(std::move(say)) {
    palette_ = std::make_unique<Panel>(Rect{0, 0, 0, 0});
    panel_ = std::make_unique<Panel>(Rect{0, 0, 0, 0});
    prefab_ = std::make_unique<Panel>(Rect{0, 0, 0, 0});
    palette_->visible = panel_->visible = prefab_->visible = false;
}

void BuildingEditor::setHost(Host host) {
    host_ = std::move(host);
    stale_ = true;
    newPrefab();
}

// ---- the Build tool

std::vector<std::string> BuildingEditor::kinds() const {
    std::vector<std::string> out;
    if (data() == nullptr) return out;
    for (const KindDef& kind : data()->kinds()) {
        if (!kind.prefab) out.push_back(kind.id);
    }
    for (const KindDef& kind : data()->kinds()) {
        if (kind.prefab) out.push_back(kind.id);
    }
    return out;
}

void BuildingEditor::setKind(int index) {
    const int count = static_cast<int>(kinds().size());
    kind_ = count == 0 ? 0 : std::clamp(index, 0, count - 1);
}

std::string BuildingEditor::currentKind() const {
    const std::vector<std::string> all = kinds();
    return all.empty() ? std::string() : all[static_cast<std::size_t>(std::clamp(kind_, 0, static_cast<int>(all.size()) - 1))];
}

std::pair<int, int> BuildingEditor::sizeOf(const PlacedBuildingSpec& spec) const {
    if (data() == nullptr) return {1, 1};
    if (spec.kind.rfind("piece:", 0) == 0) return {1, 1};
    const KindDef* kind = data()->kind(spec.kind);
    if (kind == nullptr) return {1, 1};
    const sim::buildings::Cell size = sim::buildings::turnedSize(kind->width, kind->height, spec.turns);
    return {size.x, size.y};
}

std::string BuildingEditor::whyNot(const luna::engine::TileMap& ground, int cellX, int cellY) const {
    const std::string kindId = currentKind();
    const KindDef* kind = kindId.empty() || data() == nullptr ? nullptr : data()->kind(kindId);
    if (kind == nullptr) return "Choose a building first.";
    const sim::buildings::Cell size = sim::buildings::turnedSize(kind->width, kind->height, turns_);
    const int ax = cellX - size.x / 2;
    const int ay = cellY - size.y / 2;
    for (int y = ay; y < ay + size.y; ++y) {
        for (int x = ax; x < ax + size.x; ++x) {
            if (!level_.inside(x, y)) return "It does not fit in the level.";
            if (ground.isSolid(x, y)) return "Rock or water is in the way.";
        }
    }
    for (const PlacedBuildingSpec& other : level_.buildings) {
        const auto [ow, oh] = sizeOf(other);
        if (ax < other.x + ow && ax + size.x > other.x && ay < other.y + oh && ay + size.y > other.y) return "Another building is there.";
    }
    return {};
}

bool BuildingEditor::placeAt(const luna::engine::TileMap& ground, int cellX, int cellY, bool blueprint) {
    if (const std::string why = whyNot(ground, cellX, cellY); !why.empty()) {
        say_(why);
        return false;
    }
    const KindDef* kind = data()->kind(currentKind());
    const sim::buildings::Cell size = sim::buildings::turnedSize(kind->width, kind->height, turns_);
    auto after = level_.buildings;
    const int id = level_.nextId;
    PlacedBuildingSpec spec;
    spec.id = id;
    spec.kind = kind->id;
    spec.x = cellX - size.x / 2;
    spec.y = cellY - size.y / 2;
    spec.turns = turns_;
    spec.finished = !blueprint;
    after.push_back(spec);
    change(std::format("place {}{} #{}", kind->label, blueprint ? " blueprint" : "", id), std::move(after), id + 1);
    selected_ = id;
    stale_ = true;
    return true;
}

void BuildingEditor::change(const std::string& what, std::vector<PlacedBuildingSpec> after, int nextIdAfter) {
    run_(std::make_unique<BuildingsCommand>(what, level_.buildings, std::move(after), level_.nextId, nextIdAfter));
    stale_ = true;
}

// ---- a selected building

const PlacedBuildingSpec* BuildingEditor::selectedSpec() const {
    for (const PlacedBuildingSpec& spec : level_.buildings) {
        if (spec.id == selected_) return &spec;
    }
    return nullptr;
}

bool BuildingEditor::selectAt(int worldX, int worldY) {
    const int cx = worldX >= 0 ? worldX / kTileSize : -1;
    const int cy = worldY >= 0 ? worldY / kTileSize : -1;
    for (auto it = level_.buildings.rbegin(); it != level_.buildings.rend(); ++it) {
        const auto [w, h] = sizeOf(*it);
        if (cx >= it->x && cy >= it->y && cx < it->x + w && cy < it->y + h) {
            if (selected_ != it->id) stale_ = true;
            selected_ = it->id;
            return true;
        }
    }
    return false;
}

void BuildingEditor::clearSelection() {
    if (selected_ != 0) stale_ = true;
    selected_ = 0;
}

bool BuildingEditor::turnSelected() {
    if (selectedSpec() == nullptr) return false;
    auto after = level_.buildings;
    for (PlacedBuildingSpec& spec : after) {
        if (spec.id == selected_) spec.turns = (spec.turns + 1) % 4;
    }
    change("turn building", std::move(after), level_.nextId);
    return true;
}

bool BuildingEditor::removeSelected() {
    const PlacedBuildingSpec* gone = selectedSpec();
    if (gone == nullptr) return false;
    const std::string what = "remove " + gone->kind;
    auto after = level_.buildings;
    std::erase_if(after, [this](const PlacedBuildingSpec& spec) { return spec.id == selected_; });
    change(what, std::move(after), level_.nextId);
    selected_ = 0;
    return true;
}

bool BuildingEditor::setInterior(const std::string& mode, const std::string& levelName) {
    if (selectedSpec() == nullptr || (!mode.empty() && mode != "fade" && mode != "map")) return false;
    auto after = level_.buildings;
    for (PlacedBuildingSpec& spec : after) {
        if (spec.id != selected_) continue;
        spec.interior = mode;
        spec.interiorLevel = levelName;
    }
    if (after == level_.buildings) return true;
    change(mode.empty() ? "interior follows the kind" : "interior " + mode, std::move(after), level_.nextId);
    return true;
}

bool BuildingEditor::setOwner(int person) {
    if (selectedSpec() == nullptr) return false;
    auto after = level_.buildings;
    for (PlacedBuildingSpec& spec : after) {
        if (spec.id == selected_) spec.owner = std::clamp(person, -1, 100000);
    }
    if (after == level_.buildings) return true;
    change("owner " + std::to_string(person), std::move(after), level_.nextId);
    return true;
}

bool BuildingEditor::setFinished(bool finished) {
    if (selectedSpec() == nullptr) return false;
    auto after = level_.buildings;
    for (PlacedBuildingSpec& spec : after) {
        if (spec.id == selected_) spec.finished = finished;
    }
    if (after == level_.buildings) return true;
    change(finished ? "finished" : "blueprint", std::move(after), level_.nextId);
    return true;
}

// ---- the Prefab tab

void BuildingEditor::showPrefabs(bool shown) {
    prefabsShown_ = shown;
    if (shown) draftChanged_ = true;
}

void BuildingEditor::newPrefab() {
    draft_ = KindDef{};
    draft_.width = 3;
    draft_.height = 3;
    draft_.prefab = true;
    draft_.owner = "clan";
    draft_.buildable = true;
    draft_.known = false;
    draft_.colour = 0x8B6B3E;
    costText_.clear();
    secondsOverride_ = 0;
    draftIsNew_ = true;
    prefabStatus_.clear();
    if (data() != nullptr && !data()->pieces().empty()) piece_ = data()->pieces().front().id;
    draftChanged_ = true;
}

bool BuildingEditor::loadPrefab(const std::string& id) {
    if (data() == nullptr) return false;
    const KindDef* found = data()->kind(id);
    if (found == nullptr) return false;
    draft_ = *found;
    if (!found->prefab) {
        draft_.id = found->id + "-copy"; // a kind of kinds.json is never overwritten: it is saved as a prefab of its own
        draft_.prefab = true;
    }
    costText_ = sim::formatPairs(draft_.cost);
    secondsOverride_ = draft_.buildMilli / 1000;
    draftIsNew_ = !found->prefab;
    prefabStatus_.clear();
    draftChanged_ = true;
    return true;
}

bool BuildingEditor::setSize(int width, int height) {
    draft_.width = std::clamp(width, 1, kCanvasCells);
    draft_.height = std::clamp(height, 1, kCanvasCells);
    std::erase_if(draft_.layout, [this](const sim::buildings::LayoutPiece& lp) { return lp.x >= draft_.width || lp.y >= draft_.height; });
    draftChanged_ = true;
    return true;
}

bool BuildingEditor::setPiece(int cellX, int cellY, const std::string& pieceId) {
    if (data() == nullptr || cellX < 0 || cellY < 0 || cellX >= draft_.width || cellY >= draft_.height) return false;
    if (pieceId.empty()) {
        // Erase takes the topmost piece of the cell: the roof, then the wall, then the floor.
        for (const sim::buildings::Layer layer : {sim::buildings::Layer::Roof, sim::buildings::Layer::Wall, sim::buildings::Layer::Floor}) {
            const auto found = std::find_if(draft_.layout.begin(), draft_.layout.end(), [&](const sim::buildings::LayoutPiece& lp) {
                const sim::buildings::PieceDef* def = data()->piece(lp.piece);
                return lp.x == cellX && lp.y == cellY && def != nullptr && sim::buildings::layerOf(def->type) == layer;
            });
            if (found != draft_.layout.end()) {
                draft_.layout.erase(found);
                draftChanged_ = true;
                return true;
            }
        }
        return false;
    }
    const sim::buildings::PieceDef* def = data()->piece(pieceId);
    if (def == nullptr) return false;
    std::erase_if(draft_.layout, [&](const sim::buildings::LayoutPiece& lp) {
        const sim::buildings::PieceDef* other = data()->piece(lp.piece);
        return lp.x == cellX && lp.y == cellY && other != nullptr && sim::buildings::layerOf(other->type) == sim::buildings::layerOf(def->type);
    });
    draft_.layout.push_back({pieceId, cellX, cellY});
    draftChanged_ = true;
    return true;
}

void BuildingEditor::setPrefabInterior(const std::string& mode, const std::string& levelName) {
    if (const auto parsed = sim::buildings::interiorModeFromName(mode)) draft_.interior = *parsed;
    draft_.interiorLevel = levelName;
    draftChanged_ = true;
}

void BuildingEditor::togglePrefabUse(const std::string& use) {
    const auto found = std::find(draft_.uses.begin(), draft_.uses.end(), use);
    if (found != draft_.uses.end()) draft_.uses.erase(found);
    else if (sim::buildings::isUse(use)) draft_.uses.push_back(use);
    draftChanged_ = true;
}

void BuildingEditor::setPrefabFlags(bool buildable, bool known) {
    draft_.buildable = buildable;
    draft_.known = known;
    draftChanged_ = true;
}

bool BuildingEditor::setPrefabCost(const std::string& text) {
    if (text.empty()) {
        costText_.clear();
        draftChanged_ = true;
        return true;
    }
    std::string problem;
    if (!sim::parsePairs(text, 1, 1000, &problem)) {
        prefabStatus_ = problem;
        say_(problem);
        return false;
    }
    costText_ = text;
    draftChanged_ = true;
    return true;
}

void BuildingEditor::setPrefabSeconds(int seconds) {
    secondsOverride_ = std::clamp(seconds, 0, 3600);
    draftChanged_ = true;
}

bool BuildingEditor::savePrefab() {
    if (data() == nullptr) return false;
    const auto refuse = [this](const std::string& why) {
        prefabStatus_ = why;
        say_(why);
        return false;
    };
    if (!validId(draft_.id)) return refuse("The id needs lower-case letters, digits and - (for example my-lodge).");
    if (const KindDef* clash = data()->kind(draft_.id); clash != nullptr && !clash->prefab) return refuse("\"" + draft_.id + "\" is a kind of kinds.json: choose another id.");
    if (draft_.layout.empty()) return refuse("Place at least one piece on the grid.");
    if (draft_.label.empty()) draft_.label = draft_.id;
    KindDef out = draft_;
    out.prefab = true;
    out.cost = sim::buildings::layoutCost(*data(), out.layout);
    if (!costText_.empty()) {
        std::string problem;
        const auto parsed = sim::parsePairs(costText_, 1, 1000, &problem);
        if (!parsed) return refuse(problem);
        out.cost = *parsed;
    }
    out.buildMilli = secondsOverride_ > 0 ? secondsOverride_ * 1000 : sim::buildings::layoutBuildMilli(*data(), out.layout);
    sim::rules::LoadReport report;
    const std::string name = "buildings/prefabs/" + out.id + ".json";
    const auto parsed = data()->parseKind(sim::buildings::toJson(out), name, report, true, out.id);
    if (!parsed) return refuse(report.errors.empty() ? "The prefab has a mistake." : report.errors.front().text());
    std::error_code error;
    std::filesystem::create_directories(host_.prefabFolder, error);
    const std::filesystem::path file = host_.prefabFolder / (out.id + ".json");
    const std::filesystem::path temporary = file.string() + ".tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        stream << sim::buildings::toJson(out);
        stream.flush();
        if (!stream) return refuse("The prefab file could not be written.");
    }
    std::filesystem::rename(temporary, file, error);
    if (error) return refuse("The prefab file could not be written: " + error.message());
    host_.data->addKind(*parsed);
    if (host_.saved) host_.saved(*parsed);
    draft_ = *parsed;
    draftIsNew_ = false;
    stale_ = true;
    prefabStatus_ = "saved " + out.id;
    say_("saved prefab " + out.id);
    return true;
}

// ---- the panels

Rect BuildingEditor::canvasRect() const {
    const Rect box = prefab_->bounds;
    return {box.x + 110, box.y + 46, kCanvasCells * kCanvasCell, kCanvasCells * kCanvasCell};
}

void BuildingEditor::buildPalette() {
    const std::vector<std::string> all = kinds();
    int widest = 40;
    for (const std::string& id : all) {
        const KindDef* kind = data()->kind(id);
        widest = std::max(widest, UiPainter::textWidth((kind != nullptr && kind->prefab ? "* " : "") + (kind != nullptr ? kind->label : id)));
    }
    palette_ = std::make_unique<Panel>(Rect{2, kToolbarHeight + 4, widest + 16, std::max(1, static_cast<int>(all.size())) * kRow + 4});
    for (std::size_t i = 0; i < all.size(); ++i) {
        const KindDef* kind = data()->kind(all[i]);
        const Rect row{4, kToolbarHeight + 6 + static_cast<int>(i) * kRow, widest + 12, kRow - 2};
        Button& button = palette_->add<Button>(row, (kind != nullptr && kind->prefab ? "* " : "") + (kind != nullptr ? kind->label : all[i]), [this, i] { setKind(static_cast<int>(i)); });
        button.hint = kind != nullptr && !kind->note.empty() ? kind->note : all[i];
    }
}

void BuildingEditor::buildPanel() {
    panel_ = std::make_unique<Panel>(Rect{viewWidth_ - kPanelWidth - 2, kToolbarHeight + 4, kPanelWidth, 4 * kRow + 76});
    panel_->visible = false;
    const PlacedBuildingSpec* shown = selectedSpec();
    if (shown == nullptr) return;
    panel_->visible = true;
    const Rect box = panel_->bounds;
    const int left = box.x + 4;
    const int width = box.width - 8;
    int y = box.y + 4;
    panel_->add<Button>(Rect{left, y, width, 11}, std::format("{} #{}", shown->kind, shown->id), [] {});
    y += kRow;
    panel_->add<Button>(Rect{left, y, width, 11}, "Turn (R): " + std::to_string(shown->turns * 90) + " deg", [this] { turnSelected(); });
    y += kRow;
    const std::string mode = shown->interior.empty() ? "kind's own" : shown->interior;
    Button& interior = panel_->add<Button>(Rect{left, y, width, 11}, "Interior: " + mode, [this] {
        const PlacedBuildingSpec* spec = selectedSpec();
        if (spec == nullptr) return;
        const std::string next = spec->interior.empty() ? "fade" : (spec->interior == "fade" ? "map" : "");
        setInterior(next, spec->interiorLevel);
    });
    interior.hint = "How the inside shows in the game: the roof fades, or the door leads into an interior level. The kind's own mode is the default.";
    y += kRow;
    panel_->add<TextField>(Rect{left, y, width, 11}, "Level", shown->interiorLevel, 30, [this](const std::string& text) {
        if (const PlacedBuildingSpec* spec = selectedSpec()) setInterior(spec->interior.empty() && !text.empty() ? "map" : spec->interior, text);
    });
    y += kRow;
    panel_->add<NumberField>(Rect{left, y, width, 11}, "Owner", shown->owner, -1, 100000, [this](int person) { setOwner(person); });
    y += kRow;
    panel_->add<Button>(Rect{left, y, width, 11}, shown->finished ? "Finished" : "Blueprint", [this] {
        if (const PlacedBuildingSpec* spec = selectedSpec()) setFinished(!spec->finished);
    });
    y += kRow;
    panel_->add<Button>(Rect{left, y, width, 11}, "Delete", [this] { removeSelected(); });
}

void BuildingEditor::buildPrefabPanel() {
    constexpr int kHeight = 306;
    prefab_ = std::make_unique<Panel>(Rect{viewWidth_ - kPrefabWidth - 2, kToolbarHeight + 4, kPrefabWidth, kHeight});
    prefab_->visible = false;
    if (data() == nullptr) return;
    prefab_->visible = true;
    const Rect box = prefab_->bounds;
    const int left = box.x + 4;
    const int full = box.width - 8;
    int y = box.y + 14;
    prefab_->add<TextField>(Rect{left, y, 140, 11}, "Id", draft_.id, 40, [this](const std::string& text) { setPrefabId(text); });
    prefab_->add<TextField>(Rect{left + 146, y, full - 146, 11}, "Name", draft_.label, 30, [this](const std::string& text) { setPrefabLabel(text); });
    y += kRow;
    prefab_->add<NumberField>(Rect{left, y, 66, 11}, "W", draft_.width, 1, kCanvasCells, [this](int w) { setSize(w, draft_.height); });
    prefab_->add<NumberField>(Rect{left + 70, y, 66, 11}, "H", draft_.height, 1, kCanvasCells, [this](int h) { setSize(draft_.width, h); });
    prefab_->add<Button>(Rect{left + 140, y, 46, 11}, "New", [this] { newPrefab(); });
    Button& load = prefab_->add<Button>(Rect{left + 190, y, full - 190, 11}, "Open next", [this] {
        // Cycles through the prefabs and kinds: the draft becomes a copy of the next one.
        const std::vector<std::string> all = kinds();
        if (all.empty()) return;
        const auto at = std::find(all.begin(), all.end(), draft_.id);
        const std::size_t next = at == all.end() ? 0 : static_cast<std::size_t>(at - all.begin() + 1) % all.size();
        loadPrefab(all[next]);
    });
    load.hint = "Opens the next kind or prefab as the draft (a kind of kinds.json is saved under a new id).";
    y += kRow + 4;
    // The pieces on the left (the first row erases), the grid on the right.
    std::vector<std::string> names = {"(erase)"};
    int selectedRow = 0;
    for (const sim::buildings::PieceDef& def : data()->pieces()) {
        if (def.id == piece_) selectedRow = static_cast<int>(names.size());
        names.push_back(def.label);
    }
    luna::engine::ListBox& list = prefab_->add<luna::engine::ListBox>(Rect{left, y, 102, 14 * 9}, names, [this](int row) {
        piece_ = row <= 0 ? std::string() : data()->pieces()[static_cast<std::size_t>(row - 1)].id;
    });
    list.selected = selectedRow;
    // The fields under the grid.
    y = canvasRect().y + canvasRect().height + 6;
    prefab_->add<Button>(Rect{left, y, 150, 11}, std::string("Interior: ") + sim::buildings::interiorModeName(draft_.interior), [this] {
        setPrefabInterior(draft_.interior == sim::buildings::InteriorMode::Fade ? "map" : "fade", draft_.interiorLevel);
    });
    prefab_->add<TextField>(Rect{left + 154, y, full - 154, 11}, "Level", draft_.interiorLevel, 30, [this](const std::string& text) { draft_.interiorLevel = text; draftChanged_ = true; });
    y += kRow;
    int x = left;
    for (const char* use : sim::buildings::kUses) {
        const bool on = std::find(draft_.uses.begin(), draft_.uses.end(), use) != draft_.uses.end();
        Button& button = prefab_->add<Button>(Rect{x, y, 70, 11}, std::string(on ? "[x] " : "[ ] ") + use, [this, use] { togglePrefabUse(use); });
        button.hint = "What the building is for: shelter, sleep, store, work.";
        x += 74;
    }
    y += kRow;
    prefab_->add<Button>(Rect{left, y, 100, 11}, std::string(draft_.buildable ? "[x]" : "[ ]") + " Buildable", [this] { setPrefabFlags(!draft_.buildable, draft_.known); });
    prefab_->add<Button>(Rect{left + 104, y, 100, 11}, std::string(draft_.known ? "[x]" : "[ ]") + " Known at start", [this] { setPrefabFlags(draft_.buildable, !draft_.known); });
    y += kRow;
    prefab_->add<TextField>(Rect{left, y, full, 11}, "Cost", costText_, 60, [this](const std::string& text) { setPrefabCost(text); });
    y += kRow;
    prefab_->add<NumberField>(Rect{left, y, 150, 11}, "Seconds", secondsOverride_, 0, 3600, [this](int seconds) { setPrefabSeconds(seconds); });
    prefab_->add<Button>(Rect{left + 156, y, 60, 11}, "Save", [this] { savePrefab(); });
    prefab_->add<Button>(Rect{left + 220, y, full - 220, 11}, "Close", [this] { showPrefabs(false); });
}

bool BuildingEditor::typing() const { return palette_->typing() || panel_->typing() || prefab_->typing(); }

void BuildingEditor::applyHelp(EditorHelp& help) {
    help.apply(*panel_, "building");
    help.apply(*prefab_, "prefab");
}

bool BuildingEditor::handle(const luna::engine::UiInput& input, bool buildTool) {
    if (data() == nullptr) return false;
    if (stale_ && !typing()) {
        buildPalette();
        buildPanel();
        stale_ = false;
    }
    if (draftChanged_ && !prefab_->typing()) {
        buildPrefabPanel();
        draftChanged_ = false;
    }
    palette_->visible = buildTool;
    prefab_->visible = prefabsShown_;
    panel_->visible = panel_->visible && selectedSpec() != nullptr;
    bool used = palette_->handle(input) || panel_->handle(input);
    if (prefabsShown_) {
        used = prefab_->handle(input) || used;
        // A click on the grid places the chosen piece; the right button takes the top piece of the cell away.
        const Rect canvas = canvasRect();
        const luna::engine::Pointer& pointer = input.pointer;
        if (pointer.inside() && pointer.x >= canvas.x && pointer.y >= canvas.y && pointer.x < canvas.x + canvas.width && pointer.y < canvas.y + canvas.height) {
            const int cx = (pointer.x - canvas.x) / kCanvasCell;
            const int cy = (pointer.y - canvas.y) / kCanvasCell;
            if (pointer.wasPressed(luna::engine::PointerButton::Left)) setPiece(cx, cy, piece_);
            else if (pointer.wasPressed(luna::engine::PointerButton::Right)) setPiece(cx, cy, std::string());
            used = true;
        }
    }
    return used;
}

void BuildingEditor::drawPanels(UiPainter& painter, luna::engine::Renderer& renderer, bool buildTool) const {
    if (data() == nullptr) return;
    if (buildTool) palette_->draw(painter);
    panel_->draw(painter);
    if (!prefabsShown_) return;
    prefab_->draw(painter);
    const Rect box = prefab_->bounds;
    painter.text(box.x + 5, box.y + 3, "PREFAB   click: place   right click: erase", UiColor::Gold);
    const Rect canvas = canvasRect();
    painter.fill({canvas.x - 1, canvas.y - 1, canvas.width + 2, canvas.height + 2}, UiColor::Border);
    painter.fill(canvas, UiColor::Dark);
    painter.fill({canvas.x, canvas.y, draft_.width * kCanvasCell, draft_.height * kCanvasCell}, UiColor::Panel);
    for (int i = 0; i <= draft_.width; ++i) painter.fill({canvas.x + i * kCanvasCell, canvas.y, 1, draft_.height * kCanvasCell}, UiColor::Grid);
    for (int i = 0; i <= draft_.height; ++i) painter.fill({canvas.x, canvas.y + i * kCanvasCell, draft_.width * kCanvasCell, 1}, UiColor::Grid);
    // Floors first, then walls, then roofs (see-through), in the colour of each piece.
    for (const sim::buildings::Layer layer : {sim::buildings::Layer::Floor, sim::buildings::Layer::Wall, sim::buildings::Layer::Roof}) {
        for (const sim::buildings::LayoutPiece& lp : draft_.layout) {
            const sim::buildings::PieceDef* def = data()->piece(lp.piece);
            if (def == nullptr || sim::buildings::layerOf(def->type) != layer) continue;
            const int inset = layer == sim::buildings::Layer::Wall ? 1 : 0;
            const Rect cell{canvas.x + lp.x * kCanvasCell + 1 + inset, canvas.y + lp.y * kCanvasCell + 1 + inset, kCanvasCell - 1 - 2 * inset, kCanvasCell - 1 - 2 * inset};
            if (layer_ != nullptr) layer_->drawFlat(renderer, lp.piece, cell, layer == sim::buildings::Layer::Roof ? 120 : 255);
        }
    }
    const std::string cost = costText_.empty() ? sim::formatPairs(sim::buildings::layoutCost(*data(), draft_.layout)) : costText_;
    painter.text(box.x + 5, box.y + box.height - 12, prefabStatus_.empty() ? "Cost: " + (cost.empty() ? std::string("free") : cost) : prefabStatus_, prefabStatus_.empty() ? UiColor::Dim : UiColor::Gold);
}

void BuildingEditor::drawOverlay(UiPainter& painter, bool buildTool) const {
    if (data() == nullptr) return;
    if (buildTool) palette_->drawOverlay(painter);
    panel_->drawOverlay(painter);
    if (prefabsShown_) prefab_->drawOverlay(painter);
}

void BuildingEditor::drawWorld(luna::engine::Renderer& renderer, UiPainter& painter, const Rect& view, const luna::engine::TileMap& ground, bool buildTool,
                               std::optional<std::pair<int, int>> hover) const {
    if (data() == nullptr || layer_ == nullptr) return;
    for (const PlacedBuildingSpec& spec : level_.buildings) layer_->drawLayout(renderer, spec.kind, spec.x, spec.y, spec.turns, view, spec.finished ? 255 : 110, spec.finished ? 150 : 90);
    if (const PlacedBuildingSpec* spec = selectedSpec()) {
        const auto [w, h] = sizeOf(*spec);
        painter.outline({spec->x * kTileSize - view.x, spec->y * kTileSize - view.y, w * kTileSize, h * kTileSize}, UiColor::Gold);
    }
    if (buildTool && hover && !currentKind().empty()) {
        const KindDef* kind = data()->kind(currentKind());
        if (kind == nullptr) return;
        const sim::buildings::Cell size = sim::buildings::turnedSize(kind->width, kind->height, turns_);
        const int ax = hover->first - size.x / 2;
        const int ay = hover->second - size.y / 2;
        layer_->drawLayout(renderer, kind->id, ax, ay, turns_, view, 170, 110);
        const bool valid = whyNot(ground, hover->first, hover->second).empty();
        painter.outline({ax * kTileSize - view.x, ay * kTileSize - view.y, size.x * kTileSize, size.y * kTileSize}, valid ? UiColor::Gold : UiColor::Red);
    }
}

} // namespace odysseus::game
