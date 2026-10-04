# Project Odyssey

[![CI](https://github.com/marius-stoian/Project-Odyssey/actions/workflows/ci.yml/badge.svg)](https://github.com/marius-stoian/Project-Odyssey/actions/workflows/ci.yml)

Codename **Odysseus**. A 2D pixel-art life and civilization simulation written from scratch in C++20 on SDL3. Windows x64 first, Android and iOS later.

You start as a 12-year-old in a stone-age clan and grow into its leader. The clan lives on without you: people have needs, memories, grudges and loves, and the story of each run comes from those systems interacting. The first release (the MVP) is a vertical slice of Age 1: one generated region, one hero, five professions, and a win condition of leading the whole region.

The game runs on **Luna**, our own game-agnostic engine, including a deterministic 3D physics layer. Everything is seeded: the same seed gives the same world, the same clan and the same history.

## Status

Early development. The simulation, the playable world, the Editor and the new GPU renderer exist. Lighting is being built now. Quests, data editors, politics, trade and a full NPC economy are planned. Progress is tracked in [Milestone.md](Milestone.md) (one snapshot per session) and [docs/status.md](docs/status.md) (every build prompt).

## Features

### Clan simulation
- People have needs that change over time, and choose what to do with a utility AI.
- Events are remembered and spread as gossip.
- Every death and feud has a recorded cause. People quarrel, blame, take revenge, court rivals for a partner, share food, nurse the sick, teach the young and hunt together.
- A calendar with day, night and four seasons. Summer days run 15 hours, winter days 8.
- The simulation writes a readable **chronicle** of the clan and tells its story in episodes.
- Saves are versioned JSON, written to a temp file and renamed, with three backups kept.

### Playing the hero
- Walk with WASD, the arrow keys or a gamepad. Aim with the mouse; the hero faces eight directions.
- Fight with sword, axe, whip, bow, crossbow, thrown weapons and staff bolts. Weapons carry elements (fire, ice, poison, lightning, void). Shots fly on real arcs.
- A hotbar, pickups, hit points and death.
- Five professions: hunter, gatherer, flint-knapper, fire-keeper and shaman-healer, each with a skill, tools and actions. They feed two of the game's pillars, Trade and Religion.
- Crafting at the fire or at a stone, knapping flint, gathering, chopping, hunting, trading and tending the sacred fire.
- A tutorial, plus a procedurally generated region with plants, trees, animals and weather (drizzle to storm).

### World interactions and conversation
- Objects advertise what can be done with them. A rule language over tags and world state decides what is available, and a context menu is built from that data.
- Timed actions, world state, and NPCs and animals use the same interactions as the player.
- Hot reload with **F5** and an on-screen validation panel that names the file and field of any mistake.
- Characters talk to the hero and to each other, using a `.dlg` text format for written scenes and generated small talk for the rest. Conversations are remembered.

### Rendering and lighting
- Luna draws through SDL_GPU with shaders, and falls back to SDL's renderer.
- A 960x540 virtual screen with windowed, borderless and exclusive full screen, camera zoom (1x or 2x) and UI scale.
- A lit sprite shader with up to 64 point lights, normal maps generated from the sprite art, a day-night and seasonal sky, fires, torches and glowing effects, and a sun and moon that cast light and cause eclipses.

### Level Editor
- **F2** stops the world and opens the Editor, **F1** plays your changes. Painting ground (brush, rectangle, fill, erase), placing characters, plants, animals, weather effects and world objects, with undo. Levels are plain JSON files.
- See [docs/guides/editor.md](docs/guides/editor.md).

### Content is data
Characters, plants, animals, weapons, materials, interactions, dialogue, light and simulation rules live as JSON in [assets/data/](assets/data/). Errors name the file and the field.

## Planned

Quests, data editors for rules and routines, world editing and region generator overrides, politics as a third pillar, a technology pillar, NPC classes, a trade economy, and buildings with interiors. The roadmap is chapter 12 of the requirements in [docs/project/](docs/project/README.md).

## What you need
- Visual Studio 2026 (or 2022) with the "Desktop development with C++" workload and "C++ AddressSanitizer"
- CMake 3.28 or newer
- Git
- vcpkg, with the environment variable `VCPKG_ROOT` pointing at it (for example `C:\dev\vcpkg`)

Do not put the repository under the Windows Temp folder. Visual Studio then prints warning MSB8029, which breaks the zero-warnings rule.

## Build
From the repository folder:

```powershell
cmake --preset windows-x64-debug            # once: finds Visual Studio, installs libraries from vcpkg.json
cmake --build --preset windows-x64-debug    # Debug: slower, with AddressSanitizer and asserts
cmake --build --preset windows-x64-release  # Release: fast
```

The programs land in `build\windows-x64\bin\Debug\` and `build\windows-x64\bin\Release\`:

| Program | What it is |
|---|---|
| `odysseus.exe` | The game. |
| `odysseus_headless.exe` | The console clan simulator: `--seed 7 --years 100` runs a century and reports, `--chronicle` prints the clan's story, `--help` lists the options. |
| `odysseus_atlas.exe` | Tool that builds sprite atlases and normal maps. |
| `odysseus_tests.exe` | All automated tests. |
| `luna_physics_tests.exe` | Luna Physics tests: fixed-point math, rotations, determinism. |

## Run
`odysseus.exe` opens the game window. Walk with WASD, the arrow keys or a gamepad's left stick or D-pad. E, Space or the gamepad's South button uses the held item. Useful options:

| Option | Effect |
|---|---|
| `--editor` | Start in the Editor |
| `--level <file>` | Play or edit another level file |
| `--quit-after <seconds>` | Close automatically |
| `--screenshot <file.bmp>` | Save the last frame |
| `--log-dir <folder>` | Write the log elsewhere |
| `--hold MoveRight:0.5:3` | Hold an intent between two times (scripted play) |

```powershell
.\build\windows-x64\bin\Release\odysseus.exe
```

Settings (window mode, size, scaling, zoom, UI scale) are in [docs/guides/settings.md](docs/guides/settings.md).

## Logs
Every run writes a timestamped log to `%APPDATA%\Project Odyssey\Odysseus\logs\` (the last five runs are kept). Attach the newest file to bug reports. In Debug builds, `ODYSSEUS_ASSERT(condition, "message")` logs the file and line of a broken assumption and stops in the debugger.

## Test
```powershell
ctest --preset windows-x64-debug
```

CI builds Debug and Release on Windows for `qa` and `main` and runs all tests ([.github/workflows/ci.yml](.github/workflows/ci.yml)). Keep `main` green.

## Debug in Visual Studio
1. File > Open > Folder, choose this folder.
2. Pick configuration `windows-x64-debug` and startup item `odysseus.exe`.
3. Click in the left margin of a line to set a breakpoint, then press F5.

## Architecture

Six layers. Each is a CMake library target, and a target links only the layers below it.

| Folder | Target | Depends on |
|---|---|---|
| `src/core/` | `odysseus_core` | nothing |
| `src/luna/platform/` | `luna_platform` | core |
| `src/luna/physics/` | `luna_physics` | core |
| `src/luna/engine/` | `luna_engine` | platform, physics |
| `src/sim/` | `odysseus_sim` | core, physics |
| `src/game/` | `odysseus_game` | engine, sim |

Rules worth knowing:
- Only `luna_platform` talks to the operating system, through SDL3. A mobile port needs only a new platform layer.
- The simulation has no graphics. It runs headless in tests and in `odysseus_headless`.
- Randomness comes from seeded PCG32 streams, physics uses 32.32 fixed-point numbers, and the simulation runs on a fixed 20 ticks per second.
- Luna never includes simulation or game code.

[cmake/ValidateLayerIncludes.cmake](cmake/ValidateLayerIncludes.cmake) checks these boundaries on every build, and [tests/architecture/](tests/architecture/) proves them. The rationale is in [ADR-016](docs/adr/ADR-016-layer-boundary-enforcement.md), and the full charter is [CLAUDE.md](CLAUDE.md).

## Repository layout
| Folder | What |
|---|---|
| `apps/` | The programs: game, headless simulator, atlas tool |
| `assets/` | Game data, levels, sprites, audio, fonts |
| `tests/` | doctest tests |
| `docs/` | Codex, status, decisions, plans, guides, evidence, learning journal. See [docs/README.md](docs/README.md) |
| `docs/project/` | Requirements, backlog, diagrams and archive, mirrored from Google Drive |
| `tools/` | Codex and workspace sync scripts |

## How it is built

Built by Mraw (the Dominus Full Team) following [docs/Codex.md](docs/Codex.md), a sequence of build prompts written by Anima. The requirements document is the source of truth for what to build, and the Codex for how and in what order. Owner decisions are in [docs/decisions.md](docs/decisions.md), and every story ends with a teach-back note in [docs/learning-journal.md](docs/learning-journal.md).

When an AI coding session starts in this folder, [tools/sync-codex.ps1](tools/sync-codex.ps1) copies in a newer Codex from Google Drive and regenerates `CLAUDE.md` and `.claude/agents/`. [tools/sync-workspace.ps1](tools/sync-workspace.ps1) mirrors the project documents. The Dominus and Anima skills live in [.claude/skills/](.claude/skills/).
