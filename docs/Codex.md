# Project Odyssey Codex v1.2

Author: **Anima** (Prompt Architect) for **Mraw** (Dominus Full Team / Dominus Avengers) | Date: 2026-09-29 | Source of truth: Project Odyssey.docx v1.4 (chapter 12: MVP; chapter 7: architecture) | Executor: Claude Code autonomous agents (target model: Opus 5.5 for orchestrator, architect and acceptor; any current model for the others) | Human gate: owner design decisions only

## 0. How to use this Codex

- Phase 3 of the Amek workflow: Mraw assembles Project Odyssey by executing these prompts **in order**, exactly as written.
- Start: put this file in an empty folder `odysseus/`, open Claude Code there, paste **A-000**. Every later session: paste **A-001**.
- P-000 turns the Charter into `CLAUDE.md` and the role prompts into `.claude/agents/`, so every agent loads them automatically.
- One story in progress at a time. Every session ends with an assembly report.
- Blocked, wrong or ambiguous prompts become codex issues; the owner takes them to Anima with **A-002**; Anima issues a new Codex version.

## 1. Delivery format

Hybrid: **stage gates** at milestones M0-M6 (with kill gates at M2 and M6) and **Kanban flow** inside each milestone, WIP 1. Milestone kickoff (K) batches owner decisions; exit review (X) demonstrates exit criteria and tags the repo.

## 2. Charter (C-01)
Written verbatim to `CLAUDE.md` by P-000.

```markdown
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
```

## 3. Mraw roles (R-01..R-07)
Written verbatim to `.claude/agents/<name>.md` by P-000.

### R-01 mraw-orchestrator (Project Manager + Product Owner)
```markdown
---
name: mraw-orchestrator
description: Runs the Mraw build loop for one Codex prompt at a time: checks readiness, delegates to the other Mraw agents, runs gates, keeps docs/status.md, merges and pushes. Use for every Codex story prompt and milestone prompt.
tools: Read, Grep, Glob, Edit, Write, Bash
---

You are the Mraw orchestrator. You own the build loop (Codex L-01) and the order of work. You never write production code yourself; you delegate to mraw-architect, mraw-tester, mraw-programmer, mraw-acceptor, mraw-writer and mraw-designer, and you integrate their results. You keep one story in progress at a time (WIP 1) because a solo project with parallel half-finished stories cannot be reviewed or taught. You are the only agent that edits docs/status.md, merges to main and pushes.
```

### R-02 mraw-architect (Solution Architect + Full Stack Developer)
```markdown
---
name: mraw-architect
description: Plans a story before coding: which files and layers change, which ADRs apply, risks. Guards the five-layer rules. Use at the start of every story.
tools: Read, Grep, Glob, Write
---

You are the Mraw architect. For the given story, write docs/plans/<US-xxx>.md: goal, files to create or change (with their layer), interfaces (function and type signatures), data formats, how each acceptance criterion will be tested, risks. Check every planned include against the layer rules in the Charter; if the story cannot be done without breaking a rule, raise a Codex issue instead of bending the rule. Record an ADR in docs/adr/ when you introduce a new library or pattern.
```

### R-03 mraw-tester (Software Tester)
```markdown
---
name: mraw-tester
description: Writes tests from acceptance criteria before implementation and runs the full suite, determinism and soak tests. Use before and after implementation.
tools: Read, Grep, Glob, Edit, Write, Bash
---

You are the Mraw tester. Before implementation, turn every acceptance scenario that can run headless into a doctest test case named after the scenario (TEST_CASE("US-xxx <scenario>")). For scenarios that need a window or a human eye, write a manual check in docs/plans/<US-xxx>.md with exact steps and expected result. After implementation, run the Debug and Release builds, all tests, and the determinism test; report exact results. Never delete or weaken a test to make it pass.
```

### R-04 mraw-programmer (Programmer)
```markdown
---
name: mraw-programmer
description: Implements the planned change in C++20 until the tests pass with zero warnings. Use after the architect's plan and the tester's failing tests exist.
tools: Read, Grep, Glob, Edit, Write, Bash
---

You are the Mraw programmer. Implement exactly the plan in docs/plans/<US-xxx>.md, following the Charter's coding standards. Keep code simple and readable for a C++ beginner: prefer clear names and small functions over clever templates. Build and run the tests until they pass with zero warnings. If the plan is wrong, tell the orchestrator; do not silently redesign.
```

### R-05 mraw-acceptor (Product Owner (acceptance) + UX Designer)
```markdown
---
name: mraw-acceptor
description: Verifies every acceptance criterion and the Definition of Done against evidence, and accepts or rejects the story with reasons. Use after tests pass.
tools: Read, Grep, Glob, Bash
---

You are the Mraw acceptor. Check each acceptance scenario of the story against evidence (test output, manual check results, screenshots or logs). Check the Definition of Done line by line. Answer ACCEPT or REJECT; for REJECT, list exactly which scenario or DoD line failed and why. You accept only what is demonstrated, never what is claimed.
```

### R-06 mraw-writer (Technical Writer + Teacher)
```markdown
---
name: mraw-writer
description: Updates docs and writes the owner's teach-back entry after a story is accepted.
tools: Read, Grep, Glob, Edit, Write
---

You are the Mraw writer and teacher. Update README and docs affected by the story. Then append a teach-back entry to docs/learning-journal.md for the owner, who is learning C++: what we built (2 sentences); the C++ concept from the story's <teach_back> explained in plain words (at most 150 words, with one tiny code example from our code); where to look (file:line); one exercise he can try in 15 minutes; one question to check understanding. Friendly, concrete, no jargon without explanation.
```

### R-07 mraw-designer (Game Designer + Novel Writer)
```markdown
---
name: mraw-designer
description: Writes game content as data (events, professions, recipes, texts, tuning values) within Decided design. Use for content-heavy stories in E5-E7 and E9.
tools: Read, Grep, Glob, Edit, Write
---

You are the Mraw designer. You write content only as data files in assets/data/ (JSON), following the schemas in the architect's plan and the Decided design in the source of truth. Tone: cozy near home, harsh in the wild, with mythic touches and gentle humor. You never invent new rules or scope: if content needs a design decision, write a decision request.
```

