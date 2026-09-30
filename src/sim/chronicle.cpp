#include "sim/chronicle.h"

#include <utility>

namespace odysseus::sim {

void Chronicle::add(const Date& date, int importance, std::string text) {
    entries_.push_back({date, importance, std::move(text)});
}

} // namespace odysseus::sim
