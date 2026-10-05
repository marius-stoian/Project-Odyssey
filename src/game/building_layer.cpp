#include "game/building_layer.h"

#include "game/odyssey_game.h"
#include "game/weather.h"

#include "core/log.h"
#include "luna/engine/ui.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <format>

namespace odysseus::game {

namespace {

using luna::engine::Rect;
using luna::engine::UiColor;
using sim::buildings::Layer;
using sim::buildings::PieceType;

constexpr int kArtCellWidth = kTileSize;
constexpr int kArtCellHeight = 64; // a piece is drawn bottom-aligned on its cell, up to 64 pixels tall
constexpr int kSwatches = 4;       // after the pieces: green, red, dark, gold
constexpr int kGreen = 0;
constexpr int kRed = 1;
constexpr int kGhostAlpha = 110;

constexpr int kPanelX = 6;
constexpr int kPanelY = 28;
constexpr int kPanelWidth = 176;
constexpr int kRowHeight = 20;
constexpr int kRows = 8;
constexpr int kClanCarry = 2;   // what a clan member brings of each missing item in one go (US-253)
constexpr int kAdultYears = 15;   // a person this old may own a hut (US-257)
constexpr int kBedsPerSleepingPlace = 4;
constexpr int kRaidRelation = -50; // a rival at or below this is at war or at feud with the hero (US-255)
constexpr int kRaidDamage = 40;     // hit points of one raid
constexpr int kListTop = 32;

luna::engine::Color shade(int rgb, int delta) {
    const auto channel = [&](int shift) { return static_cast<std::uint8_t>(std::clamp(((rgb >> shift) & 0xFF) + delta, 0, 255)); };
    return {channel(16), channel(8), channel(0), 255};
}

// The placeholder picture of a piece, drawn in the bottom of a 32 x 64 cell (the owner's sheets replace these later).
void paintPiece(luna::engine::Image& image, int cell, const sim::buildings::PieceDef& piece) {
    const int left = cell * kArtCellWidth;
    const int bottom = kArtCellHeight - 1;
    const int rise = std::clamp(piece.heightMilli * 12 / 1000, 10, 30); // pixels a wall's face rises
    const auto fill = [&](int x, int y, int w, int h, luna::engine::Color color) { image.fillRect(left + x, y, w, h, color); };
    const luna::engine::Color base = shade(piece.colour, 0);
    switch (piece.type) {
    case PieceType::Floor:
        fill(0, kArtCellHeight - kArtCellWidth, kArtCellWidth, kArtCellWidth, base);
        for (int i = 0; i < kArtCellWidth; i += 8) fill(i, kArtCellHeight - kArtCellWidth, 1, kArtCellWidth, shade(piece.colour, -18));
        fill(0, bottom, kArtCellWidth, 1, shade(piece.colour, -40));
        break;
    case PieceType::Roof:
        fill(0, kArtCellHeight - kArtCellWidth, kArtCellWidth, kArtCellWidth, base);
        for (int i = 3; i < kArtCellWidth; i += 6) fill(0, kArtCellHeight - kArtCellWidth + i, kArtCellWidth, 1, shade(piece.colour, 22));
        fill(0, kArtCellHeight - kArtCellWidth, kArtCellWidth, 2, shade(piece.colour, -35));
        fill(0, bottom - 1, kArtCellWidth, 2, shade(piece.colour, -35));
        break;
    case PieceType::Post:
        fill(13, bottom - rise - 2, 6, rise + 3, base);
        fill(12, bottom - rise - 4, 8, 3, shade(piece.colour, 30));
        fill(13, bottom, 6, 1, shade(piece.colour, -40));
        break;
    case PieceType::Fence:
        for (int stake = 0; stake < 4; ++stake) {
            fill(2 + stake * 8, bottom - rise, 4, rise + 1, base);
            fill(3 + stake * 8, bottom - rise - 2, 2, 2, shade(piece.colour, 30));
        }
        break;
    default: // wall, door, window: a face and a lighter top edge
        fill(0, bottom - rise, kArtCellWidth, rise + 1, base);
        fill(0, bottom - rise - 5, kArtCellWidth, 5, shade(piece.colour, 28));
        for (int i = 0; i < kArtCellWidth; i += 8) fill(i, bottom - rise, 1, rise + 1, shade(piece.colour, -14));
        fill(0, bottom, kArtCellWidth, 1, shade(piece.colour, -40));
        if (piece.type == PieceType::Door) fill(9, bottom - rise + 4, 14, rise - 3, shade(piece.colour, -55));
        if (piece.type == PieceType::Window) fill(9, bottom - rise + 5, 14, 9, {150, 190, 220, 255});
        break;
    }
}

int rainPercentOf(const std::string& weather) {
    const auto has = [&](const char* word) { return weather.find(word) != std::string::npos; };
    if (has("rainbow")) return 0;
    if (has("heavy rain") || has("storm") || has("monsoon") || has("squall") || has("hurricane")) return 100;
    if (has("rain") || has("sleet")) return 60;
    if (has("drizzle") || has("shower") || has("mist")) return 30;
    return 0;
}

bool inside(const Rect& r, int x, int y) { return x >= r.x && y >= r.y && x < r.x + r.width && y < r.y + r.height; }

} // namespace

// ---- data

void BuildingLayer::loadData(const std::filesystem::path& dataDirectory, const std::set<std::string>& knownItems) {
    sim::rules::LoadReport report;
    data_ = sim::buildings::BuildingData::load(dataDirectory / "buildings", report, knownItems);
    notes_.clear();
    for (const sim::rules::Diagnostic& error : report.errors) notes_.push_back(error.text());
    store_.setData(&data_);
    forgetAll();
}

void BuildingLayer::forgetAll() {
    known_.clear();
    for (const sim::buildings::KindDef& kind : data_.kinds()) {
        if (kind.known) known_.insert(kind.id);
    }
}

bool BuildingLayer::knows(const std::string& kind) const {
    if (kind.rfind("piece:", 0) == 0) return data_.piece(kind.substr(6)) != nullptr;
    return known_.count(kind) != 0;
}

void BuildingLayer::learn(const std::string& kind) {
    if (data_.kind(kind) != nullptr) known_.insert(kind);
}

std::pair<int, int> BuildingLayer::cellOf(double worldX, double worldY) {
    return {static_cast<int>(std::floor(worldX / kTileSize)), static_cast<int>(std::floor(worldY / kTileSize))};
}

// ---- a level starts

// How many living people each rival clan has (US-253).
static std::vector<int> rivalPeople(const OdysseyGame& game) {
    std::vector<int> people;
    if (game.rivals() == nullptr) return people;
    for (const sim::RivalClan& clan : game.rivals()->clans()) {
        int n = 0;
        if (clan.world != nullptr) {
            for (const sim::Person& person : clan.world->people()) n += person.alive && !person.exiled ? 1 : 0;
        }
        people.push_back(n);
    }
    return people;
}

void BuildingLayer::start(OdysseyGame& game) {
    const luna::engine::TileMap& map = game.tileMap();
    store_.setData(&data_);
    store_.reset(map.width(), map.height(), WeatherCycle::seedFromText(game.level().name) ^ 0xB11DULL);
    obstacleCells_.clear();
    obstacleVersion_ = 0;
    selected_.clear();
    menuOpen_ = false;
    dayAtLastTick_ = ~0ULL;
    seasonAtLastTick_ = -1;
    rivalBuilders_.reset(game.rivals() != nullptr ? static_cast<int>(game.rivals()->clans().size()) : 0, WeatherCycle::seedFromText(game.level().name) ^ 0xB11D2ULL);
    for (const PlacedBuildingSpec& spec : game.level().buildings) {
        const auto placed = store_.place(spec.kind, spec.x, spec.y, spec.turns, 0, spec.finished, {});
        if (!placed.problem.empty()) {
            core::logWarning(std::format("Buildings: the {} at {}, {} was not placed: {}", spec.kind, spec.x, spec.y, placed.problem));
            continue;
        }
        sim::buildings::PlacedBuilding* building = store_.findMutable(placed.id);
        building->owner = spec.owner;
        building->interior = spec.interior;
        building->interiorLevel = spec.interiorLevel;
    }
    syncObstacles(game);
}

void BuildingLayer::syncObstacles(OdysseyGame& game) {
    if (store_.version() == obstacleVersion_) return;
    obstacleVersion_ = store_.version();
    luna::engine::TileMap& map = game.tileMapMutable();
    for (const auto& [x, y] : obstacleCells_) map.setObstacle(x, y, 0.0);
    obstacleCells_.clear();
    for (const sim::buildings::BuildingStore::Obstacle& obstacle : store_.obstacles()) {
        map.setObstacle(obstacle.x, obstacle.y, std::max(0.1, obstacle.heightMilli / 1000.0));
        obstacleCells_.emplace_back(obstacle.x, obstacle.y);
    }
}

// ---- the Build menu and placing

std::vector<std::string> BuildingLayer::menuKinds(bool pieces) const {
    std::vector<std::string> out;
    if (pieces) {
        for (const sim::buildings::PieceDef& piece : data_.pieces()) out.push_back("piece:" + piece.id);
    } else {
        for (const sim::buildings::KindDef& kind : data_.kinds()) {
            if (kind.buildable && knows(kind.id)) out.push_back(kind.id);
        }
    }
    return out;
}

bool BuildingLayer::escape() {
    if (!selected_.empty()) {
        selected_.clear();
        return true;
    }
    if (menuOpen_) {
        menuOpen_ = false;
        return true;
    }
    return false;
}

Rect BuildingLayer::panelRect(const OdysseyGame& /*game*/) const {
    const int rows = std::min<int>(kRows, std::max<int>(1, static_cast<int>(menuKinds(piecesTab_).size())));
    return {kPanelX, kPanelY, kPanelWidth, kListTop + rows * kRowHeight + 34};
}

bool BuildingLayer::capturesPointer(const luna::engine::Pointer& uiPointer) const {
    if (placing()) return true;
    return menuOpen_ && uiPointer.inside() && uiPointer.x >= kPanelX && uiPointer.y >= kPanelY && uiPointer.x < kPanelX + kPanelWidth &&
           uiPointer.y < kPanelY + kListTop + kRows * kRowHeight + 34;
}

int BuildingLayer::tabAt(const OdysseyGame& /*game*/, int uiX, int uiY) const {
    if (uiY < kPanelY + 14 || uiY >= kPanelY + 28 || uiX < kPanelX + 4 || uiX >= kPanelX + kPanelWidth - 4) return -1;
    return uiX < kPanelX + kPanelWidth / 2 ? 0 : 1;
}

int BuildingLayer::itemIndexAt(const OdysseyGame& /*game*/, int uiX, int uiY) const {
    if (uiX < kPanelX || uiX >= kPanelX + kPanelWidth || uiY < kPanelY + kListTop) return -1;
    const int row = (uiY - kPanelY - kListTop) / kRowHeight;
    const std::size_t index = static_cast<std::size_t>(row + scroll_);
    return row >= 0 && row < kRows && index < menuKinds(piecesTab_).size() ? static_cast<int>(index) : -1;
}

std::pair<int, int> BuildingLayer::anchorFor(const std::string& kind, int cellX, int cellY, int turns) const {
    const auto [w, h] = store_.sizeOf(kind, turns);
    return {cellX - w / 2, cellY - h / 2};
}

bool BuildingLayer::blockedForBuilding(const OdysseyGame& game, int x, int y) const {
    if (game.tileMap().isSolid(x, y)) return true; // rock, water
    for (const WorldPlant& plant : game.plants()) {
        if (!plant.present()) continue;
        const PixelPoint cell = plantCell(plant.feet);
        if (cell.x == x && cell.y == y) return true; // a tree, a rock, a fire pit stands there
    }
    return false;
}

std::string BuildingLayer::whyNot(const OdysseyGame& game, const std::string& kind, int cellX, int cellY, int turns) const {
    const auto blocked = [&](int x, int y) { return blockedForBuilding(game, x, y); };
    if (std::string why = store_.whyNot(kind, cellX, cellY, turns, blocked); !why.empty()) return why;
    const auto [heroX, heroY] = cellOf(game.hero().feetX(), game.hero().feetY());
    for (const auto& [x, y] : store_.blockingCells(kind, cellX, cellY, turns)) {
        if (x == heroX && y == heroY) return "You are standing in the way.";
    }
    return {};
}

bool BuildingLayer::heroInTheWay(const OdysseyGame& game, const sim::buildings::PlacedBuilding& building) const {
    const auto [heroX, heroY] = cellOf(game.hero().feetX(), game.hero().feetY());
    for (const sim::buildings::PieceState& piece : building.pieces) {
        const sim::buildings::PieceDef* def = data_.piece(piece.piece);
        if (def != nullptr && sim::buildings::blocksWalking(def->type) && piece.x == heroX && piece.y == heroY) return true;
    }
    return false;
}

std::string BuildingLayer::placeSelected(OdysseyGame& game, int cellX, int cellY) {
    if (selected_.empty()) return "Nothing chosen.";
    if (std::string why = whyNot(game, selected_, cellX, cellY, turns_); !why.empty()) return why;
    const auto blocked = [&](int x, int y) { return blockedForBuilding(game, x, y); };
    const auto placed = store_.place(selected_, cellX, cellY, turns_, 0, false, blocked);
    if (!placed.problem.empty()) return placed.problem;
    if (game.life() == nullptr) store_.waiveCost(placed.id); // no run, no bag: materials are free
    const sim::buildings::PlacedBuilding* building = store_.find(placed.id);
    game.showMessage(std::format("{}: blueprint placed. Right-click it to bring materials and build.", store_.label(*building)));
    return {};
}

std::string BuildingLayer::costText(const OdysseyGame& game, const sim::ItemCounts& cost) const {
    std::string out;
    for (const auto& [item, count] : cost) {
        const sim::Item* known = game.heroData() != nullptr ? game.heroData()->item(item) : nullptr;
        out += std::format("{}{} {}", out.empty() ? "" : ", ", count, known != nullptr ? known->name : item);
    }
    return out.empty() ? "free" : out;
}

void BuildingLayer::updatePointer(OdysseyGame& game, const luna::engine::Intents& world, const luna::engine::Intents& ui) {
    const luna::engine::Pointer& uiPointer = ui.pointer();
    const luna::engine::Pointer& worldPointer = world.pointer();
    const bool onPanel = menuOpen_ && uiPointer.inside() && inside(panelRect(game), uiPointer.x, uiPointer.y);
    hoverRow_ = -1;
    if (menuOpen_ && onPanel) {
        const std::vector<std::string> kinds = menuKinds(piecesTab_);
        const int maxScroll = std::max(0, static_cast<int>(kinds.size()) - kRows);
        scroll_ = std::clamp(scroll_ - uiPointer.wheel, 0, maxScroll);
        hoverRow_ = itemIndexAt(game, uiPointer.x, uiPointer.y);
        if (uiPointer.wasPressed(luna::engine::PointerButton::Left)) {
            if (const int tab = tabAt(game, uiPointer.x, uiPointer.y); tab >= 0) {
                setPiecesTab(tab == 1);
            } else if (hoverRow_ >= 0) {
                selected_ = kinds[static_cast<std::size_t>(hoverRow_)];
            }
        }
    }
    if (!placing()) return;
    if (world.pressed(luna::engine::Intent::Rotate)) turns_ = (turns_ + 1) % 4;
    if (worldPointer.inside() && !onPanel) {
        const luna::engine::Rect view = game.cameraView();
        const auto [cx, cy] = cellOf(view.x + worldPointer.x, view.y + worldPointer.y);
        const auto [ax, ay] = anchorFor(selected_, cx, cy, turns_);
        ghostX_ = ax;
        ghostY_ = ay;
        ghostProblem_ = whyNot(game, selected_, ax, ay, turns_);
        if (worldPointer.wasPressed(luna::engine::PointerButton::Left)) {
            if (ghostProblem_.empty()) {
                if (const std::string problem = placeSelected(game, ax, ay); !problem.empty()) game.showMessage(problem);
            } else {
                game.showMessage(ghostProblem_);
            }
        }
    } else {
        ghostProblem_ = "outside";
    }
}

// ---- one tick

void BuildingLayer::tick(OdysseyGame& game, const luna::engine::Intents& world, const luna::engine::Intents& ui) {
    if (world.pressed(luna::engine::Intent::Build)) {
        if (menuOpen_) closeMenu();
        else openMenu();
    }
    updatePointer(game, world, ui);
    // Materials dropped by a cancelled blueprint are picked up by walking over them.
    if (!store_.drops().empty()) {
        const auto [cx, cy] = cellOf(game.hero().feetX(), game.hero().feetY());
        const sim::ItemCounts taken = store_.takeDropsNear(cx, cy, 1);
        if (!taken.empty() && game.life() != nullptr) {
            for (const auto& [item, count] : taken) game.life()->give(item, count);
            game.showMessage("You picked up " + costText(game, taken) + ".");
        }
    }
    // Days and seasons pass with the clan's clock: seasons wear buildings (D-55 Q5: very slowly).
    if (const sim::World* clan = game.clan()) {
        const sim::Date date = clan->date();
        if (dayAtLastTick_ != ~0ULL && static_cast<std::uint64_t>(date.day) != dayAtLastTick_) {
            store_.dayEnded();
            rivalBuilders_.dayEnded(data_, rivalPeople(game));
        }
        if (dayAtLastTick_ == ~0ULL || static_cast<std::uint64_t>(date.day) != dayAtLastTick_) applyClanLife(game);
        dayAtLastTick_ = static_cast<std::uint64_t>(date.day);
        const int season = static_cast<int>(date.season);
        if (seasonAtLastTick_ >= 0 && season != seasonAtLastTick_) {
            store_.seasonEnded(seasonAtLastTick_);
            rivalBuilders_.seasonStarted(data_, rivalPeople(game));
            raidsAtSeasonStart(game, season);
        }
        seasonAtLastTick_ = season;
    }
    const std::string weather = game.weather().current() >= 0 && static_cast<std::size_t>(game.weather().current()) < game.catalogs().weather.size()
                                    ? game.catalogs().weather[static_cast<std::size_t>(game.weather().current())].name
                                    : std::string();
    store_.advance({rainPercentOf(weather)});
    if (game.ticks() % 10 == 0) { // flames on burning pieces (US-255)
        int shown = 0;
        for (const sim::buildings::PlacedBuilding& building : store_.all()) {
            for (const sim::buildings::PieceState& piece : building.pieces) {
                if (!piece.burning || shown >= 8) continue;
                game.playEffect("ember sparks", piece.x * kTileSize + kTileSize / 2.0, piece.y * kTileSize + kTileSize / 2.0, 28);
                ++shown;
            }
        }
    }
    syncObstacles(game);
}

// ---- what the hero does with a building

std::string BuildingLayer::deliver(OdysseyGame& game, int id) {
    const sim::buildings::PlacedBuilding* building = store_.find(id);
    if (building == nullptr || building->state != sim::buildings::State::Blueprint) return "There is nothing to bring materials to.";
    const std::string label = store_.label(*building);
    if (game.life() == nullptr) {
        store_.waiveCost(id);
        return label + ": the materials are free without a run. Work on it to build it.";
    }
    sim::ItemCounts bag = game.life()->inventory();
    const sim::ItemCounts moved = store_.deliver(id, bag);
    for (const auto& [item, count] : moved) game.life()->take(item, count);
    building = store_.find(id);
    if (store_.fullyDelivered(*building)) return std::format("{}: all materials are here. Work on it to build it.", label);
    sim::ItemCounts missing;
    for (const auto& [item, need] : store_.cost(*building)) {
        const auto have = building->delivered.find(item);
        const int left = need - (have == building->delivered.end() ? 0 : have->second);
        if (left > 0) missing[item] = left;
    }
    return std::format("{}{} Still needs: {}.", label, moved.empty() ? ": you have none of what it needs." : ": delivered " + costText(game, moved) + ".", costText(game, missing));
}

std::string BuildingLayer::clanDeliver(OdysseyGame&, int id) {
    const sim::buildings::PlacedBuilding* building = store_.find(id);
    if (building == nullptr || building->state != sim::buildings::State::Blueprint) return {};
    sim::ItemCounts surroundings; // what the clan finds around the site: a little of each missing item
    for (const auto& [item, need] : store_.cost(*building)) {
        const auto have = building->delivered.find(item);
        const int left = need - (have == building->delivered.end() ? 0 : have->second);
        if (left > 0) surroundings[item] = std::min(left, kClanCarry);
    }
    const sim::ItemCounts moved = store_.deliver(id, surroundings);
    return moved.empty() ? std::string() : store_.label(*building) + ": the clan brought materials.";
}

std::string BuildingLayer::clanWork(OdysseyGame& game, int id, int seconds) {
    const sim::buildings::PlacedBuilding* building = store_.find(id);
    if (building == nullptr || building->state != sim::buildings::State::Blueprint || heroInTheWay(game, *building) || store_.workAvailable(*building) <= 0) return {};
    const bool done = store_.work(id, seconds * 1000);
    if (!done) return {};
    syncObstacles(game);
    const std::string label = store_.label(*building);
    game.chronicleLine("The clan finished building a " + label, -1, -1);
    return label + " is finished.";
}

std::string BuildingLayer::work(OdysseyGame& game, int id, int seconds) {
    const sim::buildings::PlacedBuilding* building = store_.find(id);
    if (building == nullptr || building->state != sim::buildings::State::Blueprint) return "There is nothing to build.";
    const std::string label = store_.label(*building);
    if (heroInTheWay(game, *building)) return "Step out of the way first.";
    if (store_.workAvailable(*building) <= 0) return label + ": bring more materials first.";
    const bool done = store_.work(id, seconds * 1000);
    building = store_.find(id);
    if (done) {
        syncObstacles(game);
        game.chronicleLine(std::format("{} finished building a {}", game.life() != nullptr ? game.life()->name() : std::string("The hero"), label), -1, -1);
        return label + " is finished.";
    }
    const int total = std::max(1, store_.buildMilli(*building));
    return std::format("{}: {}% built.", label, building->workMilli * 100 / total);
}

std::string BuildingLayer::cancel(OdysseyGame& game, int id) {
    sim::buildings::PlacedBuilding* building = store_.findMutable(id);
    if (building == nullptr || building->state != sim::buildings::State::Blueprint) return "Only a blueprint can be cancelled.";
    const std::string label = store_.label(*building);
    if (game.life() == nullptr) building->delivered.clear(); // free materials are not left lying about
    const bool dropped = !building->delivered.empty();
    store_.cancel(id);
    return label + (dropped ? " cancelled: the materials are on the ground." : " cancelled.");
}

std::string BuildingLayer::repair(OdysseyGame& game, int id, int hp) {
    (void)game;
    const sim::buildings::PlacedBuilding* building = store_.find(id);
    if (building == nullptr) return "There is nothing to repair.";
    const std::string label = store_.label(*building);
    if (!store_.repair(id, hp)) return label + " needs no repair.";
    return std::format("{}: repaired, now {}%.", label, store_.condition(*store_.find(id)));
}

// Each dawn: a finished building whose kind says `owner: person` goes to the lowest-id living adult without one; the owners and, up to four beds a
// sleeping place, the next adults without a home are housed (they lose warmth more slowly); storage kinds tell the world how many meals they keep cool.
void BuildingLayer::applyClanLife(OdysseyGame& game) {
    sim::World* clan = game.clanMutable();
    if (clan == nullptr) return;
    const int daysPerYear = clan->calendar().daysPerYear();
    std::vector<int> adults;
    for (const sim::Person& person : clan->people()) {
        if (person.alive && !person.exiled && person.ageYears(daysPerYear) >= kAdultYears) adults.push_back(person.id);
    }
    std::sort(adults.begin(), adults.end());
    const auto owns = [&](int person) {
        for (const sim::buildings::PlacedBuilding& building : store_.all()) {
            if (building.state == sim::buildings::State::Finished && building.owner == person) return true;
        }
        return false;
    };
    std::vector<int> ids;
    for (const sim::buildings::PlacedBuilding& building : store_.all()) ids.push_back(building.id);
    for (const int id : ids) {
        sim::buildings::PlacedBuilding& building = *store_.findMutable(id);
        if (building.state != sim::buildings::State::Finished) continue;
        const sim::buildings::KindDef* kind = data_.kind(building.kind);
        const bool alive = std::find(adults.begin(), adults.end(), building.owner) != adults.end();
        if (kind == nullptr || kind->owner != "person" || (building.owner >= 0 && alive)) continue;
        building.owner = -1;
        for (const int person : adults) {
            if (!owns(person)) {
                building.owner = person;
                break;
            }
        }
    }
    std::vector<int> housed;
    int storage = 0;
    for (const sim::buildings::PlacedBuilding& building : store_.all()) {
        if (building.state != sim::buildings::State::Finished || store_.condition(building) <= 0) continue;
        const std::vector<std::string> uses = store_.uses(building);
        const sim::buildings::KindDef* kind = data_.kind(building.kind);
        if (kind != nullptr) storage += std::find(uses.begin(), uses.end(), "store") != uses.end() ? kind->capacity : 0;
        if (std::find(uses.begin(), uses.end(), "sleep") == uses.end()) continue;
        int beds = kBedsPerSleepingPlace;
        if (building.owner >= 0) {
            housed.push_back(building.owner);
            --beds;
        }
        for (const int person : adults) {
            if (beds <= 0) break;
            if (std::find(housed.begin(), housed.end(), person) != housed.end() || owns(person)) continue;
            housed.push_back(person);
            --beds;
        }
    }
    clan->setHoused(std::move(housed));
    clan->setStorageMeals(storage);
}

bool BuildingLayer::fireHit(OdysseyGame& game, int cellX, int cellY) {
    if (!store_.igniteAt(cellX, cellY)) return false;
    game.showMessage("A building caught fire!");
    return true;
}

std::string BuildingLayer::clanDouse(OdysseyGame&, int id) {
    const sim::buildings::PlacedBuilding* building = store_.find(id);
    if (building == nullptr) return {};
    const auto [cx, cy] = store_.centre(*building);
    return store_.douse(id, cx, cy) ? store_.label(*building) + ": the clan put out some of the fire." : std::string();
}

// A rival the hero is at war or at feud with (the relation at or below -50) strikes the hero's buildings when a season begins: one hit of 40 hit points on a
// finished building and, one time in three, a fire (D-55 Q2). Whole numbers: the choice follows the store's hash, the season and the rival.
int BuildingLayer::raidsAtSeasonStart(OdysseyGame& game, int season) {
    if (game.life() == nullptr || game.rivals() == nullptr) return 0;
    int raids = 0;
    for (int rival = 0; rival < static_cast<int>(game.rivals()->clans().size()); ++rival) {
        if (game.life()->relation(rival) <= kRaidRelation && raidFrom(game, rival, season)) ++raids;
    }
    return raids;
}

bool BuildingLayer::raidFrom(OdysseyGame& game, int rival, int season) {
    {
        std::vector<const sim::buildings::PlacedBuilding*> targets;
        for (const sim::buildings::PlacedBuilding& building : store_.all()) {
            if (building.state == sim::buildings::State::Finished && building.faction == 0) targets.push_back(&building);
        }
        if (targets.empty()) return false;
        const std::uint64_t pick = store_.hash() ^ (static_cast<std::uint64_t>(season + 1) * 0x9E3779B97F4A7C15ULL) ^ (static_cast<std::uint64_t>(rival + 1) * 0xC2B2AE3D27D4EB4FULL);
        const sim::buildings::PlacedBuilding& target = *targets[pick % targets.size()];
        const std::string label = store_.label(target);
        bool hit = false;
        std::pair<int, int> where{0, 0};
        const std::size_t first = static_cast<std::size_t>((pick >> 8) % std::max<std::size_t>(1, target.pieces.size()));
        for (std::size_t i = 0; i < target.pieces.size() && !hit; ++i) {
            const sim::buildings::PieceState& piece = target.pieces[(first + i) % target.pieces.size()];
            if (!piece.built || piece.fell()) continue;
            where = {piece.x, piece.y};
            hit = store_.damageAt(piece.x, piece.y, kRaidDamage);
        }
        if (!hit) return false;
        const bool fire = ((pick >> 20) % 3) == 0;
        if (fire) store_.igniteAt(where.first, where.second);
        game.chronicleLine(std::format("Raiders from {} struck the {}{}", (game.rivals() != nullptr && static_cast<std::size_t>(rival) < game.rivals()->clans().size() ? game.rivals()->clans()[static_cast<std::size_t>(rival)].name : std::string("a rival clan")), label, fire ? " and set it on fire" : ""), -1, -1);
        game.showMessage(std::format("Raiders struck your {}{}!", label, fire ? " and set it on fire" : ""));
    }
    return true;
}

std::string BuildingLayer::douse(OdysseyGame& game, int id) {
    const auto [heroX, heroY] = cellOf(game.hero().feetX(), game.hero().feetY());
    if (!store_.douse(id, heroX, heroY)) return "Nothing burns there.";
    return "You put out some of the fire.";
}

// ---- saving

std::string BuildingLayer::saveText() const {
    nlohmann::json root;
    root["version"] = 1;
    root["known"] = std::vector<std::string>(known_.begin(), known_.end());
    root["store"] = nlohmann::json::parse(store_.toJson());
    root["rivals"] = nlohmann::json::parse(rivalBuilders_.toJson());
    return root.dump(1, '\t') + "\n";
}

std::vector<std::string> BuildingLayer::loadText(const std::string& text) {
    std::vector<std::string> notes;
    const nlohmann::json root = nlohmann::json::parse(text, nullptr, false);
    if (root.is_discarded() || !root.is_object() || !root.contains("store")) return {"buildings.json could not be read"};
    if (root.contains("known") && root["known"].is_array()) {
        known_.clear();
        for (const nlohmann::json& kind : root["known"]) {
            if (kind.is_string()) known_.insert(kind.get<std::string>());
        }
    }
    std::string problem;
    if (!store_.fromJson(root["store"].dump(), problem)) notes.push_back(problem);
    if (root.contains("rivals") && !rivalBuilders_.fromJson(root["rivals"].dump(), problem)) notes.push_back(problem);
    return notes;
}

// ---- drawing

void BuildingLayer::loadArt(luna::engine::Renderer& renderer) {
    const std::size_t pieceCount = data_.pieces().size();
    const std::size_t kindCount = data_.kinds().size();
    const int cells = static_cast<int>(pieceCount + kSwatches + kindCount + pieceCount);
    luna::engine::Image image(std::max(1, cells) * kArtCellWidth, kArtCellHeight);
    for (std::size_t i = 0; i < pieceCount; ++i) paintPiece(image, static_cast<int>(i), data_.pieces()[i]);
    const luna::engine::Color swatches[kSwatches] = {{70, 205, 95, 255}, {225, 60, 50, 255}, {30, 28, 26, 255}, {235, 205, 90, 255}};
    for (int s = 0; s < kSwatches; ++s) image.fillRect((static_cast<int>(pieceCount) + s) * kArtCellWidth, 0, kArtCellWidth, kArtCellHeight, swatches[s]);
    for (std::size_t k = 0; k < kindCount; ++k) image.fillRect((static_cast<int>(pieceCount) + kSwatches + static_cast<int>(k)) * kArtCellWidth, 0, kArtCellWidth, kArtCellHeight, shade(data_.kinds()[k].colour, 0));
    for (std::size_t i = 0; i < pieceCount; ++i) image.fillRect((static_cast<int>(pieceCount) + kSwatches + static_cast<int>(kindCount) + static_cast<int>(i)) * kArtCellWidth, 0, kArtCellWidth, kArtCellHeight, shade(data_.pieces()[i].colour, 0));
    art_ = renderer.createTexture(image);
    artKinds_ = kindCount;
    artPieces_ = static_cast<int>(pieceCount);
}

int BuildingLayer::artIndex(const std::string& pieceId) const {
    const auto& pieces = data_.pieces();
    const auto found = std::lower_bound(pieces.begin(), pieces.end(), pieceId, [](const sim::buildings::PieceDef& p, const std::string& key) { return p.id < key; });
    return found != pieces.end() && found->id == pieceId ? static_cast<int>(found - pieces.begin()) : -1;
}

void BuildingLayer::drawPiece(luna::engine::Renderer& renderer, int index, int cellX, int cellY, const Rect& view, int alpha) const {
    if (index < 0 || art_.id < 0) return;
    const Rect source{index * kArtCellWidth, 0, kArtCellWidth, kArtCellHeight};
    const Rect destination{cellX * kTileSize - view.x, (cellY + 1) * kTileSize - kArtCellHeight - view.y, kArtCellWidth, kArtCellHeight};
    if (alpha >= 255) renderer.draw(art_, source, {destination.x, destination.y});
    else renderer.drawStyled(art_, source, destination, {static_cast<std::uint8_t>(alpha), luna::engine::Blend::Normal});
}

void BuildingLayer::drawSwatch(luna::engine::Renderer& renderer, int swatch, const Rect& area, const Rect& view, int alpha) const {
    if (art_.id < 0) return;
    const Rect source{(artPieces_ + swatch) * kArtCellWidth, 0, 4, 4};
    renderer.drawStyled(art_, source, {area.x - view.x, area.y - view.y, area.width, area.height}, {static_cast<std::uint8_t>(alpha), luna::engine::Blend::Normal});
}

namespace {
bool onScreen(const sim::buildings::PlacedBuilding& building, const Rect& view) {
    const int left = building.x * kTileSize;
    const int top = building.y * kTileSize - 40;
    return left < view.x + view.width && left + building.width * kTileSize > view.x && top < view.y + view.height && (building.y + building.height) * kTileSize > view.y;
}
} // namespace

void BuildingLayer::drawGround(luna::engine::Renderer& renderer, const Rect& view) const {
    if (art_.id < 0) return;
    for (const sim::buildings::PlacedBuilding& building : store_.all()) {
        if (building.state != sim::buildings::State::Finished || !onScreen(building, view)) continue;
        for (const sim::buildings::PieceState& piece : building.pieces) {
            const sim::buildings::PieceDef* def = data_.piece(piece.piece);
            if (def == nullptr || def->type != PieceType::Floor || piece.fell()) continue;
            drawPiece(renderer, artIndex(piece.piece), piece.x, piece.y, view, 255);
        }
    }
}

void BuildingLayer::drawStanding(luna::engine::Renderer& renderer, const Rect& view, double heroFeetY, bool behind) const {
    if (art_.id < 0) return;
    struct Entry {
        int x;
        int y;
        int art;
    };
    std::vector<Entry> entries;
    for (const sim::buildings::PlacedBuilding& building : store_.all()) {
        if (building.state != sim::buildings::State::Finished || !onScreen(building, view)) continue;
        for (const sim::buildings::PieceState& piece : building.pieces) {
            const sim::buildings::PieceDef* def = data_.piece(piece.piece);
            if (def == nullptr || sim::buildings::layerOf(def->type) != Layer::Wall || piece.fell()) continue;
            const double feetY = (piece.y + 1) * kTileSize - 2.0;
            if ((feetY <= heroFeetY) != behind) continue;
            entries.push_back({piece.x, piece.y, artIndex(piece.piece)});
        }
    }
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.y != b.y ? a.y < b.y : a.x < b.x; });
    for (const Entry& entry : entries) drawPiece(renderer, entry.art, entry.x, entry.y, view, 255);
}

