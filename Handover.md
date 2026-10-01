# Handover to ChatGPT: continue assembling Project Odyssey (2026-10-01)

Written by Mraw (Dominus Avengers) at the owner's request. You are taking over the build. Read this file, then `CLAUDE.md` (the Charter; `AGENTS.md` points to it), `docs/Codex.md` (v2.3, the prompts), `docs/status.md` and `Limit.md`, then continue with the first prompt in Codex order that is `To do`.

## 1. What this is
Project Odyssey (codename Odysseus): a C++20 / SDL3 pixel-art civilization simulation for Windows x64, built by the owner (a C++ beginner; every story ends with a teach-back in `docs/learning-journal.md`). Repository: `C:\Users\Amek\.amek-ai\Odysseus\odysseus`, remote https://github.com/marius-stoian/Project-Odyssey.git. The owner works in three phases: Brief (Mraw to Anima), Codex (Anima writes `docs/Codex.md`), Assemble (Mraw builds prompt by prompt, in order). Do not build from the requirements directly; if a Codex prompt is wrong or blocked, append a `CI-###` line to `docs/codex-issues.md` and work elsewhere (the owner takes it to Anima).

## 2. Hard rules (permanent)
- Never write the names of AI vendors or models, "Co-Authored-By" or "Generated with" lines into code, comments, docs, commits, PRs or branch names. Credit "Mraw", "Dominus", "Anima" or "the AI team".
- Design and scope decisions are the owner's: ask in chat, in rounds of 2 to 4 options with the recommended one first, then record "Decided (owner, <date>)" in `docs/decisions.md`. M8 to M9 and M8b to M8e keep D-35 (the owner answers the design questions at each kickoff `K-xxx`). M10 to M14 design is delegated to Dominus (D-41): record each choice in `docs/decision-requests/<ID>.md` as "Decided by Dominus (delegated)".
- Technical choices (how to build what the owner decided) are yours: record them in an ADR (`docs/adr/`) or the story plan.
- Human gates: kill-gate results that need people (X-M2, X-M6), accounts, credentials, money, destructive actions outside the repo. Never create accounts or type credentials.
- Never touch the owner's uncommitted work: `assets/levels/valley.json` (leave it alone) and the Drive mirror `docs/project/requirements/Project Odyssey - MVP Backlog - Copy.xlsx` (Excel keeps it open; `git status` shows it as ` D`). Never `git add docs` or `git add -A`: add explicit paths.
- Architecture rules (Charter): six layers (Game > Engine > Platform > Core; Engine > Physics > Core; Game > Simulation > Physics > Core). Only `src/luna/platform/` touches SDL (and SDL_GPU, rule 11). The simulation is deterministic (seeded PCG32, one stream per system, one draw per call, no wall clock, no unordered iteration, integers for money and resources). Every project header starts with `#pragma once` then `#include "boundary.h"` (ADR-016). `src/game/*` and `apps/odysseus/main.cpp` must not contain the string "SDL" (the US-021 check fails otherwise, even in a comment).
- Code style: RAII, no raw new/delete, PascalCase types, camelCase functions, `kPascalCase` constants, comments explain WHY in plain English.

## 3. The build loop for every story (L-01, Charter)
1. `git checkout qa && git pull`, then `git checkout -b story/US-xxx`.
2. Read the Codex prompt; write `docs/plans/US-xxx.md`; implement; write tests where testable headless (except M10 to M14, D-41: tests are written and compile, but run at the exit review).
3. `pwsh tools/verify.ps1 -Story US-xxx` (since D-46 it builds and tests **Debug** by default with AddressSanitizer; `-Config Both` reproduces a Release problem; it saves evidence to `docs/evidence/US-xxx/`). Zero warnings and every ctest group green (27 groups at this point). Do not build under `%TEMP%` (warning MSB8029).
4. Update: a new `Milestone-<n>.md` at the root with the next AP-### id (the newest is `Milestone-76.md`, AP-077), a `CHANGELOG.md` entry (newest on top), the row in `docs/status.md`, a teach-back in `docs/learning-journal.md`, and `Limit.md`.
5. Commit "US-xxx: <imperative summary>" (explicit paths only), `git checkout qa`, `git merge --no-ff story/US-xxx -m "Merge story/US-xxx: <title>"`, `git push origin qa`. Green CI on `qa` (Release) makes the story Done. CI runs only on pushes to `qa` and `main`, skips docs-only pushes and cancels superseded runs (D-46); do not push story branches.
6. Milestone exits (`X-Mx`): review, merge `qa` into `main`, push, CI green on `main` (Debug and Release), tag `mx-done` and push the tag.
7. End every session with the assembly report (format in `CLAUDE.md`).

