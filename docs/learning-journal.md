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

*Built by ChatGPT, verified on Windows by Mraw.*

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

## US-025: Build deterministic 3D math (2026-09-30)

**What we built.** Luna has a sixth layer, Physics, with its own number type `Fixed`, 3D vectors and quaternion rotations. A million mixed calculations give the very same 64-bit hash in Debug, in Release and on the CI machine.

**The idea: fixed-point numbers and operator overloading.** A `Fixed` is a whole number that counts tiny steps of 1/2^32 (about 0.00000000023). 1.5 is stored as 1.5 x 2^32 = 6442450944. Adding two of them is ordinary integer addition, which every CPU does identically; floats round in ways that can differ between compilers. Multiplying needs care: (a x 2^32) x (b x 2^32) has an extra 2^32, so we compute the 128-bit product and shift it back. Operator overloading lets us hide all that behind a normal `*`:

```cpp
friend Fixed operator+(Fixed a, Fixed b) { return fromRaw(checkedAdd(a.raw_, b.raw_)); }
```

Now `a + b * c` reads like school maths. A quaternion is four such numbers describing a turn; rotating a vector by it never suffers "gimbal lock".

**Where to look.** [src/luna/physics/fixed.h:55](../src/luna/physics/fixed.h) (the operators), [src/luna/physics/fixed.cpp:108](../src/luna/physics/fixed.cpp) (multiplication with 32-bit "digits", like long multiplication on paper), [src/luna/physics/quat.cpp:25](../src/luna/physics/quat.cpp) (rotating a vector).

**Try it (15 minutes).** In `tests/physics/rotation_test.cpp`, rotate by `physics::degrees(45)` eight times instead of 90 four times, and run `luna_physics_tests.exe --test-case="US-025 Rotations"`. Then print `Fixed::fromRatio(1, 3).raw()` and check by hand that it is 2^32 / 3, rounded.

**Check yourself.** Why is `Fixed::fromRatio(1, 3) * Fixed::fromInt(3)` one step short of 1, and why is that still perfectly deterministic?

## US-026: Detect hits between shapes (2026-09-30)

**What we built.** Luna Physics knows spheres, capsules (pills) and boxes. It can tell whether two of them touch (and where, and which way to push them apart), and it can follow a fast-moving ball along its whole path, so a spear moving 10 m per tick still hits a 20 cm target instead of jumping over it. A grid of 2 m cells makes 1,000 bodies cheap: only 114 pairs need an exact test.

**The idea: plain structs, pure functions and a spatial grid.** Shapes are just data:

```cpp
struct Sphere {
    Vec3 center;
    Fixed radius;
};
```

The geometry lives in pure functions such as `overlap(a, b)`: same inputs, same answer, no hidden state, which makes them easy to test and deterministic. The grid is the "phone book" trick: instead of asking every one of 1,000 people whether they stand next to you (499,500 questions), you only ask the people filed under your street.

