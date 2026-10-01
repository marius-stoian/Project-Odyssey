# Lighting data: `assets/data/light/lights.json`

The light of the game (US-240, M8c). One file; a mistake stops the game with the file and the field named (for example `lights.json: ambient.color[2]: must be a whole number from 0 to 255`). A missing file is not an error: the game is then drawn unlit.

| Field | Values | Meaning |
|---|---|---|
| `version` | `1` | the format |
| `ambient.color` | `[red, green, blue]`, each 0 to 255 | the colour every lit sprite is multiplied by |
| `ambient.strength` | 0 to 2 | how much of it; `[255,255,255]` at `1.0` changes nothing |
| `lights[].name` | text, unique | the kind of light (`campfire`, `torch`) |
| `lights[].color` | `[r, g, b]`, 0 to 255 | the light's colour |
| `lights[].radiusTiles` | 0.5 to 30 | how far it reaches, in tiles (32 pixels each) |
| `lights[].strength` | 0 to 4 | how bright it is at its centre |
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

**Making the normal maps (US-241).** `odysseus_atlas --normals` writes a normal atlas next to every atlas picture in `assets/sprites/atlas/`: `characters_n.png`, `tiles_n.png` and `content-<page>_n.png` (nine pictures), each the size of its atlas. The height of every pixel comes from its distance to the sprite's edge (the middle of a body stands higher) and its brightness; the slopes of that height (Sobel filter) become the surface direction. Bodies use strength 2.0, ground tiles only 0.6 (a hint of grain). The normal atlases are committed; run the tool again after changing a sheet or a cut.

**Painting one yourself.** Put a PNG named like the frame with `_n` in `assets/sprites/` (next to `cuts.json`), for example `hero.S.0_n.png` or `iron sword_n.png`, exactly the size of the frame (32 x 48 for a character, the frame's rectangle for content). The tool pastes it over the generated map of that frame and says how many it used ("N frames from hand-made maps"). A file of the wrong size stops the tool with its name.

**Normal maps.** A sprite sheet may have a normal map of the same size (red: left to right, green: up to down, blue: toward the viewer; 128 128 255 is a flat surface). The side of a sprite that faces a light is the bright one. The maps are generated from the art in US-241; until then a sprite is flat. The fallback renderer (`--renderer sdl`) has no shaders: it only applies the ambient colour.

# The sky: `assets/data/light/sky.json` and the daylight of `assets/data/sim/calendar.json`

The time of day tints the ambient light (US-242). The simulation is not touched: it reads `calendar.json` as before and ignores the `daylight` part.

`calendar.json` gains `daylight`: the hour of sunrise and sunset in each season (0 to 24; at least four hours of day). The shipped values: Spring 6 to 18, Summer 5 to 20 (a 15-hour day), Autumn 6.5 to 17.5, Winter 8 to 16 (an 8-hour day).

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

The light between two keyframes is a straight blend; the last keyframe of the day joins the first over midnight. The shipped keyframes are the owner's answers D-49: night is a dimming to 55% with a blue tint, dawn and dusk are warm orange and last about an hour either side of sunrise and sunset, day is white. The sun crosses from east to west between sunrise and sunset, the moon takes the night the same way; at night the moon is the light (a dim blue ambient and a direction for shadows). A level without a clan has no clock and stays at noon.