#include "sim/building_store.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <queue>

namespace odysseus::sim::buildings {

namespace {

constexpr const char* kPiecePrefix = "piece:";

bool isPieceKind(const std::string& kind) { return kind.rfind(kPiecePrefix, 0) == 0; }
std::string pieceOfKind(const std::string& kind) { return kind.substr(std::char_traits<char>::length(kPiecePrefix)); }

void mix(std::uint64_t& h, std::uint64_t value) {
    h ^= value + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2);
    h *= 0x100000001B3ULL;
}
void mixText(std::uint64_t& h, const std::string& text) {
    mix(h, text.size());
    for (const char c : text) mix(h, static_cast<unsigned char>(c));
}
void mixCounts(std::uint64_t& h, const ItemCounts& counts) {
    mix(h, counts.size());
    for (const auto& [item, count] : counts) {
        mixText(h, item);
        mix(h, static_cast<std::uint64_t>(count));
    }
}

nlohmann::json countsToJson(const ItemCounts& counts) {
    nlohmann::json out = nlohmann::json::object();
    for (const auto& [item, count] : counts) out[item] = count;
    return out;
}

ItemCounts countsFromJson(const nlohmann::json& value) {
    ItemCounts out;
    if (!value.is_object()) return out;
    for (const auto& [item, count] : value.items()) {
        if (count.is_number_integer() && count.get<int>() > 0) out[item] = count.get<int>();
    }
    return out;
}

} // namespace

const char* stateName(State state) {
    switch (state) {
    case State::Blueprint: return "blueprint";
    case State::Finished: return "finished";
    case State::Rubble: return "rubble";
    }
    return "blueprint";
}

void BuildingStore::reset(int mapWidth, int mapHeight, std::uint64_t seed) {
    width_ = std::max(0, mapWidth);
    height_ = std::max(0, mapHeight);
    for (auto& layer : grid_) layer.assign(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), Occupant{});
    buildings_.clear();
    drops_.clear();
    nextId_ = 1;
    random_ = core::Pcg32(seed, 11);
    fireSecondTicks_ = 0;
    touch();
}

// ---- the pieces of a kind, turned and put at a cell

std::vector<PieceState> BuildingStore::makePieces(const std::string& kind, int x, int y, int turns, int& width, int& height, bool finished) const {
    std::vector<PieceState> pieces;
    width = 1;
    height = 1;
    if (data_ == nullptr) return pieces;
    std::vector<LayoutPiece> layout;
    int kindWidth = 1;
    int kindHeight = 1;
    if (isPieceKind(kind)) {
        layout.push_back({pieceOfKind(kind), 0, 0});
    } else if (const KindDef* def = data_->kind(kind)) {
        layout = def->layout;
        kindWidth = def->width;
        kindHeight = def->height;
    } else {
        return pieces;
    }
    const Cell size = turnedSize(kindWidth, kindHeight, turns);
    width = size.x;
    height = size.y;
    for (const LayoutPiece& lp : layout) {
        const PieceDef* def = data_->piece(lp.piece);
        if (def == nullptr) continue;
        const Cell offset = turnedOffset(lp.x, lp.y, kindWidth, kindHeight, turns);
        PieceState piece;
        piece.piece = lp.piece;
        piece.x = x + offset.x;
        piece.y = y + offset.y;
        piece.built = finished;
        piece.hpMilli = finished ? def->hp * 1000 : 0;
        pieces.push_back(std::move(piece));
    }
    // They rise floors first, then walls, then roofs (a stable order, so the same blueprint always rises the same way).
    std::stable_sort(pieces.begin(), pieces.end(), [this](const PieceState& a, const PieceState& b) {
        const PieceDef* da = data_->piece(a.piece);
        const PieceDef* db = data_->piece(b.piece);
        return static_cast<int>(layerOf(da->type)) < static_cast<int>(layerOf(db->type));
    });
    return pieces;
}

std::vector<std::pair<int, int>> BuildingStore::footprintCells(const std::string& kind, int x, int y, int turns) const {
    int w = 1;
    int h = 1;
    std::vector<std::pair<int, int>> cells;
    for (const PieceState& piece : makePieces(kind, x, y, turns, w, h, false)) cells.emplace_back(piece.x, piece.y);
    std::sort(cells.begin(), cells.end());
    cells.erase(std::unique(cells.begin(), cells.end()), cells.end());
    return cells;
}

std::vector<PieceState> BuildingStore::layoutPieces(const std::string& kind, int x, int y, int turns) const {
    int w = 1;
    int h = 1;
    return makePieces(kind, x, y, ((turns % 4) + 4) % 4, w, h, false);
}

std::vector<std::pair<int, int>> BuildingStore::blockingCells(const std::string& kind, int x, int y, int turns) const {
    std::vector<std::pair<int, int>> cells;
    for (const PieceState& piece : layoutPieces(kind, x, y, turns)) {
        const PieceDef* def = pieceDef(piece);
        if (def != nullptr && blocksWalking(def->type)) cells.emplace_back(piece.x, piece.y);
    }
    return cells;
}

