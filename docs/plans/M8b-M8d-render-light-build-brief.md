# Build brief: M8b Resolution and GPU renderer, M8c Lighting and shadows, M8d Buildings

Mraw to Anima, 2026-10-01. Source of truth: requirements v2.3 (Round 16, D-42). Anima turns this brief into Codex v2.2 (P-011, K-M8b..X-M8d). Built **right after M8, before M9**; kill gate 2 stays after M14.

## 1. Goal
A bigger, sharper picture; light that follows the time of day, the seasons and the weather; shadows from characters, plants and buildings; and buildings that the hero and the clans build in the game and the owner places as pre-made prefabs in the Editor.

Today Luna renders at 480 x 270 through SDL_Renderer with textures and two blend modes (normal, add) behind the `Renderer` interface (ADR-003, `src/luna/engine/renderer.h`). There is no lighting and no building system; world objects ride on the plant catalog (CI-008).

## 2. Owner decisions (D-42, three chat rounds, 2026-10-01)
| Topic | Owner answer |
|---|---|
| Resolution | **960 x 540** virtual (2x today): whole-step scaling (2x at 1080p, 4x at 2160p), Fill option, windowed sizes, borderless and exclusive full screen; **camera zoom** (default 2x = today's view, 1x shows 30 x 17 tiles) and **UI scale**. |
| Renderer | **Move to SDL_GPU with shaders** (ADR-021); SDL_Renderer stays as fallback and for tests. |
| Placement | **Everything right after M8**: M8b, M8c, M8d before M9. |
| Lights | **Sun and moon cycle**, **fire, torch and effect lights**, **seasons change day length**, **weather dims the light** (with lightning flashes). |
| Shadows | **Sun and moon, plus nearby fires** at night; characters, plants and buildings cast them; they fade when overcast. |
| Normal maps | **Generated from the existing sprites by a tool**; placeholders for building art until the owner adds sheets. |
| Buildings | **Both** whole blueprints and modular pieces. **Clan members help build, rival clans build, decay and repair, damage and fire.** |
| Interiors | **Per building, set in the Editor**: roof fade, or an interior map entered through the door. |
| Prefabs | **Composed from pieces in the Editor**, placed whole, offered as blueprints. |

## 3. Requirements added or changed (v2.3)
ENV-18 resolution and window, ENV-19 global lighting, ENV-20 day length and interiors, ENV-21 shadows, ARC-11 GPU renderer, INT-07 buildings, INT-08 building condition, EDT-07 prefabs, MVP-16, ADR-021; MVP-11 changed. Cut list: fire spreading between building pieces is in the MVP. D-06 (minimum PC) is now needed by M8b.

## 4. Formats (the contract)
- `settings.json` gains `resolution` (window size, mode: windowed, borderless, fullscreen; scaling: whole, fill), `cameraZoom` (1 or 2), `uiScale` (1 or 2), `lighting` (low, medium, high).
- `assets/data/light/sky.json`: keyframes per hour (sun and moon direction and elevation, ambient colour and strength, shadow strength); day length per season comes from `calendar.json`.
- `assets/data/light/lights.json`: light kinds (colour, radius, strength, flicker), max shadow-casting fires per object, quality presets. Catalog entries (objects, effects, weapons, items) may name a `light`; `weather.json` entries get `light` (dim, tint) and `flash` (lightning).
- Catalogs gain `height` (metres, for shadow length) and, where needed, `shadow: false`.
- `assets/data/buildings/pieces.json`: wall, floor, roof, door, window, post, fence (size on a 1 m building grid, material, hp, build time, materials, art).
- `assets/data/buildings/kinds.json` and `assets/data/buildings/prefabs/<id>.json`: footprint, piece layout, materials, build time, `interior: "fade" | "map"`, interior level name, uses (interactions: shelter, sleep, store, work), light, owner rules, wear per season, buildable (offered as a blueprint).
- Levels and world files gain a `buildings` list (kind or prefab, position, rotation, condition, owner, contents, interior override); a version bump with migration.

## 5. Milestones and stories
### M8b Resolution and GPU renderer (E22)
| ID | Story | Size | Priority | Depends on |
|---|---|---|---|---|
| US-230 | Luna's SDL_GPU renderer (fallback kept) | L | Must | US-020 |
| US-231 | 960x540 and window modes | M | Must | US-230, US-081 |
| US-232 | Camera zoom and UI scale | M | Must | US-231 |
| US-233 | Every screen at the new size | L | Must | US-232 |
| US-234 | Frame budget at the new size | S | Must | US-233, D-06 |

### M8c Lighting and shadows (E23)
| ID | Story | Size | Priority | Depends on |
|---|---|---|---|---|
| US-240 | The lighting pipeline | L | Must | US-230 |
| US-241 | Generated normal maps | M | Must | US-240, US-120 |
| US-242 | Day, night and seasons | M | Must | US-240, US-010 |
| US-243 | Fires, torches and glowing effects | M | Must | US-240, US-132 |
| US-244 | Sun and moon shadows | L | Must | US-242 |
| US-245 | Shadows from fires | M | Must | US-244, US-243 |
| US-246 | Weather and light | S | Must | US-242, US-138 |
| US-247 | Lighting in the Editor and quality settings | M | Must | US-243, US-244 |

### M8d Buildings (E24)
| ID | Story | Size | Priority | Depends on |
|---|---|---|---|---|
| US-250 | Building pieces and building kinds as data | L | Must | US-150, US-155 |
| US-251 | Building from blueprints | L | Must | US-250, US-153 |
| US-252 | Building piece by piece | L | Must | US-251 |
| US-253 | Clans build together | M | Must | US-251, US-154 |
| US-254 | Interiors: roof fade or interior map | L | Must | US-252, US-122 |
| US-255 | Wear, repair, damage and fire | L | Must | US-251, US-029 |
| US-256 | Prefab editor and placing buildings | L | Must | US-252, US-254, US-124 |
| US-257 | Buildings in the clan's life | M | Must | US-253, US-011 |

Acceptance criteria are in requirements v2.3, section 12.9, and the backlog. Estimates (opt / likely / pess weeks): M8b 6/10/13, M8c 9/14/19, M8d 14/21/28.

## 6. Architecture rules
- **Renderer (ADR-021):** SDL_GPU lives only in `src/luna/platform/` and `src/luna/engine/`. The `Renderer` interface gains what lighting needs (lights, normal-map textures, shadow casters, a lit pass) without any graphics-API type leaking to the Game. Shaders are HLSL in `src/luna/engine/shaders/`, compiled at build time (the tool and its vcpkg or third_party source are recorded in ADR-021). The SDL_Renderer path stays for PCs without SDL_GPU and for headless tests; game code is unchanged by the switch (US-230 acceptance).
- **Determinism:** lighting and shadows are presentation only; the simulation never reads them. Building, wear, fire spread and construction are Simulation (seeded, saved, in the world hash).
- **Fire spread** uses Luna Physics material properties (flammability) between neighbouring building pieces only; general heat and fire simulation stays cut.
- **Buildings get their own list** in levels, world files and saves (not the plant machinery, CI-008); world objects stay as they are.
- **Layout:** screens are laid out from the virtual size and the UI scale, never from fixed 480 x 270 numbers.
- **Tests:** CI runners have no GPU, so GPU pixel comparisons run on the owner's PC (manual check with screenshots) or a software device if available; everything else runs headless through the RecordingRenderer as today.

## 7. Assembly rules for M8b-M8d
These come before M9, so the **D-35 rules** (M7-M9) apply unless the owner says otherwise: the owner answers each kickoff's design questions in one chat round, and every story runs the full verification with green CI on `qa`. **K-M8b must ask D-06 (minimum PC spec)**, open since M4, because the frame budget (US-234) and the lighting quality presets depend on it.

## 8. Council notes and risks
| Risk | Mitigation |
|---|---|
| SDL_GPU is a renderer rewrite plus a shader build step: the largest engine change since M1 | Same-picture acceptance (US-230) before any lighting; fallback renderer kept; ADR-021 |
| CI cannot test GPU output | Headless tests through the RecordingRenderer; GPU screenshots checked on the owner's PC at each story |
| M8d is the largest milestone so far (8 stories, 6 of them L, about 21 weeks likely) | Kickoff may split it (M8d buildings, M8e building life) if it passes 6 L stories in practice |
| Kill gate 2 slips again (now about 245 weeks likely at the old part-time pace) | M8b-M8d make the playtest build look far better; the M6 package keeps building |
| Minimum PC (D-06) still undecided | Asked at K-M8b; lighting quality presets give a Low path |
| 960 x 540 changes every screen | US-233 re-lays out every screen once, before M9-M12 build more UI |
| Interiors as maps mean two levels per building | Interior maps are ordinary levels made in the Editor and linked by doors |

## 9. Answered with Anima (D-43, D-06, Round 17, 2026-10-01; requirements v2.4)
- **M8d is split:** M8d Buildings (US-250, US-251, US-252, US-256: data, blueprints, pieces, prefab editor) and **M8e Building life** (epic E25: US-253, US-254, US-255, US-257: clans build, interiors, wear/repair/fire, clan life). US-256 no longer waits for US-254: the interior mode is data from US-250 and works in the game from US-254. Estimates: M8d 8/12/16, M8e 6/9/12.
- **Assembly rules:** M8b-M8e follow D-35 (the owner answers each kickoff's design questions in one chat round; every story runs the full verification with green CI on qa).
- **D-06 minimum PC:** a mid-range PC: 6-core CPU, 16 GB RAM, RX 6600 / RTX 3060 class GPU (8 GB) with DirectX 12, Windows 10/11; 60 FPS at 1080p on High lighting; Low lighting for weaker PCs. Development PC: RX 7900 XTX, 32 GB RAM, 6-core CPU.
- Still for Dominus (technical): the shader compiler and its source, recorded in ADR-021 at US-230.
