#include "sim/chronicle.h"

#include <utility>

namespace odysseus::sim {

void Chronicle::add(const Date& date, int importance, std::string text) {
    entries_.push_back({date, importance, std::move(text)});
}

std::vector<ChronicleEntry> Chronicle::select(int year, int threshold) const {
    std::vector<ChronicleEntry> chosen;
    for (const ChronicleEntry& entry : entries_) {
        if ((year == 0 || entry.date.year == year) && entry.importance >= threshold) {
            chosen.push_back(entry);
        }
    }
    return chosen;
}

std::string formatEntry(const ChronicleEntry& entry) {
    return describe(entry.date) + ": " + entry.text;
}

} // namespace odysseus::sim