std::pair<int, int> BuildingStore::sizeOf(const std::string& kind, int turns) const {
    if (data_ == nullptr) return {0, 0};
    if (isPieceKind(kind)) return data_->piece(pieceOfKind(kind)) != nullptr ? std::pair<int, int>{1, 1} : std::pair<int, int>{0, 0};
    const KindDef* def = data_->kind(kind);
    if (def == nullptr) return {0, 0};
    const Cell size = turnedSize(def->width, def->height, ((turns % 4) + 4) % 4);
    return {size.x, size.y};
}

std::string BuildingStore::whyNot(const std::string& kind, int x, int y, int turns, const Blocked& blocked) const {
    if (data_ == nullptr) return "No building data.";
    if (isPieceKind(kind) ? data_->piece(pieceOfKind(kind)) == nullptr : data_->kind(kind) == nullptr) return "Unknown building.";
    int w = 1;
    int h = 1;
    const std::vector<PieceState> pieces = makePieces(kind, x, y, turns, w, h, false);
    const bool whole = !isPieceKind(kind);
    for (int cy = y; cy < y + h; ++cy) {
        for (int cx = x; cx < x + w; ++cx) {
            if (!inside(cx, cy)) return "Outside the map.";
            if (blocked && blocked(cx, cy)) return "Something is in the way.";
            if (whole && footprintAt(cx, cy) != 0) return "Another building is there.";
        }
    }
    for (const PieceState& piece : pieces) {
        if (!inside(piece.x, piece.y)) return "Outside the map.";
        if (blocked && blocked(piece.x, piece.y)) return "Something is in the way.";
        const PieceDef* def = pieceDef(piece);
        if (def != nullptr && occupant(layerOf(def->type), piece.x, piece.y).building != 0) return "That spot is taken.";
    }
    return {};
}

BuildingStore::Placed BuildingStore::place(const std::string& kind, int x, int y, int turns, int faction, bool finished, const Blocked& blocked) {
    Placed result;
    result.problem = whyNot(kind, x, y, turns, blocked);
    if (!result.problem.empty()) return result;
    PlacedBuilding building;
    building.id = nextId_++;
    building.kind = kind;
    building.x = x;
    building.y = y;
    building.turns = ((turns % 4) + 4) % 4;
    building.faction = faction;
    building.pieces = makePieces(kind, x, y, building.turns, building.width, building.height, finished);
    building.state = finished ? State::Finished : State::Blueprint;
    index(building);
    result.id = building.id;
    buildings_.push_back(std::move(building));
    touch();
    return result;
}

void BuildingStore::index(PlacedBuilding& building) {
    for (std::size_t i = 0; i < building.pieces.size(); ++i) {
        const PieceState& piece = building.pieces[i];
        const PieceDef* def = pieceDef(piece);
        if (def == nullptr || piece.fell() || !inside(piece.x, piece.y)) continue;
        occupant(layerOf(def->type), piece.x, piece.y) = {building.id, static_cast<int>(i)};
    }
}

void BuildingStore::unindex(const PlacedBuilding& building) {
    for (const PieceState& piece : building.pieces) {
        const PieceDef* def = pieceDef(piece);
        if (def == nullptr || !inside(piece.x, piece.y)) continue;
        Occupant& at = occupant(layerOf(def->type), piece.x, piece.y);
        if (at.building == building.id) at = {};
    }
}

void BuildingStore::rebuildGrid() {
    for (auto& layer : grid_) layer.assign(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), Occupant{});
    for (PlacedBuilding& building : buildings_) {
        if (building.state != State::Rubble) index(building);
    }
}

const PlacedBuilding* BuildingStore::find(int id) const {
    const auto found = std::lower_bound(buildings_.begin(), buildings_.end(), id, [](const PlacedBuilding& b, int key) { return b.id < key; });
    return found != buildings_.end() && found->id == id ? &*found : nullptr;
}

PlacedBuilding* BuildingStore::findMutable(int id) { return const_cast<PlacedBuilding*>(find(id)); }

int BuildingStore::buildingAt(int x, int y, int* pieceIndex) const {
    if (!inside(x, y)) return 0;
    for (const Layer layer : {Layer::Wall, Layer::Roof, Layer::Floor}) {
        const Occupant& at = occupant(layer, x, y);
        if (at.building != 0) {
            if (pieceIndex != nullptr) *pieceIndex = at.piece;
            return at.building;
        }
    }
    if (pieceIndex != nullptr) *pieceIndex = -1;
    return 0;
}

int BuildingStore::footprintAt(int x, int y) const {
    for (const PlacedBuilding& building : buildings_) {
        if (building.state == State::Rubble || isPieceKind(building.kind)) continue;
        if (x >= building.x && y >= building.y && x < building.x + building.width && y < building.y + building.height) return building.id;
    }
    return 0;
}

bool BuildingStore::remove(int id) {
    const auto found = std::lower_bound(buildings_.begin(), buildings_.end(), id, [](const PlacedBuilding& b, int key) { return b.id < key; });
    if (found == buildings_.end() || found->id != id) return false;
    unindex(*found);
    buildings_.erase(found);
    touch();
    return true;
}

// ---- what a building is

std::string BuildingStore::label(const PlacedBuilding& building) const {
    if (data_ == nullptr) return building.kind;
    if (isPieceKind(building.kind)) {
        const PieceDef* piece = data_->piece(pieceOfKind(building.kind));
        return piece != nullptr ? piece->label : building.kind;
    }
    const KindDef* kind = data_->kind(building.kind);
    return kind != nullptr ? kind->label : building.kind;
}

