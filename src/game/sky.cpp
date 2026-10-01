#include "game/sky.h"

#include "sim/data.h"
#include "sim/json_data.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <numbers>

namespace odysseus::game {

using nlohmann::json;

namespace {

constexpr std::array<const char*, 4> kSeasons{"Spring", "Summer", "Autumn", "Winter"};

double numberIn(const json& object, const std::filesystem::path& file, const std::string& where, const std::string& field, double low, double high) {
    if (!object.contains(field) || !object.at(field).is_number() || object.at(field).get<double>() < low || object.at(field).get<double>() > high) {
        throw sim::DataError(file, where.empty() ? field : where + "." + field, std::format("must be a number from {} to {}", low, high));
    }
    return object.at(field).get<double>();
}

double anchorHour(const SkyData& data, const SkyKeyframe& key, int season) {
    const Daylight& day = data.daylight[season];
    return (key.anchor == "sunrise" ? day.sunrise : day.sunset) + key.offset;
}

double wrap24(double hour) {
    hour = std::fmod(hour, 24.0);
    return hour < 0.0 ? hour + 24.0 : hour;
}

} // namespace

SkyData loadSky(const std::filesystem::path& skyFile, const std::filesystem::path& calendarFile) {
    SkyData data;
    // The daylight of the seasons, from calendar.json (the simulation reads the same file and ignores this part).
    if (std::filesystem::exists(calendarFile)) {
        const json calendar = sim::readJsonFile(calendarFile);
        if (calendar.contains("daylight")) {
            const json& daylight = calendar.at("daylight");
            if (!daylight.is_object()) throw sim::DataError(calendarFile, "daylight", "must list the four seasons");
            for (std::size_t s = 0; s < kSeasons.size(); ++s) {
                const std::string where = std::string("daylight.") + kSeasons[s];
                if (!daylight.contains(kSeasons[s]) || !daylight.at(kSeasons[s]).is_object()) {
                    throw sim::DataError(calendarFile, where, "must have a sunrise and a sunset hour");
                }
                const json& season = daylight.at(kSeasons[s]);
                data.daylight[s].sunrise = numberIn(season, calendarFile, where, "sunrise", 0.0, 24.0);
                data.daylight[s].sunset = numberIn(season, calendarFile, where, "sunset", 0.0, 24.0);
                if (data.daylight[s].sunset - data.daylight[s].sunrise < 4.0) {
                    throw sim::DataError(calendarFile, where, "the day must be at least four hours long (sunrise before sunset)");
                }
            }
        }
    }
    if (!std::filesystem::exists(skyFile)) {
        data.enabled = false; // no sky: plain bright day
        return data;
    }
    const json root = sim::readJsonFile(skyFile);
    if (!root.is_object() || !root.contains("version") || !root.at("version").is_number_integer() || root.at("version").get<int>() != 1) {
        throw sim::DataError(skyFile, "version", "must be 1");
    }
    if (root.contains("enabled")) {
        if (!root.at("enabled").is_boolean()) throw sim::DataError(skyFile, "enabled", "must be true or false");
        data.enabled = root.at("enabled").get<bool>();
    }
    if (!root.contains("keyframes") || !root.at("keyframes").is_array() || root.at("keyframes").size() < 2) {
        throw sim::DataError(skyFile, "keyframes", "must be a list of at least two keyframes");
    }
    for (std::size_t i = 0; i < root.at("keyframes").size(); ++i) {
        const json& entry = root.at("keyframes").at(i);
        const std::string where = std::format("keyframes[{}]", i);
        SkyKeyframe key;
        if (!entry.is_object() || !entry.contains("anchor") || !entry.at("anchor").is_string() ||
            (entry.at("anchor").get<std::string>() != "sunrise" && entry.at("anchor").get<std::string>() != "sunset")) {
            throw sim::DataError(skyFile, where + ".anchor", "must be \"sunrise\" or \"sunset\"");
        }
        key.anchor = entry.at("anchor").get<std::string>();
        key.offset = numberIn(entry, skyFile, where, "offset", -6.0, 6.0);
        if (!entry.contains("color") || !entry.at("color").is_array() || entry.at("color").size() != 3) {
            throw sim::DataError(skyFile, where + ".color", "must be three numbers: [red, green, blue], each 0 to 255");
        }
        int* parts[3] = {&key.red, &key.green, &key.blue};
        for (std::size_t c = 0; c < 3; ++c) {
            const json& part = entry.at("color").at(c);
            if (!part.is_number_integer() || part.get<int>() < 0 || part.get<int>() > 255) {
                throw sim::DataError(skyFile, std::format("{}.color[{}]", where, c), "must be a whole number from 0 to 255");
            }
            *parts[c] = part.get<int>();
        }
        key.strength = numberIn(entry, skyFile, where, "strength", 0.0, 2.0);
        key.shadow = numberIn(entry, skyFile, where, "shadow", 0.0, 1.0);
        data.keyframes.push_back(key);
    }
    if (root.contains("sunPeakDegrees")) {
        for (std::size_t s = 0; s < kSeasons.size(); ++s) data.sunPeakDegrees[s] = numberIn(root.at("sunPeakDegrees"), skyFile, "sunPeakDegrees", kSeasons[s], 5.0, 90.0);
    }
    if (root.contains("moonPeakDegrees")) data.moonPeakDegrees = numberIn(root, skyFile, "", "moonPeakDegrees", 5.0, 90.0);
    return data;
}

std::string skyToText(const SkyData& data) {
    json root;
    root["version"] = 1;
    root["enabled"] = data.enabled;
    root["keyframes"] = json::array();
    for (const SkyKeyframe& key : data.keyframes) {
        root["keyframes"].push_back({{"anchor", key.anchor}, {"offset", key.offset}, {"color", {key.red, key.green, key.blue}},
                                     {"strength", key.strength}, {"shadow", key.shadow}});
    }
    root["sunPeakDegrees"] = json::object();
    for (std::size_t s = 0; s < kSeasons.size(); ++s) root["sunPeakDegrees"][kSeasons[s]] = data.sunPeakDegrees[s];
    root["moonPeakDegrees"] = data.moonPeakDegrees;
    return root.dump(2);
}

SkyState skyAt(const SkyData& data, double hour, int season) {
    SkyState state;
    season = std::clamp(season, 0, 3);
    state.sunrise = data.daylight[season].sunrise;
    state.sunset = data.daylight[season].sunset;
    if (!data.enabled || data.keyframes.size() < 2) {
        return state; // full day, no tint
    }
    hour = wrap24(hour);

    // The keyframes of this season in time order; the light between two of them is a straight blend, and the last joins the first over midnight.
    struct Timed {
        double at;
        const SkyKeyframe* key;
    };
    std::vector<Timed> keys;
    for (const SkyKeyframe& key : data.keyframes) keys.push_back({wrap24(anchorHour(data, key, season)), &key});
    std::stable_sort(keys.begin(), keys.end(), [](const Timed& a, const Timed& b) { return a.at < b.at; });
    std::size_t previous = keys.size() - 1; // before the first keyframe of the day: the last of yesterday
    for (std::size_t i = 0; i < keys.size(); ++i) {
        if (keys[i].at <= hour) previous = i;
    }
    const std::size_t next = (previous + 1) % keys.size();
    double from = keys[previous].at;
    double to = keys[next].at;
    double now = hour;
    if (to <= from) to += 24.0;      // the next keyframe is after midnight
    if (now < from) now += 24.0;     // and so is now
    const double span = to - from;
    const double t = span > 0.0 ? std::clamp((now - from) / span, 0.0, 1.0) : 0.0;
    const SkyKeyframe& a = *keys[previous].key;
    const SkyKeyframe& b = *keys[next].key;
    const auto mix = [t](double x, double y) { return x + (y - x) * t; };
    state.ambientR = static_cast<float>(mix(a.red / 255.0 * a.strength, b.red / 255.0 * b.strength));
    state.ambientG = static_cast<float>(mix(a.green / 255.0 * a.strength, b.green / 255.0 * b.strength));
    state.ambientB = static_cast<float>(mix(a.blue / 255.0 * a.strength, b.blue / 255.0 * b.strength));
    state.shadowStrength = mix(a.shadow, b.shadow);

    // The sun crosses the sky from east to west between sunrise and sunset; the moon takes the night the same way.
    constexpr double kPi = std::numbers::pi;
    const double day = state.sunset - state.sunrise;
    const double night = 24.0 - day;
    state.sunUp = hour >= state.sunrise && hour <= state.sunset;
    const double sinceSunrise = hour - state.sunrise;
    const double sinceSunset = wrap24(hour - state.sunset);
    if (state.sunUp) {
        const double f = sinceSunrise / day;
        state.sunElevation = data.sunPeakDegrees[season] * std::sin(kPi * f);
        state.sunAzimuth = 90.0 + 180.0 * f;
        state.moonElevation = -data.moonPeakDegrees * std::sin(kPi * f);
        state.moonAzimuth = 90.0 + 180.0 * f + 180.0;
    } else {
        const double f = sinceSunset / night;
        state.sunElevation = -data.sunPeakDegrees[season] * std::sin(kPi * f);
        state.sunAzimuth = 270.0 + 180.0 * f;
        state.moonElevation = data.moonPeakDegrees * std::sin(kPi * f);
        state.moonAzimuth = 90.0 + 180.0 * f;
    }
    state.sunAzimuth = std::fmod(state.sunAzimuth, 360.0);
    state.moonAzimuth = std::fmod(state.moonAzimuth, 360.0);
    state.moonlit = !state.sunUp;
    state.lightElevation = state.sunUp ? state.sunElevation : state.moonElevation;
    const double azimuth = (state.sunUp ? state.sunAzimuth : state.moonAzimuth) * kPi / 180.0;
    // Compass to picture: north is up (y negative), east is right. The shadow falls away from the light.
    const double towardX = std::sin(azimuth);
    const double towardY = -std::cos(azimuth);
    state.shadowDirX = -towardX;
    state.shadowDirY = -towardY;
    return state;
}

} // namespace odysseus::game
