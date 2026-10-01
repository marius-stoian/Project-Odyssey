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

**Normal maps.** A sprite sheet may have a normal map of the same size (red: left to right, green: up to down, blue: toward the viewer; 128 128 255 is a flat surface). The side of a sprite that faces a light is the bright one. The maps are generated from the art in US-241; until then a sprite is flat. The fallback renderer (`--renderer sdl`) has no shaders: it only applies the ambient colour.