ItemCounts BuildingStore::cost(const PlacedBuilding& building) const {
    if (data_ == nullptr) return {};
    if (isPieceKind(building.kind)) {
        const PieceDef* piece = data_->piece(pieceOfKind(building.kind));
        return piece != nullptr ? piece->cost : ItemCounts{};
    }
    const KindDef* kind = data_->kind(building.kind);
    return kind != nullptr ? kind->cost : ItemCounts{};
}

int BuildingStore::buildMilli(const PlacedBuilding& building) const {
    if (data_ == nullptr) return 0;
    if (isPieceKind(building.kind)) {
        const PieceDef* piece = data_->piece(pieceOfKind(building.kind));
        return piece != nullptr ? piece->buildMilli : 0;
    }
    const KindDef* kind = data_->kind(building.kind);
    return kind != nullptr ? kind->buildMilli : 0;
}

std::vector<std::string> BuildingStore::uses(const PlacedBuilding& building) const {
    if (data_ == nullptr || isPieceKind(building.kind)) return {};
    const KindDef* kind = data_->kind(building.kind);
    return kind != nullptr ? kind->uses : std::vector<std::string>{};
}

std::string BuildingStore::interiorMode(const PlacedBuilding& building) const {
    if (!building.interior.empty()) return building.interior;
    if (data_ != nullptr && !isPieceKind(building.kind)) {
        if (const KindDef* kind = data_->kind(building.kind)) return interiorModeName(kind->interior);
    }
    return "fade";
}

std::string BuildingStore::interiorLevelOf(const PlacedBuilding& building) const {
    if (!building.interiorLevel.empty()) return building.interiorLevel;
    if (data_ != nullptr && !isPieceKind(building.kind)) {
        if (const KindDef* kind = data_->kind(building.kind)) return kind->interiorLevel;
    }
    return {};
}

int BuildingStore::fullHpMilli(const PieceState& piece) const {
    const PieceDef* def = pieceDef(piece);
    return def != nullptr ? def->hp * 1000 : 0;
}

int BuildingStore::condition(const PlacedBuilding& building) const {
    if (building.state == State::Rubble) return 0;
    long long have = 0;
    long long full = 0;
    for (const PieceState& piece : building.pieces) {
        if (!piece.built) continue;
        have += std::max(0, piece.hpMilli);
        full += fullHpMilli(piece);
    }
    return full == 0 ? 100 : static_cast<int>(have * 100 / full);
}

bool BuildingStore::burning(const PlacedBuilding& building) const {
    return std::ranges::any_of(building.pieces, [](const PieceState& piece) { return piece.burning; });
}

bool BuildingStore::fullyDelivered(const PlacedBuilding& building) const {
    for (const auto& [item, need] : cost(building)) {
        const auto found = building.delivered.find(item);
        if (found == building.delivered.end() || found->second < need) return false;
    }
    return true;
}

std::string BuildingStore::ruleState(const PlacedBuilding& building) const {
    if (building.state == State::Rubble) return "rubble";
    if (building.state == State::Blueprint) return fullyDelivered(building) ? "ready" : "waiting";
    if (burning(building)) return "burning";
    return condition(building) < 100 ? "damaged" : "finished";
}

std::vector<std::string> BuildingStore::tags(const PlacedBuilding& building) const {
    std::vector<std::string> out = {"building"};
    if (building.state == State::Blueprint) out.push_back("construction");
    if (building.state == State::Finished) {
        if (condition(building) < 100) out.push_back("repairable");
        for (const std::string& use : uses(building)) out.push_back(use);
    }
    if (burning(building)) out.push_back("burning");
    return out;
}

std::pair<int, int> BuildingStore::centre(const PlacedBuilding& building) const { return {building.x + building.width / 2, building.y + building.height / 2}; }

// ---- building

ItemCounts BuildingStore::deliver(int id, ItemCounts& from) {
    ItemCounts moved;
    PlacedBuilding* building = findMutable(id);
    if (building == nullptr || building->state != State::Blueprint) return moved;
    for (const auto& [item, need] : cost(*building)) {
        const int have = building->delivered.count(item) != 0 ? building->delivered[item] : 0;
        const auto stock = from.find(item);
        const int give = std::min(need - have, stock == from.end() ? 0 : stock->second);
        if (give <= 0) continue;
        building->delivered[item] = have + give;
        stock->second -= give;
        if (stock->second == 0) from.erase(stock);
        moved[item] = give;
    }
    return moved;
}

void BuildingStore::waiveCost(int id) {
    PlacedBuilding* building = findMutable(id);
    if (building == nullptr || building->state != State::Blueprint) return;
    building->delivered = cost(*building);
}

int BuildingStore::workAvailable(const PlacedBuilding& building) const {
    if (building.state != State::Blueprint) return 0;
    const ItemCounts need = cost(building);
    long long total = 0;
    long long have = 0;
    for (const auto& [item, count] : need) {
        total += count;
        const auto found = building.delivered.find(item);
        have += found == building.delivered.end() ? 0 : std::min(count, found->second);
    }
    const long long milli = buildMilli(building);
    const long long cap = total == 0 ? milli : milli * have / total;
    return static_cast<int>(std::max<long long>(0, cap - building.workMilli));
}

