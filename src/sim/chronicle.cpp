#include "sim/chronicle.h"

#include <algorithm>
#include <utility>

namespace odysseus::sim {

int Chronicle::add(const Date& date, int importance, std::string text) {
    return record(date, importance, EventKind::Note, -1, -1, -1, {}, std::move(text));
}

int Chronicle::record(const Date& date, int importance, EventKind kind, int who, int other, int aux, std::vector<int> causes,
                      std::string text) {
    const int id = static_cast<int>(entries_.size());
    entries_.push_back({date, importance, std::move(text), id, kind, who, other, aux, std::move(causes)});
    return id;
}

const ChronicleEntry* Chronicle::find(int id) const {
    return id >= 0 && id < static_cast<int>(entries_.size()) ? &entries_[static_cast<std::size_t>(id)] : nullptr;
}

std::string joinNames(const std::vector<std::string>& names) {
    std::string joined;
    for (std::size_t i = 0; i < names.size(); ++i) {
        joined += i == 0 ? "" : (i + 1 == names.size() ? " and " : ", ");
        joined += names[i];
    }
    return joined;
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

namespace {

constexpr int kExplainDepth = 6;

void explainInto(const Chronicle& chronicle, int id, int depth, std::vector<int>& seen, std::vector<std::string>& lines) {
    const ChronicleEntry* entry = chronicle.find(id);
    if (entry == nullptr) {
        return;
    }
    lines.push_back(std::string(static_cast<std::size_t>(depth) * 2, ' ') + (depth > 0 ? "because " : "") + "[#" +
                    std::to_string(id) + "] " + formatEntry(*entry));
    if (depth >= kExplainDepth) {
        return;
    }
    for (const int cause : entry->causes) {
        if (std::find(seen.begin(), seen.end(), cause) == seen.end()) {
            seen.push_back(cause);
            explainInto(chronicle, cause, depth + 1, seen, lines);
        }
    }
}

} // namespace

std::vector<std::string> explainEvent(const Chronicle& chronicle, int id) {
    std::vector<std::string> lines;
    std::vector<int> seen{id};
    explainInto(chronicle, id, 0, seen, lines);
    return lines;
}

} // namespace odysseus::sim
