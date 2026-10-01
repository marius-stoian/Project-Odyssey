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
