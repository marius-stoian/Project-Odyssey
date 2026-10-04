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


## US-135: Elements (2026-09-30)

**The idea: components, small structs attached to a character.** A goblin used to be "a position and some HP". Now a goblin can also be burning, poisoned or slowed. We did not add three loose variables to `Enemy`; we made one small struct, `StatusEffects`, and gave every enemy one:

```cpp
struct StatusEffects {
    Drip burn;              // HP per second, ticks left
    Drip poison;
    double slowFactor = 1.0;
    int slowTicksLeft = 0;
    void apply(Element, const ElementDef&);
    int tick();             // counts down, returns the whole HP lost this tick
};
class Enemy { ... StatusEffects status; ... };
```

The struct knows nothing about goblins, swords or drawing; it only counts down. The game asks it "how much HP did this cost now?" once per tick and does the rest. That split (data that belongs to a thing, small logic next to the data, the game doing the big decisions) is how big games keep hundreds of effects manageable, and it is the idea behind the "components" of the EnTT library we will meet in M3.

One detail worth understanding: 2 HP per second is 0.1 HP per tick, and HP is a whole number. `Drip::carry` keeps the part of an HP not yet lost (0.1, 0.2, ...), and when it reaches 1 the goblin loses a whole HP. Over 3 seconds that is exactly 6.

**Where to look.** [src/game/status.cpp](../src/game/status.cpp) (`dripTick`), `OdysseyGame::applyElement` and `tickStatus` in [src/game/odyssey_game.cpp](../src/game/odyssey_game.cpp); the numbers in the `elements` part of [assets/data/weapons.json](../assets/data/weapons.json).

**Try it (10 minutes).** In weapons.json change fire to `"perSecond": 10` and `"seconds": 1`, build again (the build copies the assets next to the game), then run the game with `--level docs/evidence/US-135/levels/fire.json` and hold Interact. How much HP does the goblin lose? Then set `"slowTo": 0.25` for ice and watch the warning last longer.

**Check yourself.** Why does a second fire hit restart the timer instead of adding a second burn? (Hint: D-24, and what would 5 quick hits do to a stacking burn.)


## US-139: Mouse aiming (2026-10-01)

**The idea: coordinate spaces and angles.** The mouse lives on the screen (480x270 picture pixels); the goblins live in the world (2048 x 2048 pixels); the camera is the window between them. To aim, we turn a screen point into a world point by adding the camera's corner, then turn the difference from the hero into a direction:

```cpp
const Rect view = camera_.view();
aimTargetX_ = view.x + pointer.x;            // screen -> world
const double dx = aimTargetX_ - hero_.feetX();
const double length = std::hypot(dx, dy);
aimDx_ = dx / length;                        // a unit vector: length 1, only the direction
```

A *unit vector* is the tidy way to say "which way" without saying "how far". The hero sprite only has 8 directions, so `facingToward` turns the angle (`std::atan2(dy, dx)`, which gives the angle of any direction) into the nearest of 8 sectors of 45 degrees. But the sword arc uses the exact vector, so you can hit a goblin the sprite does not quite face.

**Where to look.** `OdysseyGame::updateAim` and `drawAim` in [src/game/odyssey_game.cpp](../src/game/odyssey_game.cpp); `facingToward` and `MeleeBehaviour::swingToward` in [src/game/weapons.cpp](../src/game/weapons.cpp).

**Try it (10 minutes).** Run `odysseus.exe --level docs/evidence/US-139/levels/aim.json`, walk with the keys and aim with the mouse: click to swing. Notice the crosshair turns red when the goblin is out of the sword's reach. Then change the `90.0` of the sword in `behaviourOf` to `30.0`, build, and see how much more exactly you must aim.

**Check yourself.** Why does the game keep a unit vector for the aim and a separate `Facing` for drawing the hero?


## US-140: Arc ballistics for shots (2026-10-01)

**The idea: fixed-point numbers versus floating point.** A computer stores most fractions as *floating point* (`double`): fast, but the result of the same sum can differ in the last digits between different machines or compilers. Luna Physics avoids that: its numbers are `Fixed`, a whole number counting 1/4,294,967,296 of a metre, so `Fixed` arithmetic gives the identical answer everywhere, every run. That matters for a game that may replay a recorded game or check a simulation hash, and it is why the tests can say "two runs land on the exact same point" with `==`.

```cpp
luna::physics::Projectile body{hand, {}, mass, dragArea};          // metres, m/s, kg: all Fixed
const auto angle = luna::physics::aimLaunchAngle(body, air, aimPoint, speed);
body.velocity = luna::physics::launchVelocity(hand, aimPoint, speed, *angle);
// each tick: flyTick(body, air, tipRadius, obstacles) moves it in 10 small sub-steps
```

The only floating point is at the edges: the mouse (pixels, a `double`) is rounded to 1/1024 m when it enters the physics, and the position is turned back into pixels only to draw. The arrow's *height* is physics (z); the screen shows it by lifting the sprite, with a shadow left on the ground so your eye can judge the distance.

**Where to look.** `launchArcShot` and `stepArcShots` in [src/game/arc_shots.cpp](../src/game/arc_shots.cpp); the drawing at the end of `OdysseyGame::drawHeld` in [src/game/odyssey_game.cpp](../src/game/odyssey_game.cpp).

**Try it (15 minutes).** Run `odysseus.exe --level docs/evidence/US-140/levels/arc-rock.json`, aim just behind the rock and click, then aim far past it. Watch the shadow. Then change `kSolidHeightMetres` in `arc_shots.h` to `2.0`, build, and see which shots still clear the rock.

**Check yourself.** Why can a bow shot aimed at the ground 4 m away be stopped by a 1 m rock 2 m in front of you, while one aimed 9 m away flies over it?


## US-141: Bows, crossbows, thrown weapons and staff bolts (2026-10-01)

**The idea: data-driven tuning.** The speed of an arrow is a number. We could have written `16.0` inside the C++ code; instead it lives in `assets/data/weapons.json`:

```json
"classes": {
  "bow":    {"launchSpeed": 16},
  "thrown": {"launchSpeed": 10},
  "staff":  {"launchSpeed": 12}
}
```

The game reads it once at start (`loadCatalogs`), checks it (a number from 1 to 100, and a missing class is an error that names the file and the field), and hands it to the shot. To make arrows faster you edit a text file, not the program, and you never risk breaking the code. That is why the damage, range and rate of fire of all 150 weapons are in the same file. The rule of thumb: *if a designer might want to change it, it is data; if changing it could crash the game, it is code.*

Notice that crossbows are in the `bow` class: one set of numbers and one behaviour for both, with the individual weapon still free to have its own damage and range. Classes group behaviour; weapons carry numbers.

**Where to look.** The `classes` part of [assets/data/weapons.json](../assets/data/weapons.json); `loadCatalogs` in [src/game/catalogs.cpp](../src/game/catalogs.cpp); the use in `OdysseyGame::attackWith` in [src/game/odyssey_game.cpp](../src/game/odyssey_game.cpp).

**Try it (10 minutes).** Open `assets/levels/range.json` in the game (`odysseus.exe --level assets/levels/range.json`), walk east along the weapons, and shoot the goblins with keys 1-7 and the mouse. Then set the bow's `launchSpeed` to `8`, build (the build copies the assets), and notice how far the arrows now fall short of a far pointer.

**Check yourself.** Why is a missing `"bow"` entry reported as an error when the game starts, instead of the bow quietly using speed 0?


## Follow-up to US-139: steady orientation, hysteresis (2026-10-01)

**The idea: hysteresis.** A thermostat that switches the heating on at 20.0 degrees and off at 20.0 degrees would click on and off all day as the temperature wobbles around 20. Real ones switch on at 19.5 and off at 20.5: the gap is *hysteresis*. The hero's facing had the same problem: the pointer decides between "east" and "north-east" at exactly 22.5 degrees, so a pointer near that edge (or a hero walking past it, the camera lagging behind him) flipped the sprite back and forth. The fix is the same gap:

```cpp
if (offDegrees <= 22.5 + hysteresisDegrees) return current;   // close enough: keep facing this way
return facingToward(dirX, dirY);                              // clearly somewhere else: turn
```

Two smaller ingredients: a *dead zone* (a pointer within 16 pixels of his chest means nothing, because its direction changes wildly for tiny moves), and measuring from the chest instead of the feet, because the pointer is usually level with the sprite's body. And the order of the tick matters: walking used to set the facing first and the pointer overwrote it, so the "current" facing was never the last one shown; now the facing from before the tick is what we compare with.

**Where to look.** `facingToward` (the second version) in [src/game/weapons.cpp](../src/game/weapons.cpp); `OdysseyGame::updateAim` in [src/game/odyssey_game.cpp](../src/game/odyssey_game.cpp).

**Try it (10 minutes).** In `odyssey_game.cpp` change `kFacingHysteresisDegrees` to `0.0` and `kFacingDeadZonePixels` to `0.0`, build, and walk sideways with the mouse near the hero: watch him flicker. Then run the test `US-139 No flicker walking past the pointer`: it fails. Put the numbers back.

**Check yourself.** Why is the facing compared with the facing *before* this tick's walking, and not the facing after it?



## US-136: Plants (2026-10-01)

**The idea: random numbers from a seeded stream, and spatial queries.** When you cut a plant down, it must grow back at a random place, but the game must still be replayable: the same play must give the same result. The trick is that a computer's "random" numbers are a recipe, not chance. `Pcg32(1, 5)` is a recipe started from the *seed* 1 on *stream* 5; every call to `below(n)` gives the next number of the recipe. Start the game again and the recipe restarts, so the plant grows back in the same places. (In Charter rule 6 every system has its own stream, so adding a new random thing in another system never changes where plants grow.)

