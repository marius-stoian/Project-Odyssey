#include "game/celestial.h"

#include "game/placeholder_art.h"
#include "sim/data.h"
#include "sim/json_data.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>

namespace odysseus::game {

using nlohmann::json;

namespace {

constexpr double kRadiansToDegrees = 180.0 / std::numbers::pi;

double numberIn(const json& object, const std::filesystem::path& file, const std::string& where, const std::string& field, double low, double high) {
    if (!object.contains(field) || !object.at(field).is_number() || object.at(field).get<double>() < low || object.at(field).get<double>() > high) {
        throw sim::DataError(file, where + "." + field, std::format("must be a number from {} to {}", low, high));
    }
    return object.at(field).get<double>();
}

double wrap360(double degrees) {
    degrees = std::fmod(degrees, 360.0);
    return degrees < 0.0 ? degrees + 360.0 : degrees;
}

bool isMoon(const CelestialBody& body) { return body.object->sky.body == "moon"; }

// A sun is only a light by day and a moon by night; a placed body does not rise and set, so the clock decides when it counts.
bool rightTime(const CelestialBody& body, const SkyState& sky) { return isMoon(body) ? !sky.sunUp : sky.sunUp; }

} // namespace

CelestialEvents loadCelestialEvents(const std::filesystem::path& file) {
    CelestialEvents result;
    if (!std::filesystem::exists(file)) return result;
    const json root = sim::readJsonFile(file);
    if (!root.is_object() || !root.contains("version") || !root.at("version").is_number_integer() || root.at("version").get<int>() != 1) {
        throw sim::DataError(file, "version", "must be 1");
    }
    if (!root.contains("events") || !root.at("events").is_array()) throw sim::DataError(file, "events", "must be a list of events");
    for (std::size_t i = 0; i < root.at("events").size(); ++i) {
        const json& entry = root.at("events").at(i);
        const std::string where = std::format("events[{}]", i);
        if (!entry.is_object()) throw sim::DataError(file, where, "must be an object with body, startDay, startHour, lengthHours and depth");
        CelestialEvent event;
        if (!entry.contains("body") || !entry.at("body").is_string() || (entry.at("body").get<std::string>() != "sun" && entry.at("body").get<std::string>() != "moon")) {
            throw sim::DataError(file, where + ".body", "must be \"sun\" or \"moon\"");
        }
        event.body = entry.at("body").get<std::string>();
        if (!entry.contains("startDay") || !entry.at("startDay").is_number_integer() || entry.at("startDay").get<int>() < 0 || entry.at("startDay").get<int>() > 1000000) {
            throw sim::DataError(file, where + ".startDay", "must be a whole number from 0 to 1000000");
        }
        event.startDay = entry.at("startDay").get<int>();
        event.startHour = numberIn(entry, file, where, "startHour", 0.0, 23.99);
        event.lengthHours = numberIn(entry, file, where, "lengthHours", 0.1, 24.0);
        event.depth = numberIn(entry, file, where, "depth", 0.0, 1.0);
        result.events.push_back(event);
    }
    return result;
}

std::string celestialEventsToText(const CelestialEvents& events) {
    json root;
    root["version"] = 1;
    root["events"] = json::array();
    for (const CelestialEvent& event : events.events) {
        root["events"].push_back({{"body", event.body}, {"startDay", event.startDay}, {"startHour", event.startHour},
                                  {"lengthHours", event.lengthHours}, {"depth", event.depth}});
    }
    return root.dump(2);
}

double eclipseFactor(const CelestialEvents& events, const std::string& body, int day, double hour) {
    const double now = day * 24.0 + hour;
    double factor = 1.0;
    for (const CelestialEvent& event : events.events) {
        if (event.body != body) continue;
        const double start = event.startDay * 24.0 + event.startHour;
        const double into = now - start;
        if (into < 0.0 || into > event.lengthHours) continue;
        // Ease in over the first fifth, hold the full depth, ease out over the last fifth.
        const double ramp = event.lengthHours * 0.2;
        const double edge = std::min(into, event.lengthHours - into);
        const double t = ramp > 0.0 ? std::clamp(edge / ramp, 0.0, 1.0) : 1.0;
        const double smooth = t * t * (3.0 - 2.0 * t);
        factor = std::min(factor, 1.0 - event.depth * smooth);
    }
    return factor;
}

std::vector<CelestialBody> bodiesOf(const std::vector<const PlantDef*>& defaults, const std::vector<CelestialBody>& placed) {
    std::vector<CelestialBody> result = placed;
    for (const char* kind : {"sun", "moon"}) {
        const bool has = std::any_of(placed.begin(), placed.end(), [&](const CelestialBody& b) { return b.object->sky.body == kind; });
        if (has) continue;
        for (const PlantDef* object : defaults) {
            if (object->sky.body == kind && object->sky.followsClock) {
                result.push_back({object, 0.0, 0.0});
                break; // one default of each kind
            }
        }
    }
    return result;
}

BodyView viewOf(const CelestialBody& body, const SkyState& sky, double referenceX, double referenceY) {
    BodyView view;
    const CelestialDef& def = body.object->sky;
    if (def.followsClock) {
        view.azimuth = wrap360((isMoon(body) ? sky.moonAzimuth : sky.sunAzimuth) + def.tilt);
        view.elevation = isMoon(body) ? sky.moonElevation : sky.sunElevation;
    } else {
        const double dx = (body.x - referenceX) / kTileSize; // metres: 1 tile = 1 m
        const double dy = (body.y - referenceY) / kTileSize;
        const double distance = std::hypot(dx, dy);
        view.azimuth = distance > 0.0 ? wrap360(std::atan2(dx, -dy) * kRadiansToDegrees) : 0.0; // compass: north is up, east is right
        view.elevation = std::atan2(def.height, distance) * kRadiansToDegrees;
    }
    view.up = view.elevation > 0.0 && rightTime(body, sky);
    return view;
}

CelestialLight currentLight(const std::vector<CelestialBody>& bodies, const SkyData& skyData, const LightingData& lighting, const CelestialEvents& events,
                            const GameClock& clock, double referenceX, double referenceY) {
    const SkyState sky = skyAt(skyData, clock.hour, clock.season);
    CelestialLight best;
    double bestElevation = 0.0;
    for (const CelestialBody& body : bodies) {
        const BodyView view = viewOf(body, sky, referenceX, referenceY);
        if (!view.up) continue;
        const LightKindDef* kind = lighting.kind(body.object->sky.lightKind);
        const double dimming = eclipseFactor(events, body.object->sky.body, clock.day, clock.hour);
        const double strength = (kind != nullptr ? kind->strength : 1.0) * dimming;
        if (best.valid && strength <= best.strength) continue; // the strongest wins and nothing is summed; a tie keeps the first
        best.valid = true;
        best.source = body.object->name;
        best.moon = isMoon(body);
        best.strength = strength;
        best.dimming = dimming;
        bestElevation = view.elevation;
        const double azimuth = view.azimuth / kRadiansToDegrees;
        // The light comes from the body, so a shadow falls the opposite way (compass to picture: north is up, east is right).
        best.dirX = -std::sin(azimuth);
        best.dirY = std::cos(azimuth);
    }
    if (!best.valid) return best;
    best.elevation = std::clamp(bestElevation, kMinLightElevation, 90.0);
    if (best.elevation >= 89.5) {
        best.dirX = best.dirY = 0.0; // straight overhead: no direction
    }
    const double cap = best.moon ? kMoonShadowCap : kSunShadowCap;
    best.lengthPerHeight = std::min(1.0 / std::tan(best.elevation / kRadiansToDegrees), cap);
    return best;
}

std::vector<SkySprite> skySprites(const std::vector<CelestialBody>& bodies, const SkyData& skyData, const CelestialEvents& events, const GameClock& clock,
                                  double referenceX, double referenceY) {
    const SkyState sky = skyAt(skyData, clock.hour, clock.season);
    std::vector<SkySprite> sprites;
    for (const CelestialBody& body : bodies) {
        const BodyView view = viewOf(body, sky, referenceX, referenceY);
        if (!view.up) continue;
        SkySprite sprite;
        sprite.name = body.object->name;
        sprite.frame = body.object->frame;
        sprite.moon = isMoon(body);
        sprite.dimming = eclipseFactor(events, body.object->sky.body, clock.day, clock.hour);
        // East (90 degrees) is the left edge of the picture, south the middle, west the right edge; a body in the north is shown at the nearer edge.
        const double a = wrap360(view.azimuth - 90.0);
        sprite.x = a <= 180.0 ? a / 180.0 : (a < 270.0 ? 1.0 : 0.0);
        sprite.y = 1.0 - std::clamp(view.elevation, 0.0, 90.0) / 90.0;
        sprites.push_back(sprite);
    }
    return sprites;
}

} // namespace odysseus::game
