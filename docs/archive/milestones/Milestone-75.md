# Project Odyssey: assembly progress (75)

## AP-076 · 2026-10-01 · K-M8b kickoff: Resolution and GPU renderer

| | |
|---|---|
| Assembly plan / requirements | **v2.2** / **v2.4** |
| Repository | `chore/k-m8b` merged into `qa`; `main` has M8 (tag `m8-done` once CI on `main` is green) |
| Milestone | M8b Resolution and GPU renderer: K-M8b Done; US-230 is next |

### State
- The owner answered the M8b design questions in one chat round (D-44): windowed sizes 1280x720, 1600x900, 1920x1080, 2560x1440 plus borderless and exclusive full screen; whole steps with black bars by default (Fill in Settings); camera zoom and UI scale in the Settings screen and camera zoom also on the mouse wheel and keys; first start windowed 1280x720, zoom 2x, UI scale 1x, lighting Medium.
- Design written: `docs/plans/M8b-renderer-design.md` (the GPU backend behind `Window`, a 960x540 virtual screen texture, batching, HLSL shaders compiled at build time with the Windows SDK's `dxc.exe`, fallback to SDL_Renderer, presentation maths, zoom and UI scale, layout rules, tests without a GPU, performance method).
- Codex issue CI-009 raised for Anima: K-M8b step 1 asks to confirm M8e, which cannot be done before M8b (read as "M8 is done").
- M8 itself: `qa` was merged into `main` (CI on `main` was still running when this was written; tag `m8-done` follows when it is green).

### Decisions
- D-44 Decided (owner, 2026-10-01), see above. No delegated decisions.

### Next
- S-US-230 Luna's SDL_GPU renderer (L): the first story that needs the owner's PC for the pixel comparison.
