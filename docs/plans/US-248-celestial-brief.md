# Build brief: US-248 Celestial bodies (sun and moon as placeable objects), before US-244

**From:** Mraw. **To:** Anima (amend the Codex to the next version). **Owner answers:** 2026-10-04, D-50.

## Goal
The sun and the moon become real objects of the world. The owner places them in the Editor; their position decides the light direction and the shadows. Shadows of characters, plants and buildings (US-244) are then cast from those objects, the way a real sun or moon would cast them.

## Owner decisions (D-50)
1. **Meaning:** light-source objects placed in the Editor, with a world position (x, y, height). Their position, not the elevation keyframes of `sky.json`, decides direction and shadows. Every level gets a default sun and moon that travel an orbit that follows the game clock (sunrise and sunset from the season, as US-242).
2. **Shadow math:** ground-plane projection from the light's true position. Direction = away from the light; length = height / tan(elevation of the light above the caster), capped at 2.5 x height for the sun and 1.5 x for the moon (D-49). Sun and moon are far, so shadows of one scene are nearly parallel. Shear of the silhouette (no height-map and no shadow map in this milestone).
3. **Look:** both are drawn as sprites in the sky band or at the edge of the picture, moving by the clock. Eclipses and other rare events are included as scripted events (data-driven, a dimming of sun or moon light for a while).
4. **Not chosen:** moon phases, shadow maps, normal-map-aware shadow height. Out of scope; record as ideas.

## Scope
- A `celestial` kind of world object (sun, moon) in the object catalog: sprite, light kind from `lights.json`, orbit (radius, tilt, height), `follows: "clock"` or fixed position.
- Editor: place, move and delete them; preview of any time of day already exists (US-242).
- Light direction and elevation per frame come from the sun and moon objects; `sky.json` keeps ambient colour and shadow strength. Without a placed body the level uses the default pair.
- Eclipse events in a data file (`celestial-events.json`): which body, start day and hour, length, depth of dimming. The simulation never reads them (presentation only, ADR-016).
- Tests: orbit follows the clock and the season; a moved body changes the direction; eclipse dims the light and ends; round trip of the new files; determinism hash unchanged.
- Hand US-244 a function giving the current light (direction, elevation, strength, source body) so that US-244 only draws shadows.

## Out of scope
Shadows themselves (US-244), fire shadows (US-245), moon phases, shadow maps.

## Order and dependencies
US-248 depends on US-242 and US-243; US-244 then depends on US-248 (and keeps US-242). Codex order: ... US-243 -> **US-248** -> US-244 -> US-245 ... Milestone M8c grows from 8 to 9 stories.

## Requirements to update first
ENV-21 (shadows) extended; new ENV-22 Celestial bodies (Must, M8c); backlog row US-248; D-50 in decisions.

## Risks
- Orbit height vs. 2D top-down: choose one convention (height above ground in metres, tile = 1 m) and document it in the lighting guide.
- A body placed low or below the horizon gives infinite or no shadow: clamp elevation to a minimum (cap in D-49 applies).
- Several suns: allowed by data; US-244 uses the strongest by default and sums nothing (ask owner if this matters later).
