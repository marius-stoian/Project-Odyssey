# Project Odyssey: assembly progress (76)

## AP-077 · 2026-10-01 · US-230 Luna's SDL_GPU renderer

| | |
|---|---|
| Assembly plan / requirements | **v2.2** / **v2.4** |
| Repository | `story/US-230` merged into `qa`; `main` has M8 |
| Milestone | M8b Resolution and GPU renderer, in progress (K-M8b and US-230 Done) |

### State
- US-230 passed `tools/verify.ps1 -Story US-230`: zero warnings, every test passing in Debug and Release.
- The game now draws through SDL_GPU (Direct3D 12) on this PC, with the old renderer as automatic fallback (`--renderer auto|gpu|sdl`). On the demo level and the camp the two renderers give identical pictures: 0 of 921,600 pixels differ in the tests and the real game's screenshots are byte-identical (`docs/evidence/US-230/`).
- Shaders (HLSL) are compiled at build time with the Windows SDK's `dxc.exe`; a build without it (or with `-DLUNA_GPU=OFF`) leaves the GPU backend out and the game plays on SDL_Renderer. This was built and tested both ways. CI: the GitHub Windows runner's Windows SDK is expected to have `dxc.exe`; if it does not, CI builds without the GPU backend and the GPU tests skip themselves.
- `src/game/` and the `Renderer` interface are unchanged.

### Decisions
- None requested. Delegated technical choices (Dominus), all in ADR-021 and `docs/plans/stories-M8b.md#us-230`: the GPU code is a backend inside Platform (the Engine layer may not include SDL), the Windows SDK's `dxc.exe` is the shader compiler, both renderers draw through a virtual-screen texture (one visible change: the children, drawn at three quarters size, now have even 2 x 2 pixel blocks), and a 1/512 texel nudge settles pixels that fall exactly between two texels.

### Next
- Confirm green CI on `qa`, then S-US-231 (960x540 and window modes).
