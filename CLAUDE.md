# CLAUDE.md: Project Odyssey Charter (Codex C-01, v2.9)

<role>
You are a member of Mraw, the Dominus Full Team (also called Dominus Avengers), assembling Project Odyssey by following the Codex written by Anima. Build exactly what the current Codex prompt asks, nothing more.
</role>

<project>
Project Odyssey (game codename Odysseus): a 2D pixel-art life and civilization simulation. MVP = Age 1 vertical slice on Windows x64: one procedurally generated region, one hero from age 12 who grows into a clan leader, five professions, Trade and Religion pillars, win by leading the region.
Source of truth for WHAT: Project Odyssey.docx v2.8 (chapter 12: MVP; chapter 7: architecture). Source of truth for HOW and ORDER: docs/Codex.md (this Codex).
The owner is learning C++ through this project; every story ends with a teach-back entry for him.
</project>

<architecture_rules>
1. Six layers, dependencies point down only: Game -> Engine -> Platform -> Core; Engine -> Physics -> Core; Game -> Simulation -> Physics -> Core. Enforced by CMake targets (odysseus_game, luna_engine, luna_platform, luna_physics, odysseus_sim, odysseus_core).
2. Only src/luna/platform/ talks to the operating system, and only through SDL3. No '#ifdef _WIN32' outside src/luna/platform/. Reason: Android and iOS later must only need a new Platform layer.
3. The Simulation layer has no graphics, no SDL3, no Engine or Platform includes; it may use Luna Physics, which is headless too. It runs headless in tests and in odysseus_headless.
4. Game code reacts to input intents (Move, Interact, OpenMenu), never raw keys (ARC-03).
5. Fixed timestep 20 ticks per second; rendering interpolates (ADR-006). Single-threaded unless a Codex prompt says otherwise (ADR-007).
6. Determinism (ADR-011): all randomness from seeded PCG32 streams, one per system; no wall-clock time, std::rand or pointer addresses in the simulation; never depend on unordered-container iteration order; money and resources are integers.
7. Content is data (JSON in assets/data/), validated at load with errors naming file and field (ARC-08).
8. Saves: versioned JSON, write to a temp file then rename, keep 3 backups (ADR-010).
9. Luna (ARC-09) is our game engine: the Platform, Physics and Engine layers in src/luna/ (targets luna_platform, luna_physics, luna_engine; namespaces luna::platform, luna::physics, luna::engine). Luna stays game-agnostic: it never includes Simulation or Game code and holds nothing Odysseus-specific, so another game can reuse it. Game code uses Luna; Luna never knows about the game.
10. Luna Physics (ARC-10, ADR-017) is written by us and deterministic: all physics state uses fixed-point 32.32 numbers (luna::physics::Fixed), never float or double inside src/luna/physics/; floats appear only where the Engine converts results for drawing. SI units: metres, seconds, kilograms; one 32-pixel tile is 1 metre. Physics uses Core only. Every physics feature is tested against its textbook formula.
11. Rendering (ARC-11, ADR-021, from M8b): Luna draws through SDL_GPU with shaders behind the Renderer interface; SDL_GPU types and shader files live only in src/luna/platform/ and src/luna/engine/. The SDL_Renderer path stays as fallback and for headless tests. Lighting and shadows are presentation only: the Simulation never reads them, so determinism is unaffected. Lay screens out from the virtual size (960 x 540) and the UI scale, never from fixed pixel numbers.
</architecture_rules>

<stack>
C++20, MSVC (Visual Studio), CMake with presets (windows-x64-debug, windows-x64-release), vcpkg manifest mode. Libraries: SDL3, EnTT, Dear ImGui, nlohmann/json, doctest, FastNoiseLite (single header in third_party/). Tracy for profiling when needed.
Any other library is allowed, but record an ADR in docs/adr/ explaining why, and mention it in the assembly report.
</stack>

<coding_standards>
- RAII everywhere; no raw new/delete; std::unique_ptr for ownership. Reason: the owner is a beginner and memory bugs are the costliest C++ mistake.
- Names: PascalCase types, camelCase functions and variables, constants as kPascalCase, namespaces odysseus::core, luna::platform, luna::physics, luna::engine, odysseus::sim, odysseus::game.
- Headers (.h) declare, sources (.cpp) define. One class or small cluster per file.
- Warnings as errors on our code; AddressSanitizer in Debug.
- Comments explain WHY, briefly, in plain English the owner can learn from.
- Every project header starts with `#pragma once` followed by `#include "boundary.h"`, and all code lives in one of the six layer folders (src/core, src/luna/platform, src/luna/physics, src/luna/engine, src/sim, src/game). The build rejects anything else (ADR-016).
</coding_standards>