```cpp
const int cellX = firstX + plantRng_.below(cellsX);     // a random column inside the camera view
const int cellY = firstY + plantRng_.below(cellsY);
if (!plantSpotFree(cellX, cellY)) continue;              // try another; give up after 64 tries
```

`plantSpotFree` is a *spatial query*: "is anything near this place?" It asks the map (solid?), the other plants (same cell?), the characters, and the hero (distance). Whenever the game needs to know "what is here", it asks such a question instead of keeping a big table of everything.

A second idea hides in the trees: a tree is not a tile, but it must block like one. So the map got a small extra layer, `setObstacle(x, y, height)`: "something this tall stands on this cell". Walking and flat shots ask `isSolid`, which now also answers yes for an obstacle cell. The same question, one more reason to say yes.

**Where to look.** `OdysseyGame::tickPlants`, `plantSpotFree` and `destroyPlant` in [src/game/odyssey_game.cpp](../src/game/odyssey_game.cpp); `setObstacle` in [src/luna/engine/tile_map.cpp](../src/luna/engine/tile_map.cpp).

**Try it (15 minutes).** Run `odysseus.exe --level docs/evidence/US-136/levels/garden.json`, cut down a flower with the sword (click toward it), and watch where it comes back 15 seconds later. Restart the level and cut it again: it comes back in the very same place. Then change the seed `core::Pcg32(1, 5)` in `populatePlants` to `Pcg32(2, 5)`, build, and see it move.

**Check yourself.** Why does the regrow test run the same scenario twice and expect the same position, and what would make it fail?


## US-137: Animals in the Editor (2026-10-01)

**The idea: data-driven behaviour flags.** A wolf and a deer are both in `animals.json`; the only difference the game needs is one word:

```json
{"name":"grey wolf","frame":"grey wolf","hp":80,"enemy":true,"strikeDamage":10,"reach":1.5},
{"name":"deer","frame":"deer","hp":80,"enemy":false,"strikeDamage":0,"reach":1.5}
```

At the start the game reads every line into a `CharacterKindDef` and, for each placed animal, asks `if (kind->enemy)`: an enemy becomes an `Enemy` (it can be hit, strikes back, dies); anything else is a bystander that is simply drawn. No `if (name == "wolf")` anywhere. To make the deer dangerous you change `false` to `true` in a text file. A *flag* in the data turns a whole behaviour on or off, so the code stays short and the designer stays in control. The same trick made the 50 animals cost almost no new game logic: they are just more character kinds.

**Where to look.** The animals part of `loadDefinitions` in [src/game/level.cpp](../src/game/level.cpp); `OdysseyGame::populate` (where `kind->enemy` decides) in [src/game/odyssey_game.cpp](../src/game/odyssey_game.cpp); `drawAnimal` in [src/game/animals.cpp](../src/game/animals.cpp).

**Try it (10 minutes).** Open `docs/evidence/US-137/levels/animals.json` with `odysseus.exe --level`, hit the deer: nothing. Edit `assets/data/animals.json`, set the deer's `enemy` to `true` and `strikeDamage` to `12`, build, and hit it again.

**Check yourself.** Why is it better that the wolf's danger is a `true` in a file than an `if` about wolves in the code?


## US-138: Placed effects and random weather (2026-10-01)

**The idea: blending two layers with alpha.** When rain fades into sunshine, the screen shows *both* for three seconds, the rain a little less each moment and the new weather a little more. The renderer blends each layer with an *alpha*, a number from 0 (invisible) to 255 (solid). Every tick of the fade we work out how far along it is, from 0.0 to 1.0, and draw the old weather with `1 - fade` and the new one with `fade`:

```cpp
layer(weather_.previous(), 1.0 - weather_.fade());   // the old one fades out
layer(weather_.current(),  weather_.fade());          // the new one fades in
```

Two blend *modes* are used: *add* for light things (rain and sparks are added to the picture below, so they glow) and *normal* for fog (laid over the picture, hiding a little of it). The weather itself is another seeded stream like the plants: the same seed brings the same weathers in the same order, which is how the test can check a whole day of weather in a moment.

**Where to look.** `OdysseyGame::drawWeather` in [src/game/odyssey_game.cpp](../src/game/odyssey_game.cpp); `WeatherCycle::update` in [src/game/weather.cpp](../src/game/weather.cpp).

**Try it (10 minutes).** Run `odysseus.exe --level docs/evidence/US-138/levels/ambient.json --weather "steady rain"`, then `--weather "dense fog"`. Change `kStrength` in `drawWeather` from `0.8` to `0.3` and see the rain thin out.

**Check yourself.** Why must the two alphas of the old and the new weather add up to about 1 during the fade?


## M3: US-030, US-032 and US-031, the clan on screen (2026-10-01)

**The idea: mapping data to presentation.** The simulation knows a person only as numbers (needs, an action, an age). The player needs a face. The game keeps these two worlds apart and translates between them in one direction only: *data in, pictures out*. `emoteOf(person)` turns needs into an emote; `lookOf(person)` turns an id into a look; `ClanView::targetOf(...)` turns "hunting" into "a spot east of the camp". None of them changes the person; if the drawing is thrown away the simulation is unharmed, and the same simulation can be drawn in another way (another art style, or a text report, as the headless runner does).

```cpp
Emote emoteOf(const sim::Person& p) {
    if (p.needs[sim::Need::Warmth] <= 20) return Emote::Cold;   // the most urgent first
    if (p.needs[sim::Need::Hunger] <= 20) return Emote::Hungry;
    ...
}
```

A second idea is in the people themselves: a *layer* is just a picture of the same size as all the others, with some pixels see-through. Stack body, outfit, hair and spear, and you have a person; change one layer and you change only that part. A *palette swap* turns one picture into many: the hair drawn once in a marker colour (brown) is recoloured to black, blond, red... by replacing that exact colour. Four skins x 3 hairs x 6 hair colours x 3 outfits x 6 outfit colours x spear or not is over 2,000 different people from about 8 small pictures.

**Where to look.** `recoloured` and `composed` in [src/luna/engine/sprite_layers.cpp](../src/luna/engine/sprite_layers.cpp); `composeLook` and `lookOf` in [src/game/clan_art.cpp](../src/game/clan_art.cpp); `ClanView::update` in [src/game/clan_view.cpp](../src/game/clan_view.cpp).

**Try it (15 minutes).** Run `odysseus.exe --level assets/levels/camp.json --clan-speed 40`: twenty people, each different, moving between the fire, the gathering ground and the hunting ground; hover one. Then add a fourth hair style in `drawHair` (style 3) and a colour to `kHairs`.

**Check yourself.** Why does `ClanView` keep a *previous* and a *current* position for every person?


## M4: the region, rivals and saves (2026-10-01)

**The idea: a world that is a function.** The whole region is 65,536 tiles, yet a saved game stores only a number (the *seed*) and a few changes. How? Every tile is *computed*, not stored: `biomeAt(x, y)` takes the seed and the place and always gives the same answer. Ask for the same tile a million times, on any machine, and the answer never differs. Such a function is called *pure*. Because it is pure, the game can make a piece of land (a *chunk*) only when somebody walks near it, forget it when they leave, and make it again later, identical. Saving then means: the seed, plus a list of what *changed* (this berry bush was picked on day 100).

```cpp
Biome Region::biomeAt(int x, int y) const;                    // pure: seed + place -> land
std::optional<Resource> Region::resourceAt(int x, int y) const;  // pure too
```

The noise that shapes the land uses only whole numbers (a hashed lattice, blended with a smooth step in integers), for the same reason the physics does: whole-number sums never differ between machines, so seed 7 is the same region for everyone, forever. The rival clans use a second idea, *levels of detail*: what is far from you does not need 20 simulation steps a second; one a second is enough to see them grow, shrink and move, and costs one twentieth.

**Where to look.** `Region::generatedBiome` and `noise` in [src/sim/region.cpp](../src/sim/region.cpp); `Rivals::tick` in [src/sim/rivals.cpp](../src/sim/rivals.cpp); `saveRegion` in [src/sim/region_save.cpp](../src/sim/region_save.cpp); `ChunkStreamer::update` in [src/luna/engine/chunk_streamer.cpp](../src/luna/engine/chunk_streamer.cpp).

**Try it (15 minutes).** Run `odysseus.exe --region 1` and `--region 2`; press F12 (Debug build) and click a person. Change `lakeLevel` in `assets/data/sim/region.json` from 140 to 300 and see how much more water the region has.

**Check yourself.** Why can the game make a chunk "whenever somebody walks near it" without ever storing it, and what would break if `biomeAt` used the time of day?

## M5: a whole life in data

A run is a short list of numbers (affinities, skills, inventory) that two yearly choices nudge, multiplied by an *imprint*: the same choice teaches three times as much at 12 as at 26. Keeping the curve in `hero.json` means you tune the feel of youth without recompiling. The screens are rebuilt from the state every tick, so they can never show stale numbers.

**Where to look.** `HeroLife::imprintPercent` and `liveYear` in [src/sim/hero_life.cpp](../src/sim/hero_life.cpp); `RunFlow::build` in [src/game/run_flow.cpp](../src/game/run_flow.cpp).

**Try it (15 minutes).** Change `peakPercent` in `assets/data/hero/hero.json` and start a new game; compare the affinities after the first year.

**Check yourself.** Why is it safer to rebuild a screen's buttons every tick than to update them when something changes?

## M6: asking before recording

A playtest needs numbers, but numbers about people need their yes. The statistics file is written only if the player agreed, lives on their own computer and the game contains no network code at all, so nothing is sent by construction, not by promise. The elder script is data like everything else: the hint timer counts game ticks, never the wall clock, so it behaves the same in tests.

