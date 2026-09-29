# Learning journal

Welcome, Amek. This is your C++ notebook for Project Odyssey. Every time Mraw finishes a story, the writer adds one short entry here: what we built, one C++ idea explained in plain words with a tiny example from our own code, where to find it, a 15-minute exercise and one question to check you got it. Read them in order and do the exercises; by the end of the MVP you will have learned C++ by building your own game, one small step at a time.

## US-001: Build and debug from a clean checkout (2026-09-29)

**What we built.** One command now turns our source files into three programs: the game, the headless simulator and the tests. Anyone who clones the repository gets the same result, with no warnings.

**The idea: compiler, linker, build system.** The *compiler* (MSVC's `cl.exe`) reads one `.cpp` file at a time and turns it into machine code, an object file. `main.cpp` only *promises* that `versionString()` exists somewhere; the compiler trusts the header. The *linker* then glues the object files and libraries into an `.exe` and checks every promise was kept. If `versionString()` were missing you would get a linker error, not a compiler error. The *build system* (CMake) decides which files to compile, with which settings, and what to link. A *preset* is a saved set of those choices with a name, like `windows-x64-debug`.

```cpp
add_executable(odysseus apps/odysseus/main.cpp)        // compile this
target_link_libraries(odysseus PRIVATE odysseus_core)  // then link Core into it
```

**Where to look.** [CMakeLists.txt:62](../CMakeLists.txt) (the two lines above), [apps/odysseus/main.cpp:8](../apps/odysseus/main.cpp) (the call into Core), [CMakePresets.json](../CMakePresets.json).

**Try it (15 minutes).** In `CMakeLists.txt` change `project(Odysseus VERSION 0.1.0 ...)` to `0.2.0`, rebuild Debug and run `odysseus.exe`. Then comment out line 63 (`target_link_libraries(odysseus ...)`), rebuild and read the error: it says "unresolved external symbol". Put the line back.

**Check yourself.** When you commented out `target_link_libraries`, why did the compiler still succeed and only the linker fail?

## US-002: Run the build and tests on every push (2026-09-29)

**What we built.** Every time code reaches GitHub, a Windows machine in the cloud builds the game and runs all tests. If something breaks, the run turns red and names the failing test, even if we forgot to test on our PC.

**The idea: commit, branch, push.** A *commit* is a saved snapshot of the whole project with a message saying why it changed. A *branch* is a separate line of commits, so a story can be built without disturbing `main`, the version that always works. *Push* sends your commits to GitHub; that push is what wakes up CI. When the story is accepted, we *merge* its branch into `main`.

```powershell
git checkout -b story/US-002      # new branch for the story
git commit -am "US-002: ..."      # snapshot with a message
git push -u origin story/US-002   # send to GitHub, CI starts
```

**Where to look.** [.github/workflows/ci.yml](../.github/workflows/ci.yml) (the CI recipe), the Actions tab on GitHub, `git log --oneline --graph` in the repo folder.

**Try it (15 minutes).** Create a branch `practice/red-ci`, add `CHECK(false);` to the test in `tests/core/version_test.cpp`, commit and push. Watch the run go red on GitHub and find the test name in the log. Then delete the branch: `git push origin --delete practice/red-ci`.

**Check yourself.** Why do we build each story on its own branch instead of committing straight to `main`?

## US-003: Enforce the layer rules in the build (2026-09-30)

*Built by ChatGPT, verified on Windows by Claude.*

**What we built.** Five library targets now represent our five layers, with explicit downward dependencies. Header checks and a validator reject forbidden includes, including shortcuts through relative paths.

**The idea: CMake targets and `target_link_libraries`.** A target is a named thing CMake builds, such as a library or executable. `add_library` lists the source files belonging to a library; `target_link_libraries` connects it to libraries it needs. `PUBLIC` means both this target and its callers receive the dependency's build requirements, including public header paths. `PRIVATE` applies the dependency only to the target itself. Simulation links Core, so it can use Core's headers without receiving Engine's headers. A link is a dependency declaration, not a complete include barrier: a relative path could bypass header search paths. Our boundary checks and build validator close that accidental loophole.

```cmake
add_library(odysseus_sim STATIC src/sim/layer.cpp)
target_link_libraries(odysseus_sim PUBLIC odysseus_core)
```

**Where to look.** [CMakeLists.txt:71](../CMakeLists.txt) (Simulation's target), [CMakeLists.txt:76](../CMakeLists.txt) (Game's dependencies), [cmake/LayerRules.cmake:2](../cmake/LayerRules.cmake) (target registration), [src/luna/engine/boundary.h](../src/luna/engine/boundary.h) (include boundary).

**Try it (15 minutes).** On a practice branch, add `#include "../luna/engine/layer.h"` to `src/sim/layer.cpp`. Build Debug and read the include error naming the forbidden layer. Replace it with `#include "core/version.h"`, build again and compare; then restore the original file.

**Check yourself.** Why does Game receive Core's public headers through its dependencies, while Simulation still cannot include Engine through a relative path?

**Bonus lesson from verification.** The same test passed on Linux and failed on Windows, only because Visual Studio words its error message differently (`fatal  error C1083`, with two spaces). Tests that read compiler output should match the stable part, the error code, not the wording.

## US-004: Log what happens and stop on broken assumptions (2026-09-30)

**What we built.** Every run of the game now writes a log file in your user folder (`%APPDATA%\Project Odyssey\Odysseus\logs`), keeping the last five runs. And `ODYSSEUS_ASSERT` stops a Debug build the moment an assumption is false, after writing the file and line to the log.

**The idea: header and source files, namespaces, macros.** A *header* (`log.h`) is the promise: it declares what exists (`class LogSession`, `logInfo`). A *source file* (`log.cpp`) keeps the promise: it defines how it works. Other files include only the header, so they compile without seeing the details. A *namespace* (`odysseus::core`) is a surname for names: our `logInfo` can never clash with someone else's. A *macro* is text the preprocessor pastes in before compiling. We use one for the assert because only a macro can capture the *caller's* `__FILE__` and `__LINE__`:

```cpp
ODYSSEUS_ASSERT(health > 0, "a hero cannot act when dead");
// becomes: if (!(health > 0)) { reportAssertionFailure("health > 0", ..., __FILE__, __LINE__); __debugbreak(); }
```

**Where to look.** [src/core/log.h](../src/core/log.h) (the promise), [src/core/log.cpp](../src/core/log.cpp) (how rotation works), [src/core/assertions.h](../src/core/assertions.h) (the macro, Debug and Release versions), [apps/odysseus/main.cpp](../apps/odysseus/main.cpp) (one `LogSession` for the whole run).

**Try it (15 minutes).** In `apps/odysseus/main.cpp`, add `ODYSSEUS_ASSERT(version.empty(), "testing my first assert");` after the version line (and `#include "core/assertions.h"`). Press F5 in Visual Studio: it stops on your line. Open the newest file in the logs folder and find the file and line. Then build Release and run it: nothing happens, because asserts vanish in Release. Remove the line.

**Check yourself.** Why is `ODYSSEUS_ASSERT` a macro while `logInfo` is a normal function?

## US-020: Open a window with a steady game loop (2026-09-30)

**What we built.** Project Odyssey now opens a real 1280 x 720 window through our own engine, Luna, and runs a game loop: 60 frames per second on your screen, while the world ticks exactly 20 times per second whatever the monitor.

**The idea: an RAII wrapper around SDL_Window, with unique_ptr and a custom deleter.** SDL is a C library: you create a window with `SDL_CreateWindow` and must remember to call `SDL_DestroyWindow`. Forget it, or return early by mistake, and it leaks. `std::unique_ptr` normally calls `delete`; we give it a *deleter* that calls SDL's function instead. Now the window is destroyed automatically when the `Window` object goes away, even if an error is thrown.

```cpp
struct WindowDeleter { void operator()(SDL_Window* w) const { SDL_DestroyWindow(w); } };
std::unique_ptr<SDL_Window, WindowDeleter> window_;   // destroys itself
```

**Where to look.** [src/luna/platform/window.h](../src/luna/platform/window.h) (the deleters), [src/luna/engine/fixed_step_clock.cpp](../src/luna/engine/fixed_step_clock.cpp) (why the world speed does not depend on the monitor), [src/luna/engine/application.cpp](../src/luna/engine/application.cpp) (the loop).

**Try it (15 minutes).** In `src/game/odyssey_game.cpp`, change the three `clear...` numbers to your favourite colour and run the game. Then run `odysseus.exe --quit-after 10` from a terminal and read the "Average frame rate" line in the log.

**Check yourself.** Why does the game loop count *ticks* separately from *frames*?

## US-021: Control the game through intents (2026-09-30)

**What we built.** Keyboard and gamepad now drive the same actions. Pressing W, the Up arrow, pushing the stick up or the D-pad up all mean one thing to the game: *Move Up*. Touch screens can plug in later without changing a line of game code.

**The idea: separating "what happened" from "what it means" (the intent pattern, ARC-03).** Platform reports raw facts ("key W went down"); the Engine's `InputMap` translates facts into meanings through a table of bindings; the game only asks "does the player want to move up?". Each layer knows one thing. An `enum class` gives each meaning a safe name the compiler checks:

```cpp
enum class Intent { MoveUp, MoveDown, MoveLeft, MoveRight, Interact, OpenMenu, Count };
if (intents.held(Intent::MoveUp)) { /* walk */ }   // no keys here
```

**Where to look.** [src/luna/engine/input.cpp](../src/luna/engine/input.cpp) (`keyBinding()`: the default bindings), [src/luna/platform/sdl_events.cpp](../src/luna/platform/sdl_events.cpp) (the only place that knows SDL's names).

**Try it (15 minutes).** Add a binding so that the key Q also means `OpenMenu`: add `Q` to `Key` in `events.h`, map `SDL_SCANCODE_Q` in `sdl_events.cpp`, and add it to `keyBinding()`. Then add a check to `tests/luna/input_test.cpp` and run `luna_tests`.

**Check yourself.** Why does Luna use `SDL_SCANCODE_W` (a key's position) instead of the letter W?

## US-022: Draw sprites with crisp pixels (2026-09-30)

**What we built.** The hero appears on screen as sharp pixel art. Everything is drawn on a small 480 x 270 "virtual screen" and scaled up by a whole number (x2, x3, x4...), with black bars for the leftover space, so pixels never blur.

**The idea: interfaces with virtual functions.** `Renderer` is an *interface*: a list of promises (`createTexture`, `draw`) with no code. `WindowRenderer` keeps the promises by drawing into the real window; `RecordingRenderer` keeps them by just writing down what was asked, which is perfect for tests. The game only knows `Renderer&`, so it works with either:

```cpp
class Renderer {                       // the promise
public:
    virtual ~Renderer() = default;
    virtual void draw(const Texture& texture, const Rect& source, Point at) = 0;
};
class WindowRenderer final : public Renderer { ... };  // one way to keep it
```

`virtual` means "decide at run time which version to call". When we switch to SDL_GPU later (ADR-003), only one new class is written; the game does not change.

**Where to look.** [src/luna/engine/renderer.h](../src/luna/engine/renderer.h), [src/game/placeholder_art.cpp](../src/game/placeholder_art.cpp) (the hero is drawn with rectangles), [tests/luna/pixels_window_test.cpp](../tests/luna/pixels_window_test.cpp) (how we prove "no blur").

**Try it (15 minutes).** In `placeholder_art.cpp`, change `kTunic` to your favourite colour. Run the game with `odysseus.exe --quit-after 3 --screenshot hero.bmp` and open `hero.bmp`. Then resize the game window while it runs and watch the log report the new scale.

**Check yourself.** Why can the game's drawing code be tested without opening a window?

## US-023: Show a tile map with a following camera (2026-09-30)

**What we built.** A 64 x 64 world of grass, paths, rocks and a pond around the hero, seen through a camera that glides after its target and stops at the edges of the world. Only the roughly 135 tiles on screen are drawn, not all 4096.

**The idea: a 2D grid stored in a 1D vector.** A map feels two-dimensional, but memory is one long line. So we store row after row in one `std::vector` and compute where cell (x, y) lives:

```cpp
// row y starts after y full rows of `width` cells, then step x along it
return tiles_[y * width_ + x];
```

One allocation instead of 64 separate rows, and neighbours sit next to each other in memory, which is fast. The same trick is used for images (4 bytes per pixel, row after row).

**Where to look.** [src/luna/engine/tile_map.cpp](../src/luna/engine/tile_map.cpp) (`at()` and `visibleTiles()`), [src/luna/engine/camera.cpp](../src/luna/engine/camera.cpp) (`follow()`: a quarter of the way per tick), [src/game/test_map.cpp](../src/game/test_map.cpp) (how the valley is laid out).

**Try it (15 minutes).** In `test_map.cpp`, make the pond bigger (change `20` to `40` in the pond formula) and move it next to the crossing. Run the game with `--quit-after 2 --screenshot map.bmp` and look.

**Check yourself.** In a map 64 cells wide, at which index does cell (3, 2) live?

## US-024: Walk the character around the map (2026-09-30)

**What we built.** You can play! Arrow keys, WASD or a gamepad move the hero in 8 directions through the valley, with a walking animation; rocks and water block the way; let go and the hero stops, facing where they went. The camera follows.

**The idea: game state updated in fixed ticks, drawn with interpolation.** The hero moves only in `update()`, 20 times per second, exactly 4.8 pixels per tick. But the screen shows 60 frames per second. So we remember where the hero was at the previous tick and draw them part of the way (`alpha`) towards where they are now:

```cpp
double Hero::feetX(double alpha) const {
    return previousX_ + (x_ - previousX_) * alpha; // 0 = last tick, 1 = this tick
}
```

The rules stay simple and deterministic (ticks), while the picture stays smooth (frames).

**Where to look.** [src/game/hero.cpp](../src/game/hero.cpp) (`update()`: speed, diagonals, animation), [src/luna/engine/collision.cpp](../src/luna/engine/collision.cpp) (how a box stops at a wall).

**Try it (15 minutes).** In `src/game/hero.h`, change `speedPixelsPerSecond` from 96 to 160 and play. Then run `ctest --preset windows-x64-debug`: `US-024 Walk right` still passes (it reads the speed from the config), but `US-024 Walk to the rock` too? Find out why.

**Check yourself.** Why does the hero's position change 20 times per second while the picture changes 60 times per second?

## US-010: Advance a seeded world clock (2026-09-30)

**What we built.** The simulation has its own clock and calendar: 20 ticks per game second, days, four seasons, years, a daily weather roll, and speed control (pause, 1x, 2x, 4x). The same seed always gives the same world, down to one 64-bit "world hash".

**The idea: the accumulator, and integers versus floats.** The clock collects real time and pays it out in whole ticks; the leftover waits for the next frame. We use whole nanoseconds (integers), not seconds as decimals (floats), because floats round: `0.1 + 0.2` is not exactly `0.3`, and tiny errors add up over 6 million ticks and differ between computers. Integers never drift, so two computers always agree:

```cpp
accumulated_ += realNanoseconds * speed;           // whole numbers only
const std::uint64_t ticks = accumulated_ / kNanosecondsPerTick;
accumulated_ %= kNanosecondsPerTick;               // keep the remainder for next time
```

**Where to look.** [src/sim/game_clock.cpp](../src/sim/game_clock.cpp), [src/core/random.cpp](../src/core/random.cpp) (PCG32), [assets/data/sim/calendar.json](../assets/data/sim/calendar.json) (the year length is data).

**Try it (15 minutes).** Change `daysPerSeason` in `calendar.json` to 10 and run `odysseus_headless.exe --days 40`. Then break the file on purpose (write `"ten"`) and read the error: it names the file and the field.

**Check yourself.** Why would `double seconds += 0.05` twenty times per second eventually give two computers different worlds?
