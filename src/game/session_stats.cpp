#include "game/session_stats.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <format>
#include <fstream>

namespace odysseus::game {

void SessionStats::record(const std::string& event, std::uint64_t tick) {
    if (!enabled_) return;
    events_.push_back({event, tick});
}

std::filesystem::path SessionStats::finish(const std::filesystem::path& folder, std::uint64_t ticks, const std::string& stamp) {
    if (!enabled_ || written_) return {};
    written_ = true;
    std::filesystem::create_directories(folder);
    nlohmann::json data;
    data["version"] = 1;
    data["playSeconds"] = ticks / 20; // 20 game ticks a second
    nlohmann::json list = nlohmann::json::array();
    for (const Event& event : events_) list.push_back({{"event", event.name}, {"second", event.tick / 20}});
    data["events"] = list;
    const std::filesystem::path file = folder / ("session-" + stamp + ".json");
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    out << data.dump(1) << '\n';
    return file;
}

std::string sessionStamp() {
    const auto now = std::chrono::zoned_time{std::chrono::current_zone(), std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now())};
    return std::format("{:%Y%m%d-%H%M%S}", now.get_local_time());
}

} // namespace odysseus::game
