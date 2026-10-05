// US-262: the placed people of a level are persons of the simulation (sim::NpcPopulation); animals and monsters stay creatures.
#include "game/odyssey_game.h"

#include "core/log.h"
#include "game/game_rules.h"
#include "game/npc_life.h"
#include "game/weapons.h"
#include "luna/engine/collision.h"
#include "sim/npc_population.h"
#include "sim/save.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <fstream>
#include <iterator>
#include <set>

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
        for (const PlacedCharacter& figure : bystanders_) {
            if (npcPopulation_.indexOf(figure.id) >= 0) consider(npcSubject(*this, figure.id)); // the placed people, where their figures stand now
        }
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
    // The life of the people (US-290): the places of the level, every person's home where it was placed, and the schedules.
    npcDirector_ = sim::NpcDirector(scheduleConfig_);
    npcDirector_.setInteractions(&interactions_);
    npcDirector_.setMarket(&tradeMarket_); // the stocks the persons trade and give from (US-292)
    npcBubbles_.clear();
    npcDirector_.setEvents(eventCatalog_);
    npcDirector_.setSeed(weatherSeed_ ^ 0x4C494645ULL);
    std::vector<sim::Place> places;
    for (const PlacedPlace& place : level_.places) places.push_back({place.name, place.at.x, place.at.y, place.tags});
    npcDirector_.setPlaces(std::move(places));
    for (const PlacedCharacter& placed : level_.characters) {
        if (const int index = npcPopulation_.indexOf(placed.id); index >= 0) npcDirector_.setHome(index, placed.feet.x, placed.feet.y);
    }
    npcStuck_.clear();
    refreshLife();
}

PixelPoint OdysseyGame::npcPosition(int placedId) const {
    for (const PlacedCharacter& figure : bystanders_) {
        if (figure.id == placedId) return figure.feet;
    }
    const PlacedCharacter* placed = placedCharacter(placedId);
    return placed != nullptr ? placed->feet : PixelPoint{};
}

// Every placed person gets the schedule of its classes, its kind and its own fields (the highest layer that has one). What a schedule names that is not there (a place
// the level does not have, an activity nobody knows) is logged with the person's name, and the person stays at home for it.
void OdysseyGame::refreshLife() {
    std::set<std::string> places;
    for (const PlacedPlace& place : level_.places) places.insert(place.name);
    std::set<std::string> activities;
    for (const auto& [word, effect] : scheduleConfig_.activities) activities.insert(word);
    std::set<std::string> interactions;
    for (const sim::rules::Interaction& interaction : interactions_.all()) interactions.insert(interaction.id);
    for (const PlacedCharacter& placed : level_.characters) {
        const int index = npcPopulation_.indexOf(placed.id);
        if (index < 0) continue;
        const sim::rules::ResolvedNpc resolved = npcClasses_.resolve(placed);
        npcDirector_.setSchedule(index, resolved.extras.schedule);
        // What the person is: its classes and tags, the actions of its classes and its own, the lists of what it may and may not do (US-291).
        sim::NpcProfile profile;
        profile.classes = resolved.classes;
        profile.tags = resolved.tags;
        if (std::find(profile.tags.begin(), profile.tags.end(), "npc") == profile.tags.end()) profile.tags.push_back("npc");
        profile.classActions = resolved.classActions;
        profile.customActions = resolved.customActions;
        for (const auto& [id, state] : resolved.actions) {
            if (state == sim::rules::ActionState::Denied) profile.deny.push_back(id);
            else if (state == sim::rules::ActionState::Allowed) profile.allow.push_back(id);
        }
        profile.partnerActions = resolved.extras.partnerActions; // the defaults of the partner types under the NPC's own lists (US-293)
        npcDirector_.setProfile(index, profile);
        npcDirector_.setCombat(index, placed.hp, placed.swordDamage); // the numbers of the hero's own fights (US-292)
        for (const std::vector<std::string>* actions : {&resolved.classActions, &resolved.customActions}) {
            for (const std::string& id : *actions) {
                const sim::rules::Interaction* known = interactions_.find(id);
                if (known == nullptr) core::logWarning(std::format("Actions of {}: \"{}\" is not an interaction", placed.name, id));
                else if (!known->npc) core::logWarning(std::format("Actions of {}: \"{}\" has no npc block, so an NPC never chooses it", placed.name, id));
            }
        }
        for (const std::string& problem : sim::rules::scheduleProblems(resolved.extras.schedule, places, activities, interactions)) core::logWarning(std::format("Schedule of {}: {}", placed.name, problem));
    }
}