## 4. Build loop (L-01)
```xml
<prompt id="L-01" codex="1.2" name="Mraw build loop">
<context>
Used by mraw-orchestrator for every story prompt S-US-xxx. The Charter (CLAUDE.md) is already loaded.
</context>
<instructions>
1. Readiness: for each D-xx in the story's <dependencies>, read docs/decisions.md. If any is not Decided, follow the Charter's human_gates (decision request, mark Blocked, move on). For each US-xxx dependency, confirm it is Done in docs/status.md.
2. Branch: create story/US-xxx from main.
3. Plan: delegate to mraw-architect -> docs/plans/US-xxx.md.
4. Tests first: delegate to mraw-tester -> failing tests for every headless-testable scenario; manual checks for the rest.
5. Content (only if the story needs data): delegate to mraw-designer.
6. Implement: delegate to mraw-programmer until tests pass with zero warnings.
7. Verify: delegate to mraw-tester -> full Debug and Release builds, all tests, determinism test.
8. Accept: delegate to mraw-acceptor. On REJECT, return to step 6 with the reasons. After 3 rejections, mark the story Failed, write a codex issue, and stop this story.
9. Document and teach: delegate to mraw-writer -> docs + teach-back entry.
10. Integrate: commit, merge to main, push; set the story to Done in docs/status.md.
11. Report: write the assembly report (Charter report_format). Then continue with the next prompt in the Codex, unless the session is getting long; in that case end with the report so the next session starts fresh with A-001.
</instructions>
<stop_conditions>
Blocked dependency (step 1); 3 acceptance rejections (step 8); a codex issue that affects this story; git push needs authentication that is not configured.
</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

## 5. Human gates and owner decisions
Agents stop only for owner design decisions. The decision log starts with these dependencies (from the source of truth, section 12.6):

| ID | Decision | Needed by | Blocks | Status |
|---|---|---|---|---|
| D-01 | Confirm time model: pausable real time with speed control (OPEN-04) | M2 | US-010 | Open |
| D-02 | Side Characters interview: needs, traits, relationships (OPEN-10) | M2 (week 3) | US-011, US-012, US-013 | Open |
| D-03 | Confirm the Age 1 needs set: Hunger, Energy, Warmth, Social | M2 | US-011 | Proposed |
| D-04 | Sprite size and facing directions (OPEN-11); owner answer 2026-09-29: 32x48 px, 8 directions | M1 | US-022, US-024, US-030 | Decided |
| D-05 | Art source for placeholders: own, free asset pack, or hired (OPEN-12) | M3 | US-030 | Open |
| D-06 | Minimum PC spec (OPEN-19) | M4 | US-082 | Open |
| D-07 | Calendar display (OPEN-15) | M4 | US-050, US-083 | Open |
| D-08 | Interactions interview: verbs, objects, crafting (OPEN-09) | M4 | US-061, US-062 | Open |
| D-09 | Confirm the five Age 1 professions (MVP-07) | M4 | US-060 | Proposed |
| D-10 | Confirm MVP pillars Trade + Religion (MVP-08) and victory thresholds (MVP-09) | M5 | US-070..US-073 | Proposed |
| D-11 | Story interview: tone of events, onboarding elder (OPEN-08) | M5 | US-052, US-090 | Open |
| D-12 | Visual Studio, CMake, Git, vcpkg installed; GitHub account and private repo | M0 | US-001, US-002 | Decided |
| D-13 | SDL3, EnTT, Dear ImGui, nlohmann/json, doctest, FastNoiseLite available via vcpkg or third_party | M0-M4 | US-020, US-032, US-083, US-016, US-040 | Decided |
| D-14 | Eight outside playtesters recruited | M6 | Kill gate 2 | Open |
| D-15 | Technical chain: M0 > M1 > M2 > M3 > M4 > M5 > M6 (each milestone needs the previous one) | All | All | Planned |

## 6. Assembly prompts

### A-000 Start assembly (owner pastes this once)
```text
Dominus Avengers Assemble.
You are Mraw, the Dominus Full Team, assembling Project Odyssey with Codex v1.2 written by Anima.
Read Codex.md in this folder completely. Execute prompt P-000. Then, acting as mraw-orchestrator, execute the Codex prompts strictly in order (K-M0, then the M0 story prompts, X-M0, K-M1, ...), each through the build loop L-01.
Stop only where the Charter's human_gates say so. End every session with an assembly report.
```

### A-001 Continue assembly (owner pastes this to start each new session)
```text
Mraw, continue assembly.
Read CLAUDE.md, docs/Codex.md and docs/status.md. Check docs/decisions.md for decisions the owner has answered since the last session and unblock those stories. Then continue with the first prompt in Codex order that is To do or newly unblocked, through the build loop L-01. End with an assembly report.
```

### A-002 Take codex issues to Anima (owner pastes this in a Dominus session)
```text
Anima, amend the Codex.
Read docs/Codex.md and docs/codex-issues.md. For each open issue: decide the fix with the owner if it changes design, update the affected prompts, bump the Codex minor version, add a line per change to the amendment log, and mark the issue Resolved with the new version. Hand the new Codex back to Mraw.
```

## 7. Prompts by milestone
### P-000 Bootstrap
```xml
<prompt id="P-000" codex="1.2" name="Bootstrap the Mraw workspace">
<context>
Runs once, in an empty folder named odysseus/ that contains only Codex.md. Creates the files every later prompt relies on.
</context>
<instructions>
1. Create the folder layout from the architecture (source of truth, section 7.9): src/luna/platform, src/luna/engine, src/core, src/sim, src/game, apps/odysseus, apps/headless, tests, assets/data, assets/sprites, assets/audio, assets/fonts, tools, third_party, docs/adr, docs/plans, docs/gates, docs/decision-requests.
2. Move Codex.md to docs/Codex.md.
3. Write CLAUDE.md at the root with the exact text of Codex section C-01 (the Charter).
4. Write one file per Mraw role in .claude/agents/ with the exact text of Codex section R-01..R-07 (frontmatter + body).
5. Write docs/decisions.md: a table ID | Decision | Needed by | Blocks | Status | Owner answer, seeded with D-01..D-15 from the Codex dependency list and their current statuses.
6. Write docs/status.md: a table Prompt | Story | Milestone | Status (To do / In progress / Blocked / Done / Failed) | Last report date, seeded with every prompt in this Codex in order.
7. Write docs/learning-journal.md (title and a one-paragraph welcome to the owner), docs/codex-issues.md (empty table), docs/adr/README.md (index of ADR-001..ADR-015 with one line each from the source of truth).
8. git init, first commit "P-000: bootstrap Mraw workspace". If a remote is configured, push.
9. Check the toolchain: Visual Studio C++ compiler, CMake, Git, vcpkg. For anything missing, write docs/decision-requests/D-12.md telling the owner exactly what to install (this is an external dependency only he can resolve) and stop.
</instructions>
<stop_conditions>Missing toolchain (step 9).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

### P-001 Adopt Codex v1.2 (Luna first)
```xml
<prompt id="P-001" codex="1.2" name="Adopt Codex v1.2 in an existing workspace">
<context>
Codex v1.2 builds the Luna engine first (source of truth v1.4, ARC-09): M1 is now the Luna engine walking skeleton and M2 the console clan simulator with Kill Gate 1. Workspaces bootstrapped with Codex v1.1 still have the old folders and prompt order. A workspace bootstrapped with v1.2 needs nothing: mark this prompt Done with the note "not needed".
</context>
<instructions>
1. Folders: git mv src/platform to src/luna/platform and src/engine to src/luna/engine. They should hold only .gitkeep files; if either holds code, stop (see stop conditions).
2. docs/status.md: rebuild the table in the v1.2 Execution order (section 7), adding P-001 and keeping the status and last report date of every prompt that already has one.
3. docs/decisions.md: update "Needed by" for D-01, D-02, D-03 (M2) and D-04 (M1) from the section 5 table. Record owner answers the owner gave to Mraw since the last session (D-04: 32x48 px, 8 directions; D-13: all libraries from vcpkg, FastNoiseLite in third_party/) with status Decided. Never change an existing owner answer.
4. docs/codex-issues.md: mark CI-001, CI-002 and CI-003 Resolved in v1.2 (amendment log, section 9).
5. docs/adr/README.md: add one line under the table: "Luna (ARC-09): the Platform and Engine layers form our game-agnostic engine in src/luna/." Luna is a requirement, not a new library or pattern, so it needs no ADR.
6. Commit on main "P-001: adopt Codex v1.2 (Luna first)", push, and set P-001 to Done.
</instructions>
<stop_conditions>Code (not only .gitkeep) already exists in src/luna/platform/ or src/luna/engine/: write a codex issue and stop, because moving code is a design change for Anima.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

### M0 Tooling ready
Exit criteria: A clean checkout builds in Visual Studio; you pause the program on a breakpoint; CI runs on push.

```xml
<prompt id="K-M0" codex="1.2" name="Kick off M0 Tooling ready">
<instructions>
1. Confirm the previous milestone's exit review exists in docs/gates/ and passed (skip for M0).
2. Read docs/decisions.md. For every decision this milestone needs (D-12) that is not Decided, write its decision request now, all at once, so the owner can answer them in one sitting.
3. Set this milestone's stories to To do in docs/status.md in this order: US-001, US-002, US-003, US-004.
4. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, decisions requested.</output_format>
</prompt>
```

#### S-US-001 Build and debug from a clean checkout
```xml
<prompt id="S-US-001" codex="1.2" milestone="M0" story="US-001" priority="Must" size="S">
<context>
Story US-001: Build and debug from a clean checkout.
As a developer, I want to clone the repository and build every target with one CMake preset, so that I can start working in minutes and never fight the build.
Epic E0 Foundations: Every change builds, is tested and is safe to make.
Traces to: ADR-001, ADR-012, TEC-04.
</context>
<dependencies>
Stories that must be Done: none.
Owner decisions that must be Decided: D-12.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Build system and Core: CMakeLists.txt, CMakePresets.json, vcpkg.json, .github/workflows/, src/core/.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Clean build">
Given a fresh clone and the installed toolchain
When I run the windows-x64-debug preset and build
Then odysseus.exe, odysseus_headless.exe and odysseus_tests.exe are produced with zero warnings
</scenario>
<scenario name="Debugging">
Given the Debug build
When I set a breakpoint in main() and start debugging in Visual Studio
Then execution pauses at the breakpoint and I can inspect variables
</scenario>
<scenario name="Missing dependency">
Given a library missing from vcpkg.json
When I build
Then CMake fails with a message naming the missing library
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-001 ..." cases. The determinism hash test starts with US-010.
Manual checks in docs/plans/US-001.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: What compiler, linker and build system each do; CMake presets.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-002 Run the build and tests on every push
```xml
<prompt id="S-US-002" codex="1.2" milestone="M0" story="US-002" priority="Must" size="S">
<context>
Story US-002: Run the build and tests on every push.
As a developer, I want GitHub Actions to build and test every push on a Windows runner, so that mistakes are caught automatically, even when I forget to run tests.
Epic E0 Foundations: Every change builds, is tested and is safe to make.
Traces to: ADR-014.
</context>
<dependencies>
Stories that must be Done: US-001.
Owner decisions that must be Decided: D-12.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Build system and Core: CMakeLists.txt, CMakePresets.json, vcpkg.json, .github/workflows/, src/core/.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Green push">
Given a commit that builds and passes tests
When I push to GitHub
Then the CI run succeeds within 15 minutes
</scenario>
<scenario name="Broken test">
Given a commit with a failing test
When I push
Then the CI run fails and names the failing test
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-002 ..." cases. The determinism hash test starts with US-010.
Manual checks in docs/plans/US-002.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Git basics: commit, branch, push.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-003 Enforce the layer rules in the build
```xml
<prompt id="S-US-003" codex="1.2" milestone="M0" story="US-003" priority="Must" size="S">
<context>
Story US-003: Enforce the layer rules in the build.
As a developer, I want the build to reject code that breaks the five-layer rule or pulls game code into Luna, so that the architecture cannot rot by accident.
Epic E0 Foundations: Every change builds, is tested and is safe to make.
Traces to: ARC-01, ARC-02, ARC-09, ADR-004.
</context>
<dependencies>
Stories that must be Done: US-001.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Build system and Core: CMakeLists.txt, CMakePresets.json, vcpkg.json, .github/workflows/, src/core/, src/luna/ (empty layer targets are enough).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Allowed use">
Given Game code that includes an Engine header
When I build
Then the build succeeds
</scenario>
<scenario name="Forbidden use">
Given Simulation code that includes an Engine or SDL3 header
When I build
Then the build fails with an include error
</scenario>
<scenario name="Luna stays game-agnostic">
Given Luna code that includes a Simulation or Game header
When I build
Then the build fails with an include error
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-003 ..." cases. The determinism hash test starts with US-010.
Manual checks in docs/plans/US-003.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: CMake targets and target_link_libraries.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-004 Log what happens and stop on broken assumptions
```xml
<prompt id="S-US-004" codex="1.2" milestone="M0" story="US-004" priority="Must" size="S">
<context>
Story US-004: Log what happens and stop on broken assumptions.
As a developer, I want a logging system and asserts in Core, so that I can see what the game did before a problem.
Epic E0 Foundations: Every change builds, is tested and is safe to make.
Traces to: Architecture 7.8.
</context>
<dependencies>
Stories that must be Done: US-001.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Build system and Core: CMakeLists.txt, CMakePresets.json, vcpkg.json, .github/workflows/, src/core/.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Log file">
Given the game has run
When I open the per-user logs folder
Then a log file for the session exists with timestamped lines
</scenario>
<scenario name="Rotation">
Given six sessions have run
When I open the logs folder
Then only the last five session logs are kept
</scenario>
<scenario name="Assert">
Given a Debug build and an assert whose condition is false
When that line runs
Then the debugger stops there and the log records file and line
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-004 ..." cases. The determinism hash test starts with US-010.
Manual checks in docs/plans/US-004.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Header/source files, namespaces, macros.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M0" codex="1.2" name="Exit review M0">
<instructions>
1. Demonstrate the exit criteria: A clean checkout builds in Visual Studio; you pause the program on a breakpoint; CI runs on push.
2. Collect evidence (test output, headless run logs, FPS logs, screenshots) into docs/gates/M0.md, one section per criterion, each marked met or not met.
3. If all are met: tag the repository m0-done and push. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the gate result.</output_format>
</prompt>
```

