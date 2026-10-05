# M8b Resolution and GPU renderer: design (K-M8b)

Architect: Mraw (Solution Architect hat). Date: 2026-10-01. Codex v2.2. Source of truth: requirements v2.4 (ENV-18, ARC-11, ADR-021), brief `docs/plans/M8b-M8d-render-light-build-brief.md`, decisions D-06, D-42, D-43, D-44. Charter rule 11 (rendering).
Goal of M8b: the game draws a 960 x 540 picture through Luna's SDL_GPU renderer (the SDL_Renderer path stays as fallback and for tests), looks the same as today at camera zoom 2x, runs windowed, borderless and exclusive full screen, and every screen and editor panel is laid out for the new size. M8b changes the renderer and the resolution without changing what the game shows; lighting is M8c.

## 1. Owner decisions that shape this design (D-42, D-06, D-44)
| Question | Answer | Consequence |
|---|---|---|
| Virtual resolution | 960 x 540, whole-step scaling | one constant pair in Luna, no 480/270 left in code (US-231 removes the 36 hard-coded uses outside Core/Simulation) |
| Windowed sizes | 1280x720, 1600x900, 1920x1080, 2560x1440, plus borderless and exclusive full screen | a list in Luna's window settings; the Settings screen shows it |
| Odd window or screen sizes | whole steps with black bars by default; Fill is the option | `Presentation::mode` = Whole (default) or Fill |
| Camera zoom and UI scale | Settings screen; camera zoom also on the mouse wheel and keys in play; UI scale in Settings only | the camera has `zoom` 1 or 2 (default 2 = today's view); the UI painter has `scale` 1 or 2 |
| First start | windowed 1280x720, zoom 2x, UI scale 1x, lighting Medium | defaults of `settings.json` |
| Target PC (D-06) | 6-core CPU, 16 GB, RX 6600 / RTX 3060 class GPU with DirectX 12; 60 FPS at 1080p on High lighting, Low for weaker PCs | the performance method in section 9 |

## 2. Where things live (layers)
Charter rule 11: SDL_GPU is touched only in Luna Platform and Engine; nothing above them sees a graphics-API type.
| Part | Files | Layer |
|---|---|---|
| The window, events, choice of backend, presentation of the finished picture | `src/luna/platform/window.{h,cpp}` (as today) | Platform |
| `RenderBackend` (internal interface of the Window: create texture, begin frame, draw quad, end frame, read pixels) and its two implementations | `src/luna/platform/backend.h`, `sdl_renderer_backend.cpp` (today's code moved), `gpu_backend.cpp` (new) | Platform |
| Shaders (HLSL) and generated headers | `src/luna/platform/shaders/*.hlsl`, `build/.../shaders/*.h` (generated) | Platform |
| `Renderer` interface and `WindowRenderer` | `src/luna/engine/renderer.{h,cpp}` | Engine |
| Presentation maths (whole step, fill, bars), window sizes, zoom and UI scale values | `src/luna/engine/presentation.{h,cpp}` | Engine |

Why the backend sits behind `Window` and not behind `Renderer`: the Game already draws only through `Renderer` (ADR-003) and `Window` is the only class that knows SDL. A `RenderBackend` inside Platform lets the two paths share the window, events, gamepads and screenshot code, and keeps the `Renderer` interface unchanged for US-230 (lighting adds calls to it in M8c).

## 3. The GPU path (US-230)
- **Device and swapchain.** `Window` creates the `SDL_Window`, then asks the GPU backend to create an `SDL_GPUDevice` (shader formats DXIL and SPIR-V; debug layer in Debug builds) and claim the window (SDR swapchain, VSYNC present mode). It owns both; destruction order is textures, pipelines, swapchain release, device, window.
- **Virtual screen texture.** All game drawing goes to an offscreen RGBA8 texture of the virtual size (960 x 540). At the end of the frame one more pass draws it to the swapchain with nearest-neighbour sampling inside the presentation rectangle (whole step or fill), the rest black. Pixel art stays crisp at any window size, and M8c's lighting passes work in virtual space.
- **Uploads.** `createTexture` copies the RGBA pixels through a transfer buffer in a copy pass (once, at load); textures are nearest-sampled, clamp-to-edge.
- **Batching.** `drawTexture` appends two triangles (position, texture coordinates, alpha) to a CPU vertex array in call order (today's painter's algorithm is kept: no reordering). The batch is flushed when the texture or blend mode changes and at the end of the frame; vertex data goes up in one buffer per frame (grown when needed). Two pipelines: Normal (source alpha) and Add (adds light), as today's `Blend`.
- **Shaders.** `sprite.vert.hlsl`, `sprite.frag.hlsl` (texture times alpha), `blit.frag.hlsl` (the picture to the window). Compiled at build time by `dxc.exe` from the Windows SDK (found with `find_program` in the SDK bin folder) to DXIL for the D3D12 backend and embedded as byte arrays in generated headers, so the game ships no shader files. ADR-021 records the alternatives (SDL_shadercross, DXC from vcpkg, runtime compilation) with trade-offs; the SDK DXC wins because it adds no dependency and is on the CI image and the developer's machine. A Vulkan (SPIR-V) build with `dxc -spirv` is a later option, not in the MVP. When `dxc.exe` is not found, CMake prints a clear message and builds without the GPU backend (`LUNA_GPU=0`); the game then uses SDL_Renderer.
- **Fallback.** The default is `--renderer auto`: try the GPU backend; if the device, swapchain or a shader fails, log why (once, at warning level) and run on SDL_Renderer. `--renderer gpu` fails loudly; `--renderer sdl` forces the old path (tests, the owner's comparisons).
- **Screenshots.** `readPixels` copies the swapchain (or, simpler and exact, the virtual screen plus the presentation) into a download transfer buffer and waits for the GPU; slow, used by `--screenshot` and tests only.

## 4. Resolution, windows and scaling (US-231)
- `kVirtualWidth/Height` = 960 x 540 in one Luna header; `WindowSettings.virtualWidth/Height` default to it.
- `Presentation` (engine): inputs are window size in real pixels, virtual size and mode (Whole or Fill); outputs are the destination rectangle and the scale. Whole = the largest whole number that fits (never below 1), centred; Fill = the largest rectangle with the virtual aspect ratio that fits (fractional scale). `integerScale` becomes the Whole case; its tests stay.
- Window modes: windowed (resizable; the sizes of section 1 in Settings), borderless full screen (SDL fullscreen at the desktop mode), exclusive full screen (SDL fullscreen with a chosen display mode). `settings.json` v2 adds `resolution` (width, height, mode, scaling), `cameraZoom`, `uiScale`, `lighting`; version 1 files load with the defaults of section 1 (a migration, never an error).
- Mouse positions are mapped through the presentation rectangle (as today) into virtual pixels.

## 5. Camera zoom and UI scale (US-231, US-232)
- The camera shows `virtualSize / zoom` of the world: zoom 2 (default) shows 480 x 270 world pixels, exactly today's view, each drawn 2 x 2 virtual pixels; zoom 1 shows 960 x 540 (30 x 17 tiles). Drawing code takes the zoom from the camera; the map, entities and effects use one `worldToScreen` helper (US-232).
- UI scale: `UiPainter` gets a `scale` (1 or 2): text and boxes are drawn at that multiple. Screens are laid out in *UI units* of `virtualSize / uiScale` (960 x 540 at 1x, 480 x 270 at 2x, i.e. today's layout). At 1x everything gets room to breathe (US-233); at 2x the existing layouts must still fit, which is the regression check.
- Mouse wheel and keys change the camera zoom in play (D-44); the Settings screen has both buttons.

## 6. Layout rules (US-233)
No panel, button row or text block may use a number tied to 480 x 270. Each screen takes its area from `UiPainter::screen()` (virtual size divided by UI scale) and positions from it; wide text wraps to the area; the Editor's panels dock to the edges and grow with the area. A test per screen builds it at UI scale 1 and 2 and checks that nothing lies outside the area and no two widgets overlap.

## 7. Tests (CI has no GPU)
- Headless tests keep using the `RecordingRenderer`; presentation maths, settings migration, zoom and UI-scale layout checks are ordinary unit tests.
- The GPU backend is checked on the owner's PC: `odysseus.exe --renderer gpu --screenshot` against `--renderer sdl` on the demo and camp levels, compared pixel by pixel (a small tool, `tools/compare-shots`), saved in `docs/evidence/US-230/`. The GPU code is compiled in CI (so it cannot rot) but not run there.

## 8. Risks
| Risk | Mitigation |
|---|---|
| SDL_GPU or the driver fails on a machine | automatic fallback to SDL_Renderer with a logged reason; `--renderer sdl` |
| `dxc.exe` missing on a build machine | the GPU backend is compiled out with a CMake message; the game still builds and plays |
| 960x540 touches all layouts | US-231 moves the constants first; US-233 lays the screens out; the UI-scale-2 test keeps today's layouts valid |
| Whole-step scaling leaves bars on 1366x768 | the owner chose bars by default (D-44); Fill is in Settings |
| Two renderers drift apart | the pixel comparison tool on the demo and camp levels at every renderer story |

## 9. Performance method (D-06, US-234)
A fixed scene (the camp level with the clan, rain, and the editor closed) at 1920x1080, whole-step 2x: `--screenshot` free, the F3 overlay's frame time over 60 s, run on the owner's PC and on a second, weaker PC; the numbers go into `docs/evidence/US-234/performance.md`. Target 60 FPS at 1080p on High lighting (M8c), Low lighting for weaker PCs.

## 10. Story order (Codex)
US-230 -> US-231 -> US-232 -> US-233 -> US-234 -> X-M8b.
