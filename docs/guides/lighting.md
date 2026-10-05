# Lighting data: `assets/data/light/lights.json`

The light of the game (US-240, M8c). One file. A mistake stops the game and names the file and field (for example `lights.json: ambient.color[2]: must be a whole number from 0 to 255`). A missing file is not an error: the game is then drawn unlit.

| Field | Values | Meaning |
|---|---|---|
| `version` | `1` | the format |
| `ambient.color` | `[red, green, blue]`, each 0 to 255 | the colour every lit sprite is multiplied by |
| `ambient.strength` | 0 to 2 | how much of it; `[255,255,255]` at `1.0` changes nothing |
| `lights[].name` | text, unique | the kind of light (`campfire`, `torch`) |
| `lights[].color` | `[r, g, b]`, 0 to 255 | the light's colour |
| `lights[].radiusTiles` | 0.5 to 30 | how far it reaches, in tiles (32 pixels each) |
| `lights[].strength` | 0 to 4 | brightness at its centre |
| `lights[].height` | 1 to 200 | pixels above the ground; a higher light is flatter on a surface |

```json
{
  "version": 1,
  "ambient": { "color": [200, 220, 255], "strength": 0.55 },
  "lights": [
    { "name": "campfire", "color": [255, 199, 115], "radiusTiles": 6.0, "strength": 0.9, "height": 24 }
  ]
}
```

Fires and torches are steady, no flicker (D-49): campfire 6 tiles, carried torch 4. The kinds are placed on objects, effects and items in US-243; the sky (day, night, seasons) comes in US-242.

**Placing lights (US-243).** A catalog entry gets light with a `light` field naming a kind of `lights.json`; without it, it gives none. A wrong name stops the game at load, naming file and field.

```json
{"name":"flame","frames":4,"ticksPerFrame":3,"loop":true,"light":"campfire"}
{"name":"fire pit", "...":"...", "states":["cold","burning"], "light":"campfire", "lightState":"burning"}
```