### M1 Luna engine: walking skeleton
Exit criteria: Luna opens a window; a demo character walks around a tile map at 60 FPS with crisp pixels at any window size; the build proves Luna contains no Odysseus code.

```xml
<prompt id="K-M1" codex="1.2" name="Kick off M1 Luna engine: walking skeleton">
<instructions>
1. Confirm the previous milestone's exit review exists in docs/gates/ and passed (skip for M0).
2. Read docs/decisions.md. For every decision this milestone needs (D-04, D-13) that is not Decided, write its decision request now, all at once, so the owner can answer them in one sitting.
3. Set this milestone's stories to To do in docs/status.md in this order: US-020, US-021, US-022, US-023, US-024.
4. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, decisions requested.</output_format>
</prompt>
```

#### S-US-020 Open a window with a steady game loop
```xml
<prompt id="S-US-020" codex="1.2" milestone="M1" story="US-020" priority="Must" size="M">
<context>
Story US-020: Open a window with a steady game loop.
As a player, I want the game to open a window that runs smoothly and closes cleanly, so that the game feels solid from the first second.
Epic E2 Luna engine walking skeleton: The player sees and controls a character in a window.
Traces to: ADR-002, ADR-006, TEC-05.
</context>
<dependencies>
Stories that must be Done: US-001.
Owner decisions that must be Decided: D-13.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Luna (Platform and Engine layers): src/luna/platform/, src/luna/engine/; wiring in apps/odysseus/. Keep Luna game-agnostic (Charter rule 9).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Open">
Given the game is started
When it launches
Then a 1280 x 720 window opens within 3 seconds
</scenario>
<scenario name="Steady">
Given the loop is running
When the frame time is measured for 60 seconds
Then the average is 60 FPS on the development PC and simulation speed is identical at 30 and 144 Hz monitors
</scenario>
<scenario name="Close">
Given the window is open
When I press the close button
Then the game shuts down with no error in the log
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-020 ..." cases. The determinism hash test starts with US-010.
Manual checks in docs/plans/US-020.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: RAII wrapper around SDL_Window; unique_ptr with a custom deleter.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-021 Control the game through intents
```xml
<prompt id="S-US-021" codex="1.2" milestone="M1" story="US-021" priority="Must" size="S">
<context>
Story US-021: Control the game through intents.
As a player, I want keyboard, mouse and gamepad to control the same actions, so that I can play the way I like, and touch can be added later.
Epic E2 Luna engine walking skeleton: The player sees and controls a character in a window.
Traces to: ARC-03.
</context>
<dependencies>
Stories that must be Done: US-020.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Luna (Platform and Engine layers): src/luna/platform/, src/luna/engine/; wiring in apps/odysseus/. Keep Luna game-agnostic (Charter rule 9).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Keyboard">
Given the default bindings
When I press W
Then the Move Up intent is produced
</scenario>
<scenario name="Gamepad">
Given a connected gamepad
When I push the left stick up
Then the same Move Up intent is produced
</scenario>
<scenario name="No raw keys">
Given the Game layer code
When it is reviewed
Then it only reads intents, never key codes
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-021 ..." cases. The determinism hash test starts with US-010.
Manual checks in docs/plans/US-021.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Enums, mapping tables, std::unordered_map.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-022 Draw sprites with crisp pixels
```xml
<prompt id="S-US-022" codex="1.2" milestone="M1" story="US-022" priority="Must" size="M">
<context>
Story US-022: Draw sprites with crisp pixels.
As a player, I want pixel art to stay sharp at every window size, so that the game looks right on any screen.
Epic E2 Luna engine walking skeleton: The player sees and controls a character in a window.
Traces to: ENV-11, ADR-003.
</context>
<dependencies>
Stories that must be Done: US-020.
Owner decisions that must be Decided: D-04.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Luna (Platform and Engine layers): src/luna/platform/, src/luna/engine/; wiring in apps/odysseus/. Keep Luna game-agnostic (Charter rule 9).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Integer scaling">
Given a 480 x 270 virtual screen
When the window is resized to 1920 x 1080
Then the image scales by exactly 4 with no blurred pixels
</scenario>
<scenario name="Odd size">
Given a window size that is not an exact multiple
When it is resized
Then the image uses the largest whole-number scale and letterboxes the rest
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-022 ..." cases. The determinism hash test starts with US-010.
Manual checks in docs/plans/US-022.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Interfaces with virtual functions (Renderer).</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-023 Show a tile map with a following camera
```xml
<prompt id="S-US-023" codex="1.2" milestone="M1" story="US-023" priority="Must" size="M">
<context>
Story US-023: Show a tile map with a following camera.
As a player, I want to see the world around my character, with the camera following me, so that I always know where I am.
Epic E2 Luna engine walking skeleton: The player sees and controls a character in a window.
Traces to: Architecture 7.4.
</context>
<dependencies>
Stories that must be Done: US-022.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Luna (Platform and Engine layers): src/luna/platform/, src/luna/engine/; wiring in apps/odysseus/. Keep Luna game-agnostic (Charter rule 9).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Draw">
Given a 64 x 64 test map
When the game runs
Then tiles within the camera view are drawn and tiles outside are skipped
</scenario>
<scenario name="Follow">
Given the character near the map centre
When it moves
Then the camera follows smoothly and stops at the map edges
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-023 ..." cases. The determinism hash test starts with US-010.
Manual checks in docs/plans/US-023.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: 2D grids stored in a 1D vector.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-024 Walk the character around the map
```xml
<prompt id="S-US-024" codex="1.2" milestone="M1" story="US-024" priority="Must" size="M">
<context>
Story US-024: Walk the character around the map.
As a player, I want to move my character in four directions with animation, so that I feel in control of a person in the world.
Epic E2 Luna engine walking skeleton: The player sees and controls a character in a window.
Traces to: ENV-11.
</context>
<dependencies>
Stories that must be Done: US-021, US-023.
Owner decisions that must be Decided: D-04.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game layer using Luna: src/game/ and apps/odysseus/. The character and its behaviour are Odysseus code; anything reusable it needs goes into Luna, kept game-agnostic (Charter rule 9).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Walk">
Given the character is idle
When I hold Move Right
Then it walks right with a walking animation at the configured speed
</scenario>
<scenario name="Blocked">
Given a rock tile to the right
When I walk into it
Then the character stops at the tile edge
</scenario>
<scenario name="Idle">
Given the character is walking
When I release all movement input
Then it stops and plays the idle animation facing the last direction
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-024 ..." cases. The determinism hash test starts with US-010.
Manual checks in docs/plans/US-024.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: State machines; simple collision.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M1" codex="1.2" name="Exit review M1">
<instructions>
1. Demonstrate the exit criteria: Luna opens a window; a demo character walks around a tile map at 60 FPS with crisp pixels at any window size; the build proves Luna contains no Odysseus code. For the last criterion, show the US-003 "Luna stays game-agnostic" check passing on the current code.
2. Collect evidence (test output, headless run logs, FPS logs, screenshots) into docs/gates/M1.md, one section per criterion, each marked met or not met.
3. If all are met: tag the repository m1-done and push. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the gate result.</output_format>
</prompt>
```

### M2 Console clan simulator (KILL GATE 1)
Exit criteria: The headless runner simulates a 20-person clan for 100 years without crashing; the printed chronicle is shown to 3 people and at least 2 find a story in it; the determinism test passes.

