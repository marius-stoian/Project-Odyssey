# Architecture decision records

From Project Odyssey.docx v1.3, section 7.5. New ADRs get their own file here (ADR-016-<slug>.md) and a line in this index.

| ADR | Decision |
|---|---|
| ADR-001 | C++20, MSVC compiler, CMake build, Visual Studio IDE. |
| ADR-002 | SDL3 is the only door to the operating system. |
| ADR-003 | Start with SDL_Renderer behind our own Renderer interface; move to SDL_GPU when we need shaders. |
| ADR-004 | Five layers; the simulation has no graphics (ARC-01, TEC-05). |
| ADR-005 | EnTT for the entity-component-system. |
| ADR-006 | Fixed timestep: 20 simulation ticks per second; rendering interpolates. |
| ADR-007 | Single-threaded first; a job system for world generation and far LOD later. |
| ADR-008 | Offline single-player, no backend (ARC-04). |
| ADR-009 | Planet = east-west wraparound tile grid in 64x64 chunks, generated from the seed; saves store only changes (ARC-06). |
| ADR-010 | JSON for content data and saves; versioned; atomic writes; 3 rolling backups. |
| ADR-011 | Determinism rules (ARC-07). |
| ADR-012 | vcpkg manifest (vcpkg.json) for libraries; single-header libraries in third_party/. |
| ADR-013 | Four simulation LOD tiers (ARC-05). |
| ADR-014 | Git + GitHub (private repo) + GitHub Actions: build and test on every push (Windows runner). |
| ADR-015 | Debug safety net: warnings as errors, AddressSanitizer, asserts. |
| [ADR-016](ADR-016-layer-boundary-enforcement.md) | Enforce layer boundaries with narrow target interfaces, private identities, guarded headers and build-time include validation. |
| [ADR-017](ADR-017-luna-physics.md) | Own deterministic 3D physics in Luna (Luna Physics), fixed-point 32.32 math, SI units (1 tile = 1 m). |

Luna (ARC-09, ARC-10): the Platform, Physics and Engine layers form our game-agnostic engine in src/luna/.