Tooling tips (Windows): CMake presets `windows-x64-debug` and `windows-x64-release`; vcpkg at `C:\dev\vcpkg`; Visual Studio 2026 (MSVC 19.51); `gh` is logged in as marius-stoian. On merge conflicts in `CHANGELOG.md` keep both sides (newest entry on top, no duplicates). Write files with an editor tool rather than shell heredocs with apostrophes. Keep simulation checks small (at most 10 seeds) and tool output short.

## 4. Where the work stands
- Codex v2.3, requirements v2.5 (Drive; mirrored in `docs/project/requirements/`). M0 to M8 are done and tagged (`m8-done` at d058dfc). M8b "Resolution and GPU renderer" is in progress: K-M8b Done, **S-US-230 Done locally** (merged into `qa` at 4d97913 and pushed).
- **First thing to do:** check the GitHub Actions run for `qa` head 4d97913 (`gh run list --branch qa --limit 3`). The runner is `windows-latest`; if the Windows SDK there has no `dxc.exe`, CMake builds without the GPU backend and the GPU tests skip themselves (this is expected and intended, ADR-021); if CI is red for another reason, fix it before starting US-231. When it is green, US-230 is Done (the row in `docs/status.md` already says Done; correct it if CI says otherwise).
- Local verification of US-230: Debug and Release, zero warnings, 27/27 tests (log in `docs/evidence/US-230/`).

### What US-230 built (read `docs/plans/US-230.md` and `docs/adr/ADR-021-sdl-gpu-renderer.md`)
- `src/luna/platform/backend.h`: `RenderBackend` interface; `sdl_renderer_backend.cpp` (the old SDL_Renderer drawing, now into a virtual-screen texture, then enlarged); `gpu_backend.cpp` (SDL_GPU, Direct3D 12, HLSL shaders compiled at build time by the Windows SDK `dxc.exe` into DXIL headers, batching, Normal and Add blend pipelines, whole-step blit with black bars, screenshots through a download buffer). Shaders: `src/luna/platform/shaders/*.hlsl`.
- `Window` owns the OS window, events and gamepads and asks a backend to draw. `--renderer auto|gpu|sdl` (default auto: GPU, fall back to SDL_Renderer with the reason logged). `-DLUNA_GPU=OFF` builds without the GPU backend.
- The `Renderer` interface and everything in `src/game/` are unchanged. GPU and SDL_Renderer pictures are identical to the pixel (tests compare 921,600 pixels, 0 different; `kTexelNudge = 1/512` in both backends).

## 5. Next stories, in order
1. **S-US-231** (M8b): 960x540 virtual screen and window modes. Uses the owner's answers D-44: window sizes 1280x720, 1600x900, 1920x1080, 2560x1440 (2560x1440 recommended); whole-number scaling with black bars by default; zoom and UI scale in Settings plus mouse wheel and keys for zoom; first start windowed 1280x720, zoom 2x, UI scale 1x, lighting Medium. Both backends read the virtual size from `WindowSettings`. Design: `docs/plans/M8b-renderer-design.md`.
2. S-US-232, S-US-233, S-US-234, then **X-M8b** (exit review, merge `qa` to `main`, tag `m8b-done`).
3. K-M8c (Lighting and shadows: ask the owner its design questions first), then M8c stories and X-M8c; M8d Buildings; M8e Building life; K-M9 (this one also checks that M8e is done). Brief: `docs/plans/M8b-M8d-render-light-build-brief.md`. Decided so far: D-06, D-42, D-43, D-44, D-46.
4. M10 to M14 follow D-41; X-M6 waits for X-M14.

## 6. Open items
- `docs/codex-issues.md` CI-009 (K-M8b step 1 says to confirm M8e is done; it should say M8 is done): open, for Anima.
- The owner reads `docs/gates/M8-smalltalk.md` (50 NPC small-talk samples) and judges the greeting rule `kGreetingBubblesAtOnce = 2` whenever convenient; do not change either without the owner.
- Dialogue format (`.dlg`) and the conversation system are documented in `docs/guides/dialogue-format.md`; M8 gate review in `docs/gates/M8.md`.

## 7. Useful commands
```text
pwsh tools/verify.ps1 -Story US-231        # local build + tests (Debug), evidence saved
pwsh tools/verify.ps1 -Story US-231 -Config Both
build\...\odysseus.exe --renderer sdl      # force the old renderer
build\...\odysseus.exe --screenshot a.bmp --quit-after 3   # screenshot (BMP)
```
Seeded run for screenshots: a packaged copy of the game (odysseus.exe, SDL3.dll, the sanitizer DLL, `assets/` in one folder) with `--new-game --type 7:1.2 --click 128:101:0.5 --click 264:56:1.0 --click 68:119:1.5 --click 67:147:3.0`; settings go to `--save-dir`.
