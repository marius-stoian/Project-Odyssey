// US-262: the placed people of a level are persons of the simulation (sim::NpcPopulation); animals and monsters stay creatures.
#include "game/odyssey_game.h"

#include "core/log.h"
#include "game/game_rules.h"
#include "sim/npc_population.h"
#include "sim/save.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>

namespace odysseus::game {

namespace {
constexpr double kMeetingDistance = 48.0; // pixels: this close to the hero, a person remembers meeting them
constexpr int kStartAgeYears = 20;
} // namespace

// A placed character is a person unless it is an animal or a monster (by its kind or by its classes) or the hero's own kind.
bool OdysseyGame::isPersonKind(const PlacedCharacter& placed) const {
    const CharacterKindDef* kind = definitions_.character(placed.kind);
    if (kind == nullptr || kind->animal || kind->name == "hero") return false;
    const sim::rules::ResolvedNpc resolved = npcClasses_.resolve(placed);
    for (const std::string& name : resolved.classes) {
        if (name == "monster" || name == "animal") return false;
    }
    return !(kind->enemy && resolved.classes.empty()); // an enemy kind with no class at all keeps fighting as before
}

const PlacedCharacter* OdysseyGame::placedCharacter(int id) const {
    for (const PlacedCharacter& placed : level_.characters) {
        if (placed.id == id) return &placed;
    }
    return nullptr;
}

const sim::rules::DlgScript* OdysseyGame::npcDialogueFor(int placedId) const {
    const PlacedCharacter* placed = placedCharacter(placedId);
    if (placed == nullptr) return nullptr;
    const sim::rules::ResolvedNpc resolved = npcClasses_.resolve(*placed);
    const auto file = resolved.dialogues.find("player");
    if (file == resolved.dialogues.end()) return nullptr;
    return dialogues_.find(file->second.substr(0, file->second.size() - 4)); // "trader.dlg" is the script "trader"
}

bool OdysseyGame::fightsHero(const PlacedCharacter& placed) const {
    const CharacterKindDef* kind = definitions_.character(placed.kind);
    if (kind == nullptr) return false;
    if (npcClasses_.kinds().find(placed.kind) == nullptr) return kind->enemy; // no kind file: the switch of characters.json
    return npcClasses_.resolve(placed).attitude == "hostile";
}

std::string OdysseyGame::attitudeWordOf(int placedId) const {
    if (npcPopulation_.hasOpinions(placedId)) return sim::attitudeName(npcPopulation_.attitude(placedId, sim::NpcPopulation::kHero)); // a person or a creature
    for (const PlacedCharacter& placed : level_.characters) {
        if (placed.id == placedId) return npcClasses_.resolve(placed).attitude;
    }
    return "neutral";
}

// Creatures with a kind file (animals and monsters) hold an opinion of the hero like persons do, from their attitude (US-266).
void OdysseyGame::registerCreatures() {
    for (const PlacedCharacter& placed : level_.characters) {
        if (isPersonKind(placed) || npcClasses_.kinds().find(placed.kind) == nullptr) continue;
        npcPopulation_.addCreature(placed.id, sim::attitudeFromName(npcClasses_.resolve(placed).attitude).value_or(sim::Attitude::Neutral));
    }
}

bool OdysseyGame::startFight(int placedId) {
    for (Enemy& enemy : enemies_) {
        if (enemy.id == placedId && enemy.isAlive()) {
            enemy.provoke();
            return true;
        }
    }
    for (auto it = bystanders_.begin(); it != bystanders_.end(); ++it) {
        if (it->id != placedId) continue;
        const CharacterKindDef* kind = definitions_.character(it->kind);
        if (kind == nullptr) return false;
        Enemy enemy(it->feet.x, it->feet.y, it->hp);
        enemy.id = it->id;
        enemy.name = it->name;
        enemy.frames = kind->frames;
        enemy.directions = kind->directions;
        enemy.animal = kind->animal;
        enemy.kindName = kind->name;
        enemy.facing = it->facing;
        enemy.swordDamage = it->swordDamage;
        enemy.reachMetres = kind->reach;
        enemy.provoke();
        enemies_.push_back(enemy);
        bystanders_.erase(it);
        return true;
    }
    return false;
}

bool OdysseyGame::calmFight(int placedId) {
    for (Enemy& enemy : enemies_) {
        if (enemy.id == placedId && enemy.isAlive()) {
            enemy.calm();
            return true;
        }
    }
    return false;
}

void OdysseyGame::confrontKey(const luna::engine::Pointer& pointer) { npcKey(pointer, true); }
void OdysseyGame::actionsKey(const luna::engine::Pointer& pointer) { npcKey(pointer, false); }

// The NPC under the pointer, else the nearest within 6 m, for the Confront key and the Actions key.
void OdysseyGame::npcKey(const luna::engine::Pointer& pointer, bool confront) {
    constexpr double kReach = 6.0 * kTileSize;
    std::optional<Subject> chosen;
    const auto isNpc = [](const Subject& subject) { return std::find(subject.info.tags.begin(), subject.info.tags.end(), "npc") != subject.info.tags.end(); };
    if (pointer.inside()) {
        const luna::engine::Rect view = camera_.view();
        if (auto under = subjectAt(*this, view.x + pointer.x, view.y + pointer.y); under && isNpc(*under)) chosen = std::move(under);
    }
    if (!chosen) {
        double best = kReach + 1.0;
        const auto consider = [&](std::optional<Subject> subject) {
            if (!subject || !isNpc(*subject)) return;
            const double distance = std::hypot(subject->x - hero_.feetX(), subject->y - hero_.feetY());
            if (distance < best) {
                best = distance;
                chosen = std::move(subject);
            }
        };
        for (const int index : npcPopulation_.near(static_cast<int>(hero_.feetX()), static_cast<int>(hero_.feetY()), static_cast<int>(kReach))) consider(npcSubject(*this, npcPopulation_.id(index)));
        for (std::size_t i = 0; i < enemies_.size(); ++i) {
            if (enemies_[i].isAlive()) consider(animalSubject(*this, i));
        }
    }
    if (!chosen) {
        say(confront ? "There is no one to confront here." : "There is no one here to ask about.");
        return;
    }
    if (confront) runFlow_.openConfront(*this, *chosen);
    else runFlow_.openActions(*this, *chosen);
}

void OdysseyGame::buildNpcPopulation() {
    npcPopulation_ = sim::NpcPopulation(npcCalendar_, npcNeeds_);
    npcPopulation_.setOpinionConfig(npcOpinions_);
    npcMetDay_.clear();
    const int daysPerYear = sim::Calendar(npcCalendar_).daysPerYear();
    for (const PlacedCharacter& placed : level_.characters) {
        if (!isPersonKind(placed)) continue;
        // Varied but fixed ages: the same level always starts the same people.
        const int age = kStartAgeYears * daysPerYear + (placed.id * 37) % (10 * daysPerYear);
        const sim::Attitude start = sim::attitudeFromName(npcClasses_.resolve(placed).attitude).value_or(sim::Attitude::Neutral);
        npcPopulation_.add(placed.id, placed.kind, age, placed.family, placed.feet.x, placed.feet.y, start);
    }
    registerCreatures();
    tradeMarket_ = sim::TradeMarket(tradeConfig_, weatherSeed_ ^ 0x54524144ULL); // the world seed of the level, its own stream for trade
    tradeDay_ = npcPopulation_.day();
    refreshTraders();
}

// Every placed person whose classes, kind and own fields give a trade profile is a trader (D-54 Q7: the Trader class is only a default profile). A trader that is
// already registered keeps its stock and takes the new profile.
void OdysseyGame::refreshTraders() {
    for (const PlacedCharacter& placed : level_.characters) {
        if (!isPersonKind(placed)) continue;
        const sim::rules::ResolvedNpc resolved = npcClasses_.resolve(placed);
        if (!resolved.extras.trade.empty()) tradeMarket_.addTrader(placed.id, resolved.extras.trade, tradeDay_);
    }
}

// One game tick of the placed people: the clock of their days, and meeting the hero (found through the grid, so a crowd costs nothing here).
void OdysseyGame::tickNpcPopulation() {
    npcPopulation_.setFocus(static_cast<int>(hero_.feetX()), static_cast<int>(hero_.feetY())); // the hero is the centre of the detail (ADR-022)
    npcPopulation_.tick();
    if (const std::int64_t today = npcPopulation_.day(); today > tradeDay_) {
        tradeDay_ = today;
        tradeMarket_.dailyUpdate(today, level_.economy); // a new day: every trader, near or far, gets its delivery (US-281)
    }
    if (ticks_ % 20 != 0) return;
    const std::int64_t today = npcPopulation_.day();
    for (const int index : npcPopulation_.near(static_cast<int>(hero_.feetX()), static_cast<int>(hero_.feetY()), static_cast<int>(kMeetingDistance))) {
        const int id = npcPopulation_.id(index);
        const auto met = npcMetDay_.find(id);
        if (met != npcMetDay_.end() && met->second == today) continue;
        npcMetDay_[id] = today;
        npcPopulation_.meetHero(index);
    }
}

bool OdysseyGame::saveNpcPopulation() const {
    try {
        sim::writeSaveText(saveDirectory_ / "npcs.json", npcPopulation_.toText());
        sim::writeSaveText(saveDirectory_ / "trade.json", tradeMarket_.toText());
    } catch (const std::exception& error) {
        core::logWarning(std::string("NPC save failed: ") + error.what());
        return false;
    }
    return true;
}

// Brings the saved persons back: those the level still has take their saved state; the rest of the level's people stay as freshly made.
std::string OdysseyGame::loadNpcPopulation() {
    const std::filesystem::path file = saveDirectory_ / "npcs.json";
    std::ifstream in(file, std::ios::binary);
    if (!in) return {};
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    try {
        sim::NpcPopulation saved = sim::NpcPopulation::fromText(text, npcCalendar_, npcNeeds_);
        saved.setOpinionConfig(npcOpinions_);
        npcPopulation_ = std::move(saved);
        // A person the level gained since the save joins as new; one the level lost stays in the save (the owner may bring them back).
        const int daysPerYear = sim::Calendar(npcCalendar_).daysPerYear();
        for (const PlacedCharacter& placed : level_.characters) {
            if (isPersonKind(placed) && npcPopulation_.indexOf(placed.id) < 0) {
                const sim::Attitude start = sim::attitudeFromName(npcClasses_.resolve(placed).attitude).value_or(sim::Attitude::Neutral);
                npcPopulation_.add(placed.id, placed.kind, kStartAgeYears * daysPerYear, placed.family, placed.feet.x, placed.feet.y, start);
            }
        }
        registerCreatures(); // creatures the save did not know (the level gained them)
        tradeDay_ = npcPopulation_.day();
        if (std::ifstream tradeIn(saveDirectory_ / "trade.json", std::ios::binary); tradeIn) {
            const std::string tradeText((std::istreambuf_iterator<char>(tradeIn)), std::istreambuf_iterator<char>());
            tradeMarket_.restoreState(tradeText);
        }
        return {};
    } catch (const std::exception& error) {
        return std::string("The saved people could not be read: ") + error.what();
    }
}

} // namespace odysseus::game