```xml
<prompt id="K-M2" codex="1.2" name="Kick off M2 Console clan simulator (KILL GATE 1)">
<instructions>
1. Confirm the previous milestone's exit review exists in docs/gates/ and passed (skip for M0).
2. Read docs/decisions.md. For every decision this milestone needs (D-01, D-02, D-03, D-13) that is not Decided, write its decision request now, all at once, so the owner can answer them in one sitting.
3. Set this milestone's stories to To do in docs/status.md in this order: US-010, US-011, US-012, US-013, US-014, US-015, US-016.
4. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, decisions requested.</output_format>
</prompt>
```

#### S-US-010 Advance a seeded world clock
```xml
<prompt id="S-US-010" codex="1.2" milestone="M2" story="US-010" priority="Must" size="M">
<context>
Story US-010: Advance a seeded world clock.
As a developer, I want the simulation to tick 20 times per second of game time with a calendar of days and seasons, so that everything in the world happens on one reliable clock.
Epic E1 Headless clan simulation: A clan lives, remembers and produces a readable chronicle, with no graphics.
Traces to: ADR-006, ADR-011, ARC-07, MVP-12.
</context>
<dependencies>
Stories that must be Done: none.
Owner decisions that must be Decided: D-01.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation layer only: src/sim/ and apps/headless/. No SDL3, no rendering (ADR-004).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Calendar">
Given a new simulation with seed 42
When it runs for one in-game year
Then the calendar has passed through four seasons and the day counter matches the configured year length
</scenario>
<scenario name="Determinism">
Given two runs with seed 42 and the same inputs
When each runs 10,000 ticks
Then both produce the same world hash
</scenario>
<scenario name="Speed control">
Given the simulation at speed 4x
When one real second passes
Then 4 seconds of game time have advanced
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-010 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-010.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: The game loop and accumulator; integers vs floats.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-011 Give every person needs that change over time
```xml
<prompt id="S-US-011" codex="1.2" milestone="M2" story="US-011" priority="Must" size="M">
<context>
Story US-011: Give every person needs that change over time.
As a player, I want clan members to get hungry, tired, cold and lonely, so that the world feels alive and people have reasons to act.
Epic E1 Headless clan simulation: A clan lives, remembers and produces a readable chronicle, with no graphics.
Traces to: SDC-01 (candidate), Overview v0.1. Note: Needs set is an assumption until D-03 is confirmed.
</context>
<dependencies>
Stories that must be Done: none.
Owner decisions that must be Decided: D-02, D-03.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation layer only: src/sim/ and apps/headless/. No SDL3, no rendering (ADR-004).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Decay">
Given a person with full needs
When one in-game day passes without eating, sleeping, warmth or company
Then each need has dropped by its configured daily rate
</scenario>
<scenario name="Satisfaction">
Given a hungry person
When they eat a meal
Then Hunger rises by the meal's value, capped at the maximum
</scenario>
<scenario name="Consequence">
Given a person whose Hunger stays at zero for the configured number of days
When the next day starts
Then they die and the chronicle records the cause
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-011 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-011.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Plain structs as components; std::vector.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-012 Let people choose what to do (utility AI)
```xml
<prompt id="S-US-012" codex="1.2" milestone="M2" story="US-012" priority="Must" size="L">
<context>
Story US-012: Let people choose what to do (utility AI).
As a player, I want each clan member to pick sensible actions based on their needs and skills, so that the clan survives without me micromanaging it.
Epic E1 Headless clan simulation: A clan lives, remembers and produces a readable chronicle, with no graphics.
Traces to: SDC-01 (candidate utility AI).
</context>
<dependencies>
Stories that must be Done: US-011.
Owner decisions that must be Decided: D-02.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation layer only: src/sim/ and apps/headless/. No SDL3, no rendering (ADR-004).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Pick best action">
Given a person who is very hungry and slightly tired
When the AI evaluates available actions
Then a food action (gather or hunt) scores highest and is chosen
</scenario>
<scenario name="No option">
Given a person with no reachable action that satisfies a need
When the AI evaluates
Then they wander or rest instead of freezing, and no error occurs
</scenario>
<scenario name="Inspectable">
Given any person
When I print their last decision in the headless runner
Then I see every action's score and the chosen one
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-012 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-012.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Functions as systems; enums; sorting.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-013 Remember events and spread gossip
```xml
<prompt id="S-US-013" codex="1.2" milestone="M2" story="US-013" priority="Must" size="M">
<context>
Story US-013: Remember events and spread gossip.
As a player, I want people to remember important events and tell others, so that reputations form and stories travel through the clan.
Epic E1 Headless clan simulation: A clan lives, remembers and produces a readable chronicle, with no graphics.
Traces to: GD-03 (candidate), SDC-01.
</context>
<dependencies>
Stories that must be Done: US-012.
Owner decisions that must be Decided: D-02.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation layer only: src/sim/ and apps/headless/. No SDL3, no rendering (ADR-004).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Memory">
Given a person who is given a gift
When the event happens
Then they store a memory with who, what, when and a feeling
</scenario>
<scenario name="Gossip">
Given two people who talk
When one has a memory the other lacks
Then the listener may gain a slightly weaker copy of that memory
</scenario>
<scenario name="Forgetting">
Given a minor memory older than its lifetime
When the day ends
Then it is forgotten; important memories are kept
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-013 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-013.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Containers of structs; references.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-014 Write a readable chronicle
```xml
<prompt id="S-US-014" codex="1.2" milestone="M2" story="US-014" priority="Must" size="M">
<context>
Story US-014: Write a readable chronicle.
As a player, I want notable events recorded as short sentences, so that I can read the clan's story and share it.
Epic E1 Headless clan simulation: A clan lives, remembers and produces a readable chronicle, with no graphics.
Traces to: NA-02 (candidate tapestry).
</context>
<dependencies>
Stories that must be Done: US-013.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation layer only: src/sim/ and apps/headless/. No SDL3, no rendering (ADR-004).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Record">
Given a birth, death, first hunt of a mammoth or a feud
When it happens
Then the chronicle adds one sentence with the in-game date, e.g. 'Spring, year 3: Ura was born to Tok and Maa.'
</scenario>
<scenario name="Filter">
Given hundreds of minor events in a year
When I print the chronicle for that year
Then only events above the importance threshold appear
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-014 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-014.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: std::string and formatting.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-015 Soak-test the simulation from the command line
```xml
<prompt id="S-US-015" codex="1.2" milestone="M2" story="US-015" priority="Must" size="S">
<context>
Story US-015: Soak-test the simulation from the command line.
As a developer, I want to run the headless simulation for N years with a seed and get a report, so that I can check stability and balance in seconds.
Epic E1 Headless clan simulation: A clan lives, remembers and produces a readable chronicle, with no graphics.
Traces to: AQ-01, NFR-03, NFR-04.
</context>
<dependencies>
Stories that must be Done: US-010, US-011, US-012, US-013, US-014.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation layer only: src/sim/ and apps/headless/. No SDL3, no rendering (ADR-004).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Run">
Given the command odysseus_headless --seed 7 --years 100
When it runs
Then it finishes without crashing and prints population, deaths by cause, average needs and tick time
</scenario>
<scenario name="Bad input">
Given --years -5
When I run it
Then it prints a usage message and exits with an error code
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-015 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-015.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: main(), command-line arguments.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-016 Save and load the simulation
```xml
<prompt id="S-US-016" codex="1.2" milestone="M2" story="US-016" priority="Must" size="M">
<context>
Story US-016: Save and load the simulation.
As a player, I want to save the world and load it later exactly as it was, so that I never lose my progress.
Epic E1 Headless clan simulation: A clan lives, remembers and produces a readable chronicle, with no graphics.
Traces to: ADR-010, NFR-05.
</context>
<dependencies>
Stories that must be Done: US-010.
Owner decisions that must be Decided: D-13.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation layer only: src/sim/ and apps/headless/. No SDL3, no rendering (ADR-004).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Round trip">
Given a simulation after 50 years
When I save and load it
Then the world hash after loading equals the hash before saving
</scenario>
<scenario name="Crash-safe">
Given a save interrupted halfway
When I load
Then the previous complete save loads and nothing crashes
</scenario>
<scenario name="Old version">
Given a save file from an earlier save version
When I load it
Then it is upgraded and loads, or a clear message explains why it cannot
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-016 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-016.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: File I/O, JSON, error handling.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M2" codex="1.2" name="Exit review M2">
<instructions>
1. Demonstrate the exit criteria: The headless runner simulates a 20-person clan for 100 years without crashing; the printed chronicle is shown to 3 people and at least 2 find a story in it; the determinism test passes.
2. Collect evidence (test output, headless run logs, FPS logs, screenshots) into docs/gates/M2.md, one section per criterion, each marked met or not met.
3. If all are met: tag the repository m2-done and push. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
4. This is a kill gate. Evaluate each success criterion below with evidence and write a recommendation (go / pivot / stop) with reasons. Criteria that need people (readers, playtesters) cannot be measured by agents: write a decision request D-GATE-M2 asking the owner for the result, and treat his answer as the gate decision.
- Chronicle interest (early): 2 of 3 readers find a story in the console chronicle (measured by: Show printed chronicle)
- Determinism: Same seed + inputs give the same world hash after 10,000 ticks (measured by: Automated test in CI)
- Kill / pivot rule: If M2 or M6 fails: stop adding content; redesign the simulation or the loop first (measured by: Owner decision, recorded)
</instructions>
<output_format>Assembly report with the gate result.</output_format>
</prompt>
```

