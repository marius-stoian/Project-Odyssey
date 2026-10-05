# Story plans: M0

Per-story plans for milestone M0, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-001](#us-001)
- [US-002](#us-002)
- [US-003](#us-003)
- [US-004](#us-004)

---

<a id="us-001"></a>

## Plan US-001: Build and debug from a clean checkout

Codex v1.1, prompt S-US-001. Traces to ADR-001, ADR-012, ADR-015, TEC-04.

### Goal
One command builds all three programs from a fresh clone: `odysseus.exe` (the game), `odysseus_headless.exe` (simulation runner) and `odysseus_tests.exe` (unit tests), with zero warnings, and the Debug build can be stepped through in Visual Studio.

### Files

| File | Layer | What |
|---|---|---|
| `CMakeLists.txt` | Build | Project, compiler settings (C++20, /W4 /WX, AddressSanitizer in Debug), targets. |
| `CMakePresets.json` | Build | `windows-x64-debug` and `windows-x64-release` configure, build and test presets; vcpkg toolchain from `VCPKG_ROOT`. |
| `vcpkg.json` | Build | Manifest with a pinned `builtin-baseline`; only `doctest` for now (other libraries arrive with the stories that need them, D-13). |
| `.gitignore` | Build | Ignore `build/`, `.vs/`, `out/`. |
| `src/core/version.h`, `src/core/version.cpp` | Core | `odysseus::core::versionString()`: the first piece of real code every program links, so the build proves Core is wired in. |
| `apps/odysseus/main.cpp` | App | Prints the game name and version. The window comes in US-020. |
| `apps/headless/main.cpp` | App | Prints the runner name and version. The clan simulator comes in M1. |
| `tests/main.cpp`, `tests/core/version_test.cpp` | Tests | doctest runner and the US-001 test cases. |
| `README.md` | Docs | How to build, run, test and debug. |

### Targets
- `odysseus_core` (static library, `src/core/`). The other four layer targets are created by US-003, which owns the layer rules.
- `odysseus`, `odysseus_headless`, `odysseus_tests` (executables) link `odysseus_core`.
- All executables go to `build/windows-x64/bin/<Config>/`.

### Interfaces
```cpp
namespace odysseus::core {
// Semantic version of the build, "MAJOR.MINOR.PATCH", from project(VERSION) in CMakeLists.txt.
std::string_view versionString();
}
```

### Build settings
- Generator: CMake picks the newest installed Visual Studio (2026 locally; whatever the CI runner has later), platform x64. Multi-config, so both presets share one build folder and one vcpkg install.
- Our targets: `/W4 /WX /permissive- /utf-8`. Library headers come in as external includes, so their warnings do not fail our build.
- Debug: `/fsanitize=address`; `/RTC1` removed and incremental linking off (both are incompatible with AddressSanitizer). The AddressSanitizer runtime DLL is copied next to the Debug executables so they start by double-click and under ctest.

### How each acceptance criterion is tested

| Scenario | How |
|---|---|
| Clean build | Automated: fresh clone into a temporary folder, `cmake --preset windows-x64-debug` + build Debug and Release, grep the build log for warnings, check the three .exe files exist. Test case `US-001 Clean build` checks the programs link Core and get the project version. |
| Debugging | Manual check below (needs Visual Studio and a human eye). |
| Missing dependency | Automated: copy the repo to a temporary folder, remove `doctest` from `vcpkg.json`, configure, and check CMake fails with a message naming `doctest`. |

### Manual check: Debugging
1. Open Visual Studio 2026, File > Open > Folder, choose the `odysseus` folder.
2. In the toolbar, pick configuration `windows-x64-debug` and startup item `odysseus.exe`.
3. Open `apps/odysseus/main.cpp` and click in the left margin of the first line inside `main()` (a red dot appears).
4. Press F5.
5. Expected: the program stops on that line (yellow arrow). Hover over `version` after stepping once with F10: it shows the version text. Press F5 to finish.

Result: pass by debugger evidence (see Verification results). The owner can still repeat it in the Visual Studio window as a learning exercise.

### Risks
- AddressSanitizer with prebuilt C++ libraries from vcpkg can cause linker "annotation mismatch" errors later (EnTT, ImGui). Not an issue for doctest (header only); revisit when those libraries arrive.
- The determinism hash test named in the verification step cannot exist yet: there is no simulation state until US-010. Raised as CI-003.
- "CI is green" in the Definition of Done cannot hold before US-002 creates CI. Raised as CI-003.

### Verification results (2026-09-29, mraw-tester and mraw-acceptor)

Toolchain: Visual Studio Community 2026, MSVC 19.51, CMake 4.4.3, vcpkg baseline b8b8df22, doctest 2.5.3.

| Check | Result | Evidence |
|---|---|---|
| Scenario "Clean build" | **Pass** | Fresh `git clone` of story/US-001 into `C:\dev\odysseus-us001-cleancheck`: configure, Debug and Release builds exit 0; 0 warning lines in either build log; `bin/Debug` and `bin/Release` each contain odysseus.exe, odysseus_headless.exe, odysseus_tests.exe. |
| Test `US-001 Clean build` | **Pass** | ctest Debug and Release: 100% tests passed (doctest: 1 case, 2 assertions). |
| Scenario "Missing dependency" | **Pass** | Fresh clone with `doctest` removed from vcpkg.json: configure exits 1 with `Could not find a package configuration file provided by "doctest"`. |
| Scenario "Debugging" | **Pass** | Debug odysseus.exe run under Microsoft's cdb debugger (WinDbg 1.2606, same PDB symbols Visual Studio uses): `bp odysseus!main` -> "Breakpoint 0 hit" at `apps\odysseus\main.cpp @ 7`; after stepping two lines, `dx version` shows `"0.1.0"`. |
| Warnings as errors | Pass | Adding an unused variable to main.cpp fails the build with C4189 (exit 1). |
| AddressSanitizer in Debug only | Pass | dumpbin: Debug odysseus.exe imports clang_rt.asan_dynamic-x86_64.dll; Release does not. |
| DoD "CI is green", "Determinism test still passes" | Not applicable yet | No CI before US-002, no simulation state before US-010. See CI-003. |

Note: a first clean-check clone inside %TEMP% produced MSBuild warning MSB8029 ("output directory under the Temporary directory"). That warning comes from the clone location, not the project; do not build the repo under %TEMP%.

Acceptor verdict (updated 2026-09-29): **ACCEPT**. All three scenarios demonstrated. Debugging was shown with cdb rather than the Visual Studio window; the scenario's intent (Debug build pauses at a breakpoint in main and variables can be inspected) is met.

---

<a id="us-002"></a>

## Plan US-002: Run the build and tests on every push

Codex v1.1, prompt S-US-002. Traces to ADR-014.

### Goal
Every push to GitHub (any branch) builds Debug and Release on a Windows runner and runs all tests, so a broken build or test shows up as a red run on GitHub with the failing test named.

### Files

| File | Layer | What |
|---|---|---|
| `.github/workflows/ci.yml` | Build | The workflow: checkout, vcpkg at our pinned baseline, configure with the same preset as the owner's PC, build Debug and Release, run ctest for both. |
| `README.md` | Docs | CI badge and one line on what CI does. |

No C++ changes.

### Design
- Runner: `windows-latest`. It uses the same `windows-x64-*` presets as the local machine, so CI and the owner's PC cannot drift apart. CMake picks the newest Visual Studio on the runner.
- vcpkg: cloned by the workflow at the exact `builtin-baseline` commit from `vcpkg.json` (full history, so vcpkg can resolve versions). `VCPKG_ROOT` points to it. The workflow reads the commit from `vcpkg.json`, so there is one source of truth.
- Triggers: `push` on every branch and `pull_request`, plus a manual "Run workflow" button.
- Time limit: 20 minutes per job, above the 15-minute target, so a slow run shows as slow instead of cancelled.
- Test output: the test presets set `outputOnFailure`, so a failing doctest case is printed with its name in the log.

### How each acceptance criterion is tested

| Scenario | How |
|---|---|
| Green push | Push story/US-002; the run must succeed. Evidence: run URL and duration (must be under 15 minutes). |
| Broken test | Push a throwaway branch `ci-check/US-002-broken-test` whose only change is a deliberately failing test case `US-002 Broken test`. The run must fail and the log must contain that test name. Evidence: run URL and the log line. Then delete the throwaway branch. |

Both checks run on GitHub, so they are automated but not doctest cases (nothing to test headless in C++ for this story).

### Risks
- Runner images change their Visual Studio version over time. Using the presets (no hard-coded generator) keeps us working on 2022 and 2026.
- A full vcpkg clone adds about a minute. Binary caching can come later if runs get slow.

### Verification results (2026-09-29, mraw-tester and mraw-acceptor)

| Check | Result | Evidence |
|---|---|---|
| Scenario "Green push" | **Pass** | Push of story/US-002: run [36623795639](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36623795639) succeeded in 1 min 48 s (20:04:56 to 20:06:44 UTC), all steps green: configure, Debug and Release builds, Debug and Release tests. |
| Scenario "Broken test" | **Pass** | Push of throwaway branch ci-check/US-002-broken-test: run [36624093902](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36624093902) failed. Log: `TEST CASE: US-002 Broken test`, `version_test.cpp(41): ERROR: CHECK( 1 + 1 == 3 ) is NOT correct!`, `test cases: 2 \| 1 passed \| 1 failed`. Branch deleted afterwards. |
| DoD "CI is green on main" | Applies from now on | Checked after the merge. |

Acceptor verdict: **ACCEPT**.

---

<a id="us-003"></a>

## Plan US-003: Enforce the layer rules in the build

Codex v1.2, prompt S-US-003. Traces to ARC-01, ARC-02, ARC-09 and ADR-004.

### Goal and readiness

Represent all five layers as CMake targets and make forbidden includes fail the build, including includes that use `../` to bypass include directories. US-001 is Done; no owner decision is required. This story introduces no gameplay and no window.

The available source of truth is CLAUDE.md and docs/Codex.md. The referenced DOCX and workbook are absent from this checkout and were not located in the file search. These files state the architecture and exact scenarios sufficiently for this build-only story; this limitation must remain explicit in the assembly evidence.

### Files and layers

| File | Layer | Change |
|---|---|---|
| `CMakeLists.txt` | Build | Five targets, explicit downward links, private consumer identity, remove broad source-root include visibility. |
| `cmake/LayerRules.cmake` | Build | Helpers that register layer targets and validate header and include rules whenever the build runs. |
| `src/core/boundary.h`, existing `src/core/version.h` | Core | Compile-time boundary and guard the existing public header. |
| `src/luna/platform/boundary.h`, `src/luna/platform/layer.h`, `src/luna/platform/layer.cpp` | Platform | Empty, guarded layer scaffold. |
| `src/luna/engine/boundary.h`, `src/luna/engine/layer.h`, `src/luna/engine/layer.cpp` | Engine | Empty, guarded layer scaffold. |
| `src/sim/boundary.h`, `src/sim/layer.h`, `src/sim/layer.cpp` | Simulation | Empty, guarded layer scaffold, no graphics dependencies. |
| `src/game/boundary.h`, `src/game/layer.h`, `src/game/layer.cpp` | Game | Empty, guarded layer scaffold. |
| `tests/architecture/` and test driver | Tests | Positive and negative compile fixtures and scenario-named doctest evidence. |
| `docs/adr/ADR-016-layer-boundary-enforcement.md` and ADR index | Docs | Record the new compile guard plus build-validation pattern. |
| `README.md`, `docs/learning-journal.md` | Docs | Explain allowed links and teach CMake targets. Writer owns these changes. |

An equivalent minimal scaffold naming scheme is acceptable. Keep source/include public spelling readable, for example `core/version.h` and `luna/engine/layer.h`. Narrow build include trees or per-layer include roots may supply these spellings; avoid granting every target `${PROJECT_SOURCE_DIR}/src`.

### Dependencies and interfaces

| Target | Direct project dependencies | Allowed project headers, including downward transitive dependencies |
|---|---|---|
| `odysseus_core` | None | Core |
| `luna_platform` | Core | Platform, Core |
| `luna_engine` | Platform | Engine, Platform, Core |
| `odysseus_sim` | Core | Simulation, Core |
| `odysseus_game` | Engine, Simulation | Game, Engine, Platform, Simulation, Core |

`odysseus` links Game. `odysseus_headless` links Simulation. Existing version tests may keep a Core-only consumer identity; architecture probes compile separately with the layer identity being exercised. Tests must never redefine a production layer as having broader permissions. No new gameplay API or data format is needed: boundary headers and empty namespaces are enough, with no invented engine behavior.

Each CMake target gets exactly one PRIVATE consumer macro, for example `ODYSSEUS_LAYER_SIM`, which its dependents do not inherit. Each public project header includes its own layer's `boundary.h` before declaring anything. Boundary checks reject a forbidden consumer with `#error` identifying the include and the source layer. An Engine header therefore rejects the Simulation consumer even if its path is `../luna/engine/layer.h`.

Permissions: Core permits all five consumers; Engine permits Engine and Game; Platform permits Platform, Engine and Game; Simulation permits Simulation and Game; Game permits Game only. Unknown identities fail closed. A header guard does not weaken this check: a translation unit has one fixed consumer identity.

Build validation must check that every project header is guarded, so a newly added unguarded header cannot silently bypass the mechanism. It resolves and normalizes quoted include paths, including relative ones, before classifying their destination. It rejects disallowed cross-layer destinations and SDL headers from Simulation (including relative or absolute SDL3 paths), and also SDL from Core, Engine and Game under Charter rule 2. Macro-expanded include directives in project code must be rejected or resolved, not silently exempted. Third-party headers are not modified to add our guards. Validation runs on every build, not just configure, so a violating include added after configuration still fails. Diagnostics identify the source file, the included header and the violated rule as an include error.

This guards against accidental architectural drift, not against deliberate `#undef` or editing the validator. Keep the implementation small and dependency-free; CMake and the existing compiler suffice. Record ADR-016 because this is a new enforcement pattern.

### Tests first and acceptance evidence

The tester implements a small fixture driver before production changes. It compiles scratch fixtures using the same configured toolchain and layer identities as production; positive compile success and negative compiler diagnostics are asserted by doctest cases named after the scenarios. If a CTest wrapper builds the fixtures, named doctest scenario evidence is still retained, as the Codex requires. Negative fixtures must require the expected architecture or include diagnostic; an unrelated syntax error, missing compiler or broken toolchain is not success. Include known-valid Core usage as a control. Keep fixtures outside production source discovery.

| Scenario | Checks and expected evidence |
|---|---|
| `US-003 Allowed use` | Game includes Engine's real scaffold header and builds successfully; compile each permitted direct edge; normal executable and Core tests still build. |
| `US-003 Forbidden use` | Simulation includes Engine and SDL3 headers separately: both fail with relevant include errors. Repeat Engine rejection with a relative path and an absolute path. Confirm Simulation can include Core. |
| `US-003 Luna stays game-agnostic` | Engine and Platform each include Simulation and Game headers: all four fail with relevant include errors. Repeat at least one with a relative path; allowed Luna-to-Core probes pass. |
| Enforcement regression | A forbidden include added after configure still fails the build. Use isolated fixtures rather than modifying production source. |

Run the full Debug and Release Windows builds and ctest presets with zero warnings. This runtime lacks MSVC; a temporary CMake installation and g++ can provide supplementary fixture checks. GitHub's Windows runner supplies the required production build evidence. No visual/manual window scenario exists. Determinism is not applicable until US-010; report that explicitly. The acceptor must inspect actual Windows logs before accepting; local static inspection alone does not prove the compile scenarios.

### Risks and constraints

- Per-target links alone do not prohibit relative includes; the guards and validation address this known bypass.
- A dependency's include paths can leak broader third-party access. Simulation never links SDL; validation gives a diagnostic even if an installed SDK happens to make SDL headers discoverable.
- Validate only production layer sources and headers; intentionally invalid test fixtures stay isolated.
- Do not relax the Windows/MSVC toolchain policy to obtain a local green build. Preserve the existing CI, presets, ASan and warnings-as-errors behavior.
- Enforcement scaffolding must remain game-agnostic in Luna. Any disagreement with the unavailable charter document found later is a Codex issue, not permission to invent a different architecture.

### Verification results

#### Tests-first baseline (2026-09-29)

Before production changes, GCC C++20 compiling Game's `#include "luna/engine/layer.h"` with the original `${PROJECT_SOURCE_DIR}/src` visibility exited 1: `fatal error: luna/engine/layer.h: No such file or directory`. The Engine scaffold required by Allowed use does not exist in the original checkout.

A separate scratch fixture reproduced the original unconditional source-root include setting, added a minimal Engine header, and compiled that header from a Simulation-identified translation unit. Exit 0 shows the original visibility leaks Engine into Simulation. This synthetic baseline demonstrates the build setting's defect; it is supplementary evidence, not a required Windows/MSVC build.

Tests were authored before implementation: the three acceptance scenario doctests exercise 14 compiler probes against the production target include interfaces, with positive controls and diagnostic-specific negative checks. `US-003 Guard completeness` separately configures an isolated project and injects violations after configure, then builds to verify the validator runs during every build.

#### Supplementary implementation checks (2026-09-29)

An isolated CMake harness used the production `odysseus_register_layer` helper, real source/header files and the same five-target dependency graph with GCC 13.3.0, C++20, `-Wall -Wextra -Werror`. Only scratch harness compiler settings used GCC; the repository's Windows/MSVC policy and presets remain intact. Existing doctest v2.4.12 was fetched to scratch and used without adding a dependency.

| Configuration | Build | doctest | CTest |
|---|---|---|---|
| Debug (supplementary GCC) | Exit 0, no warnings | 5 cases / 17 assertions passed | 5/5 named CTests passed, 1.82 seconds |
| Release (supplementary GCC) | Exit 0, no warnings | 5 cases / 17 assertions passed | 5/5 named CTests passed, 1.73 seconds |

The 14 compile probes include all five permitted direct edges and all requested negative directions. Absolute and relative Simulation-to-Engine probes fail with `#error "Architecture include error: luna/engine/boundary.h is forbidden from SIM source layer"`; relative Engine-to-Simulation fails with the analogous `sim/boundary.h ... ENGINE source layer` message. Simulation-to-SDL3 fails with a compiler missing-include diagnostic; the production validator additionally rejects the SDL include explicitly, regardless of SDK visibility.

The isolated after-configure validator fixture passes clean and restored-clean controls (exit 0) and rejects six mutations (exit 2 with `Architecture include error`): unguarded header, Simulation SDL3 include, relative Engine include, absolute Engine include, macro-expanded include and relative Luna-to-Simulation include. No mutation touched the repository source files.

Exact scratch evidence: `/tmp/odyssey-us003-local/build/Testing/Temporary/LastTest.log` and `/tmp/odyssey-us003-local/build-release/Testing/Temporary/LastTest.log`; individual `us003_*-Debug.log` / `us003_*-Release.log` alongside them; `architecture-validator/results.log` beneath each build directory. One test-driver quoting defect was exposed on the first Debug run and corrected before the final passes; no assertion was weakened.

The required Windows/MSVC Debug and Release builds, AddressSanitizer execution and Windows CTest/CI were unverified at this point: the runtime was Linux, and the GitHub integration denied branch creation with HTTP 403 (`Resource not accessible by integration`). Determinism is not applicable until US-010. These local results did not satisfy the Windows acceptance or green-main CI Definition of Done.

Final registration check: the scratch harness copied the production five CTest registrations verbatim and ran with `ctest -j 4`; all five passed in both configurations. The architecture resource lock serialized nested builds correctly while the Core test ran independently. Baseline and final output are kept in `docs/evidence/US-003/baseline.txt`, `supplementary-debug.txt`, `supplementary-release.txt` and `supplementary-validator.txt`.

### Windows verification (2026-09-30, Mraw on branch qa)

ChatGPT's implementation was imported unchanged (commit `2170dfb`), merged with `main` into `qa` and verified on the owner's PC: Visual Studio Community 2026, MSVC 19.51, CMake 4.4.3.

| Check | Result | Evidence |
|---|---|---|
| Debug and Release builds | Pass: exit 0, 0 warning lines | `cmake --build --preset windows-x64-debug` / `-release` |
| First Windows test run | 3 of 5 passed. `US-003 Forbidden use` and `US-003 Luna stays game-agnostic` failed although every forbidden include **was** rejected: MSBuild prints `fatal  error C1083` (two spaces), which `run_probe.cmake` did not recognise as an include error | [windows-probe-diagnostics.txt](../evidence/US-003/windows-probe-diagnostics.txt) |
| Fix | `run_probe.cmake` matches the error code (`error C1083`) instead of the prefix. No test was weakened: the check still requires an include error that names the forbidden header | commit `803be18` |
| Scenario "Allowed use" | **Pass**, Debug and Release | [windows-debug.txt](../evidence/US-003/windows-debug.txt), [windows-release.txt](../evidence/US-003/windows-release.txt) |
| Scenario "Forbidden use" | **Pass**: Simulation including Engine or SDL3 fails with C1083; relative and absolute bypasses fail with C1189 `luna/engine/boundary.h is forbidden from SIM source layer` | same |
| Scenario "Luna stays game-agnostic" | **Pass**: Engine and Platform including Simulation or Game fail; the relative bypass fails with `sim/boundary.h is forbidden from ENGINE source layer` | same |
| Guard completeness | **Pass**: six violations added after configure are rejected; clean controls build | same |
| AddressSanitizer | Debug executables and tests run with ASan (unchanged policy) | ctest Debug |
| odysseus.exe, odysseus_headless.exe | Still start and print their version | manual run |
| Determinism hash test | Not applicable until US-010 | Charter DoD |

Acceptor verdict (2026-09-30): **ACCEPT** the three scenarios and the guard check on Windows. GitHub CI on `qa`: green, 5/5 tests in Debug and Release ([run 36635345962](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36635345962), 1 min 50 s). Under the owner's branch rule of 2026-09-30 (accepted stories integrate into `qa`; `qa` merges into `main` at milestone exits), US-003 is **Done**. The teach-back is in [the learning journal](../learning-journal.md).

### Teach-back (draft, accepted 2026-09-30)

**What we built.** Five library targets now represent our five layers, with explicit downward dependencies. Header checks and a validator reject forbidden includes, including shortcuts through relative paths.

**The idea: CMake targets and `target_link_libraries`.** A target is a named thing CMake builds, such as a library or executable. `add_library` lists the source files belonging to a library; `target_link_libraries` connects it to libraries it needs. `PUBLIC` means both this target and its callers receive the dependency's build requirements, including public header paths. `PRIVATE` applies the dependency only to the target itself. Simulation links Core, so it can use Core's headers without receiving Engine's headers. A link is a dependency declaration, not a complete include barrier: a relative path could bypass header search paths. Our boundary checks and build validator close that accidental loophole.

```cmake
add_library(odysseus_sim STATIC src/sim/layer.cpp)
target_link_libraries(odysseus_sim PUBLIC odysseus_core)
```

**Where to look.** `CMakeLists.txt:71-73` (Simulation's target), `CMakeLists.txt:76-78` (Game's dependencies), `cmake/LayerRules.cmake:2` (target registration), `src/luna/engine/boundary.h:1` (include boundary).

**Try it (15 minutes).** On a local practice branch, add `#include "../luna/engine/layer.h"` to `src/sim/layer.cpp`. Build Debug and read the include error naming the forbidden layer. Replace it with `#include "core/version.h"`, build again and compare the result; then restore the original file.

**Check yourself.** Why does Game receive Core's public headers through its dependencies, while Simulation still cannot include Engine through a relative path?

---

<a id="us-004"></a>

## Plan US-004: Log what happens and stop on broken assumptions

Codex v1.3, prompt S-US-004. Traces to architecture section 7.8 (logs in the per-user folder, last 5 sessions kept), ADR-015 (asserts in Debug).

### Goal
Every run writes a timestamped log file to the per-user logs folder, only the last five session logs are kept, and a false assert in a Debug build logs its file and line, then stops the debugger on that line.

### Files

| File | Layer | What |
|---|---|---|
| `src/core/log.h`, `src/core/log.cpp` | Core | `LogSession` (RAII: opens the session file, rotates old ones, closes on destruction) and `logInfo` / `logWarning` / `logError` |
| `src/core/assertions.h`, `src/core/assertions.cpp` | Core | `ODYSSEUS_ASSERT(condition, message)` and `reportAssertionFailure(...)` |
| `src/luna/platform/user_paths.h`, `src/luna/platform/user_paths.cpp` | Platform (Luna) | `userDataDirectory()`: the per-user folder, from SDL3's `SDL_GetPrefPath` |
| `apps/odysseus/main.cpp` | App | Start a log session in `<user data>/logs`, log start and shutdown |
| `vcpkg.json`, `CMakeLists.txt` | Build | Add `sdl3` (D-13); link it PRIVATE to `luna_platform` only |
| `tests/core/log_test.cpp`, `tests/core/assert_probe.cpp` | Tests | US-004 scenarios; a tiny program that fires a false assert |

Why Platform and SDL3 here, although the prompt lists only Build and Core: the acceptance criteria need the per-user folder, and Charter rule 2 says only `src/luna/platform/` talks to the operating system, through SDL3 (requirements 7.10: "per-user paths from SDL"). Core stays standard-library only: the logger receives its folder from the caller. SDL3 is linked PRIVATE, so Simulation and Game do not see SDL headers.

### Interfaces

```cpp
namespace odysseus::core {
inline constexpr int kKeptLogSessions = 5;
class LogSession {                       // one per program run; not copyable
public:
    explicit LogSession(const std::filesystem::path& directory, int keptSessions = kKeptLogSessions);
    ~LogSession();                       // writes a closing line and closes the file
    const std::filesystem::path& file() const;
};
void logInfo(std::string_view message);
void logWarning(std::string_view message);
void logError(std::string_view message);
void reportAssertionFailure(std::string_view condition, std::string_view message, std::string_view file, int line);
}
namespace luna::platform {
std::filesystem::path userDataDirectory(); // %APPDATA%\Project Odyssey\Odysseus\ on Windows
}
```

- Line format: `2026-09-30 01:23:45.678Z [INFO] message` (UTC, so no time-zone database is needed).
- Session file names: `session-YYYYMMDD-HHMMSS-mmm-NN.log`. They sort by time as plain text; `NN` separates two sessions in the same millisecond.
- Rotation: before creating the new file, delete the oldest `session-*.log` files until at most `keptSessions - 1` remain. Other files in the folder are never touched.
- With no active session, log calls go to standard error.
- Wall-clock time is allowed: logs are not simulation state (Charter rule 6 applies to the simulation).
- `ODYSSEUS_ASSERT` in Debug: on a false condition, log `Assertion failed: <condition> (<message>) at <file>:<line>`, flush, then break into the debugger at the assert line (`__debugbreak` on MSVC, a compiler intrinsic, not an operating-system call). Without a debugger the program stops with a breakpoint exception. In Release it compiles to nothing but still mentions the condition inside `sizeof`, so variables used only in asserts do not trigger unused-variable warnings (warnings are errors).
- The header is `assertions.h`, not `assert.h`, so it never shadows the standard `<assert.h>`.

### How each acceptance criterion is tested

| Scenario | Test |
|---|---|
| Log file | `US-004 Log file`: start a session in a temporary folder, log two lines, end it; the file exists, starts with `session-`, and every line matches the timestamp format. Manual/end-to-end: run `odysseus.exe`, then check `%APPDATA%\Project Odyssey\Odysseus\logs`. |
| Rotation | `US-004 Rotation`: run six sessions in one folder with an unrelated file beside them; five session files remain, the first is gone, the unrelated file is untouched. |
| Assert | `US-004 Assert`: `reportAssertionFailure` writes file and line to the log; the test also runs `us004_assert_probe.exe` (Debug), which fires a false assert: the process stops with the breakpoint exception and its log names `assert_probe.cpp` and the line. Manual: run the probe under the cdb debugger and confirm it stops on the assert line. |

### Risks
- SDL3 is built from source by vcpkg, so the first configure and each CI run take a few minutes longer. Acceptable (CI budget 15 minutes); vcpkg binary caching can come later.
- `SDL3.dll` must sit next to `odysseus.exe`; the vcpkg toolchain copies it automatically.
- ADR-016 probes check that Simulation cannot include SDL3; linking SDL3 PRIVATE to Platform keeps that true. Re-run the US-003 tests to confirm.

### Verification results (2026-09-30)

| Check | Result | Evidence |
|---|---|---|
| Debug and Release builds | Pass: 0 warning lines (SDL3 3.4.16 from vcpkg) | build logs |
| `US-004 Log file` | Pass | [windows-debug.txt](../evidence/US-004/windows-debug.txt) |
| `US-004 Rotation` | Pass | same |
| `US-004 Assert` (log records file and line; a false assert stops a Debug program) | Pass | same |
| All tests, Debug and Release | 5/5 ctest; US-004 doctest: 3 cases, 24 assertions | [windows-debug.txt](../evidence/US-004/windows-debug.txt), [windows-release.txt](../evidence/US-004/windows-release.txt) |
| Log file end to end | Pass: the game writes `%APPDATA%\Project Odyssey\Odysseus\logs\session-*.log` with timestamped lines | [end-to-end.txt](../evidence/US-004/end-to-end.txt) |
| Rotation end to end | Pass: 7 runs leave 5 session logs | same |
| Assert under the debugger | Pass: cdb stops at `assert_probe.cpp @ 13`, the assert line | same |
| Layer rules still hold with SDL3 installed | Pass: `US-003 Forbidden use` still rejects SDL3 in Simulation | ctest |

Acceptor verdict (2026-09-30): **ACCEPT**. Every scenario demonstrated; Definition of Done holds once CI on `qa` is green after the merge.