void BuildingStore::restage(PlacedBuilding& building) {
    const int milli = buildMilli(building);
    const std::size_t n = building.pieces.size();
    const std::size_t target = milli <= 0 ? n : std::min<std::size_t>(n, static_cast<std::size_t>(static_cast<long long>(building.workMilli) * static_cast<long long>(n) / milli));
    bool changed = false;
    for (std::size_t i = 0; i < target; ++i) {
        PieceState& piece = building.pieces[i];
        if (piece.built) continue;
        piece.built = true;
        piece.hpMilli = fullHpMilli(piece);
        changed = true;
    }
    if (changed) touch();
}

void BuildingStore::finish(PlacedBuilding& building) {
    for (PieceState& piece : building.pieces) {
        if (!piece.built) {
            piece.built = true;
            piece.hpMilli = fullHpMilli(piece);
        }
    }
    building.state = State::Finished;
    touch();
}

bool BuildingStore::work(int id, int milli) {
    PlacedBuilding* building = findMutable(id);
    if (building == nullptr || building->state != State::Blueprint || milli <= 0) return false;
    building->workMilli += std::min(milli, workAvailable(*building));
    restage(*building);
    if (building->workMilli >= buildMilli(*building) && fullyDelivered(*building)) {
        finish(*building);
        return true;
    }
    return false;
}

bool BuildingStore::cancel(int id) {
    PlacedBuilding* building = findMutable(id);
    if (building == nullptr || building->state != State::Blueprint) return false;
    if (!building->delivered.empty()) {
        const auto [cx, cy] = centre(*building);
        Drop drop{cx, cy, building->delivered};
        const auto same = std::ranges::find_if(drops_, [&](const Drop& d) { return d.x == cx && d.y == cy; });
        if (same != drops_.end()) {
            for (const auto& [item, count] : drop.items) same->items[item] += count;
        } else {
            drops_.push_back(std::move(drop));
        }
    }
    return remove(id);
}

ItemCounts BuildingStore::takeDropsNear(int x, int y, int radiusCells) {
    ItemCounts taken;
    for (auto it = drops_.begin(); it != drops_.end();) {
        if (std::abs(it->x - x) <= radiusCells && std::abs(it->y - y) <= radiusCells) {
            for (const auto& [item, count] : it->items) taken[item] += count;
            it = drops_.erase(it);
        } else {
            ++it;
        }
    }
    return taken;
}

// ---- wear, damage, fire, repair

void BuildingStore::seasonEnded(int season) {
    if (data_ == nullptr || season < 0 || season > 3) return;
    for (PlacedBuilding& building : buildings_) {
        if (building.state != State::Finished || isPieceKind(building.kind)) continue;
        const KindDef* kind = data_->kind(building.kind);
        if (kind == nullptr) continue;
        const auto found = kind->wear.find(kSeasonWords[season]);
        if (found == kind->wear.end() || found->second <= 0) continue;
        for (PieceState& piece : building.pieces) {
            if (piece.built && !piece.fell()) piece.hpMilli = std::max(1000, piece.hpMilli - found->second * 1000);
        }
    }
}

void BuildingStore::dayEnded() {
    for (std::size_t i = 0; i < buildings_.size();) {
        PlacedBuilding& building = buildings_[i];
        if (building.state == State::Rubble && ++building.rubbleDays >= 10) {
            buildings_.erase(buildings_.begin() + static_cast<std::ptrdiff_t>(i));
            touch();
        } else {
            ++i;
        }
    }
}

void BuildingStore::breakPiece(PlacedBuilding& building, PieceState& piece) {
    piece.hpMilli = 0;
    piece.burning = false;
    piece.burnTicks = 0;
    if (const PieceDef* def = pieceDef(piece); def != nullptr && inside(piece.x, piece.y)) {
        Occupant& at = occupant(layerOf(def->type), piece.x, piece.y);
        if (at.building == building.id) at = {};
    }
    touch();
    updateRubble(building);
}

void BuildingStore::updateRubble(PlacedBuilding& building) {
    if (building.state != State::Finished) return;
    if (std::ranges::all_of(building.pieces, [](const PieceState& piece) { return piece.fell(); })) {
        building.state = State::Rubble;
        building.rubbleDays = 0;
    }
}

bool BuildingStore::damagePiece(int id, std::size_t pieceIndex, int amount, bool* fell) {
    if (fell != nullptr) *fell = false;
    PlacedBuilding* building = findMutable(id);
    if (building == nullptr || building->state != State::Finished || pieceIndex >= building->pieces.size() || amount <= 0) return false;
    PieceState& piece = building->pieces[pieceIndex];
    if (!piece.built || piece.fell()) return false;
    piece.hpMilli -= amount * 1000;
    if (piece.hpMilli <= 0) {
        breakPiece(*building, piece);
        if (fell != nullptr) *fell = true;
    }
    return true;
}

bool BuildingStore::damageAt(int x, int y, int amount, bool* fell) {
    if (fell != nullptr) *fell = false;
    if (!inside(x, y)) return false;
    for (const Layer layer : {Layer::Wall, Layer::Roof, Layer::Floor}) {
        const Occupant& at = occupant(layer, x, y);
        if (at.building == 0) continue;
        const PlacedBuilding* building = find(at.building);
        if (building == nullptr || !building->pieces[static_cast<std::size_t>(at.piece)].built) continue;
        return damagePiece(at.building, static_cast<std::size_t>(at.piece), amount, fell);
    }
    return false;
}