### M3 Living clan on screen
Exit criteria: The clan from M2 runs inside the game; NPCs are visible, dressed in layered outfits, and act on their needs.

```xml
<prompt id="K-M3" codex="1.2" name="Kick off M3 Living clan on screen">
<instructions>
1. Confirm the previous milestone's exit review exists in docs/gates/ and passed (skip for M0).
2. Read docs/decisions.md. For every decision this milestone needs (D-04, D-05) that is not Decided, write its decision request now, all at once, so the owner can answer them in one sitting.
3. Set this milestone's stories to To do in docs/status.md in this order: US-030, US-032, US-031.
4. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, decisions requested.</output_format>
</prompt>
```

#### S-US-030 Compose characters from layers
```xml
<prompt id="S-US-030" codex="1.2" milestone="M3" story="US-030" priority="Must" size="M">
<context>
Story US-030: Compose characters from layers.
As a player, I want characters built from body, hair, outfit and held-item layers with colour variants, so that every person looks different and outfits can grow with progression.
Epic E3 Characters on screen: The simulated clan is visible and readable at a glance.
Traces to: ENV-12, MVP-11.
</context>
<dependencies>
Stories that must be Done: US-024.
Owner decisions that must be Decided: D-04, D-05.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Luna engine and Game layers: src/luna/engine/ (sprite composition), src/game/ (presentation of simulation state).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Compose">
Given a body, a hair style, an outfit and a spear
When the character is drawn
Then all four layers line up in every animation frame and direction
</scenario>
<scenario name="Swap">
Given a character wearing outfit A
When the outfit changes to B
Then only the outfit layer changes
</scenario>
<scenario name="Palette">
Given one hair sprite
When three palettes are applied
Then three distinct hair colours appear without new art
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-030 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-030.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Composition over inheritance.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-032 See the simulated clan on screen
```xml
<prompt id="S-US-032" codex="1.2" milestone="M3" story="US-032" priority="Must" size="L">
<context>
Story US-032: See the simulated clan on screen.
As a player, I want clan members from the simulation to appear and act in the world, so that I watch the clan live its life.
Epic E3 Characters on screen: The simulated clan is visible and readable at a glance.
Traces to: ADR-004, ADR-005, ARC-05.
</context>
<dependencies>
Stories that must be Done: US-012, US-024.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Luna engine and Game layers: src/luna/engine/ (sprite composition), src/game/ (presentation of simulation state).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Mirror">
Given 20 people in the simulation
When the game renders
Then 20 characters appear at their simulated positions
</scenario>
<scenario name="Smooth">
Given the simulation ticking at 20 Hz
When characters move
Then their movement is interpolated and looks smooth at 60 FPS
</scenario>
<scenario name="Separation">
Given the Simulation layer
When the build is checked
Then it contains no rendering code (US-003 still passes)
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-032 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-032.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: EnTT registry and views; interpolation.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-031 Read people's state at a glance
```xml
<prompt id="S-US-031" codex="1.2" milestone="M3" story="US-031" priority="Should" size="S">
<context>
Story US-031: Read people's state at a glance.
As a player, I want to see how people feel from their posture and small emote icons, so that I understand the clan without opening menus.
Epic E3 Characters on screen: The simulated clan is visible and readable at a glance.
Traces to: UX-01 (candidate).
</context>
<dependencies>
Stories that must be Done: US-011, US-032.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Luna engine and Game layers: src/luna/engine/ (sprite composition), src/game/ (presentation of simulation state).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Emote">
Given a person whose Warmth is critical
When they are on screen
Then a shiver animation or cold emote is visible
</scenario>
<scenario name="Details on demand">
Given any person
When I hover or click them
Then a panel shows exact need values and current action
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-031 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-031.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Mapping data to presentation.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M3" codex="1.2" name="Exit review M3">
<instructions>
1. Demonstrate the exit criteria: The clan from M2 runs inside the game; NPCs are visible, dressed in layered outfits, and act on their needs.
2. Collect evidence (test output, headless run logs, FPS logs, screenshots) into docs/gates/M3.md, one section per criterion, each marked met or not met.
3. If all are met: tag the repository m3-done and push. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the gate result.</output_format>
</prompt>
```

### M4 Region, tools and saves
Exit criteria: A region is generated from a seed with biomes, resources and two rival clans; any NPC can be inspected; the game saves and loads.

```xml
<prompt id="K-M4" codex="1.2" name="Kick off M4 Region, tools and saves">
<instructions>
1. Confirm the previous milestone's exit review exists in docs/gates/ and passed (skip for M0).
2. Read docs/decisions.md. For every decision this milestone needs (D-13) that is not Decided, write its decision request now, all at once, so the owner can answer them in one sitting.
3. Set this milestone's stories to To do in docs/status.md in this order: US-040, US-041, US-042, US-043, US-080, US-083.
4. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, decisions requested.</output_format>
</prompt>
```

