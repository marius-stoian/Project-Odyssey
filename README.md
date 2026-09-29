# Project Odyssey

[![CI](https://github.com/marius-stoian/Project-Odyssey/actions/workflows/ci.yml/badge.svg)](https://github.com/marius-stoian/Project-Odyssey/actions/workflows/ci.yml)

Codename **Odysseus**: a 2D pixel-art life and civilization simulation in C++20 on SDL3. Windows x64 first; Android and iOS later.

Built by Mraw (the Dominus Full Team) following [docs/Codex.md](docs/Codex.md), written by Anima. Progress: [Milestone.md](Milestone.md) (snapshot per session, AP-###) and [docs/status.md](docs/status.md) (every prompt). Owner decisions: [docs/decisions.md](docs/decisions.md). Learning notes: [docs/learning-journal.md](docs/learning-journal.md).

## Anima's Codex
`docs/Codex.md` is a copy of Anima's master Codex on Google Drive. When Claude Code starts in this folder, a hook runs [tools/sync-codex.ps1](tools/sync-codex.ps1): if Anima published a newer version, it copies it in and regenerates `CLAUDE.md` (the Charter) and `.claude/agents/` (the Mraw roles), and Claude commits the sync. You can also run it by hand: `pwsh tools/sync-codex.ps1`.

## Project documents
The requirements (source of truth), backlog, diagrams and archive live in [docs/project/](docs/project/README.md). Their masters are on Google Drive; [tools/sync-workspace.ps1](tools/sync-workspace.ps1) mirrors any changed file into the repo at session start.

## Team skills
The Dominus skill (all hats; the full team is Mraw) and the Anima skill (Prompt Architect, writes the Codex) live in [.claude/skills/](.claude/skills/), so every Claude Code session opened in this repo has them, on any machine. They are copies of the owner's personal skills; when those change, copy them in again.

## What you need
- Visual Studio 2026 (or 2022) with the "Desktop development with C++" workload and "C++ AddressSanitizer"
- CMake 3.28 or newer
- Git
- vcpkg, with the environment variable `VCPKG_ROOT` pointing at it (for example `C:\dev\vcpkg`)

Do not put the repository under the Windows Temp folder: Visual Studio then prints warning MSB8029, which breaks our zero-warnings rule.

## Build
From the repository folder:

```powershell
cmake --preset windows-x64-debug          # once: finds Visual Studio, installs libraries from vcpkg.json
cmake --build --preset windows-x64-debug    # Debug: slower, with AddressSanitizer and asserts
cmake --build --preset windows-x64-release  # Release: fast
```

The programs land in `build\windows-x64\bin\Debug\` and `build\windows-x64\bin\Release\`:

| Program | What it is |
|---|---|
| `odysseus.exe` | The game. For now it prints its version and exits; the window arrives in US-020. |
| `odysseus_headless.exe` | The simulation without graphics. Becomes the console clan simulator in M1. |
| `odysseus_tests.exe` | All automated tests. |

## Run
`odysseus.exe` opens the game window (1280 x 720, resizable). Walk the hero with WASD, the arrow keys or a gamepad's left stick / D-pad. Options: `--quit-after <seconds>` closes it automatically, `--screenshot <file.bmp>` saves the last frame, `--log-dir <folder>` writes the log elsewhere, `--hold MoveRight:0.5:3` holds an intent between two times (scripted play).

```powershell
.\build\windows-x64\bin\Release\odysseus.exe
```

## Logs
Every run writes a timestamped log to `%APPDATA%\Project Odyssey\Odysseus\logs\` (the last five runs are kept). Attach the newest file to bug reports. In Debug builds, `ODYSSEUS_ASSERT(condition, "message")` logs the file and line of a broken assumption and stops in the debugger.

## Test
```powershell
ctest --preset windows-x64-debug
```

## Continuous integration
Every push to GitHub, on any branch, builds Debug and Release on a Windows machine and runs all tests ([.github/workflows/ci.yml](.github/workflows/ci.yml)). A red run names the failing test in its log. Keep `main` green.

## Debug in Visual Studio
1. File > Open > Folder, choose this folder.
2. Pick configuration `windows-x64-debug` and startup item `odysseus.exe`.
3. Click in the left margin of a line to set a breakpoint, then press F5.

## Layout
| Folder | What |
|---|---|
| `src/core/` | Core: logging, asserts, math, random, ids |
| `src/luna/platform/`, `src/luna/engine/` | **Luna**, our game-agnostic engine (ARC-09) |
| `src/sim/`, `src/game/` | The Odysseus simulation and game |
| `apps/` | The programs: odysseus.exe, odysseus_headless.exe |
| `cmake/` | Layer rules and the include validator (ADR-016) |
| `tests/` | doctest tests; `tests/architecture/` proves the layer rules |
| `assets/` | Game data, sprites, audio, fonts |
| `docs/` | Codex, status, decisions, plans, evidence, reports, learning journal. See [docs/README.md](docs/README.md) |
| `docs/project/` | Project documents mirrored from Google Drive: requirements (source of truth), backlog, Codex analysis, diagrams, archive. See [docs/project/README.md](docs/project/README.md) |
| `tools/` | Codex and workspace sync scripts |

The layer rules are in [CLAUDE.md](CLAUDE.md).

## Layer boundaries

Each layer has its own CMake library target. A target links only the layers below it; `PUBLIC` dependencies also carry the lower layers' header paths to callers.

| Target | Direct layer dependencies |
|---|---|
| `odysseus_core` | None |
| `luna_platform` | `odysseus_core` |
| `luna_engine` | `luna_platform` |
| `odysseus_sim` | `odysseus_core` |
| `odysseus_game` | `luna_engine`, `odysseus_sim` |

The game executable links `odysseus_game`; the headless executable links `odysseus_sim`. Luna stays independent of game and simulation code, and Simulation stays independent of graphics.

[cmake/LayerRules.cmake](cmake/LayerRules.cmake) gives each target only its own public headers and a private layer identity. Header guards reject forbidden consumers, including relative-path includes. [cmake/ValidateLayerIncludes.cmake](cmake/ValidateLayerIncludes.cmake) checks production files on every build for missing boundary guards, forbidden includes and SDL use outside Platform. This catches accidental boundary violations even after configuration; it does not protect against deliberately editing the enforcement. The acceptance tests are in [tests/architecture/](tests/architecture/), and the design rationale is [ADR-016](docs/adr/ADR-016-layer-boundary-enforcement.md).
