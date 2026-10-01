#pragma once

#include "boundary.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace odysseus::game {

// Local session statistics (US-092), opt-in: how long a session lasted and its key events, written to one JSON file on this
// computer when the session ends. Nothing is ever sent anywhere (the game has no network code at all). When the player has
// not agreed, record() does nothing and finish() writes nothing.
class SessionStats {
public:
    void enable(bool on) { enabled_ = on; }
    bool enabled() const { return enabled_; }
    void record(const std::string& event, std::uint64_t tick);
    // Writes `<folder>/session-<stamp>.json` with the play time and the events; returns the file, or an empty path when off
    // (or when this session was already written).
    std::filesystem::path finish(const std::filesystem::path& folder, std::uint64_t ticks, const std::string& stamp);
    std::size_t events() const { return events_.size(); }

private:
    struct Event {
        std::string name;
        std::uint64_t tick = 0;
    };
    bool enabled_ = false;
    bool written_ = false;
    std::vector<Event> events_;
};

// "20261001-153000" from the computer's clock; the game layer may read the clock (only the simulation may not).
std::string sessionStamp();

} // namespace odysseus::game
