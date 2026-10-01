#pragma once

#include "boundary.h"

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <string>
#include <vector>

namespace odysseus::game {

// The sky (US-242, assets/data/light/sky.json and the `daylight` of assets/data/sim/calendar.json): how the light follows the game clock through
// dawn, day, dusk and night, with longer days in summer and shorter in winter. The numbers are the owner's answers D-49.
struct SkyKeyframe {
    std::string anchor;     // "sunrise" or "sunset": the keyframe sits at that hour of the season plus `offset`
    double offset = 0.0;    // hours
    int red = 255, green = 255, blue = 255;
    double strength = 1.0;  // ambient colour times this
    double shadow = 0.5;    // how dark shadows are (US-244), 0 to 1
    friend bool operator==(const SkyKeyframe&, const SkyKeyframe&) = default;
};

struct Daylight {
    double sunrise = 6.0; // hour of the day, 0 to 24
    double sunset = 18.0;
    friend bool operator==(const Daylight&, const Daylight&) = default;
};

struct SkyData {
    bool enabled = true;                 // false: the light does not follow the clock (always full day)
    std::vector<SkyKeyframe> keyframes;  // blended in time order, wrapping around midnight
    double sunPeakDegrees[4] = {50.0, 65.0, 45.0, 28.0}; // highest sun by season: spring, summer, autumn, winter
    double moonPeakDegrees = 45.0;
    Daylight daylight[4];                // sunrise and sunset by season (from calendar.json)
    friend bool operator==(const SkyData& a, const SkyData& b) {
        return a.enabled == b.enabled && a.keyframes == b.keyframes && a.moonPeakDegrees == b.moonPeakDegrees &&
               std::equal(std::begin(a.sunPeakDegrees), std::end(a.sunPeakDegrees), std::begin(b.sunPeakDegrees)) &&
               std::equal(std::begin(a.daylight), std::end(a.daylight), std::begin(b.daylight));
    }
};

// What the sky is like at one moment.
struct SkyState {
    float ambientR = 1.0F, ambientG = 1.0F, ambientB = 1.0F; // colour times strength, ready for the renderer
    double shadowStrength = 0.5;
    double sunrise = 6.0, sunset = 18.0; // today's, by the season
    bool sunUp = true;
    double sunElevation = 90.0;   // degrees above the horizon, negative while it is below
    double sunAzimuth = 180.0;    // degrees clockwise from north: 90 east at sunrise, 180 south at noon, 270 west at sunset
    double moonElevation = -90.0; // the moon is up while the sun is down: it rises at sunset and sets at sunrise
    double moonAzimuth = 0.0;
    // The direction light travels over the ground in the picture (x right, y down), pointing away from the sun or, at night, the moon:
    // the way a shadow falls. Length 1; zero when the light stands straight above.
    double shadowDirX = 0.0, shadowDirY = 0.0;
    double lightElevation = 90.0; // of the sun by day, the moon by night
    bool moonlit = false;         // night: the moon is the light
};

// Reads sky.json and the daylight of calendar.json. A mistake becomes a sim::DataError naming the file and the field; a missing sky.json gives the
// defaults of a plain bright day (disabled).
SkyData loadSky(const std::filesystem::path& skyFile, const std::filesystem::path& calendarFile);
std::string skyToText(const SkyData& data); // sky.json for `data` (the daylight is in calendar.json)

// The sky at `hour` (0 to 24, fractions allowed) in `season` (0 spring .. 3 winter).
SkyState skyAt(const SkyData& data, double hour, int season);

} // namespace odysseus::game