bool BuildingStore::ignitePiece(int id, std::size_t pieceIndex) {
    PlacedBuilding* building = findMutable(id);
    if (building == nullptr || building->state != State::Finished || data_ == nullptr || pieceIndex >= building->pieces.size()) return false;
    PieceState& piece = building->pieces[pieceIndex];
    const PieceDef* def = pieceDef(piece);
    if (def == nullptr || !piece.built || piece.fell() || piece.burning) return false;
    const MaterialFire* fire = data_->material(def->material);
    if (fire == nullptr || fire->burnSeconds <= 0) return false;
    piece.burning = true;
    piece.burnTicks = 0;
    return true;
}

bool BuildingStore::igniteAt(int x, int y) {
    if (!inside(x, y)) return false;
    for (const Layer layer : {Layer::Wall, Layer::Roof, Layer::Floor}) {
        const Occupant& at = occupant(layer, x, y);
        if (at.building != 0 && ignitePiece(at.building, static_cast<std::size_t>(at.piece))) return true;
    }
    return false;
}

void BuildingStore::advance(const StepEnv& env) {
    if (data_ == nullptr) return;
    bool any = false;
    for (PlacedBuilding& building : buildings_) {
        for (PieceState& piece : building.pieces) {
            if (!piece.burning) continue;
            any = true;
            const PieceDef* def = pieceDef(piece);
            const MaterialFire* fire = def != nullptr ? data_->material(def->material) : nullptr;
            const int total = fire != nullptr ? std::max(1, fire->burnSeconds * kTicksPerSecond) : 1;
            ++piece.burnTicks;
            const int loss = (fullHpMilli(piece) + total - 1) / total;
            piece.hpMilli -= loss;
            if (piece.hpMilli <= 0 || piece.burnTicks >= total) breakPiece(building, piece);
        }
    }
    if (!any) {
        fireSecondTicks_ = 0;
        return;
    }
    if (++fireSecondTicks_ >= kTicksPerSecond) {
        fireSecondTicks_ = 0;
        spreadFire(env);
    }
}