- Effects (`effects.json`): the light shines while the effect plays. Objects (`objects.json`): `lightState` limits it to one state (a fire pit shines only when "burning"; empty means always). Weapons (`weapons.json`): the light shines while the weapon is held.
- `"clanTorch": "torch"` at the top of `lights.json` gives every clan member that light at night; it moves with them. Leave it out for none.
- `"flicker"` (0 to 1) on a kind of light makes its strength waver by a seeded noise in the Game (never the simulation's random streams). Default 0: steady (D-49).
- Lights add only in the dark: the darker the ambient light, the stronger they shine; in full daylight they add nothing. At most 64 lights per frame.

**Making the normal maps (US-241).** `odysseus_atlas --normals` writes a normal atlas next to every atlas picture in `assets/sprites/atlas/`: `characters_n.png`, `tiles_n.png` and `content-<page>_n.png` (nine pictures), each the size of its atlas. The height of every pixel comes from its distance to the sprite's edge (the middle of a body stands higher) and its brightness; the slopes of that height (Sobel filter) become the surface direction. Bodies use strength 2.0, ground tiles only 0.6 (a hint of grain). The normal atlases are committed; run the tool again after changing a sheet or a cut.

**Painting one yourself.** Put a PNG named like the frame with `_n` in `assets/sprites/` (next to `cuts.json`), for example `hero.S.0_n.png` or `iron sword_n.png`, exactly the size of the frame (32 x 48 for a character, the frame's rectangle for content). The tool pastes it over the generated map of that frame and reports how many it used ("N frames from hand-made maps"). A file of the wrong size stops the tool with its name.

**Normal maps.** A sprite sheet may have a normal map of the same size (red: left to right, green: up to down, blue: toward the viewer; 128 128 255 is a flat surface). The side of a sprite facing a light is the bright one. The maps are generated from the art in US-241; until then a sprite is flat. The fallback renderer (`--renderer sdl`) has no shaders: it only applies the ambient colour.

# The sky: `assets/data/light/sky.json` and the daylight of `assets/data/sim/calendar.json`

The time of day tints the ambient light (US-242). The simulation is not touched: it reads `calendar.json` as before and ignores the `daylight` part.

`calendar.json` gains `daylight`: the hour of sunrise and sunset in each season (0 to 24; at least four hours of day). Shipped: Spring 6 to 18, Summer 5 to 20 (a 15-hour day), Autumn 6.5 to 17.5, Winter 8 to 16 (an 8-hour day).

```json
"daylight": {
  "Spring": { "sunrise": 6.0, "sunset": 18.0 },
  "Summer": { "sunrise": 5.0, "sunset": 20.0 },
  "Autumn": { "sunrise": 6.5, "sunset": 17.5 },
  "Winter": { "sunrise": 8.0, "sunset": 16.0 }
}
```

| `sky.json` field | Values | Meaning |
|---|---|---|
| `version` | `1` | the format |
| `enabled` | true / false | false: the light does not follow the clock (always plain day) |
| `keyframes[].anchor` | `"sunrise"` or `"sunset"` | the keyframe sits at that hour of the season ... |
| `keyframes[].offset` | -6 to 6 | ... plus this many hours |
| `keyframes[].color` | `[r, g, b]`, 0 to 255 | the ambient colour at that moment |
| `keyframes[].strength` | 0 to 2 | how much of it |
| `keyframes[].shadow` | 0 to 1 | how dark shadows are (US-244) |
| `sunPeakDegrees` | per season, 5 to 90 | the highest the sun climbs |
| `moonPeakDegrees` | 5 to 90 | the highest the moon climbs |

```json
{ "anchor": "sunset", "offset": 0.0, "color": [255, 184, 128], "strength": 0.85, "shadow": 0.5 }
```

The light between two keyframes is a straight blend; the last keyframe of the day joins the first over midnight. The shipped keyframes are the owner's answers D-49: night dims to 55% with a blue tint, dawn and dusk are warm orange and last about an hour either side of sunrise and sunset, day is white. The sun crosses from east to west between sunrise and sunset, the moon takes the night the same way; at night the moon is the light (a dim blue ambient and a direction for shadows). A level without a clan has no clock and stays at noon.
# Celestial bodies: the sun and the moon (US-248, D-50)

The sun and the moon are objects of the world. Where they are decides where the light comes from and so which way shadows fall (US-244 draws the shadows). Presentation only: the simulation never reads any of this and the determinism hash is unchanged.

**A body is a world object** of `assets/data/objects.json` with a `celestial` block:

| Field | Values | Meaning |
|---|---|---|
| `celestial.body` | `"sun"` or `"moon"` | a sun lights the day, a moon the night |
| `celestial.light` | a name of `lights.json` | its kind of light; the kind's `strength` is how strong this body's light is (the shipped `sun` and `moon` kinds are 1.0) |
| `celestial.follows` | `"clock"` or `"fixed"` | `clock`: travels its orbit by the game clock; `fixed`: stays where placed |
| `celestial.orbitRadius` | 10 to 100000 | metres: how far away a clock body is |
| `celestial.tilt` | -90 to 90 | degrees: turns the whole orbit around the vertical (the sun rises a little north or south of east) |
| `celestial.height` | 1 to 100000 | metres above the ground (1 tile = 1 m): the height of a fixed body |

```json
{ "name": "sun", "frame": "sun", "blocks": false, "inspect": "The sun.",
  "celestial": { "body": "sun", "light": "sun", "follows": "clock", "orbitRadius": 400, "tilt": 0, "height": 100 } }
```

- **Default pair.** A level that places no sun gets the first `clock` sun of the catalog; the same for the moon. The shipped `sun` and `moon` follow the clock: sunrise and sunset come from the season (US-242), the sun climbs to the season's peak, the moon takes the night. Bodies that follow the clock are not in the Editor's palette.
- **Placed bodies.** `sun (placed)` and `moon (placed)` are on the last page of the Editor's plant palette (objects). Place, move and delete them like any object. Their position and the catalog `height` give the light: direction away from the body as seen from the hero, elevation = atan(height / distance), 1 tile = 1 m. A placed sun lights the day, a placed moon the night; a level that places only a sun keeps the default moon. For another height make another catalog entry (for example a low sun at `"height": 12`).
- **Which light.** By day the strongest sun that is up, by night the strongest moon. Nothing is summed. A tie keeps the first.
- **Elevation and shadow length.** Elevation is clamped to at least 8 degrees, so a body on the horizon gives a long but finite shadow. A shadow is `height / tan(elevation)` long, at most 2.5 times the caster's height for the sun and 1.5 for the moon (D-49). Straight overhead (89.5 degrees or more) gives no direction.
- **Seen.** The bodies are drawn as sprites in the sky band at the top of the picture, unlit: east is the left edge, south the middle, west the right edge, higher in the sky means higher on the picture. A body in the north is drawn at the nearer edge.
- **For US-244.** `OdysseyGame::celestialLight()` returns the current light: `valid`, `source`, `dirX`/`dirY` (the way a shadow falls, length 1), `elevation`, `strength`, `dimming` and `lengthPerHeight`. Shadow drawing only has to use it.

## Eclipses: `assets/data/light/celestial-events.json`

| Field | Values | Meaning |
|---|---|---|
| `version` | `1` | the format |
| `events[].body` | `"sun"` or `"moon"` | whose light dims |
| `events[].startDay` | 0 to 1000000 | day of the game, counted from 0 |
| `events[].startHour` | 0 to 23.99 | hour of that day |
| `events[].lengthHours` | 0.1 to 24 | how long it lasts (it may run past midnight) |
| `events[].depth` | 0 to 1 | how much light is lost at the darkest moment |

```json
{ "version": 1, "events": [ { "body": "sun", "startDay": 20, "startHour": 11.0, "lengthHours": 2.0, "depth": 0.7 } ] }
```

The dimming eases in over the first fifth of the event, holds full depth in the middle and eases out over the last fifth. While it lasts the light of that body is multiplied by what is left (`dimming`), and the whole world's ambient light with it. Of overlapping events the darkest counts. A missing file means no events.

**Mistakes.** A mistake in the events file or in a `celestial` entry does not stop the game: the problem is shown as a message with the file and field (for example `celestial-events.json: events[0].depth: must be a number from 0 to 1`), the bad entry is left out and the level keeps the default pair (a built-in sun and moon if the catalog has none left).

# Shadows (US-244)

Characters, plants and objects cast shadows on the ground that turn and stretch with the light of the sun or the moon (see "Celestial bodies"). Presentation only: the simulation never reads them and the determinism hash is unchanged.

**How a shadow is made.** Each sprite's picture is painted black (a *silhouette* copy of its texture, made at start-up and, for the clan, the first time a look is seen) and laid down on the ground along the light, one ground row at a time (`drawShadow` in `src/luna/engine/shadow_draw.cpp`). A point of the sprite `z` pixels above its feet lands `z * length` pixels from the feet in the direction away from the light, so the upright picture is sheared and flattened. The length is the thing's catalog `height` times the light's `lengthPerHeight` (1 / tan of the elevation, at most 2.5 for the sun and 1.5 for the moon, D-49): at noon in spring a 4 m tree throws a shadow of about 3.4 m, in the morning a longer one. A shadow pointing exactly sideways on the picture is given a little depth so it stays a shape.

**How dark.** `sky.json` `shadow` of the hour (0.35 at night, 0.5 at dawn and dusk, 0.6 by day) times the light's strength (an eclipse takes it away) times what the weather leaves. Moon shadows are shorter (cap 1.5) and, because of the sky's night value, softer.

| Field | Where | Values | Meaning |
|---|---|---|---|
| `height` | `plants.json`, `objects.json`, `animals.json`, `characters.json` | metres, 0 to 100 (characters and animals 0.1 to 100) | how tall the thing is; its shadow is this times the light's factor |
| `shadow` | the same files | true / false | `false`: casts no shadow whatever its height |
| `shadowFade` | `weather.json` | 0 to 1 | how much of the shadows this weather takes away |

Defaults when a field is left out: a tree 4 m, a tall plant (bush) 1.5 m, a small plant 0 (grass casts nothing; give it a `height` to change that), an object 0.8 m, a person 1.7 m (a clan child three quarters of that), an animal 1.0 m. Weather: fog, mist, cloud, overcast and whiteout take all (1.0), haze, smog and gloom half (0.5); every other weather 0.

```json
{"name":"olive tree","frame":"olive tree","size":"tree","height":5.0, "...":"..."}
{"name":"fire pit", "...":"...", "shadow": false}
{"name":"fog","frames":4,"ticksPerFrame":4,"weight":1,"blend":"alpha","shadowFade":1.0}
```

A mistake names the file and field, for example `plants.json: plants[2].height: must be a number from 0 to 100`. Sun and moon objects (US-248) are in the sky and cast nothing. Shadows of buildings come with M8d.

## Shadows from fires (US-245)

At night, things near a fire throw faint extra shadows away from it. The fires are the point lights of US-243 whose kind has `"shadows": true`; how many fires shade one thing, and how dark they are, come from `lights.json`.

| Field | Where | Values | Meaning |
|---|---|---|---|
| `fireShadows.maxPerObject` | top level of `lights.json` | whole number 0 to 8, default 2 | how many of the nearest fires shade one thing; 0 turns fire shadows off |
| `fireShadows.strength` | same | 0 to 1, default 0.35 | how dark a shadow is right beside the fire on the darkest night |
| `lights[].shadows` | a kind of light | true / false, default false | this kind of light throws shadows (the sun and moon kinds do not) |

```json
{
  "version": 1,
  "fireShadows": { "maxPerObject": 2, "strength": 0.35 },
  "lights": [
    { "name": "campfire", "color": [255, 199, 115], "radiusTiles": 6.0, "strength": 0.9, "height": 24, "shadows": true }
  ]
}
```

A thing is shaded only by fires inside their own radius; the shadow falls straight away from the fire, is longest and darkest right beside it (up to 1.5 times the thing's height) and fades to short and faint at the edge of the light (0.4 times). Darkness follows the night: none in daylight. A light closer than 12 pixels to a thing is its own torch and shades nothing. The Low lighting preset in Settings turns fire shadows off. A mistake names the file and field, for example `lights.json: fireShadows.maxPerObject: must be a whole number from 0 to 8`.

# Weather light: `light` and `flash` in `assets/data/weather.json` (US-246)

Weather changes the mood of the world: rain dims and cools the ambient light, snow and fog grey it, a storm flashes it. Presentation only: the simulation never reads it and the determinism hash is unchanged.

| Field | Values | Meaning |
|---|---|---|
| `light.dim` | 0.1 to 1 | the ambient light is multiplied by this (1 = unchanged); default 1 |
| `light.tint` | `[r, g, b]`, each 0 to 255 | ... and by this colour (a cool blue for rain); default white |
| `flash` | 0 to 60 | lightning flashes per minute (0 = none); default 0 |

```json
{"name":"thunderstorm weather","frames":4,"ticksPerFrame":4,"weight":1,"blend":"add","light":{"dim":0.55,"tint":[170,185,215]},"flash":10}
```

- **The fade.** The light follows the weather fade (US-138): over the same 3 s as the drops arrive, it moves in a straight line from the old weather's light to the new one's, so rain starting at noon dims and cools smoothly. Fires glow stronger as the weather darkens the scene, like at dusk.
- **Lightning.** Time is cut into 0.2 s slots. A hash of the weather seed and the slot decides whether a strike starts in it, with chance `flash` x 0.2 / 60; a strike lifts the ambient light 85% of the way to white at its first instant and falls back to nothing within the slot (a few frames). The same seed strikes at the same moments; the simulation's random streams are never used. While one weather fades into another the flash rate blends the same way as the light.
- Shipped values: drizzle and light rain 0.85, steady and heavy rain 0.75, dark storm clouds 0.6, thunderstorm, close and chain lightning, storm rain and the electric storms 0.55 with 10 flashes a minute (chain lightning 18), distant lightning 0.8 with 5, snow 0.92, blizzard and whiteout 0.8, fog and cloud 0.85, dust, sand and ash 0.75 warm. Every other weather leaves the light as it is.
- A mistake stops the game naming file and field (for example `weather.json: weather[3].light.dim: must be a number from 0.1 to 1`).

# Lighting quality, placed lights and the Editor preview (US-247)

**Quality.** `settings.json` `lighting` is `Low`, `Medium` or `High` (the Settings screen). `Low` switches off what costs the most and shows the least: the normal maps (every sprite is lit as a flat surface; the ambient colour and fire glow stay) and the fire shadows (US-245). `Medium` and `High` keep both; `High` is the quality the D-06 target (60 FPS at 1080p on the mid-range PC) is held on. The fallback renderer (`--renderer sdl`) never had normal maps.

**Placed lights.** The Editor's Light tool puts a point of light of a kind of `lights.json` into the level (`lights` in the level file, level version 3):

```json
"lights": [ { "id": 12, "kind": "campfire", "x": 300, "y": 300 } ]
```

| Field | Values | Meaning |
|---|---|---|
| `id` | whole number, unique in the level | same counter as every other thing in the level |
| `kind` | a `name` of `lights.json` | colour, reach, strength and height of the light |
| `x`, `y` | world pixels inside the level | where the light is on the ground; it hangs 8 pixels above |

A wrong `kind` stops loading with `lights[0].kind: "lava lamp" is not a kind of light in lights.json`. Placed lights shine after dark exactly like the lights of effects and objects (US-243).

**Editor preview.** The Sky button in the Editor lights the level as at any hour with the sky of `sky.json` and the clan's season (spring without a clan), and the level's own lights, effects and placed lights. Weather and eclipses are not previewed. Nothing of it is saved.
