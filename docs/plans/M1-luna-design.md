# Luna v1 design (M1: walking skeleton)

Architect's design for M1, shared by US-020..US-024. Codex v1.3; requirements v1.4 (ARC-09, ADR-002, ADR-003, ADR-006, ARC-03); owner decisions D-04 (32x48 px characters, 8 facing directions) and D-13 (SDL3 from vcpkg).

## Principle
> **Update (requirements v1.5, Codex v1.4):** Luna gains a third layer, **Physics** (`luna::physics`, ARC-10, ADR-017), built in milestone M1b: deterministic fixed-point 3D math, hit detection, ballistics, rigid bodies. It sits between Core and the Engine and Simulation layers.

Luna knows nothing about Odysseus (Charter rule 9). Only `src/luna/platform/` includes SDL3 (rule 2), and the build enforces both (ADR-016). Luna therefore has two halves:

| Luna layer | Owns | Talks to |
|---|---|---|
| **Platform** (`luna::platform`) | SDL lifetime, window, SDL_Renderer (textures, draw, present), events translated into Luna's own enums, clock, user folder | SDL3 |
| **Engine** (`luna::engine`) | Game loop with fixed timestep, frame statistics, the `Game` interface, input intents and bindings, `Renderer` interface, sprites and animation, tile map, camera | Platform (never SDL directly) |

The Odysseus **Game** layer (`odysseus::game`) implements `luna::engine::Game` and draws the demo world: placeholder art, a test map, the walking character.

## Main types

```cpp
namespace luna::platform {
class System;          // RAII: SDL_Init(video, gamepad) ... SDL_Quit
class Window;          // RAII: SDL_Window + SDL_Renderer (vsync), logical 480x270 integer presentation
struct Event;          // Quit, KeyDown/KeyUp (luna Key), Gamepad button/axis, GamepadAdded/Removed
std::uint64_t nowNanoseconds();
}
namespace luna::engine {
class FixedStepClock;  // 20 ticks per second (ADR-006): advance(elapsed) -> ticks to run, alpha() for interpolation
class FrameStats;      // frame times -> average FPS, logged
class Game {           // what a game gives Luna (interface, virtual functions)
public:
    virtual ~Game() = default;
    virtual void update(const Intents& intents) = 0;   // once per fixed tick
    virtual void render(Renderer& renderer, double alpha) = 0; // once per frame
};
struct AppConfig { std::string title; int windowWidth = 1280, windowHeight = 720; int virtualWidth = 480, virtualHeight = 270; int ticksPerSecond = 20; };
int run(const AppConfig&, Game&, const RunOptions&);   // the loop; returns the exit code
}
```

## Decisions
- **Fixed timestep (ADR-006).** 20 ticks per second; each frame runs `floor(accumulated / 50 ms)` ticks (capped at 10 per frame so a stall never snowballs), then renders with `alpha = leftover / 50 ms`. The simulation speed therefore does not depend on the monitor (US-020).
- **Frame pacing.** VSync on (60 FPS on the development PC's 60 Hz monitor). If VSync is unavailable, a simple limiter sleeps to about 60 FPS.
- **Crisp pixels (US-022).** Everything is drawn to a 480x270 virtual screen; SDL's integer-scale logical presentation scales it by the largest whole number that fits and letterboxes the rest; textures use nearest-neighbour sampling. Luna also computes the same scale itself (`integerScale()`) so it can be unit-tested and logged.
- **Renderer interface (ADR-003).** `luna::engine::Renderer` is abstract; `SdlRenderer` forwards to the Platform window; `RecordingRenderer` (tests) records draw calls, so drawing logic is tested without a window.
- **Input (ARC-03, US-021).** Platform turns SDL events into Luna's `Key`, `GamepadButton` and `GamepadAxis` enums. Engine's `InputMap` turns them into intents (`MoveUp/Down/Left/Right`, `Interact`, `OpenMenu`) through bindings (WASD and arrows, E, Esc; left stick with a 0.3 dead zone, D-pad, South button, Start). Game code sees only `Intents`.
- **Tests.** Headless unit tests per layer: `luna_tests` (Engine identity) and `odysseus_game_tests` (Game identity). End-to-end: `odysseus.exe --quit-after <seconds>` runs the real window and then requests to close exactly like the close button; a ctest reads its log.
- **Placeholder art.** Generated in code by the Game layer (no files, no licences) until D-05 (M3) decides real art. Tiles are 32x32 px, characters 32x48 px (D-04).

## Delegated decisions (Dominus, recorded in docs/decisions.md)
- **D-16 Tile size: 32x32 px.** Matches the 32-px-wide characters of D-04; the 480x270 virtual screen shows 15 x 8.4 tiles.
- **D-17 Movement directions: 8.** D-04 gives characters 8 facing directions, and US-024 says "four directions". Luna supports 8-way movement (diagonals at the same speed); US-024's scenarios use single directions, so they hold unchanged.
- **D-05 (M1 only): placeholder art is programmer art generated in code.** Real art is still decided at M3.
