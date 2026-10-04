#include "sim/npc_schedule.h"

#include "sim/data.h"
#include "sim/json_data.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <format>

namespace odysseus::sim::rules {

std::optional<int> parseClock(std::string_view text) {
    if (text.size() != 5 || text[2] != ':') return std::nullopt;
    int hour = 0;
    int minute = 0;
    const auto [hourStop, hourError] = std::from_chars(text.data(), text.data() + 2, hour);
    const auto [minuteStop, minuteError] = std::from_chars(text.data() + 3, text.data() + 5, minute);
    if (hourError != std::errc() || hourStop != text.data() + 2 || minuteError != std::errc() || minuteStop != text.data() + 5) return std::nullopt;
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return std::nullopt;
    return hour * 60 + minute;
}

std::string formatClock(int minute) {
    const int clamped = std::clamp(minute, 0, 24 * 60 - 1);
    return std::format("{:02}:{:02}", clamped / 60, clamped % 60);
}

const ScheduleBlock* activeBlock(const Schedule& schedule, int minuteOfDay, bool night) {
    const std::vector<ScheduleBlock>* blocks = night && !schedule.night.empty() ? &schedule.night : &schedule.day;
    if (blocks->empty()) blocks = &schedule.night; // a schedule of a night only holds all day
    if (blocks->empty()) return nullptr;
    const ScheduleBlock* found = &blocks->back(); // before the first block of the day: the one that came over midnight
    for (const ScheduleBlock& block : *blocks) {
        if (block.minute <= minuteOfDay) found = &block;
    }
    return found;
}

std::string scheduleText(const std::vector<ScheduleBlock>& blocks) {
    std::string out;
    for (const ScheduleBlock& block : blocks) out += std::format("{}{} {} {}", out.empty() ? "" : "; ", formatClock(block.minute), block.activity, block.place.empty() ? "home" : block.place);
    return out;
}

std::optional<std::vector<ScheduleBlock>> parseScheduleText(std::string_view text, std::string& problem) {
    std::vector<ScheduleBlock> blocks;
    std::size_t at = 0;
    while (at <= text.size()) {
        std::size_t end = text.find_first_of(";\n", at);
        if (end == std::string_view::npos) end = text.size();
        const std::string_view part = text.substr(at, end - at);
        at = end + 1;
        std::vector<std::string> words;
        std::size_t w = 0;
        while (w < part.size()) {
            while (w < part.size() && (part[w] == ' ' || part[w] == '\t' || part[w] == ',')) ++w;
            std::size_t stop = w;
            while (stop < part.size() && part[stop] != ' ' && part[stop] != '\t' && part[stop] != ',') ++stop;
            if (stop > w) words.emplace_back(part.substr(w, stop - w));
            w = stop;
        }
        if (words.empty()) continue;
        if (words.size() < 2 || words.size() > 3) {
            problem = std::format("\"{}\" must be time activity place, for example 06:00 work market", std::string(part));
            return std::nullopt;
        }
        const std::optional<int> minute = parseClock(words[0]);
        if (!minute) {
            problem = std::format("\"{}\" is not a time (HH:MM, like 06:00)", words[0]);
            return std::nullopt;
        }
        const std::string place = words.size() == 3 ? words[2] : std::string("home");
        if (!validItemId(words[1]) || !validItemId(place)) {
            problem = std::format("\"{}\": the activity and the place are words (lower-case letters, digits and -)", std::string(part));
            return std::nullopt;
        }
        blocks.push_back({*minute, words[1], place});
    }
    std::stable_sort(blocks.begin(), blocks.end(), [](const ScheduleBlock& a, const ScheduleBlock& b) { return a.minute < b.minute; });
    for (std::size_t i = 1; i < blocks.size(); ++i) {
        if (blocks[i].minute == blocks[i - 1].minute) {
            problem = std::format("two blocks begin at {}", formatClock(blocks[i].minute));
            return std::nullopt;
        }
    }
    return blocks;
}

std::vector<std::string> scheduleProblems(const Schedule& schedule, const std::set<std::string>& places, const std::set<std::string>& activities, const std::set<std::string>& interactions) {
    std::vector<std::string> out;
    for (const std::vector<ScheduleBlock>* blocks : {&schedule.day, &schedule.night}) {
        for (const ScheduleBlock& block : *blocks) {
            if (block.place != "home" && places.count(block.place) == 0) out.push_back(std::format("{}: \"{}\" is not a place of this level", formatClock(block.minute), block.place));
            if (activities.count(block.activity) == 0 && interactions.count(block.activity) == 0) {
                out.push_back(std::format("{}: \"{}\" is neither an activity (see schedule.json) nor an interaction", formatClock(block.minute), block.activity));
            }
        }
    }
    return out;
}

ScheduleConfig loadScheduleConfig(const std::filesystem::path& file) {
    using nlohmann::json;
    ScheduleConfig config;
    const json data = readJsonFile(file);
    if (requireInt(data, file, "version", 1, 1) != 1) throw DataError(file, "version", "must be 1");
    if (data.contains("nightFromHour")) config.nightFromHour = requireInt(data, file, "nightFromHour", 0, 23);
    if (data.contains("nightToHour")) config.nightToHour = requireInt(data, file, "nightToHour", 0, 23);
    if (data.contains("eat")) {
        const json& eat = data.at("eat");
        if (!eat.is_object()) throw DataError(file, "eat", "must be an object");
        if (eat.contains("below")) config.eatBelow = requireInt(eat, file, "eat", "below", 0, 100);
        if (eat.contains("restore")) config.eatRestore = requireInt(eat, file, "eat", "restore", 1, 100);
        if (eat.contains("place")) {
            if (!eat.at("place").is_string()) throw DataError(file, "eat.place", "must be a place name");
            config.eatPlace = eat.at("place").get<std::string>();
        }
    }
    if (data.contains("dangerPlace")) {
        if (!data.at("dangerPlace").is_string()) throw DataError(file, "dangerPlace", "must be a place name");
        config.dangerPlace = data.at("dangerPlace").get<std::string>();
    }
    if (data.contains("scatterPixels")) config.scatterPixels = requireInt(data, file, "scatterPixels", 0, 512);
    if (data.contains("activities")) {
        if (!data.at("activities").is_object()) throw DataError(file, "activities", "must be an object of activity: {need: amount}");
        for (const auto& [word, effects] : data.at("activities").items()) {
            if (!validItemId(word)) throw DataError(file, "activities." + word, "is not a word (lower-case letters, digits and -)");
            if (!effects.is_object()) throw DataError(file, "activities." + word, "must be an object such as { \"energy\": 12 }");
            std::array<int, kNeedCount> restore{};
            for (const auto& [key, amount] : effects.items()) {
                bool known = false;
                for (std::size_t n = 0; n < kNeedCount; ++n) {
                    std::string lower = needName(static_cast<Need>(n));
                    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    if (lower == key) {
                        known = true;
                        if (!amount.is_number_integer() || amount.get<int>() < 0 || amount.get<int>() > 100) throw DataError(file, "activities." + word + "." + key, "must be a whole number from 0 to 100");
                        restore[n] = amount.get<int>();
                    }
                }
                if (!known) throw DataError(file, "activities." + word + "." + key, "is not a need (hunger, energy, warmth, social)");
            }
            config.activities[word] = restore;
        }
    }
    return config;
}

} // namespace odysseus::sim::rules
