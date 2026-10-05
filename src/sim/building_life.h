#pragma once

#include "boundary.h"

#include "sim/building_data.h"

#include <cstdint>
#include <string>
#include <vector>

namespace odysseus::sim::buildings {

// A building of a rival clan (US-253). Rival camps lie far from the level the hero plays on, so a rival's buildings are kept as a list of kinds
// and progress, not as cells of the level; the hero meets their effect (a clan with a hut is a clan with shelter), not their walls.
struct RivalBuilding {
    std::string kind;
    int workMilli = 0;       // work done so far
    bool finished = false;
    friend bool operator==(const RivalBuilding&, const RivalBuilding&) = default;
};

// Rival clans build as the player's clan does, in whole numbers and by the calendar: at the start of every season a clan of at least
// `kMinPeople` that has no job picks the first kind (in file order) whose use it lacks and works on it every day until it is finished.
class RivalBuilders {
public:
    static constexpr int kMinPeople = 6;
    static constexpr int kWorkMilliPerPersonDay = 4000;   // each person gives four seconds of work a day

    void reset(int clans, std::uint64_t seed);
    int clans() const { return static_cast<int>(buildings_.size()); }
    void seasonStarted(const BuildingData& data, const std::vector<int>& people);
    void dayEnded(const BuildingData& data, const std::vector<int>& people);
    const std::vector<RivalBuilding>& of(int clan) const;
    int finished(int clan) const;
    bool has(int clan, const BuildingData& data, const std::string& use) const; // a finished building of that use
    std::uint64_t hash() const;
    std::string toJson() const;
    bool fromJson(const std::string& text, std::string& problem);

private:
    static int buildMilliOf(const BuildingData& data, const std::string& kind);
    std::vector<std::vector<RivalBuilding>> buildings_;
    std::uint64_t seed_ = 0;
};

} // namespace odysseus::sim::buildings