// The figures of the placed people walk toward where the director has sent them, 2 pixels a tick, round what is in the way; one that makes no progress for ten seconds is put at
// its goal (the simulation does not know where the walls are).
void OdysseyGame::walkNpcPeople() {
    for (PlacedCharacter& figure : bystanders_) {
        const int index = npcPopulation_.indexOf(figure.id);
        if (index < 0) continue;
        const double toX = npcPopulation_.x(index) - figure.feet.x;
        const double toY = npcPopulation_.y(index) - figure.feet.y;
        const double distance = std::hypot(toX, toY);
        if (distance < 1.0) {
            npcStuck_.erase(figure.id);
            continue;
        }
        const double step = std::min(NpcLife::kWalkPixelsPerTick, distance);
        const luna::engine::Box box{figure.feet.x - 10.0, figure.feet.y - 10.0, 20.0, 10.0};
        const luna::engine::Box moved = luna::engine::moveAndCollide(map_, box, toX / distance * step, toY / distance * step);
        const PixelPoint next{static_cast<int>(std::lround(moved.x + 10.0)), static_cast<int>(std::lround(moved.y + 10.0))};
        if (next == figure.feet) {
            if (++npcStuck_[figure.id] >= 200) {
                figure.feet = {npcPopulation_.x(index), npcPopulation_.y(index)};
                npcStuck_.erase(figure.id);
            }
            continue;
        }
        npcStuck_.erase(figure.id);
        figure.facing = facingToward(toX, toY);
        figure.feet = next;
    }
}

void OdysseyGame::postWorldEvent(const std::string& trigger, int x, int y) { npcDirector_.postEvent(npcPopulation_, trigger, x, y); }

void OdysseyGame::reloadPartnerDefaults() {
    sim::rules::LoadReport report;
    npcClasses_.setPartnerDefaults(sim::rules::loadPartnerDefaults(dataDirectory_ / "interactions", report));
    for (const sim::rules::Diagnostic& error : report.errors) core::logWarning("Partner defaults: " + error.text());
}

// The animals near the persons (placed animals, quiet or hostile) as the director needs them: an id, a kind, a place and the tags of the kind.
void OdysseyGame::feedNpcAnimals() {
    std::vector<sim::AnimalThing> animals;
    for (const PlacedCharacter& placed : bystanders_) {
        const CharacterKindDef* kind = definitions_.character(placed.kind);
        if (kind != nullptr && kind->animal) animals.push_back({placed.id, placed.kind, placed.feet.x, placed.feet.y, kind->tags});
    }
    for (const Enemy& enemy : enemies_) {
        const CharacterKindDef* kind = definitions_.character(enemy.kindName);
        if (enemy.isAlive() && kind != nullptr && kind->animal) animals.push_back({enemy.id, enemy.kindName, static_cast<int>(enemy.feetX()), static_cast<int>(enemy.feetY()), kind->tags});
    }
    npcDirector_.setAnimals(std::move(animals));
}

void OdysseyGame::removeAnimal(int id) {
    std::erase_if(bystanders_, [id](const PlacedCharacter& figure) { return figure.id == id; });
    std::erase_if(enemies_, [id](const Enemy& enemy) { return enemy.id == id; });
}