**Where to look.** `Tutorial::tick` and `notify` in [src/game/tutorial.cpp](../src/game/tutorial.cpp); `SessionStats::finish` in [src/game/session_stats.cpp](../src/game/session_stats.cpp); `installCrashHandler` in [src/luna/platform/crash.cpp](../src/luna/platform/crash.cpp).

**Try it (15 minutes).** Change `hintAfterSeconds` in `assets/data/hero/tutorial.json` to 5, start a New Game and wait.

**Check yourself.** Why does the crash code live in the Platform layer while the statistics live in the Game layer?

## M7: a small language inside the game (US-150)

Instead of writing "if the bush is ripe and it is not winter" in C++ for every action, the game now reads those sentences from text files. To do that it needs a **parser**: code that turns the text `need(hunger) * 2 + trait(diligent) * 10` into a tree it can evaluate. The trick is the same idea at every level, called *recursive descent*: one function per level of strength. `orExpression` asks `andExpression` for its parts, which asks `notExpression`, and so on down to `primary`, which reads one number, word or bracket. Because `*` lives deeper than `+`, `1 + 2 * 3` automatically groups as `1 + (2 * 3)`; the order of the functions *is* the precedence table.

The tree nodes are `std::shared_ptr<const Expr>`: shared so that a registry can be copied cheaply, `const` so that nothing can change a loaded rule by accident. The game never crashes on a typo: a bad sum is an error message with a line number, a nonsense comparison is simply false, and the loader skips only the broken file.

**Where to look.** `Parser::orExpression`, `Parser::primary` and `evaluate` in [src/sim/rule_expr.cpp](../src/sim/rule_expr.cpp); `FileParser::run` in [src/sim/interaction.cpp](../src/sim/interaction.cpp); the language in plain words in [docs/guides/interaction-data.md](guides/interaction-data.md).

**Try it (15 minutes).** In `assets/data/interactions/gather.json` change `"range": 1.5` to `"range": 99` and read the error in the log; then change `season != winter` to `season != autumn` and think about what the game would now offer in autumn.

**Check yourself.** Why does `1 or 0 and 0` come out as 1, and which function of the parser decides that?

## M7: asking "what is it?" instead of "which one is it?" (US-151)

The old menu code said "if it is a bush, offer this; if it is moss, offer that". Every new plant needed new code. Now each thing carries **tags**, a short list of words, and an interaction says which tags it needs. Matching is a question about sets: *does the thing's list contain every tag the interaction wants?* In C++ that is a small loop over the wanted tags with `std::find` on the thing's list (see `matchesTarget` in [src/sim/interaction.cpp](../src/sim/interaction.cpp)); C++20 `std::ranges::find` and `std::ranges::all_of` say the same thing in one line, and `std::set` makes the lookup fast when the lists grow. Because the answer depends only on the tags, adding a mango to `plants.json` with the tags `edible` and `plant` is enough for Gather to appear: the code does not know a mango exists.

Tags that were never written are **derived** from the fields that were (a plant that is `edible` and does not `block` is tagged `edible`). That keeps 153 old entries working while the new tags get used, and a list written in the file replaces the derived one. One decision hides in there: a tree that bears fruit is *not* tagged `edible`, because today you chop it rather than gather from it, and a menu must not change just because the code underneath was reorganised.

**Where to look.** `matchesTarget` in [src/sim/interaction.cpp](../src/sim/interaction.cpp); `readTags` in [src/game/tags.cpp](../src/game/tags.cpp); the derived tags in `loadCatalogs` ([src/game/catalogs.cpp](../src/game/catalogs.cpp)); `GameRuleContext::path` in [src/game/game_rules.cpp](../src/game/game_rules.cpp).

**Try it (15 minutes).** Follow the manual checks in [docs/plans/US-151.md](plans/US-151.md): add a mango to `plants.json`, then change its tags and watch the menu change.

**Check yourself.** Why does a plant that bears fruit and blocks walking get the tag `fruit-bearing` and not `edible`, and what would change in the game if it got `edible`?

## M7: changing the data while the game is running (US-156)

Reloading a file sounds simple until you ask what happens if the file is wrong, or if the game is in the middle of using the old data. The safe pattern has three steps: **load into a new structure on the side, check it, swap only if it is good.** `reloadInteractions` builds a whole new `InteractionRegistry` from the folder; if even one mistake turns up, the new one is thrown away and the old one stays, untouched, while the panel lists what to fix. Because the game runs one thing at a time (single-threaded), the swap is one line, `interactions_ = std::move(fresh);`, done at the start of a tick when nothing else is reading it. `std::move` hands the new registry's insides to the old name instead of copying them.

The reason the catalogs are *not* reloaded yet is the other half of the lesson: the play state keeps **raw pointers** into them (`WorldPlant::def`). A swap would leave those pointing at freed memory, which is the classic C++ crash. Data that others point into needs either ids instead of pointers, or a re-pointing step after the swap; see codex issue CI-007.

**Where to look.** `OdysseyGame::reloadInteractions` and `drawInteractionPanel` in [src/game/odyssey_game.cpp](../src/game/odyssey_game.cpp); the key path `Key::F5` to `Intent::Reload` in [src/luna/engine/input.cpp](../src/luna/engine/input.cpp).

**Try it (15 minutes).** The three manual checks in [docs/plans/US-156.md](plans/US-156.md).

**Check yourself.** Why is "keep the old data when the new data has any mistake" safer than "load every good file and skip the bad one" during a reload, even though the game does the second at start?

## M7: swapping the engine of a menu without changing how it drives (US-152)

The right-click menu was 150 lines of "if it is a person, offer these; if it is the fire, offer those", each item with its own little function. The goal was to move all of that into data files while the game feels *exactly* the same, which is the textbook definition of **refactoring**: change the inside, keep the outside. Two habits made it safe. First, a **stable interface**: the menu still ends in "a list of labelled items with a reason when greyed out, and something to run when clicked", so the screen code that draws it did not change at all. Second, **tests first**: before the data was wired, the old behaviour was written down as tests (the same labels, order, reasons and results for a person, the fire and the stone), and the new menu had to pass the same sentences.

The old code did not disappear; it moved. Each item's body became a **built-in action** with a name (`give-berries`, `tend-camp-fire`), and a data file points at it with `do give-berries`. That is a small version of a pattern you will see everywhere: a table from a name to a function (here `runBuiltin` in [src/game/builtin_actions.cpp](../src/game/builtin_actions.cpp)). Later stories replace `do ...` lines with plain effects one at a time, and each replacement is again checked against the same tests.

**Where to look.** `RunFlow::openContext` in [src/game/run_flow.cpp](../src/game/run_flow.cpp) (now short); `subjectAt` in [src/game/game_rules.cpp](../src/game/game_rules.cpp); `runBuiltin` in [src/game/builtin_actions.cpp](../src/game/builtin_actions.cpp); any file in [assets/data/interactions/](../assets/data/interactions/).

**Try it (15 minutes).** Do the three manual checks in [docs/plans/US-152.md](plans/US-152.md); then add `"order": 5` to `eat-berries.json`, press F5 and see it jump to the top of the fire's menu.

**Check yourself.** Why was it important to write the tests for the old menu *before* connecting the new one, and what would you not know if you had written them afterwards from the new code?

## M7: a clock the game can count on (US-153)

A three-second action in a game that runs 20 times a second is just "do something when 60 ticks have passed". The runner keeps one number, the **clock** (how many play ticks have happened), and each running action remembers two numbers: the tick it started and the tick it ends. Progress is `100 * (now - start) / (end - start)`; the job is done when `now >= end`. Nothing reads the computer's real clock, so the same inputs give the same result on every machine, and when a menu is open the clock simply stops, so a job pauses with the world. A **timer** is the same idea for effects that wait (`after 15s ...`): a list of "at tick T, do X", kept in order. When two timers are due on the same tick a counter that goes up by one each time decides who goes first, so the order never depends on chance.

Saving a clock is a trap: tick 5000 means nothing to a game that starts again from tick 0. So the saved file does not say "at tick 5300" but "in 300 ticks", and loading adds that to the new clock. The same thought applies to what is saved: only the things that are *not* in their starting state (a picked plant), because everything else can be rebuilt from the level. `saveWorld` for the clan, `hero.json` for the hero and now `things.json` for the plants and their timers are written the same careful way: first to a temporary file, then renamed, keeping three older copies.

**Where to look.** `ActionRunner::tick`, `start` and `savePending` in [src/sim/action_runner.cpp](../src/sim/action_runner.cpp); `OdysseyGame::tickActions` and `thingsText` in [src/game/odyssey_game.cpp](../src/game/odyssey_game.cpp); `GameEffectHost::setState` in [src/game/builtin_actions.cpp](../src/game/builtin_actions.cpp).

**Try it (15 minutes).** The four manual checks in [docs/plans/US-153.md](plans/US-153.md); then set `"after 15s"` in `gather.json` to `"after 3s"` and see plants come back quickly.

**Check yourself.** Why does the saved file store "in 200 ticks" rather than "at tick 5200", and what would go wrong after loading if it stored the second?

## M7: data-driven object kinds (US-155)

A "fire pit" is not a new kind of C++ class. It is a row of data: a name, a picture name, some **tags** and a few **states**. Everything the game does with plants (place in the Editor, select, move, delete, undo, save in a level, look at, act on) works on that row, so a fire pit gets it all for free as soon as it is in `objects.json`. This is the idea behind "data-driven" design: write the machinery once for *any* thing with tags and states, then add new things by adding rows. The only C++ written for the seven objects is what is truly new: a tiny drawing routine for their programmer art, and two built-in actions, `warm-nearby` and `restore`.

