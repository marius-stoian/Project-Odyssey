#include "sim/building_life.h"

#include <nlohmann/json.hpp>

#include <algorithm>

namespace odysseus::sim::buildings {

namespace {

void mix(std::uint64_t& h, std::uint64_t value) {
    h ^= value + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2);
    h *= 0x100000001B3ULL;
}

const std::vector<RivalBuilding>& noBuildings() {
    static const std::vector<RivalBuilding> none;
    return none;
}

} // namespace

void RivalBuilders::reset(int clans, std::uint64_t seed) {
    buildings_.assign(static_cast<std::size_t>(std::max(0, clans)), {});
    seed_ = seed;
}

int RivalBuilders::buildMilliOf(const BuildingData& data, const std::string& kind) {
    const KindDef* def = data.kind(kind);
    if (def == nullptr) return 0;
    return def->buildMilli > 0 ? def->buildMilli : layoutBuildMilli(data, def->layout);
}

bool RivalBuilders::has(int clan, const BuildingData& data, const std::string& use) const {
    for (const RivalBuilding& building : of(clan)) {
        if (!building.finished) continue;
        const KindDef* def = data.kind(building.kind);
        if (def != nullptr && std::find(def->uses.begin(), def->uses.end(), use) != def->uses.end()) return true;
    }
    return false;
}

void RivalBuilders::seasonStarted(const BuildingData& data, const std::vector<int>& people) {
    for (std::size_t clan = 0; clan < buildings_.size(); ++clan) {
        const int count = clan < people.size() ? people[clan] : 0;
        if (count < kMinPeople) continue;
        bool busy = false;
        for (const RivalBuilding& building : buildings_[clan]) busy = busy || !building.finished;
        if (busy) continue;
        for (const KindDef& kind : data.kinds()) {
            if (!kind.buildable || kind.uses.empty()) continue;
            const bool lacking = std::any_of(kind.uses.begin(), kind.uses.end(), [&](const std::string& use) { return !has(static_cast<int>(clan), data, use); });
            if (!lacking) continue;
            buildings_[clan].push_back({kind.id, 0, false});
            break;
        }
    }
}

void RivalBuilders::dayEnded(const BuildingData& data, const std::vector<int>& people) {
    for (std::size_t clan = 0; clan < buildings_.size(); ++clan) {
        const int count = clan < people.size() ? people[clan] : 0;
        for (RivalBuilding& building : buildings_[clan]) {
            if (building.finished) continue;
            building.workMilli += count * kWorkMilliPerPersonDay;
            if (building.workMilli >= buildMilliOf(data, building.kind)) building.finished = true;
            break; // one job at a time
        }
    }
}

const std::vector<RivalBuilding>& RivalBuilders::of(int clan) const {
    return clan >= 0 && static_cast<std::size_t>(clan) < buildings_.size() ? buildings_[static_cast<std::size_t>(clan)] : noBuildings();
}

int RivalBuilders::finished(int clan) const {
    int n = 0;
    for (const RivalBuilding& building : of(clan)) n += building.finished ? 1 : 0;
    return n;
}

std::uint64_t RivalBuilders::hash() const {
    std::uint64_t h = 0xCBF29CE484222325ULL;
    mix(h, seed_);
    mix(h, buildings_.size());
    for (const auto& clan : buildings_) {
        mix(h, clan.size());
        for (const RivalBuilding& building : clan) {
            mix(h, building.kind.size());
            for (const char c : building.kind) mix(h, static_cast<unsigned char>(c));
            mix(h, static_cast<std::uint64_t>(building.workMilli));
            mix(h, building.finished ? 1 : 0);
        }
    }
    return h;
}

std::string RivalBuilders::toJson() const {
    nlohmann::json out;
    out["seed"] = seed_;
    nlohmann::json clans = nlohmann::json::array();
    for (const auto& clan : buildings_) {
        nlohmann::json list = nlohmann::json::array();
        for (const RivalBuilding& building : clan) list.push_back({{"kind", building.kind}, {"workMilli", building.workMilli}, {"finished", building.finished}});
        clans.push_back(list);
    }
    out["clans"] = clans;
    return out.dump(1);
}

bool RivalBuilders::fromJson(const std::string& text, std::string& problem) {
    const nlohmann::json in = nlohmann::json::parse(text, nullptr, false);
    if (in.is_discarded() || !in.is_object() || !in.contains("clans") || !in.at("clans").is_array()) {
        problem = "rival buildings: not a valid file";
        return false;
    }
    std::vector<std::vector<RivalBuilding>> loaded;
    for (const auto& clan : in.at("clans")) {
        std::vector<RivalBuilding> list;
        if (clan.is_array()) {
            for (const auto& entry : clan) {
                if (!entry.is_object() || !entry.contains("kind") || !entry.at("kind").is_string()) continue;
                list.push_back({entry.at("kind").get<std::string>(), entry.value("workMilli", 0), entry.value("finished", false)});
            }
        }
        loaded.push_back(std::move(list));
    }
    buildings_ = std::move(loaded);
    seed_ = in.value("seed", static_cast<std::uint64_t>(0));
    return true;
}

} // namespace odysseus::sim::buildings