void BuildingStore::spreadFire(const StepEnv& env) {
    struct Ref {
        int building;
        std::size_t piece;
    };
    std::vector<Ref> burningNow;
    for (const PlacedBuilding& building : buildings_) {
        for (std::size_t i = 0; i < building.pieces.size(); ++i) {
            if (building.pieces[i].burning) burningNow.push_back({building.id, i});
        }
    }
    const int rain = std::clamp(env.rainPercent, 0, 100);
    for (const Ref& ref : burningNow) {
        PlacedBuilding* building = findMutable(ref.building);
        if (building == nullptr) continue;
        PieceState& source = building->pieces[ref.piece];
        if (!source.burning) continue; // fell or went out meanwhile
        if (rain > 0 && random_.chance(static_cast<std::uint32_t>(rain / 4))) {
            source.burning = false;
            source.burnTicks = 0;
            continue;
        }
        const int sx = source.x;
        const int sy = source.y;
        static constexpr int kSteps[5][2] = {{0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        for (const auto& step : kSteps) {
            const int tx = sx + step[0];
            const int ty = sy + step[1];
            if (!inside(tx, ty)) continue;
            for (const Layer layer : {Layer::Wall, Layer::Roof, Layer::Floor}) {
                const Occupant& at = occupant(layer, tx, ty);
                if (at.building == 0) continue;
                PlacedBuilding* target = findMutable(at.building);
                if (target == nullptr) continue;
                PieceState& piece = target->pieces[static_cast<std::size_t>(at.piece)];
                if (piece.burning || !piece.built || piece.fell()) continue;
                const PieceDef* def = pieceDef(piece);
                const MaterialFire* fire = def != nullptr ? data_->material(def->material) : nullptr;
                if (fire == nullptr || fire->burnSeconds <= 0 || fire->flammability <= 0) continue;
                const int chance = fire->flammability * (100 - rain * 70 / 100) / 100;
                if (random_.chance(static_cast<std::uint32_t>(std::max(1, chance)))) {
                    piece.burning = true;
                    piece.burnTicks = 0;
                }
            }
        }
    }
}

bool BuildingStore::douse(int id, int cx, int cy) {
    PlacedBuilding* building = findMutable(id);
    if (building == nullptr) return false;
    PieceState* best = nullptr;
    int bestDistance = 0;
    for (PieceState& piece : building->pieces) {
        if (!piece.burning) continue;
        const int distance = std::abs(piece.x - cx) + std::abs(piece.y - cy);
        if (best == nullptr || distance < bestDistance) {
            best = &piece;
            bestDistance = distance;
        }
    }
    if (best == nullptr) return false;
    best->burning = false;
    best->burnTicks = 0;
    return true;
}

bool BuildingStore::repair(int id, int hp) {
    PlacedBuilding* building = findMutable(id);
    if (building == nullptr || building->state != State::Finished || hp <= 0) return false;
    PieceState* worst = nullptr;
    long long worstShare = 1000000;
    for (PieceState& piece : building->pieces) {
        if (!piece.built || piece.fell()) continue;
        const int full = fullHpMilli(piece);
        if (full <= 0 || piece.hpMilli >= full) continue;
        const long long share = static_cast<long long>(piece.hpMilli) * 1000 / full;
        if (share < worstShare) {
            worstShare = share;
            worst = &piece;
        }
    }
    if (worst == nullptr) return false;
    worst->hpMilli = std::min(fullHpMilli(*worst), worst->hpMilli + hp * 1000);
    return true;
}

bool BuildingStore::anyBurning() const {
    return std::ranges::any_of(buildings_, [this](const PlacedBuilding& b) { return burning(b); });
}

// ---- the world the buildings make

std::vector<BuildingStore::Obstacle> BuildingStore::obstacles() const {
    std::vector<Obstacle> out;
    for (const PlacedBuilding& building : buildings_) {
        if (building.state != State::Finished) continue; // a blueprint is only a ghost until it is complete (D-55 Q3)
        for (const PieceState& piece : building.pieces) {
            const PieceDef* def = pieceDef(piece);
            if (def == nullptr || !piece.built || piece.fell() || !blocksWalking(def->type)) continue;
            out.push_back({piece.x, piece.y, def->heightMilli});
        }
    }
    return out;
}

bool BuildingStore::hasFinishedPiece(int x, int y, PieceType type) const {
    if (!inside(x, y)) return false;
    const Occupant& at = occupant(layerOf(type), x, y);
    if (at.building == 0) return false;
    const PlacedBuilding* building = find(at.building);
    if (building == nullptr || building->state != State::Finished) return false;
    const PieceState& piece = building->pieces[static_cast<std::size_t>(at.piece)];
    const PieceDef* def = pieceDef(piece);
    return def != nullptr && def->type == type && piece.built && !piece.fell();
}

void BuildingStore::findRooms() const {
    rooms_.clear();
    roomOfCell_.assign(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), -1);
    roomsVersion_ = version_;
    if (data_ == nullptr || width_ == 0) return;
    const std::size_t cells = static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
    std::vector<unsigned char> barrier(cells, 0);
    std::vector<unsigned char> door(cells, 0);
    std::vector<int> barrierBuilding(cells, 0);
    std::vector<std::size_t> barrierCells;
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            const Occupant& at = occupant(Layer::Wall, x, y);
            if (at.building == 0) continue;
            const PlacedBuilding* building = find(at.building);
            if (building == nullptr || building->state != State::Finished) continue;
            const PieceState& piece = building->pieces[static_cast<std::size_t>(at.piece)];
            const PieceDef* def = pieceDef(piece);
            if (def == nullptr || !piece.built || piece.fell() || !closesRoom(def->type)) continue;
            const std::size_t i = static_cast<std::size_t>(y * width_ + x);
            barrier[i] = 1;
            door[i] = def->type == PieceType::Door ? 1 : 0;
            barrierBuilding[i] = building->id;
            barrierCells.push_back(i);
        }
    }
    const auto roofed = [this](int x, int y) { return hasFinishedPiece(x, y, PieceType::Roof); };
    std::vector<unsigned char> done(cells, 0);
    std::vector<int> stamp(cells, -1);
    int pass = 0;
    static constexpr int kSteps[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (const std::size_t barrierIndex : barrierCells) {
        const int bx = static_cast<int>(barrierIndex % static_cast<std::size_t>(width_));
        const int by = static_cast<int>(barrierIndex / static_cast<std::size_t>(width_));
        for (const auto& step : kSteps) {
            const int sx = bx + step[0];
            const int sy = by + step[1];
            if (!inside(sx, sy)) continue;
            const std::size_t seed = static_cast<std::size_t>(sy * width_ + sx);
            if (barrier[seed] != 0 || done[seed] != 0) continue;
            // A breadth-first fill; it gives up (open) at the map's edge or past the size limit.
            ++pass;
            std::vector<std::pair<int, int>> region;
            std::queue<std::pair<int, int>> frontier;
            frontier.emplace(sx, sy);
            stamp[seed] = pass;
            bool open = false;
            while (!frontier.empty() && !open) {
                const auto [cx, cy] = frontier.front();
                frontier.pop();
                region.emplace_back(cx, cy);
                if (static_cast<int>(region.size()) > data_->maxRoomCells()) {
                    open = true;
                    break;
                }
                for (const auto& next : kSteps) {
                    const int nx = cx + next[0];
                    const int ny = cy + next[1];
                    if (!inside(nx, ny)) {
                        open = true;
                        break;
                    }
                    const std::size_t at = static_cast<std::size_t>(ny * width_ + nx);
                    if (barrier[at] != 0 || stamp[at] == pass) continue;
                    stamp[at] = pass;
                    frontier.emplace(nx, ny);
                }
            }
            if (open) continue;
            for (const auto& [cx, cy] : region) done[static_cast<std::size_t>(cy * width_ + cx)] = 1;
            Room room;
            room.cells = region;
            std::sort(room.cells.begin(), room.cells.end());
            bool allRoofed = true;
            std::vector<int> closers;
            std::vector<std::size_t> doors;
            for (const auto& [cx, cy] : room.cells) {
                if (!roofed(cx, cy)) allRoofed = false;
                for (const auto& next : kSteps) {
                    const std::size_t at = static_cast<std::size_t>((cy + next[1]) * width_ + (cx + next[0]));
                    if (barrier[at] == 0) continue;
                    closers.push_back(barrierBuilding[at]);
                    if (door[at] != 0) doors.push_back(at);
                }
            }
            std::sort(doors.begin(), doors.end());
            doors.erase(std::unique(doors.begin(), doors.end()), doors.end());
            room.doors = static_cast<int>(doors.size());
            if (!allRoofed || room.doors == 0) continue;
            std::sort(closers.begin(), closers.end());
            closers.erase(std::unique(closers.begin(), closers.end()), closers.end());
            room.buildings = std::move(closers);
            const int roomIndex = static_cast<int>(rooms_.size());
            for (const auto& [cx, cy] : room.cells) roomOfCell_[static_cast<std::size_t>(cy * width_ + cx)] = roomIndex;
            rooms_.push_back(std::move(room));
        }
    }
    std::sort(rooms_.begin(), rooms_.end(), [](const Room& a, const Room& b) { return a.cells.front() < b.cells.front(); });
    std::fill(roomOfCell_.begin(), roomOfCell_.end(), -1);
    for (std::size_t r = 0; r < rooms_.size(); ++r) {
        for (const auto& [cx, cy] : rooms_[r].cells) roomOfCell_[static_cast<std::size_t>(cy * width_ + cx)] = static_cast<int>(r);
    }
}

