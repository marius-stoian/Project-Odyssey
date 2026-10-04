// US-262: the placed people of a level are persons of the simulation (sim::NpcPopulation); animals and monsters stay creatures.
#include "game/odyssey_game.h"

#include "core/log.h"
#include "sim/npc_population.h"
#include "sim/save.h"

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

bool OdysseyGame::fightsHero(const PlacedCharacter& placed) const {
    const CharacterKindDef* kind = definitions_.character(placed.kind);
    if (kind == nullptr) return false;
    if (npcClasses_.kinds().find(placed.kind) == nullptr) return kind->enemy; // no kind file: the switch of characters.json
    return npcClasses_.resolve(placed).attitude == "hostile";
}

std::string OdysseyGame::attitudeWordOf(int placedId) const {
    if (const int index = npcPopulation_.indexOf(placedId); index >= 0) return sim::attitudeName(npcPopulation_.attitude(placedId, sim::NpcPopulation::kHero));
    for (const PlacedCharacter& placed : level_.characters) {
        if (placed.id == placedId) return npcClasses_.resolve(placed).attitude;
    }
    return "neutral";
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
}

// One game tick of the placed people: the clock of their days, and meeting the hero (found through the grid, so a crowd costs nothing here).
void OdysseyGame::tickNpcPopulation() {
    npcPopulation_.setFocus(static_cast<int>(hero_.feetX()), static_cast<int>(hero_.feetY())); // the hero is the centre of the detail (ADR-022)
    npcPopulation_.tick();
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
        return {};
    } catch (const std::exception& error) {
        return std::string("The saved people could not be read: ") + error.what();
    }
}

} // namespace odysseus::game
