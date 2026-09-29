# Project Odyssey

[![CI](https://github.com/marius-stoian/Project-Odyssey/actions/workflows/ci.yml/badge.svg)](https://github.com/marius-stoian/Project-Odyssey/actions/workflows/ci.yml)

Codename **Odysseus**: a 2D pixel-art life and civilization simulation in C++20 on SDL3. Windows x64 first; Android and iOS later.

Built by Mraw (the Dominus Full Team) following [docs/Codex.md](docs/Codex.md), written by Anima. Progress: [docs/status.md](docs/status.md). Owner decisions: [docs/decisions.md](docs/decisions.md). Learning notes: [docs/learning-journal.md](docs/learning-journal.md).

## Anima's Codex
`docs/Codex.md` is a copy of Anima's master Codex on Google Drive. When Claude Code starts in this folder, a hook runs [tools/sync-codex.ps1](tools/sync-codex.ps1): if Anima published a newer version, it copies it in and regenerates `CLAUDE.md` (the Charter) and `.claude/agents/` (the Mraw roles), and Claude commits the sync. You can also run it by hand: `pwsh tools/sync-codex.ps1`.

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
The programs are console programs for now, so start them from a terminal (a double-click opens and closes a window instantly):

```powershell
.\build\windows-x64\bin\Release\odysseus.exe
```

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
`src/` holds the five layers (core, platform, engine, sim, game), `apps/` the programs, `tests/` the doctest tests, `assets/` the game data, `docs/` everything written. The layer rules are in [CLAUDE.md](CLAUDE.md).
