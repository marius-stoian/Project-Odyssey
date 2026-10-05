# Story plans: M8c

Per-story plans for milestone M8c, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-240](#us-240)
- [US-241](#us-241)
- [US-242](#us-242)
- [US-243](#us-243)
- [US-244](#us-244)
- [US-245](#us-245)
- [US-246](#us-246)
- [US-247](#us-247)
- [US-248 (brief)](#us-248-brief)
- [US-248](#us-248)

---

<a id="us-240"></a>

## US-240 The lighting pipeline: plan

**Codex.** Lit pass in the GPU renderer: ambient colour, up to 64 point lights per frame, per-sprite normal-map sampling; the Renderer interface gains light and normal-map calls; `lights.json`; the SDL_Renderer fallback draws with the ambient tint only. (`docs/plans/M8c-lighting-design.md` section 2 described a light buffer; the Codex's per-sprite lit shader is what is built.)

**Built.**
- Shader `sprite_lit.frag.hlsl`: colour x (ambient + sum over lights of colour x strength x reach^2 x facing), where reach = 1 - distance / radius (clamped) and facing = dot(normal, direction to the light, with the light `height` pixels above the ground). The vertex shader passes the pixel position. Up to 64 lights in one uniform buffer (2,080 bytes).
- `RenderBackend::setLighting(LightingState*)` and `setNormalMap(texture, normals)` (Platform, no new SDL types). The GPU backend batches lit and unlit draws separately (a batch carries its lighting set) and binds a flat normal for sprites without a map. Additive draws (glows, weather) are never lit.
- `Renderer::setLighting(LightFrame*)` / `setNormalMap` (Engine); `ScaledRenderer` scales the lights with the picture; `RecordingRenderer` records them (`lit` per draw).
- Game: `assets/data/light/lights.json` (`LightingData`, `loadLighting`); the world is drawn lit by the ambient colour, the interface unlit. Default file: white at strength 1, so the game looks exactly as before (GPU and SDL pictures stay identical).

**Tests.** `tests/luna/lighting_window_test.cpp` (real window): ambient tint on both renderers, point light on a dome normal map (GPU), 64 lights over a full screen cost 0.28 ms of the card (budget 16.7 ms). `tests/game/lighting_test.cpp`: lights.json shipped values, round trip, errors naming file and field, world lit and interface not.

**Manual checks (owner's PC, GPU renderer).** `docs/evidence/US-240/point-light.png` (a dome lit from the left). Run the game: unchanged picture at the default lights.json. To see the ambient, set `ambient.strength` to 0.5 in `assets/data/light/lights.json` and start the game: the world dims, the HUD does not.

---

<a id="us-241"></a>

## US-241 Generated normal maps: plan

**Codex.** `odysseus_atlas --normals` generates a normal-map atlas per atlas (height from alpha distance and luminance, Sobel slopes); a hand-made `<frame>_n.png` next to a cut wins; normal atlases are committed; missing maps light flat.

**Built.**
- `luna::engine::normalAtlas(atlas, cellW, cellH, strength)` works per cell, so cells never leak: height = 0.65 x (distance to the nearest see-through pixel, capped at 6 px, / 6) + 0.35 x brightness; a 3 x 3 blur; Sobel slopes; the normal is (-dx, -dy, 1) normalised and stored as 0..255 (128 128 255 is flat; see-through pixels are flat). `mirroredNormals` turns a map around and flips its x.
- `src/game/normal_art.*`: `writeNormalAtlases(sprites)` (reads `atlas/`, writes `characters_n.png`, `tiles_n.png`, `content-<page>_n.png`, honours hand-made files) and `loadNormalAtlas(folder, name, base, note)` (a missing file is not an error; a wrong-size file is left out with a note).
- `ArtSet` gains `heroNormals`, `tileNormals`, `charactersNormals`; `ContentAtlas` gains `normals`; `OdysseyGame` gives each texture its normal map through `Renderer::setNormalMap`. Strengths: bodies 2.0, ground 0.6.

**Not in this story.** Clan members (composed from layers) and the UI keep flat normals; the sky and the lights that use the maps come in US-242 and US-243.

**Manual checks (owner's PC, GPU renderer).** `docs/evidence/US-241/hero-lit.png`: the hero lit by a warm fire on his left and on his right; the lit side faces the fire. To see it yourself, run `odysseus_atlas --normals`, then give a campfire light in `lights.json` (US-243 places it). To swap in your own map, paint `hero.S.0_n.png` (32 x 48) into `assets/sprites/` and run the tool again.

---

<a id="us-242"></a>

## US-242 Day, night and seasons: plan

**Codex.** `sky.json` keyframes (sun and moon direction and elevation, ambient colour, shadow strength) blended from the game clock; day length per season from `calendar.json` (sunrise and sunset hours); the simulation's night hours stay as they are (no simulation change).

**Built.**
- `src/game/sky.*`: `loadSky` (sky.json and the `daylight` of calendar.json, every mistake naming file and field) and `skyAt(hour, season)`, which gives the ambient colour (times strength), shadow strength, sunrise and sunset, sun and moon elevation and azimuth, and the direction a shadow falls (`shadowDirX/Y`, length 1) with the elevation of the light casting it (sun by day, moon by night).
- Keyframes sit relative to the season's sunrise and sunset, so a long summer day and a short winter day come from the same file. D-49 values: night 0.55 with a blue tint, dawn and dusk orange, one hour either side.
- `OdysseyGame::sky()` reads the clan's clock (tick of the day, season from the calendar); the world's ambient is `lights.json` ambient times the sky. A level without a clan stays at noon. `calendar.json` gets `daylight`; the simulation ignores it (verified: the determinism hash test passes).
- `"enabled": false` in sky.json switches the cycle off.

**Tests.** `tests/game/sky_test.cpp`: the cycle (night, dawn, day, dusk, smooth over midnight, no step above 6% per tenth of an hour), seasons (summer 15 h, winter 8 h, 7:00 and 19:00 differ), moon (up at night, dim and blue, a direction), round trip and errors, and the world lit by the clan's clock. The GPU-against-SDL picture test switches the sky off (whole-number against fractional tint).

**Manual checks (owner's PC, GPU renderer).** `docs/evidence/US-242/day-cycle.png`: the camp at dawn (6:00), noon, dusk (18:00) and 22:00. Play with `--clan --clan-speed 20` to watch a day go by in a minute.

---

<a id="us-243"></a>

## US-243 Fires, torches and glowing effects: plan

**Codex.** A `light` field on objects, effects and weapons; a torch for clan members at night, carried light follows its holder; flicker from a seeded noise in the Game, never the simulation stream.

**Built.**
- `light` (kind of `lights.json`) on effects, weapons and objects; `lightState` on objects (fire pit: "burning"). Checked at load: a wrong name names file and field.
- `clanTorch` in `lights.json`; `flicker` per kind (default 0, steady, D-49); `flickerNoise(seed, seconds)`: value noise from a hash, so the same fire flickers the same way every run.
- `src/game/world_lights.cpp`: `OdysseyGame::worldLights` gathers the lights, scaled by darkness (none in daylight), capped at 64, skipped when off the picture. The lit sprite shader gains the light's height term.
- Shipped data: `flame` and `big fire` effects and the fire pit carry `campfire`; the clan carries `torch`. `valley.json` and `level-4.json` gained camp fires.

**Tests.** `tests/game/world_lights_test.cpp`: Fire, Data, Torch, Flicker, lights.json errors. The US-240 ambient test now writes the kinds the shipped data names.

**Manual checks (owner's PC, GPU renderer).** `docs/evidence/US-243/night-camp.png`: the camp at night, fires lighting circles. Play with `--clan --clan-speed 20` and watch torches walk with the clan.

---

<a id="us-244"></a>

## US-244 manual checks (sun and moon shadows)

Run on the owner's PC with the GPU renderer; screenshots in `docs/evidence/US-244/` (`odysseus.exe --level assets/levels/camp.json --clan --clan-speed 15 --weather clear --renderer gpu --screenshot x.bmp --quit-after 3` gives 09:00).

1. **Day.** At 09:00 the shadows of the hero, the clan and the stones fall to the north-west and are long; at noon (`--quit-after 4`, speed 15 gives about 12:00) they are short and fall north; in the evening they fall east and are long again.
2. **Casters.** A placed goblin, an animal, a bush, a tree and the fire pit each have a shadow sized by their height; grass has none.
3. **Moon.** At night the shadows are shorter and softer than by day.
4. **Overcast.** `--weather fog` (or "overcast bands"): the shadows are gone; "sunrise haze": half as dark.

---

<a id="us-245"></a>

## US-245 manual checks (shadows from fires)

Run on the owner's PC with the GPU renderer; screenshots in `docs/evidence/US-245/` (`odysseus.exe --level assets/levels/camp.json --clan --renderer gpu --screenshot x.bmp --quit-after 1` is the first night hour).

1. **One fire.** At night a person standing east of the camp fire throws a faint shadow to the east, away from it; a person west of it, to the west.
2. **Two fires.** A person between two fires has two faint shadows, one away from each.
3. **Budget.** With many fires, each person has at most `fireShadows.maxPerObject` fire shadows (default 2); set it to 0 and none are drawn.
4. **Low.** Settings, Lighting "Low": no fire shadows (the sun and moon shadows stay).
5. **Day.** At noon fires throw nothing.
6. **Carried torch.** A clan member's own torch throws no shadow of that member.

---

<a id="us-246"></a>

## US-246 manual checks (weather and light)

Run on the owner's PC with the GPU renderer; screenshots in `docs/evidence/US-246/` (`odysseus.exe --level assets/levels/camp.json --clan --renderer gpu --weather "steady rain" --screenshot x.bmp --quit-after 3`).

1. **Dim.** `--weather "steady rain"` at noon: the scene is darker and bluer than `--weather clear`. In play the change comes over 3 s while the drops fade in.
2. **Lightning.** `--weather "thunderstorm weather"`: about every six seconds the whole scene flashes bright for a moment; `--weather "chain lightning weather"` flashes more often.
3. **Data.** In `weather.json` give "steady rain" `"light":{"dim":0.3,"tint":[255,255,255]}` and restart: the rain is much darker. A wrong value (`"dim":3`) stops the game naming the file and the field.
4. **Night.** At night in a storm the fires still glow and the flashes light the whole scene.

---

<a id="us-247"></a>

## US-247 manual checks (lighting in the Editor and quality settings)

Run on the owner's PC with the GPU renderer; screenshots in `docs/evidence/US-247/`.

1. **Preview.** F1 into the Editor, click **Sky**: a slider appears. Drag it to the left end (midnight): the ground and everything on it turn dark blue. Middle: plain day. Dusk (about 18:00 in spring) is orange. Click **Sky** again: the level is drawn plain.
2. **Place.** Click **Light**, choose `campfire`, click the map: a gold marker. With the preview at night the light glows around it. **Ctrl+Z** takes it away, **Ctrl+Y** brings it back. **Ctrl+S**, F1 back to the game with the clan clock at night (or wait for dusk): the light shines.
3. **Move and delete.** With **Select** drag the marker; press **Delete** removes it; **Ctrl+Z** brings it back.
4. **Old levels.** Open `assets/levels/camp.json` (version 2): no lights, no message. Save: it is now version 3.
5. **Quality.** Settings, Lighting **Low**: at night around a fire the sprites look flat (no shading from the fire) and the faint fire shadows are gone; **High** brings them back. With `odysseus.exe --perf` (method of `docs/plans/stories-M8b.md#us-234`) the frame time on Low is lower than on High. Record the table in `docs/evidence/US-247/frame-times.txt`.
6. **Screenshots.** `odysseus.exe --level assets/levels/camp.json --clan --renderer gpu --screenshot x.bmp --quit-after 1` with Lighting Low and High.

---

<a id="us-248-brief"></a>

## Build brief: US-248 Celestial bodies (sun and moon as placeable objects), before US-244

**From:** Mraw. **To:** Anima (amend the Codex to the next version). **Owner answers:** 2026-10-04, D-50.

### Goal
The sun and the moon become real objects of the world. The owner places them in the Editor; their position decides the light direction and the shadows. Shadows of characters, plants and buildings (US-244) are cast from these objects, as a real sun or moon would cast them.

### Owner decisions (D-50)
1. **Meaning:** light-source objects placed in the Editor, with a world position (x, y, height). Their position, not the elevation keyframes of `sky.json`, decides direction and shadows. Every level gets a default sun and moon that travel an orbit following the game clock (sunrise and sunset from the season, as US-242).
2. **Shadow math:** ground-plane projection from the light's true position. Direction = away from the light; length = height / tan(elevation of the light above the caster), capped at 2.5 x height for the sun and 1.5 x for the moon (D-49). Sun and moon are far away, so shadows of one scene are nearly parallel. The silhouette is sheared (no height-map and no shadow map in this milestone).
3. **Look:** both are drawn as sprites in the sky band or at the edge of the picture, moving by the clock. Eclipses and other rare events are included as scripted, data-driven events (a dimming of sun or moon light for a while).
4. **Not chosen (out of scope; record as ideas):** moon phases, shadow maps, normal-map-aware shadow height.

### Scope
- A `celestial` kind of world object (sun, moon) in the object catalog: sprite, light kind from `lights.json`, orbit (radius, tilt, height), `follows: "clock"` or fixed position.
- Editor: place, move and delete them; previewing any time of day already exists (US-242).
- Light direction and elevation per frame come from the sun and moon objects; `sky.json` keeps ambient colour and shadow strength. Without a placed body the level uses the default pair.
- Eclipse events in a data file (`celestial-events.json`): which body, start day and hour, length, depth of dimming. The simulation never reads them (presentation only, ADR-016).
- Tests: orbit follows the clock and the season; a moved body changes the direction; an eclipse dims the light and ends; round trip of the new files; determinism hash unchanged.
- Give US-244 a function returning the current light (direction, elevation, strength, source body) so US-244 only draws shadows.

### Out of scope
Shadows themselves (US-244), fire shadows (US-245), moon phases, shadow maps.

### Order and dependencies
US-248 depends on US-242 and US-243; US-244 then depends on US-248 (and keeps US-242). Codex order: ... US-243 -> **US-248** -> US-244 -> US-245 ... Milestone M8c grows from 8 to 9 stories.

### Requirements to update first
ENV-21 (shadows) extended; new ENV-22 Celestial bodies (Must, M8c); backlog row US-248; D-50 in decisions.

### Risks
- Orbit height vs. 2D top-down: choose one convention (height above ground in metres, tile = 1 m) and document it in the lighting guide.
- A body placed low or below the horizon gives infinite or no shadow: clamp elevation to a minimum (the D-49 cap applies).
- Several suns: allowed by data; US-244 uses the strongest by default and sums nothing (ask the owner if this matters later).

---

<a id="us-248"></a>

## US-248 manual checks (celestial bodies)

Run on the owner's PC with the GPU renderer. Seeded run as in Limit.md (`--new-game`, `--type 7:1.2`, privacy No, Start, Begin), results and screenshots in `docs/evidence/US-248/`.

1. **Default pair.** Start a new game with a clan level. In the morning (about 8:00) a sun sprite sits at the left of the sky band, high at noon in the middle, low at the right in the evening; at night a moon travels the same way. Shadows are US-244.
2. **Placed body.** In the Editor choose the last page of the plant palette and place `sun (placed)` east of the hero. Test play: `celestialLight()` in the developer tools (F12) shows source `sun (placed)`; moving it south changes the direction.
3. **Eclipse.** Edit `celestial-events.json` to an eclipse today (or skip days with F12), watch the world dim for the event and return.
4. **Data.** Put `"depth": 9` in an event, press F5 or restart: the message names `celestial-events.json: events[0].depth`; the sun and moon still travel.

Screenshot: `odysseus.exe --screenshot x.bmp --quit-after 3 --renderer gpu`.
