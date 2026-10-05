# ADR-021: SDL_GPU renderer with HLSL shaders compiled by the Windows SDK's dxc.exe

Status: Accepted (owner decision D-42, 2026-10-01; technical choices by Dominus in US-230; requirements v2.4, ARC-11).

## Context
ADR-003 started with SDL_Renderer behind Luna's `Renderer` interface and planned a move to SDL_GPU when shaders were needed. They are needed now: M8b (a 960 x 540 picture in any window), M8c (global light, shadows, normal maps) and M8d (building interiors) cannot be done with SDL_Renderer's two blend modes. Charter rule 2 allows SDL only in Luna Platform; rule 11 allows SDL_GPU types and shader files only in Platform and Engine.

## Decisions
1. **Where the GPU code lives.** Not as a `GpuRenderer` class in the Engine layer (the Codex prompt's first sketch), because the Engine may not include SDL (rule 2). It is a `RenderBackend` inside Luna Platform, owned by the `Window`; `Window` keeps the OS window, events and gamepads and calls the backend to draw. Two backends exist: `SdlRendererBackend` (the old code, moved) and `GpuBackend` (new). The Engine's `Renderer` interface and `WindowRenderer` do not change, so **no game file changed** (US-230 scenario "Game code").
2. **A virtual screen texture.** Both backends draw everything onto a texture of the virtual-screen size, then enlarge it into the window by a whole number, with black bars around it. Both backends produce the same pictures (the old renderer enlarged each sprite by itself, so the children, drawn at three quarters, had uneven pixels; now every virtual pixel is one even block), and M8c's lighting passes can work in virtual space.
3. **Same pixels, by construction and by test.** Textures are sampled nearest-neighbour; blending is `src*alpha + dst*(1-alpha)` (Normal) or `src*alpha + dst` (Add), as in SDL_Renderer. Both backends sample `1/512` of a texel further right and down (`kTexelNudge`), which resolves pixels that fall exactly between two texels the same way (a sprite drawn at three quarters has many); without it 352 pixels of the camp differed. `tests/game/renderer_test.cpp` and `tests/luna/pixels_window_test.cpp` compare the two pixel by pixel (0 different for the demo level, the camp and a scene of translucent, additive and scaled draws; the real game's screenshots of both are byte-identical, `docs/evidence/US-230/`).
4. **Shader language and compiler.** Shaders are HLSL (`src/luna/platform/shaders/*.hlsl`), compiled at build time by the **Windows SDK's** `dxc.exe` to DXIL (shader model 6.0) for SDL_GPU's Direct3D 12 backend, and built into the program as byte arrays (`dxc -Fh`), so the game ships no shader files. CMake finds the newest `dxc.exe` under the SDK's `bin/10.*/x64` (or `-DLUNA_DXC=<path>`). With `-DLUNA_GPU=OFF`, or when `dxc.exe` is not found, the GPU backend is left out with a message and the game uses SDL_Renderer.
5. **Fallback.** `--renderer auto` (the default) tries the GPU; if the device, the swapchain or a shader fails (or the backend is not built), it logs one warning with the reason and runs on SDL_Renderer in a fresh window. `--renderer gpu` fails loudly; `--renderer sdl` forces the old path (tests, comparisons).

## Options considered for the shader compiler
| Option | For | Against |
|---|---|---|
| **Windows SDK `dxc.exe` at build time (chosen)** | No new dependency; on the developer's PC (Visual Studio) and expected on the GitHub Windows image; real DXIL; shaders checked at build time, not run time | Windows only (the future Android and iOS Platform layers bring their own shader path, which Charter rule 2 already requires); the SDK version varies |
| SDL_shadercross (SDL's own tool) | Cross-platform: one HLSL source for DXIL, SPIR-V and Metal | A separate project to build and keep in step with SDL; not in vcpkg at the time of writing |
| `directx-dxc` from vcpkg | Pinned version, same on every machine | Large (LLVM-based) package to download and build for four small shaders; slows every clean build |
| Compile at run time | No build step | Needs the compiler on the player's PC; slower first start; errors only at run time |
| Hand-written DXBC/SPIR-V | No compiler | Unreadable and unmaintainable |

## Consequences
- Shaders use SDL_GPU's register layout (vertex uniforms in space1, fragment textures and samplers in space2); M8c adds light and shadow passes in the same folder.
- Only the Direct3D 12 path (DXIL) is built. A Vulkan path (`dxc -spirv`) can be added later without changing the backend's interface.
- CI runners have no GPU: CI compiles the GPU backend (so it cannot rot) and runs the headless tests; the pixel comparisons skip themselves where no GPU renderer starts and run on the owner's PC.
- The first frame appears about 0.7 s later in a Debug build (device and pipelines); the US-020 timing test keeps its limits.
- The `RenderBackend` interface inside Platform is internal; nothing above Luna sees a graphics-API type.
