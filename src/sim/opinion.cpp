#include "sim/opinion.h"

#include "sim/data.h"
#include "sim/json_data.h"

#include <algorithm>

namespace odysseus::sim {

namespace {

constexpr const char* kNames[kAttitudeCount] = {"friendly", "neutral", "wary", "hostile", "scared", "suspicious", "enchanted", "lovingly", "enviously"};

int readOpinion(const nlohmann::json& object, const std::filesystem::path& file, const std::string& section, const std::string& field) {
    return requireInt(object, file, section, field, OpinionConfig::kMin, OpinionConfig::kMax);
}

} // namespace

const char* attitudeName(Attitude attitude) { return kNames[static_cast<std::size_t>(attitude)]; }

std::optional<Attitude> attitudeFromName(std::string_view name) {
    for (std::size_t i = 0; i < kAttitudeCount; ++i) {
        if (name == kNames[i]) return static_cast<Attitude>(i);
    }
    return std::nullopt;
}

Attitude attitudeFor(const OpinionConfig& config, int opinion, Mood mood) {
    if (mood == Mood::Scared) return Attitude::Scared;
    if (mood == Mood::Enviously) return Attitude::Enviously;
    Attitude word = config.bands.front().word;
    for (const OpinionConfig::Band& band : config.bands) {
        if (opinion >= band.from) word = band.word;
    }
    return word;
}

int startOpinion(const OpinionConfig& config, Attitude attitude) { return config.start[static_cast<std::size_t>(attitude)]; }

Mood startMood(Attitude attitude) {
    if (attitude == Attitude::Scared) return Mood::Scared;
    if (attitude == Attitude::Enviously) return Mood::Enviously;
    return Mood::None;
}

OpinionConfig loadOpinionConfig(const std::filesystem::path& file) {
    OpinionConfig config;
    const nlohmann::json data = readJsonFile(file);
    if (requireInt(data, file, "version", 1, 1) != 1) throw DataError(file, "version", "must be 1");
    // bands: where each band of the scale begins, in the order hostile, wary, suspicious, neutral, friendly, enchanted, lovingly.
    const nlohmann::json& bands = data.contains("bands") ? data.at("bands") : nlohmann::json::object();
    if (!bands.is_object()) throw DataError(file, "bands", "must be an object");
    int previous = OpinionConfig::kMin - 1;
    for (OpinionConfig::Band& band : config.bands) {
        const std::string name = attitudeName(band.word);
        if (bands.contains(name)) band.from = readOpinion(data, file, "bands", name);
        if (band.from <= previous) throw DataError(file, "bands." + name, "must begin higher than the band before it");
        previous = band.from;
    }
    if (config.bands.front().from != OpinionConfig::kMin) throw DataError(file, "bands.hostile", "must be -100: the lowest band takes everything below");
    if (data.contains("start")) {
        const nlohmann::json& start = data.at("start");
        if (!start.is_object()) throw DataError(file, "start", "must be an object");
        for (std::size_t i = 0; i < kAttitudeCount; ++i) {
            if (start.contains(kNames[i])) config.start[i] = readOpinion(data, file, "start", kNames[i]);
        }
        for (const auto& [name, value] : start.items()) {
            if (!attitudeFromName(name)) throw DataError(file, "start." + name, "is not an attitude word");
            (void)value;
        }
    }
    if (data.contains("events")) {
        const nlohmann::json& events = data.at("events");
        if (!events.is_object()) throw DataError(file, "events", "must be an object of name: amount");
        config.events.clear();
        for (const auto& [name, value] : events.items()) {
            if (!value.is_number_integer() || value.get<int>() < -200 || value.get<int>() > 200) throw DataError(file, "events." + name, "must be a whole number from -200 to 200");
            config.events[name] = value.get<int>();
        }
    }
    if (data.contains("sameFamily")) config.sameFamily = requireInt(data, file, "sameFamily", -200, 200);
    if (data.contains("hearingTiles")) config.hearingTiles = requireInt(data, file, "hearingTiles", 1, 60);
    if (data.contains("talk")) {
        const nlohmann::json& talk = data.at("talk");
        if (!talk.is_object()) throw DataError(file, "talk", "must be an object");
        static constexpr const char* kQuality[5] = {"awful", "bad", "plain", "good", "great"};
        for (std::size_t i = 0; i < 5; ++i) {
            if (talk.contains(kQuality[i])) config.talk[i] = requireInt(data, file, "talk", kQuality[i], -200, 200);
        }
        if (talk.contains("frequencyBonus")) config.talkFrequencyBonus = requireInt(data, file, "talk", "frequencyBonus", 0, 100);
        if (talk.contains("frequencyDays")) config.talkFrequencyDays = requireInt(data, file, "talk", "frequencyDays", 1, 365);
    }
    return config;
}

} // namespace odysseus::sim