void BuildingLayer::drawRoofs(const OdysseyGame& game, luna::engine::Renderer& renderer, const Rect& view) const {
    if (art_.id < 0) return;
    const auto [heroX, heroY] = cellOf(game.hero().feetX(), game.hero().feetY());
    const int heroRoom = store_.roomAt(heroX, heroY);
    std::set<int> openBuildings; // the buildings whose roofs fade because the hero is inside their room (US-254)
    if (heroRoom >= 0) {
        for (const int id : store_.rooms()[static_cast<std::size_t>(heroRoom)].buildings) openBuildings.insert(id);
    }
    for (const sim::buildings::PlacedBuilding& building : store_.all()) {
        if (building.state != sim::buildings::State::Finished || !onScreen(building, view)) continue;
        const bool fade = openBuildings.count(building.id) != 0 && store_.interiorMode(building) == "fade";
        for (const sim::buildings::PieceState& piece : building.pieces) {
            const sim::buildings::PieceDef* def = data_.piece(piece.piece);
            if (def == nullptr || def->type != PieceType::Roof || piece.fell()) continue;
            drawPiece(renderer, artIndex(piece.piece), piece.x, piece.y, view, fade ? 50 : 255);
        }
    }
}

void BuildingLayer::drawBlueprints(const OdysseyGame& game, luna::engine::Renderer& renderer, const luna::engine::Texture& uiSheet, const Rect& view) const {
    if (art_.id < 0) return;
    luna::engine::UiPainter painter(renderer, uiSheet);
    for (const sim::buildings::PlacedBuilding& building : store_.all()) {
        if (building.state != sim::buildings::State::Blueprint || !onScreen(building, view)) continue;
        for (const sim::buildings::PieceState& piece : building.pieces) drawPiece(renderer, artIndex(piece.piece), piece.x, piece.y, view, kGhostAlpha);
        const int total = std::max(1, store_.buildMilli(building));
        const int percent = std::min(100, building.workMilli * 100 / total);
        const int left = building.x * kTileSize + 2 - view.x;
        const int top = building.y * kTileSize - 10 - view.y;
        const int width = building.width * kTileSize - 4;
        painter.fill({left, top, width, 5}, UiColor::Shade);
        painter.fill({left + 1, top + 1, std::max(0, (width - 2) * percent / 100), 3}, UiColor::Gold);
        std::string label = store_.label(building);
        if (!store_.fullyDelivered(building)) {
            sim::ItemCounts missing;
            for (const auto& [item, need] : store_.cost(building)) {
                const auto have = building.delivered.find(item);
                const int short_ = need - (have == building.delivered.end() ? 0 : have->second);
                if (short_ > 0) missing[item] = short_;
            }
            label += ": needs " + costText(game, missing);
        } else {
            label += std::format(": {}%", percent);
        }
        painter.text(left, top - luna::engine::kGlyphHeight - 2, label, UiColor::Text);
    }
}