**Where to look.** [src/luna/physics/shapes.cpp](../src/luna/physics/shapes.cpp) (`sweep()`: a moving ball is a ray against the target grown by the ball's radius), [src/luna/physics/spatial_grid.cpp](../src/luna/physics/spatial_grid.cpp) (`findContacts()`: broad phase, then narrow phase).

**Try it (15 minutes).** In `tests/physics/shapes_test.cpp`, change the grid's cell size in `US-026 Many bodies` from 2 to 64 metres and run the test in Release: watch "pairs tested" and the time grow. Then try 0.5 m.

**Check yourself.** Why does checking only where the spear is at each tick miss the target, and how does sweeping fix it?

## US-027: Fly projectiles with real ballistics (2026-09-30)

**What we built.** Spears and darts fly in true arcs: gravity pulls them down, air drag slows them (more the faster they go), and wind pushes them sideways. An aim solver finds the launch angle that hits a target 25 m away, and the throw really hits it.

**The idea: numerical integration, and units.** Physics formulas describe change: velocity changes by acceleration, position by velocity. A computer cannot do "continuous", so it takes many tiny steps (here 200 per second) and adds up the changes. Semi-implicit Euler first updates the velocity, then moves with the new velocity:

```cpp
projectile.velocity += projectileAcceleration(projectile, air) * dt;
projectile.position += projectile.velocity * dt;
```

Units keep us honest: velocity (m/s) x dt (s) gives metres, so both sides of `position += ...` are metres. If the units of a formula do not match, the formula is wrong. We checked the result against the textbook: 20 m/s at 45 degrees lands 40.7 m away, v^2/g = 40.8 m.

**Where to look.** [src/luna/physics/ballistics.cpp](../src/luna/physics/ballistics.cpp) (`projectileAcceleration`: the drag equation; `aimLaunchAngle`: the secant method).

**Try it (15 minutes).** In `tests/physics/ballistics_test.cpp`, change the crosswind in `US-027 Drag and wind` from 5 to 10 m/s and read the MESSAGE lines: does the drift double? Then set `kProjectileSubsteps` to 1 and watch the range error in `US-027 Arc` grow.

**Check yourself.** Why does air drag make the aim solver choose a slightly higher angle than the vacuum formula?

## US-028: Push and bounce bodies (2026-09-30)

**What we built.** Things in the world have mass now. A shove changes a body's velocity by impulse / mass, a dropped ball bounces to a quarter of its height each time (restitution 0.5) and then lies still, and a crate sliding on grass stops exactly where the friction formula says.

**The idea: classes with invariants, and fixed-timestep integration.** An invariant is a rule that must always be true, such as "the mass is positive". `RigidBody` keeps its data `private` and checks the rules once, in the constructor:

```cpp
ODYSSEUS_ASSERT(mass > kFixedZero, "a rigid body needs a positive mass");
```

After that, the only way to change a body is through its member functions (`applyImpulse`, `step`), which keep the rules. Each `step` advances time by a fixed amount (1/20 s). Inside the step we use the exact formula for constant acceleration, x += v t + a t^2 / 2, and we work out the exact moment the ball touches the ground, so the bounce heights come out right to five digits.

**Where to look.** [src/luna/physics/rigid_body.cpp:93](../src/luna/physics/rigid_body.cpp) (`step`: flying, bouncing, sliding, sleeping), [src/luna/physics/rigid_body.cpp:57](../src/luna/physics/rigid_body.cpp) (`flyFor`).

**Try it (15 minutes).** In `tests/physics/rigid_body_test.cpp`, give the ball restitution 0.8 and change the expected ratio to 0.64 (e^2). Run `luna_physics_tests.exe --test-case="US-028 Bounce*"` and read the bounce heights.

**Check yourself.** What would go wrong if `mass_` were a public member that any code could set to 0?

## US-029: Throw a spear in the demo (2026-09-30)

**What we built.** Press Interact (E, Space or the gamepad's South button) and the hero throws a spear at the straw target in front: it flies in an arc, its shadow glides along the ground, and it sticks in the target. Throw at the target behind the boulder and the spear hits the rock instead. Flint tips hurt almost six times more than sharpened wood.

**The idea: putting it together, and content as data.** Every piece from M1 and M1b meets here: intents (Interact), the tile map (rocks become 3D boulders), ballistics (the arc), swept hits (no tunnelling) and the Engine's top-down view, which draws height by lifting the sprite:

```cpp
// screen y = (y - z) x 32: the higher the spear, the further up the screen
return {toDouble(metres.x) * kPixelsPerMetre, (toDouble(metres.y) - toDouble(metres.z)) * kPixelsPerMetre};
```

How hard a flint tip is lives in `assets/data/materials.json`, not in C++. A designer can change it without recompiling, and a typo is caught at start with a message naming the file and the field.

**Where to look.** [src/game/spear_range.cpp](../src/game/spear_range.cpp) (`throwSpear`, `update`), [src/luna/engine/physics_view.cpp](../src/luna/engine/physics_view.cpp), [assets/data/materials.json](../assets/data/materials.json).

**Try it (15 minutes).** In `materials.json`, set the flint `hardness` to 10 and run the game: throw at the west target and read the damage in the log. Then set it to 11 and read the error message.

**Check yourself.** Why is the shadow drawn at (x, y) while the spear is drawn at (x, y - z)?

## US-011: Give every person needs that change over time (2026-09-30)

**What we built.** The console world has people now: a clan of 20 with names and ages from data files. Every game hour they get a little hungrier, more tired, colder and lonelier; a meal raises Hunger (never above 100); and someone whose Hunger stays at zero for three days dies, which the chronicle writes down: "Summer, year 1: Garu died of starvation." (Nobody eats yet: choosing what to do is the next story.)

**The idea: plain structs as components, and std::vector.** A `Person` is just data, with no functions of its own that change it:

```cpp
struct Person {
    std::string name;
    Needs needs;          // four whole numbers
    bool alive = true;
};
```

The "systems" are ordinary functions that take the data and change it, such as `decayForHour(person.needs, config, winter, hour)`. Keeping data and behaviour apart makes each system easy to test alone, and it is exactly how an Entity Component System (EnTT, coming in M3) works. All the people live in one `std::vector<Person>`: a resizable array that owns its elements, keeps them next to each other in memory, and frees them automatically.

**Where to look.** [src/sim/needs.cpp:20](../src/sim/needs.cpp) (`hourlyDrop`: 24 whole-number drops that add up exactly to the daily rate), [src/sim/world.cpp:70](../src/sim/world.cpp) (`checkSurvival`), [assets/data/sim/needs.json](../assets/data/sim/needs.json).

**Try it (15 minutes).** In `needs.json`, change the Hunger rate from 30 to 50 and run `odysseus_sim_tests.exe --test-case="US-011*"`: the tests read the rate from the file, so they still pass, but the death date in the MESSAGE line moves earlier. Why?

**Check yourself.** Why do we store the dead in `people()` too, instead of removing them from the vector?

## US-012: Let people choose what to do (utility AI) (2026-09-30)

**What we built.** The clan lives by itself now. Every game hour each person looks at their needs, their traits and skills, the time of day and the food store, gives every possible action a score, and does the best one: gather, hunt, sleep, warm up by the fire, talk, rest or wander. In the evening they eat together. `odysseus_headless --inspect Garu` shows exactly why Garu did what he did.

**The idea: functions as systems, enums, and choosing the best.** An `enum class` names a fixed set of choices so the compiler catches typos:

```cpp
enum class Action { Gather, Hunt, Sleep, WarmByFire, Talk, Rest, Wander, Count };
```

`Count` is a trick: it equals the number of actions, so `std::array<int, kActionCount>` holds one score per action. The AI itself is a plain function, `decide(person, situation, available, config, random)`: it reads, it scores, it returns a `Decision`. It changes nothing, so a test can call it with any made-up person. Picking the winner is a simple loop keeping the highest score; ties go to the seeded random stream, so the same world always makes the same choices.

**Where to look.** [src/sim/ai.cpp:70](../src/sim/ai.cpp) (`decide`), `scoreActions` just above it, [assets/data/sim/actions.json](../assets/data/sim/actions.json).

**Try it (15 minutes).** Run `odysseus_headless.exe --days 3 --inspect 0` and read the scores. Then raise `traitBonus` in `actions.json` to 100 and run again: do Brave people hunt more? Try `--days 30` too.

**Check yourself.** Why is it important that `decide` only reads the person and never changes them?

## US-013: Remember events and spread gossip (2026-09-30)

**What we built.** People remember. A gift, a theft someone saw: each becomes a memory of who did what, when, and how it felt, and it changes what they think of each other. When two people talk, one may pass on a story the other has not heard, a little weaker, so reputations travel through the clan. Small things are forgotten after 60 days; a theft is remembered for life.

**The idea: containers of structs, and references.** Each person keeps a `std::vector<Memory>`. Forgetting uses a classic pair, erase and remove_if: `remove_if` moves the memories we keep to the front and returns where the rest begins; `erase` cuts them off:

```cpp
memories.erase(std::remove_if(memories.begin(), memories.end(), isOldAndMinor), memories.end());
```

In `talk`, `Person& from = people_[speaker];` is a reference: another name for the same person inside the vector, not a copy. Changing `from` changes the real person. (A copy would change nothing that lasts.)

**Where to look.** [src/sim/world.cpp:274](../src/sim/world.cpp) (`talk`: gossip), [src/sim/memory.cpp](../src/sim/memory.cpp) (`forgetOldMemories`, `remember`), [assets/data/sim/social.json](../assets/data/sim/social.json).

**Try it (15 minutes).** Set `gossipPercent` to 100 in `social.json` and run `odysseus_sim_tests.exe --test-case="US-013 Gifts*"`: how many memories are heard second-hand now? Then change `minorMemoryDays` to 7.

**Check yourself.** In `talk`, what would go wrong if we wrote `Person from = people_[speaker];` (without the `&`)?

## US-014: Write a readable chronicle (2026-09-30)

**What we built.** The clan's story writes itself: couples form, children are born and named (sometimes after a parent, "Joro the Second"), the old die, mammoths are brought down, feuds break out, and hard winters empty the food store. Every event goes into the chronicle with its importance; `odysseus_headless --days 2800 --chronicle` prints a century of the ones worth telling.

**The idea: std::string and formatting.** Each sentence is built with `std::format`, which fills the `{}` gaps in order:

```cpp
std::format("{} was born to {} and {}.", child.name, father.name, mother.name)
```

`std::string` owns its text and grows as needed, so we can join pieces with `+` without worrying about memory: `describe(entry.date) + ": " + entry.text` gives "Spring, year 3: Ura was born to Tok and Maa."

**Where to look.** [src/sim/chronicle.cpp:21](../src/sim/chronicle.cpp) (`formatEntry`), `World::giveBirth` and `World::die` in [src/sim/world.cpp](../src/sim/world.cpp), [assets/data/sim/life.json](../assets/data/sim/life.json).

**Try it (15 minutes).** Run `odysseus_headless.exe --seed 7 --days 2800 --chronicle > seed7.txt` and read it like a book: who is the clan's hero? Then try `--threshold 80` for only the biggest events.

**Check yourself.** Why do newborns wait in a separate vector until the loop over `people_` has finished?

## US-015: Soak-test the simulation from the command line (2026-09-30)

**What we built.** `odysseus_headless --seed 7 --years 100` simulates a whole century in half a second and reports how the clan fared: who is alive, what people died of, how hungry and cold the living are, and how long each tick took. Give it nonsense like `--years -5` and it explains how to use it and exits with an error code.

**The idea: main(), command-line arguments, and checking input.** Every C++ program starts in `main(int argc, char* argv[])`: `argc` is how many words were typed, `argv` the words themselves (argv[0] is the program). Words are text, so numbers must be read carefully. `std::from_chars` reads a number and tells us where it stopped, so "12x" is caught:

```cpp
const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
if (error != std::errc() || end != text.data() + text.size() || value < minimum || value > maximum) {
    return std::nullopt; // not a whole number in range
}
```

`std::optional` says "a value, or nothing"; the caller must check before using it. The program returns 2 on a wrong command line: scripts and CI can see the failure.

**Where to look.** [apps/headless/main.cpp:52](../apps/headless/main.cpp) (`readNumber`, then `parse`), [src/sim/report.cpp](../src/sim/report.cpp).

**Try it (15 minutes).** Run `odysseus_headless.exe --seed 3 --years 200` and compare with `--years 100`. Then try `--years 10001` and read the message. Finally run `echo $LASTEXITCODE` in PowerShell after a bad command.

**Check yourself.** Why does the program measure time with a clock, when the Charter forbids wall-clock time in the simulation?

## US-016: Save and load the simulation (2026-09-30)

**What we built.** The world can be saved and loaded exactly: after 50 years, save, load, and the world hash is the same; run 50 more years and you get the very world that 100 years in one go would have made. A crash while saving never destroys the last good save, and old save files are upgraded.

**The idea: file I/O, JSON and error handling.** Writing a file is not instant; if the power goes out halfway, the file is garbage. So we write to `clan.json.tmp` first and only then rename it: a rename happens in one step, so there is always one complete save.

```cpp
out << WorldArchive::toJson(world).dump(1);   // 1. write everything to clan.json.tmp
fs::rename(file, backupPath(file, 1));        // 2. the old save becomes clan.json.bak1
fs::rename(temporary, file);                  // 3. the new one takes its name, in one step
```

Loading can go wrong in many ways (a missing file, broken JSON, a field of the wrong type). Each becomes a `DataError` naming the file and the problem, and `loadWorld` catches it and tries the next backup. `try`/`catch` lets us deal with a problem where we can do something sensible about it.

**Where to look.** [src/sim/save.cpp:310](../src/sim/save.cpp) (`saveWorld`), `loadWorld` just below it, `upgradeFrom1` near the top.

**Try it (15 minutes).** Run `odysseus_headless.exe --years 10 --save clan.json`, open `clan.json` in a text editor and find your clan's names. Change one person's `"partner"` to 999 and try `--load clan.json`: read the message, and see which file was loaded instead.

**Check yourself.** Why do we rename `clan.json.tmp` at the end instead of writing straight into `clan.json`?

## US-110: Give every death and feud a reason (2026-09-30)

**What we built.** Every event in the chronicle now has a number (its id), a kind, the people it is about, and the numbers of the earlier events that caused it. A death by hunger says which empty store it came from, and which thief helped empty it; a feud says what turned the two people against each other. You can ask the runner why: `odysseus_headless --seed 7 --years 100 --why 7279`.

**The idea: small structs that point to other data, and enums.** Instead of copying a whole earlier event into a new one, we store its *number*. That is like a page reference in a book: cheap, and it always leads to the one true original.

```cpp
struct ChronicleEntry {
    int id;                    // its place in the log
    EventKind kind;            // an enum: Birth, Death, Feud, Theft ...
    int who, other, aux;       // people, by number (-1 = nobody)
    std::vector<int> causes;   // numbers of earlier events
    ...
};
```

An `enum class` is a list of named choices (`EventKind::Theft`) that the compiler checks, so you cannot mix up a kind with a plain number by accident. Because a cause always has a *smaller* id than its effect (it happened earlier), the chain of causes can never loop, and `explainEvent` can walk it safely.

**Where to look.** [src/sim/chronicle.h](../src/sim/chronicle.h) (`ChronicleEntry`, `EventKind`), `World::die` and `World::updateFeuds` in [src/sim/world.cpp](../src/sim/world.cpp), `explainEvent` in [src/sim/chronicle.cpp](../src/sim/chronicle.cpp).

**Try it (15 minutes).** Run `odysseus_headless.exe --seed 7 --years 100 --chronicle`, pick a "died of hunger" line, note its `[#id]`, and run the same command with `--why <id>` instead of `--chronicle`. Follow the "because" lines. Then change `leanAutumnPercent` in `assets/data/sim/story.json` to 0 and see how many hunger deaths remain.

**Check yourself.** Why do we store `-1` for "nobody" in `who` and `other`, and what would go wrong if we used `0`?

## US-111: Quarrel, blame and take revenge (2026-09-30)

**What we built.** People now quarrel, grieve and blame, and feuds can end in a fight or an exile. A hungry, short-tempered pair who dislike each other may quarrel ("Tok and Lin quarrelled over stolen meat while hungry."); when someone's partner starves after a theft they knew of, they blame the thief for life; and a feud that keeps worsening ends with an attack, or the clan driving the aggressor out.

**The idea: state machines with enums, and `std::optional`-style "maybe" values.** A person is always in exactly one health state: `Well`, `Sick` or `Injured`. An `enum class` makes that a closed list, and the daily rule `updateHealth` is a tiny state machine: an injured person either dies (a small chance each day), or counts their days down to zero and becomes `Well` again.

```cpp
enum class Health { Well, Sick, Injured };
...
if (person.health == Health::Injured && chance(...)) { die(...); }
else if (--person.healthDays <= 0) { person.health = Health::Well; }
```

Some questions have "no answer": *who is the heaviest grudge against this person?* might be nobody. We return a pointer that can be `nullptr`, and every caller checks it. `std::optional<T>` does the same job with a type that forces the check; the project uses `nullptr` where the value already lives inside a container (a grudge in a list) and `optional` for a number that may be absent.

**Where to look.** `World::quarrel`, `World::takeRevenge` and `World::updateHealth` in [src/sim/world_story.cpp](../src/sim/world_story.cpp); the numbers in [assets/data/sim/story.json](../assets/data/sim/story.json).

**Try it (15 minutes).** Run `odysseus_headless.exe --seed 7 --years 30 --chronicle --threshold 30` and follow one feud from its first quarrel to its ending. Then set `"percentPerDay"` under `"revenge"` to 0 and run again: what happens to the feuds?

**Check yourself.** Why does a fight's winner get *more* opinion of the victim after taking revenge (`satisfaction`), and what would happen to the feud if it did not?

## US-112: Share food and nurse the sick (2026-09-30)

**What we built.** People fall sick (more often when hungry or cold) and get hurt hunting; the kind and the close nurse them, and the patient remembers it for life. In a famine the better fed spare food for the hungriest, and orphans are taken in. Each of these says why in the chronicle.

**The idea: standard algorithms.** Choosing "the best carer" or "the hungriest person first" is a small search or sort, and the standard library already has them: `std::sort` orders a list, `std::find_if` finds the first match, `std::count_if` counts, `std::min_element` picks the smallest.

```cpp
std::sort(hungry.begin(), hungry.end(), [this](int a, int b) {
    const int ha = people_[a].needs[Need::Hunger];
    const int hb = people_[b].needs[Need::Hunger];
    return ha != hb ? ha < hb : a < b;   // hungriest first; a tie goes to the lower id
});
```

The little function in `[...]` is a *lambda*: a comparison written on the spot. The tie-break (`a < b`) matters here: without it two equally hungry people could swap places from one run to the next, and the same seed would no longer give the same history (Charter rule 6).

**Where to look.** `World::assignCarers`, `World::shareFood` and `World::adoptOrphans` in [src/sim/world_care.cpp](../src/sim/world_care.cpp).

**Try it (15 minutes).** Run `odysseus_headless.exe --seed 7 --years 100 --chronicle --threshold 30` and find a "fell sick" line followed by "nursed ... back to health". Then set `"percent"` under `"nursing"` in `assets/data/sim/story.json` to 0 and compare how many people died of sickness (the report line "sickness N").

**Check yourself.** In `assignCarers` we only replace the best score when the new one is *strictly greater* (`>`). Which carer wins when two people have the same score, and why does that keep the run repeatable?

## US-113: Court and compete for a partner (2026-09-30)

**What we built.** Pairing is now a little story. Someone who likes another begins courting; every day the loved one warms a bit; a loved one who dislikes the suitor turns them down; when both think enough of each other the loved one chooses, and the other suitors grow jealous and may quarrel. Partners who fall out of love part, and the chronicle says why.

**The idea: comparators and ranking (std::sort with a lambda).** "Who does she choose?" and "what is the reason?" are both rankings. When the parting code lists the grudges two partners hold, it sorts them heaviest first, and equal weights go to the earlier event, so the answer never depends on luck or memory layout:

```cpp
std::sort(reasons.begin(), reasons.end(), [](const Grudge& x, const Grudge& y) {
    return x.weight != y.weight ? x.weight > y.weight : x.event < y.event;
});
```

The `[](...) {...}` is a lambda, a comparison written on the spot. It must say which of two items goes first, and a tie needs its own rule, or two runs of the same seed could order equals differently.

**Where to look.** `World::courtship`, `World::pair`, `World::makeJealous` and `World::part` in [src/sim/world_love.cpp](../src/sim/world_love.cpp).

**Try it (15 minutes).** Run `odysseus_headless.exe --seed 7 --years 30 --chronicle --threshold 30` and follow one "began courting" line to the "became partners" line it caused (`--why <id>` shows the chain). Then set `"opinionPerDay"` under `"courtship"` in `assets/data/sim/story.json` to 0 and watch how many courtships end in a pairing.

**Check yourself.** Why does `pair()` clear every other suitor of both partners, even those who courted only a day and get no Jealousy event?

## US-114: Teach the young and hunt together (2026-09-30)

**What we built.** Skilled adults take youths as apprentices, who learn faster and grow close to them. When a mammoth is sighted, a party of three to five hunters goes out, with a leader, sometimes a hero and sometimes a coward. The party's strength decides the hunt, a member in danger may be saved (and owes a debt of gratitude for life), and everyone remembers what the others did.

**The idea: a class that coordinates others.** A hunting party is not a thing that lives on its own; it is a short-lived *coordinator*. `World::huntMammoth` gathers the members (as pointers to people who already exist), works out the roles, decides the outcome, and hands the consequences back to the people and the chronicle. The people stay plain data; the coordinator holds the rules of how they act together:

```cpp
std::vector<Person*> party{&sighter};      // borrowed, never owned
// ... more hunters join ...
const int event = chronicle_.record(..., "A hunting party of ...");
remembered(heroAt, MemoryKind::Heroism, ...);   // every member remembers
```

A pointer here means "look at this person, do not copy them": changing `*party[i]` changes the real person in `people_`. That is safe only because the list of people does not grow while the party is at work.

**Where to look.** `World::huntMammoth` and `World::teaching` in [src/sim/world_hunt.cpp](../src/sim/world_hunt.cpp).

**Try it (15 minutes).** Run `odysseus_headless.exe --seed 7 --years 30 --chronicle --threshold 30` and follow one "hunting party" line to the "stood firm" and "saved ... from the mammoth" lines it caused (`--why <id>`). Then set `"dangerPercent"` under `"hunt"` in `assets/data/sim/story.json` to 100 and see how the deaths change.

**Check yourself.** Why does the party keep `Person*` pointers instead of copies of the people, and what would go wrong if we copied them?

## US-115: Tell the clan's story in episodes (2026-09-30)

**What we built.** The chronicle holds thousands of small events; nobody reads that. Now the runner also finds the *episodes*: groups of linked events (a failed harvest, an empty store, thefts, hunger deaths) and tells each as one short named paragraph with a beginning, a turn and an end. `--story` prints at most 40 a century, then the births, deaths, pairings and feuds with their reasons.

**The idea: designing with data (grouping and summarising).** We never store episodes. We compute them from the events, each time, in three steps: (1) *group* events that belong together (each event points to its causes, and a small "union-find" table merges them), (2) *score* each group and keep the best, (3) *summarise* each group as a name and three sentences. Because the story is a pure function of the data, an old save tells the same story as a new one.

```cpp
for (const int cause : entry.causes) {
    groups.join(entry.id, cause);      // an event and its causes belong together
}
```

Data first, wording last: the groups and the scores are just numbers; only the final step turns them into sentences.

**Where to look.** `findEpisodes` and `formatEpisode` in [src/sim/episodes.cpp](../src/sim/episodes.cpp).

**Try it (15 minutes).** Run `odysseus_headless.exe --seed 7 --years 100 --story` and read three episodes. Then set `"maxPerCentury"` under `"episodes"` in `assets/data/sim/story.json` to 10 and see which episodes survive.

**Check yourself.** Why can `findEpisodes` be called twice on the same world and always give the same answer, and what would break that?

## US-120: Real art in the game (2026-09-30)

**What we built.** Your sprite sheets are pictures for people, with labels and dark backgrounds. A small program, `odysseus_atlas`, cuts each figure and tile out by rectangles listed in `assets/sprites/cuts.json`, clears the background, shrinks them to the game's sizes (32x48 people, 32x32 tiles) and packs them into two atlas pictures. The game draws those.

**The idea: reading binary files; structs of rectangles.** A PNG is bytes, not text: we read it whole into a `std::vector<unsigned char>` and let stb decode it into pixels. Every cut is described by a tiny struct, a rectangle:

```cpp
struct Rect { int x, y, width, height; };
```

Everything else (crop, fit, mirror) is a function from one picture and a rectangle to another picture. Small structs plus pure functions keep the art pipeline easy to test: the tests build a fake sheet in memory and check every pixel.

**Where to look.** `cutAtlas` in [src/game/art.cpp](../src/game/art.cpp); `fitInto` and `removeBackground` in [src/luna/engine/image_ops.cpp](../src/luna/engine/image_ops.cpp).

**Try it (15 minutes).** Run `odysseus_atlas.exe --preview preview.png` from the build folder and open the picture. Then run `odysseus_atlas.exe --find "assets/sprites/Retro RPG Heroes, Terrain & Monsters Sheet.png" 12 40 1515 180 12 45` and see the 20 heroes of that sheet measured.

**Check yourself.** Why does `fitInto` weight each colour by its alpha before averaging, and what would the edges of a figure look like if it did not?

## US-121: Point, click and read on screen (2026-09-30)

**What we built.** The mouse reaches the game as a *pointer* in the game's own pixels, keyboard shortcuts arrive as intents (F2 = Editor, Ctrl+Z = Undo...), and Luna has a small UI toolkit: a pixel font and widgets (buttons, lists, number and text fields, panels).

**The idea: classes with virtual functions.** A panel holds many kinds of widget but treats them all alike. Each widget is a class that *derives* from `Widget` and answers the same questions its own way:

```cpp
class Widget {
public:
    virtual ~Widget() = default;
    virtual bool handle(const UiInput& input) { return false; }
    virtual void draw(UiPainter& painter) const = 0;   // "= 0": every widget must say how
};
class Button final : public Widget { ... bool handle(...) override; void draw(...) const override; };
```

The panel keeps `std::unique_ptr<Widget>` and calls `child->draw(painter)`; C++ picks `Button::draw` or `ListBox::draw` at run time. The `virtual ~Widget()` matters: deleting a Button through a `Widget` pointer must run the Button's destructor too.

**Where to look.** [src/luna/engine/ui.h](../src/luna/engine/ui.h) and [ui.cpp](../src/luna/engine/ui.cpp).

**Try it (15 minutes).** Open [ui-showcase.png](evidence/US-121/ui-showcase.png), then change the `Gold` colour in `colorOf` in ui.cpp, rebuild, run `luna_tests.exe -tc="US-121 Showcase"` and look at the picture it names.

**Check yourself.** Why does a `Button` run its action when the mouse button is *let go* over it, and not when it is pressed?

## US-122: Levels as data (2026-09-30)

**What we built.** The demo world used to be built in code. Now it lives in `assets/levels/valley.json`: the ground, the hero's start, the straw targets and the goblin. The game reads any level file, and saves them safely.

**The idea: reading and writing JSON with validation.** A file comes from outside the program, so nothing in it is trusted. Every field is checked before use, and every error says *which file* and *which field*:

```cpp
if (definitions.character(placed.kind) == nullptr) {
    throw DataError(file, "characters[0].kind", "\"dragon\" is not a character kind in characters.json");
}
```

Saving is the mirror image, with one more rule: never leave a half-written file. We write `valley.json.tmp` first and rename it at the end; the old file becomes `valley.json.bak1`. If a crash damages a save, `loadLevel` quietly uses the last good backup and says so.

**Where to look.** `readLevelFile`, `loadLevel` and `saveLevel` in [src/game/level.cpp](../src/game/level.cpp).

**Try it (15 minutes).** Copy `assets/levels/valley.json`, change `"heroStart"` and the goblin's `"hp"`, and run `odysseus.exe --level <your copy>`. Then misspell a tile name and read the log.

**Check yourself.** Why is the ground saved as runs (`["grass", 30]`) instead of one name per cell?

## US-123: Game mode and Editor mode (2026-09-30)

**What we built.** F2 stops the world and opens the Editor; F1 plays the level again. The game now has two states, and what it does each tick depends on which one it is in.

**The idea: state machines with `enum class`.** A state machine is a value that says "what mode we are in" plus rules for moving between modes:

```cpp
enum class Mode { Game, Editor };

if (intents.pressed(Intent::ModeEditor)) switchMode(Mode::Editor);
if (mode_ == Mode::Editor) { editor_.update(intents); return; } // the world stands still
```

`enum class` (not plain `enum`) keeps the names inside `Mode::` and refuses to mix with numbers, so `mode_ == 1` does not compile. All the work of *changing* state lives in one function, `switchMode`: entering the Editor points its camera where the game looked; leaving it rebuilds the play state from the level.

**Where to look.** `OdysseyGame::switchMode` and `resetPlay` in [src/game/odyssey_game.cpp](../src/game/odyssey_game.cpp); [src/game/editor.cpp](../src/game/editor.cpp).

**Try it (15 minutes).** Run `odysseus.exe`, press F2, pan around with WASD and the right mouse button, then F1. Throw a spear, press F2 and F1 again: the level starts fresh.

**Check yourself.** Why does going back to Game rebuild the whole play state instead of carrying on where the player was?

## US-124: Paint ground tiles (2026-09-30)

**What we built.** In the Editor you paint the ground: a brush, a rectangle, a flood fill and an eraser, with a tile palette, a grid, Undo and Redo, and Ctrl+S to save.

**The idea: the command pattern (undo and redo).** Every change is an object that knows how to do itself *and* how to take itself back:

```cpp
class Command {
public:
    virtual void apply(Level& level) const = 0;
    virtual void undo(Level& level) const = 0;
};
```

A `PaintCommand` just remembers, for each cell, what it was before and what it became. The `History` keeps two stacks: done and undone. Undo moves the top command from one to the other and calls `undo`; Redo moves it back and calls `apply`. A brand-new edit empties the undone stack: you cannot redo a future you have just replaced. The test plays hundreds of random edits, undos and redos and checks the level against snapshots at every step.

**Where to look.** [src/game/editor_history.cpp](../src/game/editor_history.cpp); `Editor::useTool` in [src/game/editor.cpp](../src/game/editor.cpp).

**Try it (15 minutes).** Run `odysseus.exe --editor`, paint a moat of water around the start, press F1 and try to walk out. Press F2, Ctrl+Z a few times, F1 again.

**Check yourself.** Why does `PaintCommand::undo` go through its cells *backwards*?

## US-125: Place characters (2026-09-30)

**What we built.** The Editor places heroes and monsters, selects them, moves, turns and deletes them, and edits their name, HP and sword damage. In Game mode your sword finds them.

**The idea: owning objects in a `std::vector`; ids instead of pointers.** The level owns its characters by value: `std::vector<PlacedCharacter>`. It is tempting to remember "the selected character" as a pointer into that vector, but a vector moves its elements when it grows or shrinks, and Undo replaces the whole list, so an old pointer would point at garbage. Instead each character has an `id` that is never reused, and the editor remembers the id:

```cpp
std::optional<int> selected_;          // an id, or nothing
PlacedCharacter* find(int id);         // look it up when needed, fresh each time
```

`std::optional` says "maybe there is one" without a magic value like -1.

**Where to look.** `Editor::usePlaceOrSelect`, `Editor::find` and `CharactersCommand` in [src/game/editor.cpp](../src/game/editor.cpp) and [editor_history.cpp](../src/game/editor_history.cpp).

**Try it (15 minutes).** `odysseus.exe --editor`: choose Place, then the troll, put it on the path; choose Select, give it 20 HP; F1, Shift for the sword, and defeat it.

**Check yourself.** Why does `CharactersCommand::undo` keep `nextId` at its highest value instead of putting it back?

## US-126: Level and character settings (2026-09-30)

**What we built.** A Level panel in the Editor: name, width, height and default ground; New and Open for other levels, with a "Save the changes first?" question; the hero's start dragged by its marker. And the guide, `docs/guides/editor.md`.

**The idea: resizing a 2D grid stored in one vector.** The ground is one `std::vector<int>`, row after row: cell (x, y) lives at `y * width + x`. Change the width and every index moves, so you cannot just `resize()` the vector: the rows would slide into each other. Instead we build a new grid and copy the part both sizes share:

```cpp
out.ground.assign(newWidth * newHeight, level.defaultGround);   // all default ground
for (int y = 0; y < std::min(level.height, out.height); ++y)
    for (int x = 0; x < std::min(level.width, out.width); ++x)
        out.set(x, y, level.at(x, y));                           // the overlap, cell by cell
```

**Where to look.** `resized` in [src/game/level.cpp](../src/game/level.cpp); `Editor::requestOpen` and `Editor::answer` in [src/game/editor.cpp](../src/game/editor.cpp).

**Try it (15 minutes).** Follow [the Editor guide](guides/editor.md): make a new level, paint a lake, place a troll, move the START marker, save, and press F1.

**Check yourself.** What would go wrong if `resized` called `ground.resize(newWidth * newHeight)` and nothing else?

## US-130: Content catalogs from the new sheets (2026-09-30)

**What we built.** Your seven new sheets are cut into a second atlas (653 items: 150 weapons, 153 plants, 50 animals, 200 effects, 100 weather types), with a catalog for each in `assets/data/`. Numbered review sheets are in `docs/evidence/US-130/`.

**The idea: reading data tables with `std::map` and validating them.** A catalog is a list of small structs read from JSON. Every field is checked as it is read, and an error names the file and the exact field, so a typo in `weapons.json` says where it is instead of crashing later:

```cpp
def.weaponClass = static_cast<WeaponClass>(f.choice("class", kClassNames)); // "sword" -> 0 ... "gun" -> 7
// a wrong value throws: weapons.json: weapons[0].class: must be one of "sword", "axe", ...
```

The atlas keeps a `std::map<std::string, ContentFrame>`: from a name ("iron sword", "spark.2") to its page and cell. A map keeps its keys sorted and finds one in a few steps, which is plenty for a few thousand frames.

**Where to look.** `loadCatalogs` in [src/game/catalogs.cpp](../src/game/catalogs.cpp); `cutContent` and `makeFrame` in [src/game/content_art.cpp](../src/game/content_art.cpp).

**Try it (15 minutes).** Open `docs/evidence/US-130/icons.png` and `icons.md`, pick a weapon you like, find it in `assets/data/weapons.json` and set its `"starter"` to true. Run `odysseus_game_tests -tc="US-130*"`: which check now fails, and why?

**Check yourself.** Why does the catalog loader check that every `frame` exists in the atlas, rather than letting the game find out when it draws?

## US-131: Hero HP, fighting back and death (2026-09-30)

**What we built.** Enemies hit back now. Hit a goblin and a red "!" appears over it: half a second later it strikes, if you are still within 1.5 m. Your HP is shown top left; at 0 the screen fades and you start again at the hero start.

**The idea: a small state machine with `enum class`.** An enemy is always in exactly one state, and each tick decides whether to move to another:

```cpp
enum class Strike { Idle, WindUp };
void Enemy::provoke() { if (isAlive() && state_ == Strike::Idle) { state_ = Strike::WindUp; windUpTicks_ = 10; } }
bool Enemy::update() {           // true on the tick the strike lands
    if (state_ != Strike::WindUp) return false;
    if (--windUpTicks_ > 0) return false;
    state_ = Strike::Idle;
    return true;
}
```

Because `provoke` does nothing while winding up, hitting twice cannot make two strikes. The game, not the enemy, checks the distance when the strike lands: the enemy does not need to know where the hero is.

**Where to look.** [src/game/enemy.cpp](../src/game/enemy.cpp); `OdysseyGame::update` and `hurtHero` in [src/game/odyssey_game.cpp](../src/game/odyssey_game.cpp).

**Try it (15 minutes).** Play the valley, walk to a goblin, hit it once with the sword (Shift, then E), and step away as soon as the "!" appears. Then change the goblin's `swordDamage` in `assets/data/characters.json` to 60 and see how fast you fall.

**Check yourself.** What would happen if `update()` checked `isAlive()` only at the end, after the countdown?

## US-132: Effect player (2026-09-30)

**What we built.** Effects from your VFX sheets now play in the game: a spark where the sword lands, a smoke puff when a monster falls, puffs of dust behind a flying spear. Luna has a small effect player; the game says which effect plays where.

**The idea: timers and animation frames in a fixed timestep.** The game ticks 20 times a second. An effect only counts its age in ticks; which frame to show is worked out from the age when drawing:

```cpp
const int step = effect.age / ticksPerFrame;                 // 3 ticks per frame: 0,0,0,1,1,1,...
const Rect& frame = frames[loop ? step % count : std::min(step, count - 1)];
```

Nothing depends on how fast the computer draws: at 30 or 144 frames per second, the same tick shows the same frame. A one-shot effect removes itself once `age >= frames * ticksPerFrame`.

**Where to look.** [src/luna/engine/effects.cpp](../src/luna/engine/effects.cpp); `playEffect` in [src/game/odyssey_game.cpp](../src/game/odyssey_game.cpp).

**Try it (15 minutes).** In `odyssey_game.cpp` change the hit effect `"spark"` to `"fire nova"` (a name from `assets/data/effects.json`), build, and hit a goblin. Then set its `ticksPerFrame` to 10 in effects.json and watch it slow down.

**Check yourself.** Why does the effect player remove finished effects in `update()` and not in `draw()`?

## US-133: Weapon classes and the starter set (2026-09-30)

**What we built.** Eight kinds of weapon now fight differently: swords, axes, spears and whips hit what is in front of you; bows, thrown weapons, staffs and guns launch projectiles. Damage, speed and range come from `weapons.json`, and the 16 starters are a numbered contact sheet you can review and swap by editing the file.

**The idea: virtual functions.** One base class says what every weapon can do; each kind fills it in its own way, and the game only talks to the base:

```cpp
class WeaponBehaviour {
public:
    virtual ~WeaponBehaviour() = default;
    virtual bool melee() const = 0;
    virtual std::optional<Projectile> launch(const WeaponDef&, double x, double y, Facing) const;
};
```

The game calls `swing` or `launch` on the base, and C++ picks the right version at run time. A new class is a new subclass, not a new `if` in the game loop. `std::variant` would also work; virtual functions read more simply here.

**Where to look.** [src/game/weapons.h](../src/game/weapons.h), [src/game/weapons.cpp](../src/game/weapons.cpp).

**Try it (15 minutes).** Press Shift to cycle the starters and attack a goblin with each. Then set `"starter"` on another weapon in `assets/data/weapons.json` and rerun.

**Check yourself.** Why does `WeaponBehaviour` need a virtual destructor?

## US-134: Pickups and the hotbar (2026-09-30)

**What we built.** You can now place weapons in a level with the editor's new Weapon tool. In the game the hero starts empty-handed, walks over a weapon to put it in the first free box of a nine-box hotbar, presses 1-9 to hold one, and Shift to go to the next filled box.

**The idea: upgrading a saved file format.** Your levels live in files, and a file written today must still open after the game grows. So every level file says which version it is (`"levelVersion": 2`). Version 2 only *adds* a list, `pickups`. The reader follows two rules:

```cpp
if (version > kLevelVersion) throw ...;        // made by a newer game: refuse, never guess
if (data.contains("pickups")) { ... }           // a version 1 file simply has none
```

Old files load as they are, with no pickups. The next time you save, the game writes version 2. Nobody has to convert anything. The one thing we never do is open a file from a *newer* game: we stop and say so, so its extra data is never silently thrown away. Adding fields is easy; renaming or removing them would need a real conversion step.

**Where to look.** `readLevelFile` and `saveLevel` in [src/game/level.cpp](../src/game/level.cpp); `OdysseyGame::collectPickups` and `cycleSlot` in [src/game/odyssey_game.cpp](../src/game/odyssey_game.cpp).

**Try it (15 minutes).** Open the editor (F2), choose Weapon, place three weapons by the START marker and save. Open `valley.json` in a text editor and find the new `pickups` list. Then change `"levelVersion": 2` to `9` and start the game: read the error.

**Check yourself.** Why does a pickup list that is missing entirely count as "no pickups" and not as an error?
