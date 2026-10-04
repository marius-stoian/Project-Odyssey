#pragma once

#include "boundary.h"

#include "game/catalogs.h"
#include "game/lighting.h"
#include "game/sky.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::game {

// Celestial bodies (US-248, D-50): the sun and the moon are objects of the world. Their position decides where the light comes from and so
// which way shadows fall (US-244). Presentation only: the simulation never reads any of this (ADR-016).

// One scripted eclipse (assets/data/light/celestial-events.json): the light of a body dims for a while and then returns.
struct CelestialEvent {
    std::string body;        // "sun" or "moon"
    int startDay = 0;        // day of the game, counted from 0 at the start of the clan's life
    double startHour = 12.0; // 0 to 24
    double lengthHours = 1.0;
    double depth = 0.5;      // 0 to 1: how much of the light is lost at the darkest moment
    friend bool operator==(const CelestialEvent&, const CelestialEvent&) = default;
};

struct CelestialEvents {
    std::vector<CelestialEvent> events;
    friend bool operator==(const CelestialEvents&, const CelestialEvents&) = default;
};

// Reads celestial-events.json; a mistake becomes a sim::DataError naming the file and the field. A missing file means no events.
CelestialEvents loadCelestialEvents(const std::filesystem::path& file);
std::string celestialEventsToText(const CelestialEvents& events);

// How much of the body's light is left (1 = all) at `hour` of `day`. The dimming eases in over the first fifth of the event, holds its full
// depth in the middle and eases out over the last fifth. Overlapping events: the darkest counts, nothing is summed.
double eclipseFactor(const CelestialEvents& events, const std::string& body, int day, double hour);

// A sun or moon in the world: from the catalog, plus where it was placed.
struct CelestialBody {
    const PlantDef* object = nullptr; // the catalog entry (a PlantDef with `celestial` set)
    double x = 0.0, y = 0.0;          // world pixels, for a placed body (ignored when it follows the clock)
};

// The level's bodies: the default pair when the level places none of that kind. `placed` are the level's own bodies.
std::vector<CelestialBody> bodiesOf(const std::vector<const PlantDef*>& defaults, const std::vector<CelestialBody>& placed);

// The clock the sky and the bodies follow.
struct GameClock {
    double hour = 12.0; // 0 to 24
    int day = 0;
    int season = 0;
};

// Where one body is seen from a reference point of the scene (the hero): compass azimuth (degrees clockwise from north) and elevation.
struct BodyView {
    double azimuth = 0.0;
    double elevation = 0.0;  // degrees above the horizon (negative = below), not yet clamped
    bool up = false;         // above the horizon and the right time for its kind: a sun by day, a moon by night
};

// The light the scene has now (what US-244 draws shadows from).
struct CelestialLight {
    bool valid = false;           // false: no body is up: no directional light
    std::string source;           // the catalog name of the body the light comes from
    bool moon = false;            // the source is a moon
    double dirX = 0.0, dirY = 0.0; // the way a shadow falls over the ground in the picture (x right, y down); length 1, zero straight overhead
    double elevation = 90.0;      // degrees above the horizon, never below kMinLightElevation
    double strength = 1.0;        // the light kind's strength times what an eclipse leaves
    double dimming = 1.0;         // what an eclipse leaves of the light (1 = all): the ambient light is scaled by it
    double lengthPerHeight = 0.0; // shadow length divided by the caster's height: 1 / tan(elevation), capped at 2.5 (sun) or 1.5 (moon) (D-49)
};

// A body drawn in the sky band.
struct SkySprite {
    std::string name;    // the catalog name
    std::string frame;   // the picture of the object
    double x = 0.0;      // 0 to 1 across the picture
    double y = 0.0;      // 0 (top of the sky band) to 1 (the horizon)
    bool moon = false;
    double dimming = 1.0;
};

inline constexpr double kMinLightElevation = 8.0;  // degrees: a body on the horizon still gives a long but finite shadow
inline constexpr double kSunShadowCap = 2.5;       // shadow length at most this times the caster's height (D-49)
inline constexpr double kMoonShadowCap = 1.5;

// Where `body` is, seen from `reference` (world pixels): a clock body from the sky (sun and moon rise and set as in sky.h, turned by the tilt of
// its orbit); a placed body from its own position and height (1 tile = 32 pixels = 1 metre).
BodyView viewOf(const CelestialBody& body, const SkyState& sky, double referenceX, double referenceY);

// The light the scene has at `clock`: by day the strongest sun that is up, by night the strongest moon (nothing is summed). The level's own
// bodies win over the default pair of the same kind. A scene with no body up is not lit by a direction (valid = false).
CelestialLight currentLight(const std::vector<CelestialBody>& bodies, const SkyData& skyData, const LightingData& lighting, const CelestialEvents& events,
                            const GameClock& clock, double referenceX, double referenceY);

// Every body above the horizon, placed in the sky band for drawing.
std::vector<SkySprite> skySprites(const std::vector<CelestialBody>& bodies, const SkyData& skyData, const CelestialEvents& events, const GameClock& clock,
                                  double referenceX, double referenceY);

} // namespace odysseus::game