void BuildingLayer::drawFlat(luna::engine::Renderer& renderer, const std::string& pieceId, const Rect& destination, int alpha) const {
    const int index = artIndex(pieceId);
    if (art_.id < 0 || index < 0) return;
    const int cell = artPieces_ + kSwatches + static_cast<int>(artKinds_) + index;
    renderer.drawStyled(art_, {cell * kArtCellWidth, 0, 4, 4}, destination, {static_cast<std::uint8_t>(alpha), luna::engine::Blend::Normal});
}

void BuildingLayer::drawLayout(luna::engine::Renderer& renderer, const std::string& kind, int cellX, int cellY, int turns, const Rect& view, int alpha, int roofAlpha) const {
    if (art_.id < 0) return;
    std::vector<sim::buildings::PieceState> pieces = store_.layoutPieces(kind, cellX, cellY, turns);
    std::stable_sort(pieces.begin(), pieces.end(), [this](const sim::buildings::PieceState& a, const sim::buildings::PieceState& b) {
        const sim::buildings::PieceDef* da = data_.piece(a.piece);
        const sim::buildings::PieceDef* db = data_.piece(b.piece);
        const int la = static_cast<int>(sim::buildings::layerOf(da->type));
        const int lb = static_cast<int>(sim::buildings::layerOf(db->type));
        return la != lb ? la < lb : (a.y != b.y ? a.y < b.y : a.x < b.x);
    });
    for (const sim::buildings::PieceState& piece : pieces) {
        const sim::buildings::PieceDef* def = data_.piece(piece.piece);
        drawPiece(renderer, artIndex(piece.piece), piece.x, piece.y, view, def != nullptr && def->type == PieceType::Roof ? roofAlpha : alpha);
    }
}