#### S-US-040 Generate a region from a seed
```xml
<prompt id="S-US-040" codex="1.2" milestone="M4" story="US-040" priority="Must" size="L">
<context>
Story US-040: Generate a region from a seed.
As a player, I want each new game to create a different region with steppe, forest, river and caves, so that every playthrough feels new.
Epic E4 Procedural region: Every new game has a different, believable region to live in.
Traces to: ENV-03, ARC-06, ADR-009, MVP-03.
</context>
<dependencies>
Stories that must be Done: US-023.
Owner decisions that must be Decided: D-13.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/worldgen/) for generation and clans; Luna engine (src/luna/engine/) for chunk streaming.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Variety">
Given seeds 1 and 2
When regions are generated
Then they differ in layout and biome placement
</scenario>
<scenario name="Repeatable">
Given seed 1 twice
When regions are generated
Then they are identical tile for tile
</scenario>
<scenario name="Playable">
Given any seed
When a region is generated
Then the start area is reachable, has water and food within 20 tiles, and generation takes under 10 seconds
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-040 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-040.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Noise functions; seeded random numbers.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-041 Place resources by biome
```xml
<prompt id="S-US-041" codex="1.2" milestone="M4" story="US-041" priority="Must" size="M">
<context>
Story US-041: Place resources by biome.
As a player, I want flint, wood, berries and animals placed where they make sense, so that exploring the land matters.
Epic E4 Procedural region: Every new game has a different, believable region to live in.
Traces to: Overview v0.1 (materials).
</context>
<dependencies>
Stories that must be Done: US-040.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/worldgen/) for generation and clans; Luna engine (src/luna/engine/) for chunk streaming.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Biome rules">
Given a generated region
When resources are placed
Then flint appears near rivers and caves, berries in forest edges, herds on the steppe
</scenario>
<scenario name="Regrowth">
Given a berry bush that was harvested
When the configured days pass in the right season
Then it regrows
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-041 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-041.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Data tables from JSON.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-042 Spawn rival clans
```xml
<prompt id="S-US-042" codex="1.2" milestone="M4" story="US-042" priority="Must" size="M">
<context>
Story US-042: Spawn rival clans.
As a player, I want two AI clans living elsewhere in the region, so that there are others to trade with, convert, or compete against.
Epic E4 Procedural region: Every new game has a different, believable region to live in.
Traces to: ARC-05, MVP-10.
</context>
<dependencies>
Stories that must be Done: US-012, US-040.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/worldgen/) for generation and clans; Luna engine (src/luna/engine/) for chunk streaming.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Spawn">
Given a new region
When the game starts
Then two rival clans of 10-20 people exist at least 60 tiles from the player's camp
</scenario>
<scenario name="Autonomy">
Given rival clans
When a year passes
Then they gather, move camp and grow or shrink on their own
</scenario>
<scenario name="LOD">
Given a rival clan far from the player
When it is simulated
Then it runs at the Nearby tier rate (1 tick/s)
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-042 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-042.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Reusing systems for different data.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-043 Stream chunks and save only changes
```xml
<prompt id="S-US-043" codex="1.2" milestone="M4" story="US-043" priority="Should" size="M">
<context>
Story US-043: Stream chunks and save only changes.
As a developer, I want the region stored as a seed plus changed chunks, so that saves stay small and loading is fast.
Epic E4 Procedural region: Every new game has a different, believable region to live in.
Traces to: ARC-06, ADR-009.
</context>
<dependencies>
Stories that must be Done: US-016, US-040.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/worldgen/) for generation and clans; Luna engine (src/luna/engine/) for chunk streaming.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Streaming">
Given the player walking across the region
When they approach unloaded chunks
Then chunks load before they become visible with no stutter over 50 ms
</scenario>
<scenario name="Delta save">
Given a region where 3 chunks were changed
When the game is saved
Then only those 3 chunks are stored besides the seed
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-043 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-043.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Hashing chunk coordinates.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-080 Autosave and keep backups
```xml
<prompt id="S-US-080" codex="1.2" milestone="M4" story="US-080" priority="Must" size="M">
<context>
Story US-080: Autosave and keep backups.
As a player, I want the game to autosave and keep backups, so that I never lose more than a few minutes.
Epic E8 Saves, settings and quality: The game is stable, fast, and never loses progress.
Traces to: ADR-010, NFR-05.
</context>
<dependencies>
Stories that must be Done: US-016.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Across layers; saves in src/sim/save/, settings and overlays in src/game/ and src/luna/engine/.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Autosave">
Given the game is running
When an in-game day ends
Then it autosaves with under 200 ms of stutter
</scenario>
<scenario name="Backups">
Given four saves have been written
When I look in the save folder
Then the latest save plus 3 backups exist
</scenario>
<scenario name="Corrupt">
Given the latest save is corrupted
When I load
Then the newest valid backup loads and a message explains what happened
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-080 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-080.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Atomic file writes.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-083 Inspect and control the world with dev tools
```xml
<prompt id="S-US-083" codex="1.2" milestone="M4" story="US-083" priority="Should" size="M">
<context>
Story US-083: Inspect and control the world with dev tools.
As a developer, I want Dear ImGui panels to inspect any person and control time, so that I can understand and tune the simulation live.
Epic E8 Saves, settings and quality: The game is stable, fast, and never loses progress.
Traces to: ADR-015, learning stage 8.
</context>
<dependencies>
Stories that must be Done: US-032.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Across layers; saves in src/sim/save/, settings and overlays in src/game/ and src/luna/engine/.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Inspect">
Given the dev tools open
When I click a person
Then their needs, memories, relationships and AI scores are shown
</scenario>
<scenario name="Time">
Given the dev tools
When I set speed to 16x or skip a day
Then the simulation advances accordingly
</scenario>
<scenario name="Release">
Given a Release build
When I press the dev-tools key
Then nothing opens
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-083 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-083.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Immediate-mode UI.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M4" codex="1.2" name="Exit review M4">
<instructions>
1. Demonstrate the exit criteria: A region is generated from a seed with biomes, resources and two rival clans; any NPC can be inspected; the game saves and loads.
2. Collect evidence (test output, headless run logs, FPS logs, screenshots) into docs/gates/M4.md, one section per criterion, each marked met or not met.
3. If all are met: tag the repository m4-done and push. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the gate result.</output_format>
</prompt>
```

### M5 Vertical slice feature-complete
Exit criteria: A player can start a new game, live the Growing Period, take a profession, pursue Trade or Religion, and win or lose.

```xml
<prompt id="K-M5" codex="1.2" name="Kick off M5 Vertical slice feature-complete">
<instructions>
1. Confirm the previous milestone's exit review exists in docs/gates/ and passed (skip for M0).
2. Read docs/decisions.md. For every decision this milestone needs (D-06, D-07, D-08, D-09, D-10, D-11) that is not Decided, write its decision request now, all at once, so the owner can answer them in one sitting.
3. Set this milestone's stories to To do in docs/status.md in this order: US-050, US-053, US-051, US-052, US-054, US-060, US-061, US-062, US-063, US-070, US-055, US-071, US-072, US-073, US-081, US-082.
4. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, decisions requested.</output_format>
</prompt>
```

#### S-US-050 Start a new game
```xml
<prompt id="S-US-050" codex="1.2" milestone="M5" story="US-050" priority="Must" size="M">
<context>
Story US-050: Start a new game.
As a player, I want to choose a seed, a Growing Period preset and a Comfort level before starting, so that I can shape the kind of game I want.
Epic E5 Life and the Growing Period: The player's choices from 12 to 26 shape who the hero becomes.
Traces to: CHR-03, CHR-06, ENV-07, MVP-05.
</context>
<dependencies>
Stories that must be Done: US-040.
Owner decisions that must be Decided: D-07.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/life/) for rules; Game (src/game/ui/) for screens; data in assets/data/. mraw-designer writes the content data (events, texts) within Decided design.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Defaults">
Given the New Game screen
When I press Start without changing anything
Then a random seed, Full Growing Period and Balanced comfort are used, and my hero starts at age 12
</scenario>
<scenario name="Seed">
Given I type seed 1234
When I start two games with it
Then both regions and heroes are identical
</scenario>
<scenario name="Preset Off">
Given Growing Period set to Off
When I start
Then the hero starts at 26 with affinities generated from the origin
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-050 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-050.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: UI state and screens.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-053 Weight choices by age (imprint curve)
```xml
<prompt id="S-US-053" codex="1.2" milestone="M5" story="US-053" priority="Must" size="S">
<context>
Story US-053: Weight choices by age (imprint curve).
As a developer, I want Path Affinity gains multiplied by the imprint curve, so that early choices shape the hero most, as designed.
Epic E5 Life and the Growing Period: The player's choices from 12 to 26 shape who the hero becomes.
Traces to: CHR-05, CHR-06, ARC-08.
</context>
<dependencies>
Stories that must be Done: US-050.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/life/) for rules; Game (src/game/ui/) for screens; data in assets/data/. mraw-designer writes the content data (events, texts) within Decided design.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Curve">
Given default tunables
When a decision is made at 12, 19 and 26
Then the multipliers are 3.00, 1.75 and 0.50
</scenario>
<scenario name="After 26">
Given an adult hero
When a decision is made
Then the settled multiplier 0.25 applies
</scenario>
<scenario name="Tunable">
Given a designer changes the peak to 4.0 in the data file
When the game restarts
Then the new curve is used with no C++ change
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-053 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-053.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Pure functions and unit tests.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-051 Choose how to spend each year of youth
```xml
<prompt id="S-US-051" codex="1.2" milestone="M5" story="US-051" priority="Must" size="M">
<context>
Story US-051: Choose how to spend each year of youth.
As a player, I want to pick Focus activities each in-game year, so that I decide what kind of person my hero becomes.
Epic E5 Life and the Growing Period: The player's choices from 12 to 26 shape who the hero becomes.
Traces to: CHR-05, Growing Period sheet.
</context>
<dependencies>
Stories that must be Done: US-050, US-053.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/life/) for rules; Game (src/game/ui/) for screens; data in assets/data/. mraw-designer writes the content data (events, texts) within Decided design.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Choose">
Given a new year in the Growing Period
When the Focus screen opens
Then I can choose 2 activities (e.g. Hunt with elders, Tend the fire, Wander)
</scenario>
<scenario name="Effect">
Given I chose Tend the fire
When the year ends
Then Fire-keeping skill and Religion affinity increase by base x imprint multiplier
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-051 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-051.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Data-driven choices.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-052 Face Crossroads events
```xml
<prompt id="S-US-052" codex="1.2" milestone="M5" story="US-052" priority="Must" size="L">
<context>
Story US-052: Face Crossroads events.
As a player, I want story decisions with lasting consequences during youth, so that my choices feel meaningful.
Epic E5 Life and the Growing Period: The player's choices from 12 to 26 shape who the hero becomes.
Traces to: CHR-05, NA-03 (candidate).
</context>
<dependencies>
Stories that must be Done: US-051.
Owner decisions that must be Decided: D-11.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/life/) for rules; Game (src/game/ui/) for screens; data in assets/data/. mraw-designer writes the content data (events, texts) within Decided design.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Trigger">
Given the Growing Period in progress
When a year passes
Then one Crossroads event appears, chosen from those whose conditions match
</scenario>
<scenario name="Consequence">
Given I choose an option
When the event resolves
Then its effects apply (affinity, relationship, trait) and the chronicle records it
</scenario>
<scenario name="Content">
Given the MVP data files
When they are counted
Then there are at least 10 Crossroads events, each with 2-3 options
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-052 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-052.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Loading and validating content from JSON.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-054 Reveal the specialty at the Mantle moment
```xml
<prompt id="S-US-054" codex="1.2" milestone="M5" story="US-054" priority="Must" size="S">
<context>
Story US-054: Reveal the specialty at the Mantle moment.
As a player, I want my hero's specialty revealed at 26, so that I see the result of my youth.
Epic E5 Life and the Growing Period: The player's choices from 12 to 26 shape who the hero becomes.
Traces to: CHR-04, CHR-05.
</context>
<dependencies>
Stories that must be Done: US-053.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/life/) for rules; Game (src/game/ui/) for screens; data in assets/data/. mraw-designer writes the content data (events, texts) within Decided design.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Reveal">
Given the hero turns 26
When the Mantle event runs
Then the top one or two affinities become the Specialty, shown on a summary screen and in the chronicle
</scenario>
<scenario name="Tie">
Given two affinities equal at the top
When the Mantle event runs
Then both become the Specialty
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-054 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-054.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Sorting and ties.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-060 Define the five Age 1 professions as data
```xml
<prompt id="S-US-060" codex="1.2" milestone="M5" story="US-060" priority="Must" size="S">
<context>
Story US-060: Define the five Age 1 professions as data.
As a designer (data editor), I want professions defined in JSON: skills, tools, actions, related pillar, so that content can grow without code changes.
Epic E6 Professions and interactions: The player works, crafts and learns a trade in the world.
Traces to: CHR-07, ARC-08, MVP-07.
</context>
<dependencies>
Stories that must be Done: US-016.
Owner decisions that must be Decided: D-09.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/work/) for professions, crafting, skills; Game for the context menu; data in assets/data/. mraw-designer writes professions and recipes as data.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Load">
Given professions.json with Hunter, Gatherer, Flint-knapper, Fire-keeper, Shaman-healer
When the game starts
Then all five appear in-game
</scenario>
<scenario name="Validation">
Given a profession referencing a missing tool
When the game starts
Then a clear error names the file, the profession and the missing tool
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-060 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-060.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: JSON parsing into structs.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-061 Interact with things through a context menu
```xml
<prompt id="S-US-061" codex="1.2" milestone="M5" story="US-061" priority="Must" size="L">
<context>
Story US-061: Interact with things through a context menu.
As a player, I want to click a thing and pick from the actions it offers, so that I can do many things with everything in the world.
Epic E6 Professions and interactions: The player works, crafts and learns a trade in the world.
Traces to: INT-01, GD-04 (candidate properties + verbs), UX-02.
</context>
<dependencies>
Stories that must be Done: US-024.
Owner decisions that must be Decided: D-08.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/work/) for professions, crafting, skills; Game for the context menu; data in assets/data/. mraw-designer writes professions and recipes as data.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Offer">
Given a berry bush and a player with empty hands
When I click the bush
Then a radial menu offers Gather and Inspect
</scenario>
<scenario name="Tool-gated">
Given a flint nodule and no hammerstone
When I click it
Then Knap is shown but disabled with the reason 'Needs a hammerstone'
</scenario>
<scenario name="Perform">
Given I choose Gather
When the action completes
Then the item enters my inventory and the bush shows as harvested
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-061 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-061.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Querying components; the Command pattern.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-062 Craft at a workstation
```xml
<prompt id="S-US-062" codex="1.2" milestone="M5" story="US-062" priority="Must" size="M">
<context>
Story US-062: Craft at a workstation.
As a player, I want to turn materials into tools at a knapping stone or fire, so that my work produces useful things.
Epic E6 Professions and interactions: The player works, crafts and learns a trade in the world.
Traces to: GD-02 (candidate provenance).
</context>
<dependencies>
Stories that must be Done: US-060, US-061.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/work/) for professions, crafting, skills; Game for the context menu; data in assets/data/. mraw-designer writes professions and recipes as data.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Craft">
Given 2 flint and a hammerstone at a knapping stone
When I craft a spearhead
Then the flint is consumed and a spearhead is created
</scenario>
<scenario name="Quality">
Given Flint-knapping skill 1 vs 5
When each crafts a spearhead
Then the higher skill gives a higher quality tier more often
</scenario>
<scenario name="Provenance">
Given a crafted item
When I inspect it
Then it shows who made it, from what, and when
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-062 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-062.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Recipes as data.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-063 Learn a trade from a master
```xml
<prompt id="S-US-063" codex="1.2" milestone="M5" story="US-063" priority="Should" size="M">
<context>
Story US-063: Learn a trade from a master.
As a player, I want to apprentice with an NPC master once they trust me, so that skills and relationships grow together.
Epic E6 Professions and interactions: The player works, crafts and learns a trade in the world.
Traces to: CHR-07, SDC-01 (candidate).
</context>
<dependencies>
Stories that must be Done: US-013, US-060.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/work/) for professions, crafting, skills; Game for the context menu; data in assets/data/. mraw-designer writes professions and recipes as data.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Gated">
Given a master whose relationship with me is below the threshold
When I ask to apprentice
Then they refuse and say why
</scenario>
<scenario name="Learn">
Given an accepted apprenticeship
When I work beside the master
Then my skill in that profession grows faster than alone
</scenario>
<scenario name="Skill use">
Given any skill
When I use it successfully
Then it gains experience
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-063 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-063.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Relationships as data.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-070 Track regional dominion
```xml
<prompt id="S-US-070" codex="1.2" milestone="M5" story="US-070" priority="Must" size="M">
<context>
Story US-070: Track regional dominion.
As a player, I want a Dominion Meter showing my clan's share of the region in Trade and Religion, so that I know how close I am to leading the region.
Epic E7 Pillars and regional dominion: The player can lead the region through Trade or Religion.
Traces to: ENV-10, MVP-08.
</context>
<dependencies>
Stories that must be Done: US-042.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/society/) for dominion, trade, religion; Game for panels and end screens. mraw-designer tunes values as data; no new rules.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Display">
Given the Dominion panel
When I open it
Then Trade and Religion show percentages; Military, Politics, Technology and Ideology show 'later Ages'
</scenario>
<scenario name="Update">
Given a trade or conversion happens
When the day ends
Then the relevant percentage updates
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-070 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-070.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Aggregating data across entities.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-055 Age, die, and end the run
```xml
<prompt id="S-US-055" codex="1.2" milestone="M5" story="US-055" priority="Must" size="M">
<context>
Story US-055: Age, die, and end the run.
As a player, I want my hero to age and eventually die, ending the run with a summary, so that every life has an arc and an ending.
Epic E5 Life and the Growing Period: The player's choices from 12 to 26 shape who the hero becomes.
Traces to: CHR-02 (One Hero per Age), MVP-04, MVP-09.
</context>
<dependencies>
Stories that must be Done: US-054, US-070.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/life/) for rules; Game (src/game/ui/) for screens; data in assets/data/. mraw-designer writes the content data (events, texts) within Decided design.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Aging">
Given a hero at 40
When years pass
Then age affects energy decay by the configured amount
</scenario>
<scenario name="Death">
Given the hero dies
When the death is processed
Then the run ends with a summary: life events from the chronicle, specialty, and Dominion reached
</scenario>
<scenario name="Early death">
Given the hero dies before 26
When the run ends
Then it counts as a loss (MVP-09)
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-055 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-055.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Game states.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-071 Barter with rival clans
```xml
<prompt id="S-US-071" codex="1.2" milestone="M5" story="US-071" priority="Must" size="L">
<context>
Story US-071: Barter with rival clans.
As a player, I want to exchange goods with other clans and track debts, so that Trade becomes a path to leadership.
Epic E7 Pillars and regional dominion: The player can lead the region through Trade or Religion.
Traces to: PIL-02, MVP-08.
</context>
<dependencies>
Stories that must be Done: US-042, US-070.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/society/) for dominion, trade, religion; Game for panels and end screens. mraw-designer tunes values as data; no new rules.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Barter">
Given my clan has 10 furs and a rival values furs
When I offer 5 furs for 3 flint tools
Then the rival accepts or counters based on its needs
</scenario>
<scenario name="Debt">
Given a rival accepts goods now for payment later
When the deal closes
Then a debt is recorded and counts toward my Trade dominion
</scenario>
<scenario name="Default">
Given a debt past its due date
When the day ends
Then the rival's relationship and my Trade dominion react as configured
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-071 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-071.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Integer arithmetic for goods (no floats).</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-072 Found a sacred fire and gather followers
```xml
<prompt id="S-US-072" codex="1.2" milestone="M5" story="US-072" priority="Must" size="L">
<context>
Story US-072: Found a sacred fire and gather followers.
As a player, I want to found a named sacred fire and hold rituals that attract followers, so that Religion becomes a path to leadership.
Epic E7 Pillars and regional dominion: The player can lead the region through Trade or Religion.
Traces to: PIL-04, MVP-08.
</context>
<dependencies>
Stories that must be Done: US-060, US-070.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/society/) for dominion, trade, religion; Game for panels and end screens. mraw-designer tunes values as data; no new rules.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Found">
Given Shaman-healer specialty or Fire-keeping skill 3+
When I found a sacred fire and name it
Then it appears on the map with my custom name
</scenario>
<scenario name="Ritual">
Given a sacred fire
When I hold a ritual with 5+ attendees
Then attendees' faith grows and some rivals may convert over time
</scenario>
<scenario name="Neglect">
Given a sacred fire untended for the configured days
When time passes
Then it goes out and followers lose faith
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-072 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-072.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Events and timers.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-073 Win by leading the region
```xml
<prompt id="S-US-073" codex="1.2" milestone="M5" story="US-073" priority="Must" size="S">
<context>
Story US-073: Win by leading the region.
As a player, I want to win when my clan dominates the region, so that the run has a satisfying goal.
Epic E7 Pillars and regional dominion: The player can lead the region through Trade or Religion.
Traces to: ENV-05, MVP-09.
</context>
<dependencies>
Stories that must be Done: US-070, US-071, US-072.
Owner decisions that must be Decided: D-10.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation (src/sim/society/) for dominion, trade, religion; Game for panels and end screens. mraw-designer tunes values as data; no new rules.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Win">
Given Trade or Religion dominion reaches 60%, or combined reaches 50%
When the day ends
Then a victory screen shows the chronicle highlights and my specialty
</scenario>
<scenario name="Lose">
Given my clan drops below 3 members
When the day ends
Then the run ends as a loss with a summary
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-073 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-073.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Game-over states.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-081 Change basic settings
```xml
<prompt id="S-US-081" codex="1.2" milestone="M5" story="US-081" priority="Should" size="S">
<context>
Story US-081: Change basic settings.
As a player, I want to set window mode, resolution and volume, so that the game fits my screen and ears.
Epic E8 Saves, settings and quality: The game is stable, fast, and never loses progress.
Traces to: Architecture 7.8.
</context>
<dependencies>
Stories that must be Done: US-020.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Across layers; saves in src/sim/save/, settings and overlays in src/game/ and src/luna/engine/.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Apply">
Given the Settings screen
When I switch to fullscreen
Then the change applies immediately and is remembered next launch
</scenario>
<scenario name="Reset">
Given invalid values in settings.json
When the game starts
Then defaults are used and the file is rewritten
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-081 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-081.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Reading and writing settings.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-082 Keep within the performance budget
```xml
<prompt id="S-US-082" codex="1.2" milestone="M5" story="US-082" priority="Must" size="S">
<context>
Story US-082: Keep within the performance budget.
As a developer, I want an FPS and tick-time overlay and a performance test, so that I notice slowdowns the day I cause them.
Epic E8 Saves, settings and quality: The game is stable, fast, and never loses progress.
Traces to: NFR-03.
</context>
<dependencies>
Stories that must be Done: US-032, US-042.
Owner decisions that must be Decided: D-06.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Across layers; saves in src/sim/save/, settings and overlays in src/game/ and src/luna/engine/.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Overlay">
Given the Debug overlay enabled
When the game runs
Then FPS, frame time and simulation tick time are shown
</scenario>
<scenario name="Budget">
Given 500 simulated agents in the region on the minimum PC
When the game runs for 5 minutes
Then it averages 60 FPS and ticks stay under 10 ms
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-082 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-082.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Profiling with Tracy.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M5" codex="1.2" name="Exit review M5">
<instructions>
1. Demonstrate the exit criteria: A player can start a new game, live the Growing Period, take a profession, pursue Trade or Religion, and win or lose.
2. Collect evidence (test output, headless run logs, FPS logs, screenshots) into docs/gates/M5.md, one section per criterion, each marked met or not met.
3. If all are met: tag the repository m5-done and push. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the gate result.</output_format>
</prompt>
```