const std::vector<Room>& BuildingStore::rooms() const {
    if (roomsVersion_ != version_) findRooms();
    return rooms_;
}

int BuildingStore::roomAt(int x, int y) const {
    if (!inside(x, y)) return -1;
    if (roomsVersion_ != version_) findRooms();
    return roomOfCell_[static_cast<std::size_t>(y * width_ + x)];
}

bool BuildingStore::sheltered(int x, int y) const {
    if (roomAt(x, y) >= 0) return true;
    for (const PlacedBuilding& building : buildings_) {
        if (building.state != State::Finished || isPieceKind(building.kind)) continue;
        if (x < building.x - 1 || y < building.y - 1 || x > building.x + building.width || y > building.y + building.height) continue;
        const std::vector<std::string> kindUses = uses(building);
        if (std::ranges::find(kindUses, "shelter") != kindUses.end()) return true;
    }
    return false;
}

std::vector<const PlacedBuilding*> BuildingStore::withUse(const std::string& use, int faction) const {
    std::vector<const PlacedBuilding*> out;
    for (const PlacedBuilding& building : buildings_) {
        if (building.state != State::Finished || (faction >= 0 && building.faction != faction)) continue;
        const std::vector<std::string> kindUses = uses(building);
        if (std::ranges::find(kindUses, use) != kindUses.end()) out.push_back(&building);
    }
    return out;
}

int BuildingStore::deposit(int id, const std::string& item, int count) {
    PlacedBuilding* building = findMutable(id);
    if (building == nullptr || building->state != State::Finished || count <= 0) return 0;
    building->contents[item] += count;
    return count;
}

int BuildingStore::withdraw(int id, const std::string& item, int count) {
    PlacedBuilding* building = findMutable(id);
    if (building == nullptr || count <= 0) return 0;
    const auto found = building->contents.find(item);
    if (found == building->contents.end()) return 0;
    const int taken = std::min(count, found->second);
    found->second -= taken;
    if (found->second == 0) building->contents.erase(found);
    return taken;
}

// ---- saving

std::string BuildingStore::toJson() const {
    nlohmann::json root;
    root["version"] = 1;
    root["nextId"] = nextId_;
    root["fireTicks"] = fireSecondTicks_;
    root["random"] = {{"state", std::to_string(random_.state())}, {"increment", std::to_string(random_.increment())}};
    nlohmann::json list = nlohmann::json::array();
    for (const PlacedBuilding& b : buildings_) {
        nlohmann::json entry;
        entry["id"] = b.id;
        entry["kind"] = b.kind;
        entry["x"] = b.x;
        entry["y"] = b.y;
        entry["turns"] = b.turns;
        entry["state"] = stateName(b.state);
        entry["work"] = b.workMilli;
        if (!b.delivered.empty()) entry["delivered"] = countsToJson(b.delivered);
        entry["faction"] = b.faction;
        entry["owner"] = b.owner;
        if (!b.interior.empty()) entry["interior"] = b.interior;
        if (!b.interiorLevel.empty()) entry["interiorLevel"] = b.interiorLevel;
        if (!b.contents.empty()) entry["contents"] = countsToJson(b.contents);
        if (b.rubbleDays != 0) entry["rubbleDays"] = b.rubbleDays;
        nlohmann::json pieces = nlohmann::json::array();
        for (const PieceState& p : b.pieces) {
            nlohmann::json piece = {{"piece", p.piece}, {"x", p.x}, {"y", p.y}, {"hp", p.hpMilli}, {"built", p.built}};
            if (p.burning) {
                piece["burning"] = true;
                piece["burn"] = p.burnTicks;
            }
            pieces.push_back(std::move(piece));
        }
        entry["pieces"] = std::move(pieces);
        list.push_back(std::move(entry));
    }
    root["buildings"] = std::move(list);
    nlohmann::json drops = nlohmann::json::array();
    for (const Drop& d : drops_) drops.push_back({{"x", d.x}, {"y", d.y}, {"items", countsToJson(d.items)}});
    root["drops"] = std::move(drops);
    return root.dump(1, '\t') + "\n";
}