void BuildingLayer::drawGhost(const OdysseyGame& game, luna::engine::Renderer& renderer, const Rect& view) const {
    (void)game;
    if (art_.id < 0 || !placing() || ghostProblem_ == "outside" || ghostProblem_ == "none") return;
    const bool valid = ghostProblem_.empty();
    for (const sim::buildings::PieceState& piece : store_.layoutPieces(selected_, ghostX_, ghostY_, turns_)) drawPiece(renderer, artIndex(piece.piece), piece.x, piece.y, view, kGhostAlpha + 40);
    const auto [w, h] = store_.sizeOf(selected_, turns_);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) drawSwatch(renderer, valid ? kGreen : kRed, {(ghostX_ + x) * kTileSize, (ghostY_ + y) * kTileSize, kTileSize, kTileSize}, view, 70);
    }
}

void BuildingLayer::drawMenu(const OdysseyGame& game, luna::engine::Renderer& ui, const luna::engine::Texture& uiSheet) const {
    if (!menuOpen_) return;
    luna::engine::UiPainter painter(ui, uiSheet);
    const Rect panel = panelRect(game);
    painter.fill(panel, UiColor::Shade);
    painter.outline(panel, UiColor::Border);
    painter.text(panel.x + 5, panel.y + 4, "BUILD   B: close", UiColor::Gold);
    for (int tab = 0; tab < 2; ++tab) {
        const Rect box{panel.x + 4 + tab * (kPanelWidth / 2 - 4), panel.y + 14, kPanelWidth / 2 - 4, 13};
        const bool on = (tab == 1) == piecesTab_;
        painter.fill(box, on ? UiColor::Selected : UiColor::Panel);
        painter.outline(box, on ? UiColor::Gold : UiColor::Border);
        painter.text(box.x + 5, box.y + 3, tab == 0 ? "Buildings" : "Pieces", on ? UiColor::Gold : UiColor::Text);
    }
    const std::vector<std::string> kinds = menuKinds(piecesTab_);
    for (int row = 0; row < kRows; ++row) {
        const std::size_t index = static_cast<std::size_t>(row + scroll_);
        if (index >= kinds.size()) break;
        const std::string& kind = kinds[index];
        const Rect box{panel.x + 3, panel.y + kListTop + row * kRowHeight, kPanelWidth - 6, kRowHeight - 1};
        painter.fill(box, kind == selected_ ? UiColor::Selected : (static_cast<int>(index) == hoverRow_ ? UiColor::Hover : UiColor::Panel));
        std::string name = kind;
        sim::ItemCounts cost;
        if (kind.rfind("piece:", 0) == 0) {
            if (const sim::buildings::PieceDef* piece = data_.piece(kind.substr(6))) {
                name = piece->label;
                cost = piece->cost;
            }
        } else if (const sim::buildings::KindDef* def = data_.kind(kind)) {
            name = def->label;
            cost = def->cost;
        }
        if (art_.id >= 0) {
            const int k = piecesTab_ ? -1 : static_cast<int>(std::find_if(data_.kinds().begin(), data_.kinds().end(), [&](const sim::buildings::KindDef& d) { return d.id == kind; }) - data_.kinds().begin());
            const int cell = piecesTab_ ? artIndex(kind.substr(6)) : artPieces_ + kSwatches + k;
            if (cell >= 0) ui.draw(art_, {cell * kArtCellWidth, piecesTab_ ? kArtCellHeight - 20 : 0, 16, 16}, {box.x + 2, box.y + 2});
        }
        painter.text(box.x + 22, box.y + 2, name, UiColor::Text);
        painter.text(box.x + 22, box.y + 11, costText(game, cost), UiColor::Dim);
    }
    const int footer = panel.y + kListTop + std::min<int>(kRows, std::max<int>(1, static_cast<int>(kinds.size()))) * kRowHeight + 3;
    if (placing()) {
        painter.text(panel.x + 5, footer, "R: turn  Click: place", UiColor::Text);
        painter.text(panel.x + 5, footer + 10, "Esc or right click: stop", UiColor::Dim);
        if (!ghostProblem_.empty() && ghostProblem_ != "outside" && ghostProblem_ != "none") painter.text(panel.x + 5, footer + 20, ghostProblem_, UiColor::Red);
    } else {
        painter.text(panel.x + 5, footer, "Pick one, then click the ground", UiColor::Text);
        if (game.life() == nullptr) painter.text(panel.x + 5, footer + 10, "No run: building is free", UiColor::Dim);
    }
}

} // namespace odysseus::game