Look at how little the fire pit's behaviour needs: its interaction file says "needs a fire drill, three seconds, set the state to burning, warm everyone within six metres now and twice more later, then go out." The runner you met in US-153 does the timing; the new `World::satisfyPersonNeed` is the one small door through which the game may help a person's need, and it is capped and deterministic like everything in the simulation.

**Where to look.** `loadCatalogs` (the objects part) in [src/game/catalogs.cpp](../src/game/catalogs.cpp); `makeObjectPage` in [src/game/object_art.cpp](../src/game/object_art.cpp); `Editor::plantKinds` and `plantPageSize` in [src/game/editor.cpp](../src/game/editor.cpp); [assets/data/objects.json](../assets/data/objects.json) and [assets/data/interactions/light-fire.json](../assets/data/interactions/light-fire.json).

**Try it (15 minutes).** The three manual checks in [docs/plans/US-155.md](plans/US-155.md); then change `warm-nearby 6 25` to `warm-nearby 12 50` in `light-fire.json`, press F5 and light a fire again.

**Check yourself.** Why does an object without `tags` in the file still get Inspect, and which one line in the code makes that so?

## M7: one system for the player and everyone else (US-154)

Until now the clan lived in two layers: the simulation decided what each person did each hour, and the view moved them to a fixed spot. The new idea is that **a clan member or an animal chooses from the same list of interactions as the hero.** Each interaction file says how much a doer wants it: `need(hunger) * 2 + trait(diligent) * 10`. Once a second an idle doer looks at what is near, evaluates that sum for every interaction it could do, and takes the biggest. That is **utility scoring**: not a script of "if hungry then gather", but a number for each choice, and the highest wins. A hungry person scores high on Gather and walks to the bush; a full one scores below the minimum and does nothing special; a deer scores Graze at 40 and Flee at `(6 - distance) * 40`, so a wolf at 3 m makes the Flee score 120 and the deer runs.

Sharing one system with the player means the same timed runner, the same menu rules and the same data files serve everyone. The pieces that differ are small and live in one place: how a doer walks (the clan view for people, a step per tick for animals), and what a built-in effect means for them (the hero's `gather-berries` gives berries; a clan member's gives a little hunger back). The one random draw per choice comes from a seeded stream, so ties never depend on chance, and the world hash test (two runs, same inputs, thousands of ticks) proves nothing slipped in.

**Where to look.** `NpcLife::think` and `NpcLife::runMind` in [src/game/npc_life.cpp](../src/game/npc_life.cpp); `pickBest` in [src/sim/npc_chooser.cpp](../src/sim/npc_chooser.cpp); the NPC rule context in [src/game/game_rules.cpp](../src/game/game_rules.cpp); [assets/data/interactions/graze.json](../assets/data/interactions/graze.json) and [flee-predator.json](../assets/data/interactions/flee-predator.json).

**Try it (15 minutes).** The manual checks in [docs/plans/US-154.md](plans/US-154.md); then change the Flee score in `flee-predator.json` to `(6 - distance) * 10` and see the deer wait longer before running.

**Check yourself.** Why does `pickBest` draw a random number even when there is no tie, and what would go wrong with the world-hash test if it drew only when there was one?
## M8: reading a small file format line by line (US-160)

A conversation file looks like writing, but the game reads it with a tiny **parser**: it takes the file one line at a time and decides what each line is from its first characters. `#` starts a note, `@` a header, `===` a node, `->` a choice, and anything else must look like `Speaker: words`. Because every line is decided on its own, every mistake can be reported with its line number (`dialogue/elder-fire.dlg:9: unknown node "hunts"`), which is what makes a data file friendly to edit by hand.

Two details are worth learning. First, the end of a choice line holds up to three little groups, `[if ...]`, `[else ...]` and `{effects}`; the parser peels them off from the *right*, matching brackets backwards, so a quote like `"{hero} shared berries"` inside the effects does not confuse it. Second, a **canonical writer** turns the parsed script back into text in one fixed layout. A test reads every shipped file, writes it back and requires exactly the same text, notes included: that is how we know the Editor of M9 can save a script without scrambling what you wrote.

**Where to look.** `Parser::parseLine`, `parseChoice` and `takeSuffix` in [src/sim/dialogue_script.cpp](../src/sim/dialogue_script.cpp); `writeDialogue` in the same file; the example [assets/data/dialogue/elder-fire.dlg](../assets/data/dialogue/elder-fire.dlg).

**Try it (15 minutes).** The manual checks in [docs/plans/US-160.md](plans/US-160.md); then add a fourth node of your own to `elder-fire.dlg` and a choice that leads to it, and press F5.

**Check yourself.** Why must the lines a character says come before the choices of a node, and what would the canonical writer have to do if they could be mixed?

## M8: a conversation is a small state machine (US-161)

When you talk to the elder, the game keeps just two things: **which script** and **which node you are at**. That is a *state machine*: a few named places and rules for moving between them. Everything else you see is worked out fresh every frame by asking `view()`: which lines have a true `[if]`, which choices to show, hide or grey out. Nothing about the screen is stored, so a screen can never be out of date (if your berries ran out a moment ago, choice 2 is already grey).

Two choices are worth learning from. First, the `Conversation` holds a **copy** of the script instead of a pointer to the library's one. The scripts live in a list that F5 replaces; a pointer into that list would dangle (point at something that no longer exists) the moment you reload, which is the classic C++ bug. A copy is slightly bigger, but it cannot break. Second, a choice's effects are not carried out by the conversation itself: it hands them to the same *action runner* the interactions use, which is why `take hero berries 1` in a `.dlg` file and in a `.json` interaction mean exactly the same thing.

**Where to look.** `Conversation::view` and `Conversation::choose` in [src/sim/conversation.cpp](../src/sim/conversation.cpp); `moodWord` in the same file; `selectScript` in [src/sim/dialogue_select.cpp](../src/sim/dialogue_select.cpp); `RunFlow::buildTalk` in [src/game/run_flow.cpp](../src/game/run_flow.cpp).

**Try it (15 minutes).** The manual checks in [docs/plans/US-161.md](plans/US-161.md); then in `elder-fire.dlg` give the thanks node a second choice with `[if opinion(npc, hero) >= 10]` and press F5.

**Check yourself.** Why does `choose` ask `view()` for the list of choices instead of numbering the node's choices itself, and what would pressing the key 2 do if it did the latter while choice 1 was hidden?


## M8: ranking candidates (US-162)