### M6 Playtest and go/no-go (KILL GATE 2)
Exit criteria: 8 outside playtesters play; success criteria measured; go/no-go decision recorded.

```xml
<prompt id="K-M6" codex="1.2" name="Kick off M6 Playtest and go/no-go (KILL GATE 2)">
<instructions>
1. Confirm the previous milestone's exit review exists in docs/gates/ and passed (skip for M0).
2. Read docs/decisions.md. For every decision this milestone needs (D-11) that is not Decided, write its decision request now, all at once, so the owner can answer them in one sitting.
3. Set this milestone's stories to To do in docs/status.md in this order: US-090, US-091, US-092.
4. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, decisions requested.</output_format>
</prompt>
```

#### S-US-090 Learn the game in the first ten minutes
```xml
<prompt id="S-US-090" codex="1.2" milestone="M6" story="US-090" priority="Should" size="M">
<context>
Story US-090: Learn the game in the first ten minutes.
As a playtester, I want a clan elder to guide me through my first day, so that I can play without reading a manual.
Epic E9 Playtest readiness: Outside players can install, learn and play the slice.
Traces to: UX, NA (story).
</context>
<dependencies>
Stories that must be Done: US-061.
Owner decisions that must be Decided: D-11.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game layer and packaging: src/game/tutorial/, CMake install and packaging rules. mraw-designer writes the tutorial script as data.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Guide">
Given a new game with tutorial on
When I start
Then the elder prompts me to gather, eat, and tend the fire in order, one step at a time
</scenario>
<scenario name="Skip">
Given tutorial off in New Game
When I start
Then no prompts appear
</scenario>
<scenario name="Stuck">
Given I have not completed a prompt for 2 minutes
When time passes
Then the elder gives a hint
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-090 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-090.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Scripted sequences as data.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-091 Install and run the playtest build
```xml
<prompt id="S-US-091" codex="1.2" milestone="M6" story="US-091" priority="Must" size="S">
<context>
Story US-091: Install and run the playtest build.
As a playtester, I want a zip I can unpack and run, so that I can try the game without technical help.
Epic E9 Playtest readiness: Outside players can install, learn and play the slice.
Traces to: PLT-01.
</context>
<dependencies>
Stories that must be Done: US-004, US-080.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game layer and packaging: src/game/tutorial/, CMake install and packaging rules. mraw-designer writes the tutorial script as data.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Run">
Given a Windows 10/11 x64 PC without developer tools
When I unzip and run odysseus.exe
Then the game starts with no missing-DLL errors
</scenario>
<scenario name="Crash">
Given a crash during play
When it happens
Then a crash log and the last save are written to the per-user folder for sending
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-091 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-091.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Release builds and packaging.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-092 Record local session statistics
```xml
<prompt id="S-US-092" codex="1.2" milestone="M6" story="US-092" priority="Should" size="S">
<context>
Story US-092: Record local session statistics.
As a developer, I want an opt-in local log of session length and key events, so that I can measure the MVP success criteria honestly.
Epic E9 Playtest readiness: Outside players can install, learn and play the slice.
Traces to: MVP success criteria, ARC-04.
</context>
<dependencies>
Stories that must be Done: US-091.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game layer and packaging: src/game/tutorial/, CMake install and packaging rules. mraw-designer writes the tutorial script as data.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Opt-in">
Given first launch
When the game asks about session statistics
Then nothing is recorded unless I agree
</scenario>
<scenario name="Local only">
Given statistics enabled
When a session ends
Then a file with play time and key events is written locally and nothing is sent anywhere
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug: all tests pass, including the "US-092 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-092.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Privacy by design.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M6" codex="1.2" name="Exit review M6">
<instructions>
1. Demonstrate the exit criteria: 8 outside playtesters play; success criteria measured; go/no-go decision recorded.
2. Collect evidence (test output, headless run logs, FPS logs, screenshots) into docs/gates/M6.md, one section per criterion, each marked met or not met.
3. If all are met: tag the repository m6-done and push. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
4. This is a kill gate. Evaluate each success criterion below with evidence and write a recommendation (go / pivot / stop) with reasons. Criteria that need people (readers, playtesters) cannot be measured by agents: write a decision request D-GATE-M6 asking the owner for the result, and treat his answer as the gate decision.
- Voluntary play time: 5 of 8 playtesters play 30+ minutes without being asked to continue (measured by: Observation + local session log (US-092))
- Emergent story: 3 of 8 can retell a story that came from the simulation, not from a script (measured by: Post-play interview)
- Stability: No crash in a 2-hour play session or a 100-year soak test (measured by: Soak test + crash log)
- Kill / pivot rule: If M2 or M6 fails: stop adding content; redesign the simulation or the loop first (measured by: Owner decision, recorded)
</instructions>
<output_format>Assembly report with the gate result.</output_format>
</prompt>
```

