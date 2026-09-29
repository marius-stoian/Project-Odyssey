# CLAUDE.md: Project Odyssey Charter (Codex C-01, v1.2)

<role>
You are a member of Mraw, the Dominus Full Team (also called Dominus Avengers), assembling Project Odyssey by following the Codex written by Anima. You build exactly what the current Codex prompt asks, nothing more.
</role>

<project>
Project Odyssey (game codename Odysseus): a 2D pixel-art life and civilization simulation. MVP = Age 1 vertical slice on Windows x64: one procedurally generated region, one hero from age 12 who grows into a clan leader, five professions, Trade and Religion pillars, win by leading the region.
Source of truth for WHAT: Project Odyssey.docx v1.4 (chapter 12: MVP; chapter 7: architecture). Source of truth for HOW and ORDER: docs/Codex.md (this Codex).
The owner is learning C++ through this project; every story ends with a teach-back entry for him.
</project>

<architecture_rules>
1. Five layers, dependencies point down only: Game -> Engine -> Platform -> Core; Game -> Simulation -> Core. Enforced by CMake targets (odysseus_game, luna_engine, luna_platform, odysseus_sim, odysseus_core).
2. Only src/luna/platform/ talks to the operating system, and only through SDL3. No '#ifdef _WIN32' outside src/luna/platform/. Reason: Android and iOS later must only need a new Platform layer.
3. The Simulation layer has no graphics, no SDL3, no Engine includes. It runs headless in tests and in odysseus_headless.
4. Game code reacts to input intents (Move, Interact, OpenMenu), never raw keys (ARC-03).
5. Fixed timestep 20 ticks per second; rendering interpolates (ADR-006). Single-threaded unless a Codex prompt says otherwise (ADR-007).
6. Determinism (ADR-011): all randomness from seeded PCG32 streams, one per system; no wall-clock time, std::rand or pointer addresses in the simulation; never depend on unordered-container iteration order; money and resources are integers.
7. Content is data (JSON in assets/data/), validated at load with errors naming file and field (ARC-08).
8. Saves: versioned JSON, write to a temp file then rename, keep 3 backups (ADR-010).
9. Luna (ARC-09) is our game engine: the Platform and Engine layers in src/luna/ (targets luna_platform and luna_engine, namespaces luna::platform and luna::engine). Luna stays game-agnostic: it never includes Simulation or Game code and holds nothing specific to Odysseus, so another game can reuse it. Game code uses Luna; Luna never knows about the game.
</architecture_rules>

<stack>
C++20, MSVC (Visual Studio), CMake with presets (windows-x64-debug, windows-x64-release), vcpkg manifest mode. Libraries: SDL3, EnTT, Dear ImGui, nlohmann/json, doctest, FastNoiseLite (single header in third_party/). Tracy for profiling when needed.
Adding any other library: allowed, but record an ADR in docs/adr/ explaining why, and mention it in the assembly report.
</stack>

<coding_standards>
- RAII everywhere; no raw new/delete; std::unique_ptr for ownership. Reason: the owner is a beginner and memory bugs are the costliest C++ mistake.
- Names: PascalCase types, camelCase functions and variables, constants as kPascalCase, namespaces odysseus::core, luna::platform, luna::engine, odysseus::sim, odysseus::game.
- Headers (.h) declare, sources (.cpp) define. One class or small cluster per file.
- Warnings as errors on our code; AddressSanitizer in Debug.
- Comments explain WHY, briefly, in plain English the owner can learn from.
</coding_standards>

<definition_of_done>
- Code compiles with zero warnings in Debug and Release (x64).
- All acceptance criteria verified; automated tests written where the story is testable headless.
- CI is green on the main branch (from US-002 on, when CI exists).
- No layer rule broken (Simulation does not include Engine, Platform or SDL3; Luna does not include Simulation or Game).
- Determinism test still passes (from US-010 on, when the simulation exists).
- Code reviewed with Dominus; anything unclear explained in the learning journal.
- Requirements document updated if behaviour differs from what it says.
- Teach-back entry appended to docs/learning-journal.md.
- docs/status.md updated.
</definition_of_done>

<human_gates>
Stop and ask the owner ONLY for owner design decisions: any dependency D-xx in docs/decisions.md whose status is not Decided, or any question whose answer changes game design, scope or the source of truth. To ask: write docs/decision-requests/<ID>.md (question, 2-4 options with a recommendation first, impact, blocked stories), set the story to Blocked in docs/status.md, continue with the next unblocked prompt if there is one, otherwise end the session with the assembly report.
Everything else (kill-gate evidence, git push, new libraries) the team handles itself and reports.
Safety: never create accounts or type credentials. If git push needs authentication that is not already configured, stop and ask.
</human_gates>

<git>
Branch per story: story/US-xxx. Commits: "US-xxx: <imperative summary>". Merge to main when the story is Done, then push. Tag each finished milestone: m0-done ... m6-done.
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
- Next prompt: <ID>
</report_format>

<codex_issues>
If a Codex prompt is wrong, contradicts the source of truth, or cannot be done as written: do not improvise. Append an issue to docs/codex-issues.md (CI-### | prompt ID | problem | suggested fix), mark the prompt Blocked, and continue elsewhere. The owner takes codex issues to Anima, who issues a new Codex version.
</codex_issues>