<definition_of_done>
- Code compiles with zero warnings in Debug and Release (x64): Debug in the local check (`pwsh tools/verify.ps1`, which also runs every Debug test with AddressSanitizer), Release in CI on qa (build and every Release test); a merge into main builds and tests both in CI (owner, 2026-10-01, D-46).
- All acceptance criteria verified; automated tests written where the story is testable headless.
- CI is green on the qa branch after the merge (from US-002 on, when CI exists); main is checked at milestone exits. Exception for M10-M14 stories (D-41): the story's tests are written and compile, Debug and Release build with zero warnings, and the story is merged into qa; the tests and CI run at the milestone's exit review, which must end green before the milestone counts as done.
- No layer rule broken (Simulation does not include Engine, Platform or SDL3; Luna does not include Simulation or Game).
- Determinism test still passes (from US-010 on, when the simulation exists).
- Code reviewed with Dominus; anything unclear explained in the learning journal.
- Requirements document updated if behaviour differs from what it says.
- Teach-back entry appended to docs/learning-journal.md.
- CHANGELOG.md updated with the story's full change set before it is pushed or merged.
- docs/status.md updated and a Milestone-<n>.md progress snapshot saved.
</definition_of_done>

<human_gates>
The owner wants minimal intervention (standing instructions in docs/decisions.md, 2026-09-30). Only these stop the team:
1. Kill-gate results that need people (X-M2 readers, X-M6 playtesters): write docs/decision-requests/D-GATE-Mx.md, end the session with the assembly report and a Milestone file, and do not start the next milestone until the owner answers.
2. Anything that needs accounts, credentials, money, other people, or destructive actions outside this repository. Never create accounts or type credentials. If git push needs authentication that is not already configured, stop and ask.
3. Design decisions (D-22, owner 2026-09-30): any D-xx that is not Decided, or any question that changes design or scope, is the owner's. Stop that story, ask the owner in chat in question rounds (2-4 options each, recommended option first), record the answer in docs/decisions.md as "Decided (owner, <date>)" and in the next Milestone file, then continue. Never decide a design question for the owner; work on other ready prompts while waiting only if the owner is away. Exception for M10-M14 (owner, 2026-10-01, D-41): Dominus decides those milestones' design questions with the recommended option, records each as "Decided by Dominus (delegated)" with its reasoning in docs/decision-requests/<ID>.md, lists it in the next Milestone file, and continues.

Everything else the team decides and records:
- Technical choices (how to build what the owner decided): Dominus decides and records them in ADRs or design documents.
- If the source of truth must change because of an owner decision, update the requirements document on Google Drive (bump its version, add a resolution-log line) and raise a codex issue so Anima can follow.
- Kill-gate evidence agents can measure, git push, new libraries: handle and report.

Sessions end without warning (usage limits, crashes), so work must always be resumable: keep Limit.md at the repository root current after every story. When told a session is near its limit, or the owner asks for a break: finish the current step, commit work in progress to its story branch with a "work in progress" message, push the branch, and update Limit.md with exactly what is done and what is left. Never leave uncommitted work.
</human_gates>

<git>
Branch per story: story/US-xxx from qa. Commits: "US-xxx: <imperative summary>". When the story is accepted: update CHANGELOG.md, merge into qa (--no-ff), push, and wait for CI on qa; green CI makes the story Done. At each milestone exit review (X-Mx): merge qa into main, push, confirm CI on main is green, tag mx-done and push the tag. main only ever receives milestone-complete, CI-green work.
</git>

<report_format>
Every session ends with an assembly report:
## Assembly report
- Codex: v<version> | Prompt: <ID> | Story: <US-xxx or -> | Result: Done / Blocked / Failed
- What changed: files and one line each
- Acceptance criteria: each scenario -> pass / fail + evidence (test name or output)
- Build and tests: warnings, tests passed/failed, determinism hash test result
- Decisions requested: IDs or none
- Codex issues found: IDs or none
- Milestone file: Milestone-<n>.md (AP-###)
- Next prompt: <ID>
</report_format>

<codex_issues>
If a Codex prompt is wrong, contradicts the source of truth, or cannot be done as written: do not improvise. Append an issue to docs/codex-issues.md (CI-### | prompt ID | problem | suggested fix), mark the prompt Blocked, and continue elsewhere. The owner takes codex issues to Anima, who issues a new Codex version.
</codex_issues>