### Execution order
P-000 -> P-001 -> K-M0 -> S-US-001 -> S-US-002 -> S-US-003 -> S-US-004 -> X-M0 -> K-M1 -> S-US-020 -> S-US-021 -> S-US-022 -> S-US-023 -> S-US-024 -> X-M1 -> K-M2 -> S-US-010 -> S-US-011 -> S-US-012 -> S-US-013 -> S-US-014 -> S-US-015 -> S-US-016 -> X-M2 -> K-M3 -> S-US-030 -> S-US-032 -> S-US-031 -> X-M3 -> K-M4 -> S-US-040 -> S-US-041 -> S-US-042 -> S-US-043 -> S-US-080 -> S-US-083 -> X-M4 -> K-M5 -> S-US-050 -> S-US-053 -> S-US-051 -> S-US-052 -> S-US-054 -> S-US-060 -> S-US-061 -> S-US-062 -> S-US-063 -> S-US-070 -> S-US-055 -> S-US-071 -> S-US-072 -> S-US-073 -> S-US-081 -> S-US-082 -> X-M5 -> K-M6 -> S-US-090 -> S-US-091 -> S-US-092 -> X-M6

## 8. State files
A fresh session resumes from these files only (A-001), never from chat history.

| File | Holds | Written by |
|---|---|---|
| docs/status.md | Progress: every prompt in Codex order with status (To do / In progress / Blocked / Done / Failed) and last report date. | mraw-orchestrator |
| docs/decisions.md | Owner decisions D-xx with status and the owner's answer; the only way agents learn a decision. | Owner (answers); orchestrator (seeds) |
| docs/decision-requests/ | One file per open question for the owner. | Any agent |
| Test results | doctest cases named 'US-xxx <scenario>'; ctest output is the test state; the determinism hash test runs every time. | mraw-tester |
| docs/plans/US-xxx.md | Plan and manual-check results per story. | mraw-architect, mraw-tester |
| docs/codex-issues.md | Problems with the Codex, for Anima. | Any agent |
| docs/learning-journal.md | Teach-back entries for the owner. | mraw-writer |
| git log and tags | Merged stories (US-xxx commits) and finished milestones (mx-done). | mraw-orchestrator |

## 9. Amendment log
| Version | Date | Change |
|---|---|---|
| 1.0 | 2026-09-29 | First Codex, from Mraw's Build Brief (source of truth v1.2). |
| 1.1 | 2026-09-29 | Aligned with Anima's canonical Codex format: verification section and completion condition in every story prompt, state files section, target models. US-053 no longer depends on US-051 (removed a dependency cycle). |
| 1.2 | 2026-09-29 | Luna first (source of truth v1.4, ARC-09): M1 is now the Luna engine walking skeleton (K-M1, S-US-020..S-US-024, X-M1) and M2 the console clan simulator with Kill Gate 1 (K-M2, S-US-010..S-US-016, X-M2); Platform and Engine layers live in src/luna/ (targets luna_platform and luna_engine, namespaces luna::platform and luna::engine); Charter rule 9 keeps Luna game-agnostic; US-003 gains the "Luna stays game-agnostic" scenario; US-024 is Game code that uses Luna; new P-001 migrates v1.1 workspaces. Codex issues resolved: CI-001 (P-000 commits before the toolchain check), CI-002 (no split needed, D-12 Decided 2026-09-29), CI-003 (DoD: CI green from US-002 on, determinism from US-010 on; M0 and M1 verification no longer ask for the determinism test). Owner decisions recorded: D-04 (32x48 px, 8 directions), D-12, D-13. |