Several scripts may fit the same person, and the game must pick one the same way every time. The rule is a ranking: most specific `@who` first (the person's name, then a role, then their kind), then the higher `@priority`. C++ has a ready-made tool for "which one is biggest under my rule": `std::ranges::max_element(candidates, better)`. You give it the list and a function that says whether one candidate is *worse* than another, and it returns the best. Because you write that function, the same line of code ranks by anything.

The tie is the interesting part. Two scripts can be equally good, and `max_element` just returns the first of them, which would make one script always win. So after finding the best, the code collects everyone who is not worse *and* not better than it (the tied ones) and picks between them with the game's seeded random stream. The detail to notice: the number is drawn **every call**, even when nobody is tied. If the code drew only when there was a tie, the stream would run ahead by a different amount depending on how many scripts happened to fit, and a new script file would quietly change what happens elsewhere. Drawing exactly once keeps each system's stream predictable (Charter rule 6).

**Where to look.** `choose` in [src/sim/dialogue_select.cpp](../src/sim/dialogue_select.cpp); `updateGreetings` in [src/game/bubbles.cpp](../src/game/bubbles.cpp); the shipped greetings in [assets/data/dialogue/](../assets/data/dialogue/).

**Try it (15 minutes).** The manual checks in [docs/plans/US-162.md](plans/US-162.md); then add a fourth greeting for anyone and see, by restarting a few times, that it joins the rotation.

**Check yourself.** Why does `choose` compare with `better(c, best) == false && better(best, c) == false` to find the tied scripts instead of `c == best`, and what would happen if the `@priority` of two scripts differed by one?


## M8: building text from facts (US-163)

Small talk is not written one sentence at a time; it is **assembled**. A template is a sentence with holes, `"I keep thinking about {memory.what}."`, and the generator fills the holes from facts it reads out of the simulation: what the person remembers, what they heard, which need is lowest. The same few templates give many different lines, because the facts differ. This is called *templating*, and most game dialogue that reacts to the world is built this way.

Two ideas are worth taking from the code. First, **the facts are turned into plain phrases once, in one place** (`phraseOf`): a memory of kind `Blame` becomes "Bo blaming Ama", and "you" or "me" replaces a name when the hero or the speaker is meant. Every template can then use the same phrase after "about", so adding a template never means writing new code. Second, **variety is a rule, not luck**: the generator remembers the last lines said (three per person, fifty in all) and avoids a line that is tired out. The test does not hope for variety; it counts, over fifty lines and four seeds, and fails if any line appears three times.

One detail echoes the last story: `say` draws three random numbers *every time*, even when it does not need them, so what the stream gives to the next system never depends on what a person happens to remember.

**Where to look.** `SmallTalk::say` and `phraseOf` in [src/sim/smalltalk.cpp](../src/sim/smalltalk.cpp); the templates in [assets/data/dialogue/smalltalk.json](../assets/data/dialogue/smalltalk.json); the checker in `SmalltalkData::parse`.

**Try it (15 minutes).** The manual checks in [docs/plans/US-163.md](plans/US-163.md); then add three templates to the `hunt` topic, press F5, and ask the elder about the hunt a few times.

**Check yourself.** Why is a template that uses `{gossip.who}` a mistake in the topic `memory`, and what would the generator print if the file checker did not catch it?


## M8: reuse beats invention (US-164)

When you insult someone, the game has to make them *remember* it, and later have their friends hear about it. The tempting plan is a new "conversation memory" system. The better plan was to look at what the simulation already does: people already have **memories** (who did what to whom, with a feeling), already **gossip** (a talk may pass on the strongest memory the listener lacks, at half strength, marked as heard), and already change their **opinion** when they hear something bad. So a `remember` effect does one thing: it builds an ordinary `Memory` and hands it to the simulation's own `remember()` function. Gossip, forgetting and the chronicle work with no new code, and the acceptance test "two days later the friends have heard it at half strength" passes by using the real rules.

Two small things are worth noticing. The feeling decides the *kind* of memory (a good one is a Gift, a bad one a Quarrel) and whether it is kept for life (strength 60 or more): a rule in one place instead of a choice every script author has to make. And **flags** are a tiny `std::map` from a name to a whole number. It is an *ordered* map on purpose: saving walks it in alphabetical order, so the saved file and the hash never depend on the order things were added, which is how determinism (Charter rule 6) is kept even for a thing as small as a note.

**Where to look.** `World::rememberConversation` in [src/sim/world.cpp](../src/sim/world.cpp); `World::talk` (the gossip) just above it; `FlagStore` in [src/sim/flag_store.cpp](../src/sim/flag_store.cpp); the three effects in `GameEffectHost::apply` in [src/game/builtin_actions.cpp](../src/game/builtin_actions.cpp).

**Try it (15 minutes).** The manual checks in [docs/plans/US-164.md](plans/US-164.md); then add `flag trust 1` to the elder's thanks node and a choice that appears only `[if flag(trust)]`.

**Check yourself.** Why does `FlagStore::set` erase a flag when you set it to 0 instead of storing a 0, and what would two saves of the same game look like if it did not?


## M8: showing what the simulation did, without changing it (US-165)

When two clan members quarrel, the *simulation* decides it and changes their opinions. The bubbles over their heads only **show** that. The code keeps that line sharp: the game never decides a quarrel, it *listens*. It has two ears. The first is the chronicle, the log the simulation already writes ("Tok quarrelled with Maa over stolen meat"); the game remembers how many entries it has read and looks only at new ones. The second is a tiny queue, `takeTalks()`, because a simple talk leaves no chronicle entry; the simulation drops a note into the queue, and the game empties it each tick.

This pattern, a one-way flow from the simulation to the screen, is why the world stays deterministic (Charter rule 6): the queue is not saved, not hashed and nothing in the simulation reads it, so showing bubbles can never change what happens. It also explains a small piece of C++: `std::exchange(talks_, {})` hands out the whole queue and leaves an empty one behind in a single step, so no note is ever read twice or lost.

The exchange itself is a little timetable: a list of lines, a counter of ticks left, and an index. Each tick the counter goes down; at zero the last speaker's bubble is removed and the next line starts. That is all "in turn, 3 seconds each" means.

**Where to look.** `Exchanges::update` and `makeExchange` in [src/game/bubbles.cpp](../src/game/bubbles.cpp); `World::takeTalks` in [src/sim/world.h](../src/sim/world.h); `selectPair` in [src/sim/dialogue_select.cpp](../src/sim/dialogue_select.cpp).

**Try it (15 minutes).** The manual checks in [docs/plans/US-165.md](plans/US-165.md); then write a `.dlg` file with `@pair elder person` and `@bark sharing`, so the elder has their own words when they share food.

**Check yourself.** Why does `Exchanges::update` check `seenEntries_ > entries.size()` before reading the chronicle, and when can the chronicle be shorter than the last time it was looked at?


## M8b: how a picture gets from the game to the screen (US-230)

When the game "draws a sprite", it does not touch the screen. It says *draw this part of that picture there*, and a **renderer** turns that into work for the graphics card. Our first renderer (SDL_Renderer) did that with a fixed set of tricks: draw a picture, maybe see-through, maybe adding light. The new one (SDL_GPU) works the way modern games do, with a **pipeline**: the card is told *how* to draw using two small programs called **shaders**. A *vertex shader* places each corner of a rectangle on the screen; a *fragment shader* decides the colour of each pixel inside it. We write them in a language called HLSL, and the Windows SDK's compiler turns them into the card's own language when the game is built.

Two ideas make this fast. **Batching:** instead of asking the card to draw each sprite with its own call (slow), the game writes every rectangle into one list, in the order they were drawn, and the card draws many at once; a new call is only needed when the picture or the blend mode changes. **A virtual screen:** everything is first drawn onto a small picture (480 by 270 pixels), and only at the end is that picture enlarged into the window by a whole number. That is why the pixels stay square and crisp, and why the two renderers, drawing on the same small picture, give the same result to the last pixel. (A test proves it: it draws the same scene with both and counts the differing pixels. The count is zero.)

One detail is worth remembering: a sprite drawn at three quarters of its size has pixels that fall *exactly* between two source pixels, and the two renderers rounded the last bit of a floating-point number differently, so 352 pixels differed. Moving both by a five-hundredth of a pixel made them agree. Floating-point numbers are not exact, and sometimes you have to decide for them.

**Where to look.** `GpuBackend::drawTexture`, `recordScene` and `recordBlit` in [src/luna/platform/gpu_backend.cpp](../src/luna/platform/gpu_backend.cpp); the four shaders in [src/luna/platform/shaders/](../src/luna/platform/shaders/); how CMake compiles them in [CMakeLists.txt](../CMakeLists.txt) (search for `LUNA_DXC`).

**Try it (15 minutes).** The manual checks in [docs/plans/US-230.md](plans/US-230.md); then in `sprite.frag.hlsl` change `texel.rgb * color.rgb` to `texel.rgb * color.rgb * 0.5`, rebuild, run with `--renderer gpu` and with `--renderer sdl`, and see the picture darken only on the GPU (then undo it).

**Check yourself.** Why does the GPU renderer collect all the rectangles of a frame into one list and send them together at the end, instead of drawing each one the moment the game asks for it?


## M8b: where the virtual picture lands (US-231)

The game draws one 960 by 540 picture. A **viewport** is the rectangle where that picture appears in the actual window. It is calculated from the window's drawable pixel size, which may differ from the window's size in desktop coordinates on a high DPI display. Both renderers use the same rectangle, so the choice of renderer cannot move the picture or change the size of the black bars.

With **Whole** scaling, the scale is the largest whole number that fits both dimensions. On a 2560 by 1440 screen, 2 copies of each virtual pixel fit: the picture is 1920 by 1080, centred at (320, 180). Each virtual pixel becomes a clean 2 by 2 block. With **Fill**, the aspect ratio is kept but the scale may be fractional. The same screen gets a 2560 by 1440 picture. A mouse click must use the very same viewport: subtract its top-left corner, then multiply by the ratio of virtual size to viewport size. A click in the black bars is outside the game.

The settings file records the chosen window mode, last windowed size and scaling method. Loading the older file translates its full-screen flag into a mode and fills the new fields with their defaults, so an update does not discard the player's volume and statistics choices.

**Where to look.** `presentationArea` in [src/core/presentation.cpp](../src/core/presentation.cpp); the two renderer backends in `src/luna/platform/`; the pointer mapping in [src/luna/engine/input.cpp](../src/luna/engine/input.cpp); the saved preferences in [src/game/settings.cpp](../src/game/settings.cpp).

**Try it (15 minutes).** Use the Settings screen to switch between Whole and Fill in a 2560 by 1440 window. Compare the bars and click near the picture edge; then restart and confirm that the choice persists.

**Check yourself.** Why would subtracting the black bar but still dividing mouse coordinates by an integer give the wrong answer in Fill mode?

## US-232 Camera zoom and UI scale: two coordinate systems

**What we built.** You can zoom the world (1x or 2x) and size the interface (1x or 2x) independently. The game still thinks in small pixels; a `ScaledRenderer` multiplies every drawing by a whole number on its way to the screen.

**The C++ idea: wrapping an interface (the decorator).** `ScaledRenderer` *is a* `Renderer` and *holds* another `Renderer`. The game code does not know the difference:

```cpp
luna::engine::ScaledRenderer world(output, settings_.cameraZoom); // draws 2x larger
luna::engine::Renderer& renderer = world;                          // the old code keeps calling renderer.draw(...)
```

Each `draw` call is turned into a `drawStyled` call with a destination `scale` times bigger, and passed on. Because the number is whole, every pixel becomes a clean block and the 5x7 font stays crisp.

**Two coordinate systems.** World pixels move with the camera; screen pixels do not. The mouse arrives in screen pixels, so it is divided by the zoom before the game asks "which world spot is this?": `world = view.x + pointer.x / zoom` (`scaledPointer` in `src/luna/engine/scaled_renderer.cpp`).

**Where to look.** `src/luna/engine/scaled_renderer.*`, `OdysseyGame::render` and the top of `OdysseyGame::update` in `src/game/odyssey_game.cpp`, `Camera::setViewSize`.

**Try it (15 minutes).** Press `-` and `+` in the game and scroll the wheel; then open Settings and set UI scale 2x. Add a `std::printf` of `view.x + pointer.x` in `updateAim` and check that it is the same number at both zooms when you point at the same tree.

**Check yourself.** Why must the UI pointer be divided by the UI scale but the world pointer by the camera zoom, instead of one pointer for both?

## US-233 Every screen at the new size: layout from data, not fixed numbers

**What we built.** The New Game, menu, Settings, crafting and dialogue panels now size and place themselves from the interface size instead of a fixed 420 x 250 box.

**The C++ idea: compute, don't hard-code.** Before, the panel was a constant: `Rect panel_{30, 10, 420, 250};`. Now it is calculated each time the screen is built:

```cpp
const int width = std::min(screenArea_.width - 40, kMaxPanelWidth);
panel_ = {(screenArea_.width - width) / 2, top, width, height};
```

`std::min` keeps it from getting too wide, and the height is measured from what was added (`bottom = max(widget bottoms, cursorY_)`). Change the window or UI scale and the same code gives a new, correct answer.

**Where to look.** `RunFlow::build` in `src/game/run_flow.cpp`; the test in `tests/game/menu_test.cpp` ("US-233 Screens fit...").

**Try it (15 minutes).** Change `kMaxPanelWidth` in `run_flow.h` to 400, rebuild, and open the menu: lines wrap earlier. Then set it back.

**Check yourself.** Why is it safer to measure the panel's content than to give every screen its own fixed height?

## US-234 Frame budget: measuring CPU and GPU time

**What we built.** F3 now shows how long the CPU spends on the simulation tick and on drawing, and how long the graphics card needs for the frame. A 10-minute run with 500 people logged the numbers.

**The C++ idea: a timer that stops itself (RAII).** We want the draw time of every frame, even when `render` leaves early. A small struct starts a clock in its constructor and reports in its destructor, which C++ runs automatically when the function ends:

```cpp
struct DrawTimer {
    OdysseyGame& game;
    std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
    ~DrawTimer() { game.recordFrame(/* now - started */ ...); }
} drawTimer{*this};
```

No matter which `return` is taken, the time is recorded. The GPU is harder: the card works while the CPU moves on, so we wait for a *fence* (a flag the card sets when it is done) and time that wait.

**Where to look.** `OdysseyGame::render` and `recordFrame` in `src/game/odyssey_game.cpp`; `present()` in `src/luna/platform/gpu_backend.cpp`.

**Try it (15 minutes).** Run the game with `--perf --people 500`, watch the F3 overlay, then try `--people 20`: which number changes, CPU or GPU?

**Check yourself.** Why did our first GPU numbers read about 16 ms for a frame that really takes 0.2 ms?

## US-240 The lighting pipeline: shader inputs and lighting maths

**What we built.** The graphics card can now light the world: an ambient colour tints every sprite, and point lights brighten the side of a sprite that faces them.

**The C++ idea: data crossing from the CPU to the GPU.** A shader is a tiny program that runs once per pixel on the card. It cannot see our C++ variables; we pack the numbers it needs into a flat array of floats (a *uniform buffer*) and push it before drawing:

```cpp
std::vector<float> data(4 + 8 * 64);       // ambient + count, then 8 floats per light
data[3] = static_cast<float>(count);
SDL_PushGPUFragmentUniformData(commands, 0, data.data(), bytes);
```

The shader declares the same layout (`float4 lights[128]`) and reads it back. The maths per pixel: how close is the light (`reach`) times how squarely the surface faces it (`dot(normal, directionToLight)`).

**Where to look.** `src/luna/platform/shaders/sprite_lit.frag.hlsl`; `pack()` in `src/luna/platform/gpu_backend.cpp`; `src/game/lighting.cpp`.

**Try it (15 minutes).** In `assets/data/light/lights.json` set `ambient.strength` to 0.5 and start the game: the world dims, the HUD does not. Set it back to 1.0.

**Check yourself.** Why does a sprite with no normal map still brighten near a light, and why only a little at the edge of the light's radius?
## US-241 Generated normal maps: height and slopes from pixels

**What we built.** A tool makes a "normal map" for every sprite sheet: a second picture that says which way each pixel of a sprite faces, so lights can shade the art without anyone painting a second image.

**The C++ idea: image processing is loops over pixels.** First a height per pixel (higher in the middle of a body, from the distance to the edge, plus brightness). Then the slope: how fast the height changes to the right and downward, measured with the Sobel filter, a weighted difference of the neighbours:

```cpp
const double dx = (s(x+1,y-1) + 2*s(x+1,y) + s(x+1,y+1)) - (s(x-1,y-1) + 2*s(x-1,y) + s(x-1,y+1));
const double nx = -dx * strength / 4.0;   // the surface leans toward lower ground
```

The direction (nx, ny, 1) is shortened to length 1 and stored in the colour channels: red is x, green is y, blue is z.

**Where to look.** `normalAtlas` in `src/luna/engine/image_ops.cpp`; `writeNormalAtlases` in `src/game/normal_art.cpp`; the tool flag in `apps/atlas/main.cpp`.

**Try it (15 minutes).** Open `assets/sprites/atlas/characters_n.png` in an image viewer: the purple-blue picture is the hero's surface directions. Then change `kBodyStrength` in `normal_art.cpp` to 4.0, run `odysseus_atlas --normals` and look again.

**Check yourself.** Why is the colour of a flat surface (128, 128, 255), a light purple-blue, and not black?
## US-242 Day, night and seasons: interpolating curves over time

**What we built.** The light now follows the game clock: night, orange dawn, bright day, orange dusk. Summer days are long, winter days short.

**The C++ idea: interpolation (lerp).** Between two known moments, the value in between is a straight blend. With `t` from 0 to 1:

```cpp
const auto mix = [t](double x, double y) { return x + (y - x) * t; };
ambientR = mix(a.red / 255.0 * a.strength, b.red / 255.0 * b.strength);
```

`t` is how far the clock has gone from one keyframe to the next: `(now - from) / (to - from)`. The sun uses a sine: `elevation = peak * sin(pi * f)`, where `f` is how far through the day it is; the sine rises, peaks at noon and falls back, which is exactly the shape of a sun's path.

**Where to look.** `skyAt` in `src/game/sky.cpp`; `OdysseyGame::sky()` in `src/game/odyssey_game.cpp`.

**Try it (15 minutes).** Run `odysseus.exe --level assets/levels/camp.json --clan --clan-speed 20` and watch a day pass. Then edit the `Winter` sunset in `calendar.json` to 12.0 and see how early the evening comes.

**Check yourself.** Why is the first keyframe of the day not at hour 0, and how does the code make the last keyframe join it over midnight?
## US-248 Celestial bodies: from a position to a direction with atan2

**What we built.** The sun and the moon are now objects of the world. Where a body is decides where the light comes from, so which way a shadow will fall. The game has a default pair that travels by the clock, and you can place your own in the Editor. Eclipses are lines in a data file.

**The C++ idea: `atan2`.** To know in which compass direction a body lies, we take its offset from the hero, `dx` east and `dy` south in metres, and ask for the angle:

```cpp
const double azimuth = std::atan2(dx, -dy) * kRadiansToDegrees; // 0 = north, 90 = east
const double elevation = std::atan2(height, std::hypot(dx, dy)) * kRadiansToDegrees;
```

`atan2(a, b)` gives the angle of the point (b, a) in all four quadrants. Plain `atan(a / b)` cannot tell north from south, because the division throws the signs away, and it breaks when `b` is 0. The shadow falls the opposite way, so its direction is the negative of the way toward the body: `(-sin(azimuth), cos(azimuth))` on the picture, where north is up and y grows downward.

**Where to look.** `viewOf` and `currentLight` in `src/game/celestial.cpp`; `OdysseyGame::celestialLight()`.

**Try it (15 minutes).** In `objects.json` change the `height` of `sun (placed)` from 40 to 5, place it in the Editor east of the hero and read `elevation` in a test: the sun is now nearly on the horizon and the shadow factor hits its cap of 2.5.

**Check yourself.** Why does the code clamp the elevation to at least 8 degrees before it computes `1 / tan(elevation)`?

## US-244 Sun and moon shadows: projecting silhouettes with a shear transform

**What we built.** Everything that stands in the world now throws a shadow along the light of the sun or the moon, longer when the light is low, fainter in fog.

**The C++ idea: a shear.** A shadow is the sprite's own picture painted black and slid sideways, more the higher the pixel is above the feet. That is a shear: each row `z` is shifted by `direction * z * length`:

```cpp
const int shift = static_cast<int>(std::lround(dirX * middle * groundPerHeight));
renderer.drawStyled(silhouette, stripOfPicture, {feet.x - width / 2 + shift, top, width, rowsPerDraw}, style);
```

We walk down the ground rows instead of up the picture rows so that every ground pixel is drawn once; drawing every picture row on top of the others would stack the translucency into dark bands.

**Where to look.** `drawShadow` in `src/luna/engine/shadow_draw.cpp`; `OdysseyGame::drawShadows` in `src/game/shadows.cpp`.

**Try it (15 minutes).** Change the `height` of the `olive tree` in `plants.json` from 4.0 to 8.0 and take the 09:00 screenshot from `docs/plans/US-244.md`: the shadow doubles in length.

**Check yourself.** Why does `drawShadow` give a shadow that points exactly sideways a minimum depth (`kMinShadowDepth`)?

## US-245 Shadows from fires: choosing the nearest lights per object within a budget

**What we built.** At night a person, an animal or a plant standing near a camp fire throws a faint shadow away from it. Between two fires there are two shadows. Only the nearest few fires count, and the Low lighting preset turns them off.

**The C++ idea: `std::partial_sort`.** For every thing we list the fires that reach it and want only the nearest N. Sorting the whole list wastes work; `partial_sort` puts just the first N in order:

```cpp
const std::size_t count = std::min(reaching.size(), static_cast<std::size_t>(lighting_.shadowLightsPerObject));
std::partial_sort(reaching.begin(), reaching.begin() + static_cast<std::ptrdiff_t>(count), reaching.end(), byDistanceThenId);
```

The comparison breaks ties by the light's id, so two fires that are equally near are always chosen in the same order and the picture does not flicker between frames.

**Where to look.** `OdysseyGame::castShadow` in `src/game/shadows.cpp`; `lightSources` in `src/game/world_lights.cpp`; `fireShadows` in `assets/data/light/lights.json`.

**Try it (15 minutes).** In `lights.json` set `fireShadows.maxPerObject` to 1, take a night screenshot between two fires (`docs/plans/US-245.md`): one of the two shadows is gone.

**Check yourself.** Why does `castShadow` skip a light that is less than 12 pixels from the thing's feet?

## US-246 Weather and light: blending settings

**What we built.** Rain, snow, fog and storms now dim and tint the whole scene, and a storm flashes it with lightning. Each weather in `weather.json` says its `light` (a dim and a tint) and how often it `flash`es.

**The C++ idea: blending (linear interpolation).** While one weather fades into the next we do not jump between two light settings; we walk in a straight line from the old to the new:

```cpp
const auto mix = [t](float a, float b) { return static_cast<float>(a + (b - a) * t); };
light.red = mix(tintOf(from->tintRed, from->lightDim), tintOf(to->tintRed, to->lightDim));
```

`t` is the weather's own fade (0 to 1 over 3 s), so the light and the raindrops arrive together. The lightning uses no random generator at all: a hash of the seed and a 0.2 s slot number decides if a strike starts, so the same seed always strikes at the same moments.

**Where to look.** `weatherLight` and `lightningFlash` in `src/game/weather.cpp`; `ambientLightFrame` in `src/game/world_lights.cpp`; `assets/data/weather.json`.

**Try it (15 minutes).** Give "steady rain" `"light":{"dim":0.3,"tint":[255,255,255]}`, run with `--weather "steady rain"`: the rain is nearly night.

**Check yourself.** Why does the lightning use a hash of the slot and not the simulation's random numbers?

## US-247 Lighting in the Editor and quality settings: a setting that changes a pipeline

**What we built.** The Editor can show the level at any hour (the Sky button and a slider), the owner can place lights with a new Light tool (saved in the level), and the lighting quality Low, Medium, High now changes what is drawn.

**The C++ idea: one flag travels down the layers.** The Low quality has to reach the graphics card code without the Game knowing about the card. We add a plain `bool normalMaps` to the light data the Game hands to the renderer, and the renderer copies it down to the platform layer:

```cpp
lightFrame.normalMaps = settings_.lighting != "Low";   // Game
state.normalMaps = frame->normalMaps;                  // Engine copies it to Platform
const bool useNormals = lightSets_[batch.light].normalMaps; // GPU backend picks the flat normal
```

Each layer only knows its own neighbour, so the rule that Game never touches SDL still holds. The level file gets a version number bump (2 to 3): an old file has no `lights` list and loads as "no lights"; saving writes version 3.

**Where to look.** `Editor::render` and `Editor::handlePanels` in `src/game/editor.cpp`; `OdysseyGame::editorLightFrame` in `src/game/world_lights.cpp`; `gpu_backend.cpp` (`useNormals`); `assets/levels/*.json` (`lights`).

**Try it (15 minutes).** Click Sky in the Editor, drag the slider to midnight, place a campfire with the Light tool: it glows. Then set Lighting to Low in the Settings and look at a sprite near a fire: it is lit flat.

**Check yourself.** Why does a level made by a newer game stop with an error instead of loading what it can?

## US-260 NPC Classes: a catalog keyed by id

**What we built.** The owner can create, edit and delete NPC Classes (trader, healer, guard...) in the Editor. Each class is one JSON file named after its id.

**The C++ idea: a catalog keyed by id, and refusing a delete that would leave a dangling reference.** The loader reads every file into a `std::vector<NpcClass>` sorted by id and offers `find(id)`. A file with a mistake is skipped and reported as `file:line: message`, so one typo never stops the other classes. Deleting asks first who still uses the class:

```cpp
const std::vector<std::string> users = usersOf(id, level);
if (!users.empty()) return std::format("{} is still used by {}", id, names);
```

A reference (a placed NPC naming a class) must never point at nothing, so the refusal names the NPCs.

**Where to look.** `src/sim/npc_class.cpp`, `src/game/npc_class_book.cpp`, `Editor::buildClassPanel` in `src/game/editor.cpp`, `assets/data/npc-classes/`.

**Try it (15 minutes).** Copy `trader.json` to `smith.json`, change `id` and `label`, press F5 in the game: the new class loads. Change the icon to `banana`: the log names file and line.

**Check yourself.** Why does the file name have to equal the `id` inside the file?

## US-261 Kind defaults and overrides: layered defaults

**What we built.** Every kind of NPC has a file of defaults (`assets/data/npcs/goblin.json`), and each placed NPC may change only what it wants. The game combines classes, kind and the NPC itself into one answer.

**The C++ idea: `std::optional` for "not set", and layering.** A layer that says nothing about attitude must not erase the layer below it, so "not set" needs its own value:

```cpp
std::optional<std::string> attitude; // empty: leave the layer below alone
if (placed.attitude) out.attitude = *placed.attitude;
else if (kind != nullptr && kind->attitude) out.attitude = *kind->attitude;
```

Allow and deny lists merge layer by layer in a `std::map<std::string, ActionState>`: the last layer that mentions an action decides it.

**Where to look.** `resolveNpc` in `src/sim/npc_kind.cpp`; `placedLayer` in `src/game/npc_class_book.cpp`; `assets/data/npcs/`.

**Try it (15 minutes).** In `tests/sim/npc_kind_test.cpp` read the precedence cases, then change `wanderer.json` to `wary` and press F5.

**Check yourself.** Why is an empty `std::vector` a fine "not set" for tags but `std::optional` is needed for classes?

## US-262 Placed NPCs are persons: one id from the level file to the save

**What we built.** A placed wanderer is now a real person of the simulation: it ages, its needs move, it remembers the days and meeting the hero, and it is saved with the game. Goblins (monsters) and deer (animals) stay creatures.

**The C++ idea: identity and lifetime.** The person keeps the id of the placed character in the level file. The same number appears in the level, in the store and in the save, so nothing has to be matched by name or by position in a list:

```cpp
int NpcPopulation::add(int id, std::string_view kind, int ageDays, int family, int x, int y) {
    if (const int existing = indexOf(id); existing >= 0) return existing; // adding twice is the same person
```

The store keeps each field in its own `std::vector` (a struct of arrays); the index of a person is only a position, the `id` is who they are.

**Where to look.** `src/sim/npc_population.cpp`, `OdysseyGame::buildNpcPopulation` in `src/game/npc_people.cpp`.

**Try it (15 minutes).** Read `tests/sim/npc_population_test.cpp`, then change `restoreBelow` in `DailyConfig` and see which test notices.

**Check yourself.** Why is the index of a person not safe to save, while the id is?

## US-263 The NPC store and detail by distance: struct of arrays, a grid and why O(n^2) breaks

**What we built.** The world can hold 100,000 NPCs. Only the ones near the hero are simulated hour by hour; the rest are brought up to date once a day, and they come out the same.

**The C++ idea: struct of arrays and a spatial grid.** Instead of a `std::vector<Person>` (an array of structs, each person a block with its own vectors), every field has its own array and a person is just an index:

```cpp
std::vector<std::int32_t> ages_;   // ages_[i] is the age of person i
std::vector<std::int16_t> needs_;  // 4 numbers per person, side by side
```

A loop over one field reads memory in a straight line, which the CPU cache loves. To find who is near, a **grid** sorts persons into 256-pixel cells; a query touches a few cells instead of 100,000 persons. Checking every pair would be 100,000 x 100,000 = 10 billion pairs a day: that is what "O(n squared) breaks" means.

**Where to look.** `NpcPopulation::near`, `hourMark`, `dailyUpdate` in `src/sim/npc_population.cpp`; `docs/adr/ADR-022-npc-store-and-detail-by-distance.md`.

**Try it (15 minutes).** In `tests/sim/npc_scale_test.cpp` change the 100,000 to 1,000,000 and run the Load case in Release: what grows, the day, the save or the load?

**Check yourself.** Why must `near()` sort its result before returning it?

## US-264 Attitudes and opinions: sparse maps keyed by pairs, words from numbers

**What we built.** Every NPC has its own opinion of the hero and of each person it has met, and a word for it (friendly, wary, hostile...). A gift raises the number; crossing a threshold changes the word.

**The C++ idea: a sparse map keyed by a pair, and thresholds.** With 100,000 persons a table of everyone-to-everyone would have 10 billion cells. We keep only the pairs that met, in a hash map whose key packs both ids into one 64-bit number:

```cpp
static std::uint64_t pairKey(int holderId, int targetId) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(holderId)) << 32) | static_cast<std::uint32_t>(targetId);
}
```

Reading an opinion looks the key up and, if there is no entry, computes the default without storing it. The word is derived from the number by walking the bands and keeping the last one whose start is not above the opinion.

**Where to look.** `NpcPopulation::opinion`, `adjust`, `pairFor` in `src/sim/npc_population.cpp`; `attitudeFor` in `src/sim/opinion.cpp`; `assets/data/sim/opinions.json`.

**Try it (15 minutes).** In `opinions.json` set `friendly` to 5 and run the opinion test: which checks notice?

**Check yourself.** Why is the family opinion a default rather than a stored entry?

## US-265 Talk with placed NPCs: one interface for many kinds of things

**What we built.** Any placed person with something to say can be talked to: right-click, Talk, the conversation panel opens and the game waits. A person with no dialogue has no Talk.

**The C++ idea: one interface, data-driven by tags.** The game does not have a special code path for each kind of thing. Everything the hero can act on is a `Subject` with a kind, a position and a list of tags, and the interaction files say which tags they apply to:

```json
"target": { "tags": ["speaks"] }
```

A clan member and a placed trader both carry `speaks`, so the same `talk.json` serves both. To add a new kind of thing we write a function that makes a `Subject` (`npcSubject`) and give it tags; the menu, the range check and the action runner already work.

**Where to look.** `npcSubject` and `subjectAt` in `src/game/game_rules.cpp`; `openConversation` in `src/game/builtin_actions.cpp`; `assets/data/interactions/talk.json`.

**Try it (15 minutes).** Give the `elder` class `"dialogues": { "player": "greet-elder.dlg" }` and talk to a placed elder.

**Check yourself.** Why do placed NPCs not carry the tag `person`?

## US-266 Confront: intents mapped from keys, and effects that spread to bystanders

**What we built.** A separate Confront menu (key C, or an entry in the right-click menu) with five ways to deal with an NPC by words: taunt, insult, ask for peace, antagonise, de-escalate. They change what the target and its friends think of the hero, and can start or stop a fight.

**The C++ idea: intents and spreading effects.** The game never asks "was the C key pressed?". The platform layer turns a key into an *intent*, and the game reacts to the intent:

```cpp
case Key::C: return KeyBinding{Intent::Confront, kKeyboardA};
...
if (intents.pressed(luna::engine::Intent::Confront)) confrontKey(worldIntents.pointer());
```

So a gamepad button or a touch can later be bound to the same intent without touching the game. An effect that spreads (`do spread-opinion -5`) asks the grid for the persons near the target and changes only those who know it:

```cpp
for (const int index : people.near(x, y, range)) if (people.knows(id, placedId)) people.adjust(id, kHero, amount);
```

**Where to look.** `confrontKey` in `src/game/npc_people.cpp`; the `do` built-ins in `src/game/builtin_actions.cpp`; `assets/data/interactions/insult.json`.

**Try it (15 minutes).** Copy `insult.json` to `shout.json`, change the id, label and amounts, press F5 and see it in the Confront menu.

**Check yourself.** Why does the hearing range use the grid instead of looping over every person?

## US-267 Actions and the Actions pop-up: filtering, and conditions as words

**What we built.** An NPC's right-click menu shows only what you can do. A separate Actions pop-up (key X) shows everything the NPC could offer and, for what is locked, what it needs, in plain words.

**The C++ idea: filtering a list, and keeping the reason with the answer.** One function builds the list of offers; the menu and the pop-up are two filters over it. Each offer carries *why* it is not available, so the pop-up can print it:

```cpp
if (placed && mode != MenuMode::All && !offer.enabled && !offer.tooFar) continue; // hidden in the menu
// the pop-up keeps it, with offer.reason as the words
```

Allow and deny lists are applied first, inside `offered()`: a denied id is skipped before anything else is looked at.

**Where to look.** `InteractionRegistry::offered` in `src/sim/interaction.cpp`; `RunFlow::openMenuFor` in `src/game/run_flow.cpp`; `docs/guides/npc-data.md`.

**Try it (15 minutes).** Add `"actions": { "deny": ["talk"] }` to a class file, press F5 and see Talk vanish for that class.

**Check yourself.** Why does an action that is only too far away stay in the menu while one that needs a friendly attitude does not?

## US-268 Editor NPC panel: forms bound to data, saving only the differences

**What we built.** Select a placed NPC in the Editor and a panel lets you set its classes, attitude, family, dialogues and allowed actions. The level file only records what differs from the NPC's classes and kind.

**The C++ idea: a form bound to data, and a normalising setter.** The panel shows what the NPC *is* (resolved from all layers) but each change goes through a setter that stores only the difference:

```cpp
const std::string inherited = kind != nullptr && kind->layer.attitude ? *kind->layer.attitude : std::string("neutral");
placed.attitude = word == inherited ? std::string() : word;
```

Every setter goes through `changeCharacters`, which makes one `Command` that remembers the list before and after: that is why every change is exactly one step of Undo.

**Where to look.** `Editor::buildNpcPanel` and the `setSelected...` functions in `src/game/editor.cpp`; `tests/game/npc_editor_test.cpp`.

**Try it (15 minutes).** Change the attitude of a goblin to friendly and press F1: the menu title shows it. Then edit `goblin.json` and see that a goblin with no override follows the file.

**Check yourself.** Why does choosing the attitude the kind already has remove the field from the level instead of writing it?

## US-269 Editor kinds tab and markers: one form for two data sources, and pictures made on demand

**What we built.** The Class panel got a second tab, Kinds, that edits the defaults of a whole kind (`assets/data/npcs/goblin.json`); and under every placed NPC the Editor now draws a ring in its class colour with the class icon.

**The C++ idea: the same widgets over a different draft.** The class tab edits a `NpcClass` draft, the kind tab edits a `NpcKind` draft, and both fill their text fields through the same two small helpers, so a mistake in the Talk line is handled in one place:

```cpp
if (const auto parsed = parseDialogues(v)) kindDraft_.layer.dialogues = *parsed;
else say("talk is partner=file.dlg, for example player=greet.dlg");
```

The JSON stays stable (same fields, same order every time) because `toJson` writes the fields in a fixed order instead of looping over a map: saving twice gives byte-identical files and a clean diff. The marker pictures are made the first time they are drawn and kept in a `std::map` keyed by icon and colours, so ten goblins cost one texture.

**Where to look.** `Editor::buildKindForm`, `Editor::markerTexture` in `src/game/editor.cpp`; `src/game/npc_marker.cpp` (the 24 icons are 8 strings of 8 characters); `tests/game/npc_kinds_tab_test.cpp`.

**Try it (15 minutes).** Change the icon bitmap of `star` in `npc_marker.cpp` and see it in the ring of an elder.

**Check yourself.** Why does saving a kind change a placed goblin that has no attitude of its own but not one that has?

## US-270 NPC test level: a smoke test that loads every shipped file

**What we built.** `assets/levels/npc-test.json`, a small level with seven NPCs (trader, talker, wary hunter, elder, guard, goblin, deer) and five written dialogues, plus a walk-through checklist in the guide.

**The C++ idea: a smoke test.** A smoke test does not check one clever thing; it loads *everything we ship* and checks that nothing complains. Here: the level loads, the logs report zero errors, every class and dialogue the level names exists, and saving what we loaded gives byte-identical text:

```cpp
game::saveLevel(first, definitions, again);
CHECK(game::loadLevel(again, definitions).level == first);
CHECK(readText(again) == readText(shippedLevel()));
```

The level file itself was written by the game's own `saveLevel` (not by hand), which is why the text round-trips: the same code writes and reads it.

**Where to look.** `tests/game/npc_test_level_test.cpp`; the table in `docs/guides/npc-data.md`.

**Try it (15 minutes).** Misspell a class in `npc-test.json` (`"traderr"`) and run the first test: the message names the NPC and the class.

**Check yourself.** Why is the byte-for-byte round trip a stronger check than loading the file and looking at a few fields?

## US-280 Currencies per region: why money is an integer

**What we built.** A region can say which items are money (`shells=1`), what the market asks for goods and which goods it delivers. The owner edits it in the Editor's Economy panel; the level file keeps it.

**The C++ idea: value types and whole-number money.** `RegionEconomy` is a *value type*: a plain struct you can copy, compare and store with no pointers and no owners to worry about. `friend bool operator==(const RegionEconomy&, const RegionEconomy&) = default;` asks the compiler to write the comparison for us, which is how the test checks that a level written and read back is the same:

```cpp
CHECK(reread.economy == editor.level().economy);
```

Money is an `int` of value units, never a `double`. Computers store 0.1 in binary as a number that is very slightly wrong, and when thousands of trades add up (and when two computers must agree on the same save file, the determinism rule) those tiny errors show. With whole numbers, `3 + 4` is always exactly `7`. The one place where whole numbers need care is division: `makeChange(13)` with a 5-value coin gives two coins and a *remainder* of 3, and the code keeps that remainder instead of rounding it away, so nothing is ever lost.

**Where to look.** `src/sim/economy.h`, `src/sim/economy.cpp`, `tests/sim/economy_test.cpp`, `tests/game/economy_editor_test.cpp`, and the Economy panel in `src/game/editor.cpp` (`buildEconomy`).

**Try it (15 minutes).** In `economy_test.cpp` change the coarse coin to value 4 and the amount to 13: predict the coins and the remainder before you run the test.

**Check yourself.** Why does `parsePairs` sort its output (a `std::map`) instead of keeping the order you typed?

## US-281 Trader stock: integer arithmetic and the rules written down

**What we built.** A trader (any NPC with a `trade` profile) has a limited stock, gets a delivery every day, wants some goods more than others, and the stock is saved.

**The C++ idea: integer arithmetic with rounding rules written down.** Computers divide whole numbers by throwing the remainder away: `7 / 2` is `3`. That is fine as long as you decide it on purpose and write it down. Here, the cap of a good is `max(start * capFactor, minimumCap)`, and a restock adds `clamp(count, 0, room)` where `room = cap - stock`; `std::clamp` keeps a number inside two limits, so a delivery can never push the shelf past the cap:

```cpp
const int room = cap(id, item) - stock(id, item);
const int added = std::clamp(count, 0, std::max(0, room));
```

The random picks use a *seeded* generator. `core::Pcg32 random(seed + day * K, id * 2 + 1)` is created fresh for each (world seed, day, trader), so a day always brings the same goods whenever and wherever it is computed: the far trader and the near one, today or after loading a save. There is no global random state to get out of step.

**Where to look.** `src/sim/trade_market.cpp` (`deliver`, `dailyUpdate`), `src/sim/npc_extras.cpp` (reading and merging the profile), `tests/sim/trade_market_test.cpp`, `tests/game/trade_stock_test.cpp`.

**Try it (15 minutes).** In `trade_market_test.cpp` change `catchUpDays` expectations: set `PriceConfig::catchUpDays` to 3 and predict how much fur a trader has after `dailyUpdate(100)`.

**Check yourself.** Why does `addTrader` for an id that already exists keep its stock but take the new profile?