// What the persons did that is worth showing (US-292): a few words in a bubble over the speaker, and the log for the rest; a death takes the figure out of the world.
void OdysseyGame::drainNpcEvents() {
    for (const sim::NpcEvent& event : npcDirector_.takeEvents()) {
        switch (event.kind) {
        case sim::NpcEvent::Kind::Talk:
            npcBubbles_[event.actor] = {event.text, 60}; // three seconds
            break;
        case sim::NpcEvent::Kind::Trade: core::logInfo(std::format("NPCs: {} and {} traded {}", event.actor, event.target, event.text)); break;
        case sim::NpcEvent::Kind::Gift: core::logInfo(std::format("NPCs: {} gave {} a gift of {}", event.actor, event.target, event.text)); break;
        case sim::NpcEvent::Kind::Confront: core::logInfo(std::format("NPCs: {} confronted {}", event.actor, event.target)); break;
        case sim::NpcEvent::Kind::Fight: core::logInfo(std::format("NPCs: {} and {} fought", event.actor, event.target)); break;
        case sim::NpcEvent::Kind::Death:
            core::logInfo(std::format("NPCs: {} was killed by {}", event.actor, event.target));
            break;
        case sim::NpcEvent::Kind::Hunted:
            core::logInfo(std::format("NPCs: {} hunted the {}", event.actor, event.text));
            removeAnimal(event.target); // the animal leaves the world
            break;
        }
    }
    removeDeadFigures();
}

void OdysseyGame::removeDeadFigures() {
    const auto dead = [this](int placedId) {
        const int index = npcPopulation_.indexOf(placedId);
        return index >= 0 && npcDirector_.mode(index) == sim::NpcDirector::Mode::Dead;
    };
    const std::size_t before = bystanders_.size() + enemies_.size();
    std::erase_if(bystanders_, [&](const PlacedCharacter& figure) { return dead(figure.id); });
    std::erase_if(enemies_, [&](const Enemy& enemy) { return dead(enemy.id); });
    if (bystanders_.size() + enemies_.size() != before) {
        for (auto it = npcBubbles_.begin(); it != npcBubbles_.end();) it = dead(it->first) ? npcBubbles_.erase(it) : std::next(it);
    }
}

// A hostile creature within 6 m of a person sends them home; when it is gone they take up their schedule again (D-54 Q10).
void OdysseyGame::updateNpcDanger() {
    for (const PlacedCharacter& figure : bystanders_) {
        const int index = npcPopulation_.indexOf(figure.id);
        if (index < 0) continue;
        bool danger = false;
        for (const Enemy& enemy : enemies_) {
            if (!enemy.isAlive()) continue;
            const CharacterKindDef* kind = definitions_.character(enemy.kindName);
            if (kind == nullptr || std::find(kind->tags.begin(), kind->tags.end(), "hostile") == kind->tags.end()) continue;
            if (std::hypot(enemy.feetX() - figure.feet.x, enemy.feetY() - figure.feet.y) <= NpcLife::kDangerMetres * kTileSize) {
                danger = true;
                break;
            }
        }
        npcDirector_.setDanger(index, danger);
    }
}

sim::ItemCounts OdysseyGame::itemValues() const {
    sim::ItemCounts out;
    if (const sim::HeroData* data = heroData()) {
        for (const sim::Item& item : data->items) out[item.id] = item.value;
    }
    return out;
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
    if (ticks_ % 20 == 0) {
        updateNpcDanger();
        feedNpcAnimals(); // the director sees the animals that are near (US-293)
    }
    npcDirector_.tick(npcPopulation_); // schedules, interruptions, their own actions and their dealings with each other (US-290..US-292)
    drainNpcEvents();
    walkNpcPeople();
    for (auto it = npcBubbles_.begin(); it != npcBubbles_.end();) it = --it->second.second <= 0 ? npcBubbles_.erase(it) : std::next(it);
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
        sim::writeSaveText(saveDirectory_ / "npc-life.json", npcDirector_.toText());
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
        if (std::ifstream lifeIn(saveDirectory_ / "npc-life.json", std::ios::binary); lifeIn) {
            const std::string lifeText((std::istreambuf_iterator<char>(lifeIn)), std::istreambuf_iterator<char>());
            std::vector<sim::Place> places = npcDirector_.places(); // the places of the level, not of the save
            npcDirector_ = sim::NpcDirector::fromText(lifeText, scheduleConfig_);
            npcDirector_.setPlaces(std::move(places));
            refreshLife(); // the data of now wins over the schedules of the save; the homes and the modes stay
            npcDirector_.setMarket(&tradeMarket_);
            removeDeadFigures(); // those who died before the save do not come back
        }
        return {};
    } catch (const std::exception& error) {
        return std::string("The saved people could not be read: ") + error.what();
    }
}

} // namespace odysseus::game