bool BuildingStore::fromJson(const std::string& text, std::string& problem) {
    nlohmann::json root = nlohmann::json::parse(text, nullptr, false);
    if (root.is_discarded() || !root.is_object() || !root.contains("buildings") || !root["buildings"].is_array()) {
        problem = "buildings.json: not a building list";
        return false;
    }
    std::vector<PlacedBuilding> loaded;
    for (const nlohmann::json& entry : root["buildings"]) {
        if (!entry.is_object() || !entry.contains("id") || !entry.contains("kind") || !entry.contains("pieces") || !entry["pieces"].is_array()) {
            problem = "buildings.json: a building lacks id, kind or pieces";
            return false;
        }
        PlacedBuilding b;
        b.id = entry.value("id", 0);
        b.kind = entry.value("kind", std::string());
        b.x = entry.value("x", 0);
        b.y = entry.value("y", 0);
        b.turns = entry.value("turns", 0);
        const std::string state = entry.value("state", std::string("blueprint"));
        b.state = state == "finished" ? State::Finished : (state == "rubble" ? State::Rubble : State::Blueprint);
        b.workMilli = entry.value("work", 0);
        if (entry.contains("delivered")) b.delivered = countsFromJson(entry["delivered"]);
        b.faction = entry.value("faction", 0);
        b.owner = entry.value("owner", -1);
        b.interior = entry.value("interior", std::string());
        b.interiorLevel = entry.value("interiorLevel", std::string());
        if (entry.contains("contents")) b.contents = countsFromJson(entry["contents"]);
        b.rubbleDays = entry.value("rubbleDays", 0);
        for (const nlohmann::json& p : entry["pieces"]) {
            PieceState piece;
            piece.piece = p.value("piece", std::string());
            piece.x = p.value("x", 0);
            piece.y = p.value("y", 0);
            piece.hpMilli = p.value("hp", 0);
            piece.built = p.value("built", false);
            piece.burning = p.value("burning", false);
            piece.burnTicks = p.value("burn", 0);
            if (data_ != nullptr && data_->piece(piece.piece) == nullptr) {
                problem = std::format("buildings.json: building {} has the unknown piece \"{}\"", b.id, piece.piece);
                return false;
            }
            b.pieces.push_back(std::move(piece));
        }
        const Cell size = turnedSize(1, 1, 0);
        b.width = size.x;
        b.height = size.y;
        if (data_ != nullptr && !isPieceKind(b.kind)) {
            if (const KindDef* kind = data_->kind(b.kind)) {
                const Cell turned = turnedSize(kind->width, kind->height, b.turns);
                b.width = turned.x;
                b.height = turned.y;
            }
        }
        loaded.push_back(std::move(b));
    }
    std::sort(loaded.begin(), loaded.end(), [](const PlacedBuilding& a, const PlacedBuilding& b) { return a.id < b.id; });
    buildings_ = std::move(loaded);
    drops_.clear();
    if (root.contains("drops") && root["drops"].is_array()) {
        for (const nlohmann::json& d : root["drops"]) drops_.push_back({d.value("x", 0), d.value("y", 0), d.contains("items") ? countsFromJson(d["items"]) : ItemCounts{}});
    }
    nextId_ = root.value("nextId", 1);
    for (const PlacedBuilding& b : buildings_) nextId_ = std::max(nextId_, b.id + 1);
    fireSecondTicks_ = root.value("fireTicks", 0);
    if (root.contains("random") && root["random"].is_object()) {
        random_.restore(std::stoull(root["random"].value("state", std::string("0"))), std::stoull(root["random"].value("increment", std::string("1"))));
    }
    rebuildGrid();
    touch();
    return true;
}

std::uint64_t BuildingStore::hash() const {
    std::uint64_t h = 0xCBF29CE484222325ULL;
    mix(h, buildings_.size());
    for (const PlacedBuilding& b : buildings_) {
        mix(h, static_cast<std::uint64_t>(b.id));
        mixText(h, b.kind);
        mix(h, static_cast<std::uint64_t>(b.x));
        mix(h, static_cast<std::uint64_t>(b.y));
        mix(h, static_cast<std::uint64_t>(b.turns));
        mix(h, static_cast<std::uint64_t>(b.state));
        mix(h, static_cast<std::uint64_t>(b.workMilli));
        mixCounts(h, b.delivered);
        mix(h, static_cast<std::uint64_t>(b.faction));
        mix(h, static_cast<std::uint64_t>(b.owner + 1));
        mixText(h, b.interior);
        mixCounts(h, b.contents);
        for (const PieceState& p : b.pieces) {
            mixText(h, p.piece);
            mix(h, static_cast<std::uint64_t>(p.x));
            mix(h, static_cast<std::uint64_t>(p.y));
            mix(h, static_cast<std::uint64_t>(p.hpMilli));
            mix(h, (p.built ? 1U : 0U) | (p.burning ? 2U : 0U));
            mix(h, static_cast<std::uint64_t>(p.burnTicks));
        }
    }
    mix(h, drops_.size());
    for (const Drop& d : drops_) {
        mix(h, static_cast<std::uint64_t>(d.x));
        mix(h, static_cast<std::uint64_t>(d.y));
        mixCounts(h, d.items);
    }
    mix(h, random_.state());
    return h;
}

} // namespace odysseus::sim::buildings
