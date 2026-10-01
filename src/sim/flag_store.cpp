#include "sim/flag_store.h"

#include <nlohmann/json.hpp>

#include <format>

namespace odysseus::sim::rules {

void FlagStore::set(const std::string& name, int value) {
    if (name.empty()) return;
    if (value == 0) flags_.erase(name); // never set and set to 0 are the same thing, so they save the same
    else flags_[name] = value;
}

int FlagStore::get(const std::string& name) const {
    const auto found = flags_.find(name);
    return found == flags_.end() ? 0 : found->second;
}

std::string FlagStore::save() const {
    nlohmann::json out = nlohmann::json::object();
    for (const auto& [name, value] : flags_) out[name] = value;
    return out.dump();
}

std::vector<std::string> FlagStore::load(const std::string& text) {
    std::vector<std::string> notes;
    flags_.clear();
    try {
        const nlohmann::json data = nlohmann::json::parse(text);
        if (!data.is_object()) return {"the flags are not an object and were not loaded"};
        for (const auto& [name, value] : data.items()) {
            if (!value.is_number_integer()) {
                notes.push_back(std::format("flag \"{}\" has no whole number and was left out", name));
                continue;
            }
            set(name, value.get<int>());
        }
    } catch (const std::exception& error) {
        notes.push_back(std::string("the flags could not be read: ") + error.what());
    }
    return notes;
}

std::uint64_t FlagStore::hash() const {
    std::uint64_t h = 0xCBF29CE484222325ULL;
    const auto mix = [&h](std::uint64_t value) { h ^= value + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2); };
    for (const auto& [name, value] : flags_) {
        for (const char c : name) mix(static_cast<unsigned char>(c));
        mix(static_cast<std::uint64_t>(static_cast<std::int64_t>(value)));
    }
    return h;
}

} // namespace odysseus::sim::rules
