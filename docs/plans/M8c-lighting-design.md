# M8c Lighting and shadows: design

Architect: Mraw (Solution Architect hat). Date: 2026-10-02. Codex v2.6. Sources: requirements v2.8 (ENV-19..ENV-21), brief `docs/plans/M8b-M8d-render-light-build-brief.md`, ADR-021, `docs/plans/M8b-renderer-design.md`, decisions D-42, D-43, D-06, **D-49** (the owner's answers of 2026-10-02).

## 1. The owner's answers (D-49)
| Question | Answer | Number used |
|---|---|---|
| Night | A light dimming only | darkest ambient strength 0.55, blue tint (0.75, 0.82, 1.0) at that strength |
| Dawn and dusk | Warm orange, about one in-game hour | tint (1.0, 0.72, 0.5), ramp one hour either side of sunrise and sunset |
| Fires and torches | Steady warm glow, no flicker | colour (1.0, 0.78, 0.45); campfire radius 6 tiles, carried torch 4; strength 0.9 |
| Shadows | Up to 2.5 x the object's height | length = height / tan(elevation), capped at 2.5 x height; moon shadows shorter (cap 1.5 x) and at 40% strength |

## 2. The pipeline (US-240)
Today the Game draws the whole frame (world, then interface) into the virtual-screen texture. Lighting must touch the world only, so the frame gets a light step between the two:

1. World drawn into the virtual-screen texture (as now).
2. **Light step** (`Renderer::applyLight(LightFrame)`): a *light buffer* of the virtual size is cleared to the ambient colour; each light is drawn into it additively as a radial gradient sprite; the buffer is then multiplied onto the world (dst = dst x light). Colour and strength of the ambient, the sun and the moon come from `sky.json`.
3. The interface is drawn afterwards, unlit.

Both backends do it with two blend modes and one generated texture, so they stay pixel-identical (the US-230 tests compare them): `Blend::Add` (exists) for the lights, new `Blend::Multiply` for the composite, a 256 x 256 radial-gradient texture made by the Game (integer falloff, no float surprises) and stretched to each light's radius. A new offscreen target (`beginLightBuffer`, `endLightBuffer`) is the only new backend concept: in SDL_GPU a second texture and one more render pass, in SDL_Renderer a second target texture.

`LightFrame` is data, built by the Game each frame from the sky and the lights; the Simulation never reads it (Charter rule 11: determinism is untouched).

## 3. Normal maps (US-241)
Normals are generated once at load from each sprite's luminance (Sobel filter, strength per sheet) into a second atlas. A lit sprite pass (a new sprite shader, SDL_GPU only; the SDL fallback shows the unshaded sprite) takes the sun or moon direction and the nearest fire and shades by N.L. Kept optional: High lighting only.

## 4. Sky (US-242)
`assets/data/light/sky.json`: keyframes per hour (sun direction and elevation, moon direction, ambient colour and strength, shadow strength), blended linearly. Day length per season comes from `calendar.json`; sunrise and sunset move with it. The clock is the simulation's date and time of day (read only).

## 5. Lights (US-243)
`assets/data/light/lights.json`: kinds (colour, radius in tiles, strength). Objects, effects, weapons and items may name a `light`; the carried torch follows the hero. Steady, no flicker (D-49).

## 6. Shadows (US-244, US-245)
Characters, plants and buildings have a `height` (metres) in their catalog (defaults by kind) and may set `shadow: false`. The shadow is the sprite's silhouette flattened onto the ground and sheared away from the light: direction opposite the sun or moon, length height / tan(elevation) capped by D-49 (2.5 x, moon 1.5 x), drawn dark and see-through (strength from `sky.json`) before the sprites, so it lies on the ground. Fires cast shadows too (US-245): each object takes shadows from at most `maxShadowFires` (lights.json, default 2) nearest fires, steady because the glow is steady.

## 7. Weather (US-246)
`weather.json` entries get `light` (dim, tint) and `flash` (lightning); rain dims the ambient 15 to 30%, a lightning flash raises the ambient for a few frames.

## 8. Editor and quality (US-247)
The Editor previews any time of day (a slider and the hour keys). `settings.json` `lighting` (Low, Medium, High): Low = ambient tint only, no shadows; Medium = ambient, lights, sun shadows; High = adds normal maps and fire shadows. Budget: 60 FPS at 1080p on the D-06 PC on High (the method of `docs/plans/US-234.md`).

## 9. Risks
| Risk | Answer |
|---|---|
| The two backends drift apart | the light step uses only Add and Multiply of a CPU-made texture; the parity tests render lit scenes on both |
| Many lights cost fill rate | radius cap, at most 32 lights per frame, culled to the view |
| Shadows overlap into mud | one flat colour at low strength, drawn under all sprites |
| Normal maps look noisy | strength per sheet, blur before Sobel, High only |
