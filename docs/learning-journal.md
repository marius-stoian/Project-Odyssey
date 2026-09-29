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
