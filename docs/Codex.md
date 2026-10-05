# Project Odyssey Codex v2.13

Author: **Anima** (Prompt Architect) for **Mraw** (Dominus Full Team / Dominus Avengers) | Date: 2026-10-05 | Source of truth: Project Odyssey.docx v2.11 (chapter 12: MVP; chapter 7: architecture) | Executor: autonomous AI coding agents (the strongest available model for orchestrator, architect and acceptor; any current model for the others) | Human gate: kill-gate results that need people, accounts, credentials and money; design decisions are taken by the owner in chat (D-22)

## 0. How to use this Codex

- Phase 3 of the Amek workflow: Mraw assembles Project Odyssey by executing these prompts **in order**, exactly as written.
- Start: put this file in an empty folder `odysseus/`, open your AI coding session there, paste **A-000**. Every later session: paste **A-001**.
- P-000 turns the Charter into `CLAUDE.md` and the role prompts into `.claude/agents/`, so every agent loads them automatically.
- One story in progress at a time. Every session ends with an assembly report.
- Who does what (owner instruction, 2026-09-30): Dominus, wearing every Mraw hat, designs, implements and tests the game; Anima alone writes and amends this Codex. Mraw never edits docs/Codex.md; problems go to Anima as codex issues.
- Continuous assembly: the owner wants the MVP as fast as quality allows, engine first. After each prompt, continue with the next one in Codex order without waiting, until a Charter human gate or the end of the session.
- Autonomous by default (owner instruction, 2026-09-30): the owner wants the game built with minimal intervention. Design decisions are delegated to Dominus, stories integrate through the `qa` branch, and a Milestone-<n>.md progress snapshot is saved after every story, so the owner can review everything later. The owner answers questions up front (before leaving the team to work), not during the run.
- Design decisions from v2.0 on (owner, 2026-10-01, D-35): the owner takes them (D-22, Charter human gate 3). The delegated period the owner granted on 2026-10-01 for M3-M6 (D-30..D-33) ended with M6. Each kickoff K-Mx asks the owner its open design questions in one chat round before its first story.
- M10b (owner, 2026-10-05, D-58) is built after X-M10 and before K-M11 under the owner-decides rules (D-22): the owner answers its kickoff questions in one chat round, every story runs the full verification with green CI on qa, and X-M10b ends with a 10-minute owner walkthrough.
- M8b-M8e (owner, 2026-10-01, D-43) are built right after M8 and before M9, under the M7-M9 rules (D-35): the owner answers each kickoff's design questions in one chat round, and every story runs the full verification with green CI on qa.
- M9a NPC foundation, M9b Trade economy and M9c NPC life (owner, 2026-10-04, D-52) are built right after M8c and before M8d, under the M7-M9 rules (D-35). The owner answered their design questions on 2026-10-04 (docs/decision-requests/D-52.md).
- M10-M14 (owner, 2026-10-01, D-41): Dominus decides the design questions of these milestones with the recommended option and records each as "Decided by Dominus (delegated)" in docs/decisions.md and the next Milestone file; the owner may override any of them. Tests are written with each story but run at each milestone's exit review, which fixes every failure before the milestone is merged into main.
- Blocked, wrong or ambiguous prompts become codex issues; the owner takes them to Anima with **A-002**; Anima issues a new Codex version.

## 1. Delivery format

Hybrid: **stage gates** at milestones M0-M14 (with kill gates at M2 and M6; kill gate 2 is held after M14, D-41) and **Kanban flow** inside each milestone, WIP 1. Milestone kickoff (K) batches owner decisions; exit review (X) demonstrates exit criteria and tags the repo.

## 2. Charter (C-01)
Written verbatim to `CLAUDE.md` by P-000.

```markdown
# CLAUDE.md: Project Odyssey Charter (Codex C-01, v2.11)

<role>
You are a member of Mraw, the Dominus Full Team (also called Dominus Avengers), assembling Project Odyssey by following the Codex written by Anima. You build exactly what the current Codex prompt asks, nothing more.
</role>

<project>
Project Odyssey (game codename Odysseus): a 2D pixel-art life and civilization simulation. MVP = Age 1 vertical slice on Windows x64: one procedurally generated region, one hero from age 12 who grows into a clan leader, five professions, Trade and Religion pillars, win by leading the region.
Source of truth for WHAT: Project Odyssey.docx v2.10 (chapter 12: MVP; chapter 7: architecture). Source of truth for HOW and ORDER: docs/Codex.md (this Codex).
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
9. Luna (ARC-09) is our game engine: the Platform, Physics and Engine layers in src/luna/ (targets luna_platform, luna_physics and luna_engine, namespaces luna::platform, luna::physics and luna::engine). Luna stays game-agnostic: it never includes Simulation or Game code and holds nothing specific to Odysseus, so another game can reuse it. Game code uses Luna; Luna never knows about the game.
10. Luna Physics (ARC-10, ADR-017) is written by us and deterministic: all physics state uses fixed-point 32.32 numbers (luna::physics::Fixed), never float or double inside src/luna/physics/; floats appear only where the Engine converts results for drawing. SI units: metres, seconds, kilograms; one 32-pixel tile is 1 metre. Physics uses Core only. Every physics feature is tested against its textbook formula.
11. Rendering (ARC-11, ADR-021, from M8b): Luna draws through SDL_GPU with shaders behind the Renderer interface; SDL_GPU types and shader files live only in src/luna/platform/ and src/luna/engine/; the SDL_Renderer path stays as fallback and for headless tests. Lighting and shadows are presentation only: the Simulation never reads them, so determinism is unaffected. Screens are laid out from the virtual size (960 x 540) and the UI scale, never from fixed pixel numbers.
</architecture_rules>

<stack>
C++20, MSVC (Visual Studio), CMake with presets (windows-x64-debug, windows-x64-release), vcpkg manifest mode. Libraries: SDL3, EnTT, Dear ImGui, nlohmann/json, doctest, FastNoiseLite (single header in third_party/). Tracy for profiling when needed.
Adding any other library: allowed, but record an ADR in docs/adr/ explaining why, and mention it in the assembly report.
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
3. Design decisions (D-22, owner 2026-09-30): any D-xx that is not Decided, or any question that changes design or scope, is the owner's. Stop that story, ask the owner in chat in question rounds (2-4 options each, the recommended option first), record the answer in docs/decisions.md as "Decided (owner, <date>)" and in the next Milestone file, then continue. Never decide a design question for the owner; work on other ready prompts while waiting only if the owner is away. Exception for M10-M14 (owner, 2026-10-01, D-41): Dominus decides those milestones' design questions with the recommended option, records each as "Decided by Dominus (delegated)" with its reasoning in docs/decision-requests/<ID>.md, lists it in the next Milestone file, and continues.
Everything else the team decides and records:
- Technical choices (how to build what the owner decided): Dominus decides and records them in ADRs or design documents.
- If the source of truth must change because of an owner decision, update the requirements document on Google Drive (bump its version, add a resolution-log line) and raise a codex issue so Anima can follow.
- Kill-gate evidence agents can measure, git push, new libraries: handle and report.
Sessions end without warning (usage limits, crashes), so work must always be resumable: Limit.md at the repository root is kept current after every story. When a session is told it is near its limit, or the owner asks for a break: finish the current step, commit work in progress to its story branch with a "work in progress" message, push the branch, and update Limit.md with exactly what is done and what is left. Never leave uncommitted work.
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
<prompt id="L-01" codex="2.1" name="Mraw build loop">
<context>
Used by mraw-orchestrator for every story prompt S-US-xxx. The Charter (CLAUDE.md) is already loaded.
</context>
<instructions>
1. Readiness: for each D-xx in the story's <dependencies>, read docs/decisions.md. If any is not Decided, follow the Charter's human gate 3: ask the owner in chat (2-4 options, recommended first), record the answer in docs/decisions.md as "Decided (owner, <date>)", then continue; while waiting, work on another ready prompt only if the owner is away. In M10-M14 (D-41) Dominus decides instead, as the Charter's human gate 3 exception says. For each US-xxx dependency, confirm it is Done in docs/status.md.
2. Branch: create story/US-xxx from qa. If that branch already exists with paused work (Limit.md says so), continue on it instead: merge the latest qa into it first, then finish what Limit.md lists as left.
3. Plan: delegate to mraw-architect -> docs/plans/US-xxx.md.
4. Tests first: delegate to mraw-tester -> failing tests for every headless-testable scenario; manual checks for the rest.
5. Content (only if the story needs data): delegate to mraw-designer.
6. Implement: delegate to mraw-programmer until tests pass with zero warnings.
7. Verify (M10-M14, D-41: build Debug with zero warnings and save the build log to docs/evidence/US-xxx/; the full run below happens at X-Mx): delegate to mraw-tester -> run `pwsh tools/verify.ps1 -Story US-xxx` (configure, the Debug build with zero warning lines and every Debug test, results saved to docs/evidence/US-xxx/; Release is built and tested by CI on qa after the merge, D-46; use `-Config Both` to reproduce a Release failure locally); add story-specific evidence (end-to-end runs, screenshots via `odysseus.exe --screenshot`, measurements) to the same folder.
8. Accept: delegate to mraw-acceptor. On REJECT, return to step 6 with the reasons. After 3 rejections, mark the story Failed, write a codex issue, and stop this story.
9. Document and teach: delegate to mraw-writer -> docs + teach-back entry.
10. Integrate: update CHANGELOG.md, commit, merge into qa, push; when CI on qa is green, set the story to Done in docs/status.md. In M10-M14 (D-41) set it to Done after the merge; X-Mx runs CI.
11. Snapshot and report: update Limit.md (next prompt, anything paused, how to resume), save the next Milestone-<n>.md at the repository root (next AP-### ID: what changed, milestone table, delegated decisions, codex issues, next prompts), commit it to qa, write the assembly report (Charter report_format) to docs/reports/US-xxx-<date>.md. Then continue with the next prompt in the Codex, unless the session is getting long; in that case end with the report so the next session starts fresh with A-001.
</instructions>
<stop_conditions>
A human gate in the Charter (step 1 only for kill gates, accounts, credentials, money); 3 acceptance rejections (step 8); a codex issue that affects this story; git push needs authentication that is not configured.
</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

## 5. Human gates and owner decisions
Agents stop only for owner design decisions. The decision log starts with these dependencies (from the source of truth, section 12.6):

| ID | Decision | Needed by | Blocks | Status |
|---|---|---|---|---|
| D-01 | Confirm time model: pausable real time with speed control (OPEN-04) | M2 | US-010 | Decided |
| D-02 | Side Characters interview: needs, traits, relationships (OPEN-10) | M2 (week 3) | US-011, US-012, US-013 | Decided (delegated to Dominus) |
| D-03 | Confirm the Age 1 needs set: Hunger, Energy, Warmth, Social | M2 | US-011 | Decided |
| D-04 | Sprite size and facing directions (OPEN-11); owner answer 2026-09-29: 32x48 px, 8 directions | M1 | US-022, US-024, US-030 | Decided |
| D-05 | Art source for placeholders: own, free asset pack, or hired (OPEN-12) | M2c | US-120, US-030 | Decided (owner, 2026-09-30): own art in assets/sprites/, placeholder quality; licence checked before any public release |
| D-06 | Minimum PC spec (OPEN-19): a mid-range PC, 6-core CPU, 16 GB RAM, RX 6600 / RTX 3060 class GPU (8 GB) with DirectX 12, Windows 10/11; 60 FPS at 1080p on High lighting, Low for weaker PCs (development PC: RX 7900 XTX, 32 GB RAM, 6-core CPU) | M8b | US-082, US-234, US-247 | Decided (owner, 2026-10-01) |
| D-07 | Calendar display (OPEN-15) | M4 | US-050, US-083 | Decided (closed by what M4-M5 built; D-31, D-32; owner D-47, 2026-10-01) |
| D-08 | Interactions interview: verbs, objects, crafting (OPEN-09) | M4 | US-061, US-062 | Decided (answered by D-34) |
| D-09 | Confirm the five Age 1 professions (MVP-07) | M4 | US-060 | Decided (closed by what M5 built; D-32; owner D-47, 2026-10-01) |
| D-10 | Confirm MVP pillars Trade + Religion (MVP-08) and victory thresholds (MVP-09) | M5 | US-070..US-073 | Superseded by D-40 and D-41 (four pillars; MVP-08, MVP-09) |
| D-11 | Story interview: tone of events, onboarding elder (OPEN-08) | M5 | US-052, US-090 | Decided (closed by what M5-M6 built; D-32, D-33; owner D-47, 2026-10-01) |
| D-12 | Visual Studio, CMake, Git, vcpkg installed; GitHub account and private repo | M0 | US-001, US-002 | Decided |
| D-13 | SDL3, EnTT, Dear ImGui, nlohmann/json, doctest, FastNoiseLite available via vcpkg or third_party | M0-M4 | US-020, US-032, US-083, US-016, US-040 | Decided |
| D-14 | Eight outside playtesters recruited | M6 | Kill gate 2 | Open: plan ready (docs/plans/M6-playtest-plan.md), recruiting from K-M13 (D-48) |
| D-15 | Technical chain: M0 > M1 > M1b > M2 > M2b > M2c > M2d > M3 > M4 > M5 > M7 > M8 > M8b > M8c > M9a > M9b > M9c > M8d > M8e > M9 > M10 > M10b > M11 > M12 > M13 > M14 > M6 (each milestone needs the previous one) | All | All | Planned |
| D-GATE-M2 | Kill Gate 1 result (M2): did 2 of 3 readers find a story? | X-M2 | M3 | Decided: Pivot (owner, 2026-09-30) |
| D-18 | Story pivot design: story arcs on a richer social simulation; quarrels, blame and revenge; sharing and nursing; courtship and rivals; teaching and hunting parties; episodes plus lines with reasons; the owner judges the retry alone | M2b | US-110..US-115 | Decided (owner, 2026-09-30) |
| D-GATE-M2b | Kill Gate 1 retry: the owner reads the M2b story and judges whether it is a story | X-M2b | M3 | Decided: Go (owner, 2026-09-30: "it is a story") |
| D-19 | Level editor scope v1: settings are level and character properties (level name, map size, default ground, hero start; per character name, HP, facing, sword damage); placed characters stand still with properties; new milestone M2c before M3 | M2c | US-120..US-126 | Decided (owner, 2026-09-30) |
| D-21 | M2d content and combat (owner, six rounds in chat): full gameplay; all 150 weapons catalogued, 16 starters (one plain and one elemental per class) placed as pickups, 9-slot hotbar (1-9, Shift cycles); 8 weapon classes; elements with status effects; hero 100 HP, respawn at the start; enemies (goblins, predators, boars) strike back after a 0.5 s wind-up within 1.5 m; deaths drop nothing; plants block (big ones), are inspected, chopped, heal 10 when edible and regrow after 15 s at a random free spot in camera view; effects on hits, plant actions and placed in the Editor; random weather every 60-120 s with a 3 s fade | M2d | US-130..US-138 | Decided (owner, 2026-09-30) |
| D-22 | Design decisions are taken by the owner in interactive question rounds in chat; Dominus no longer delegates design or scope questions (Charter human gate 3) | All | All | Decided (owner, 2026-09-30) |
| D-25 | Aiming and ballistics (owner, two rounds in chat, 2026-10-01): free aim at the mouse cursor (the hero faces it, melee swings toward it, aim line and crosshair); full arcs from Luna Physics that land at the cursor (clamped to range), hit anything in their path at body height, are blocked by rocks and trees when low and clear them when high; bows, crossbows, thrown weapons and staffs (staff bolts flat and fast) are shootable, guns are out of scope; no ammo, only rate-of-fire cooldown; three stories US-139..US-141 before plants | M2d | US-139..US-141 | Decided (owner, 2026-10-01) |
| D-34 | World interactions and dialogue (owner, one chat round, 2026-10-01): plain-text `.dlg` dialogue scripts + JSON interaction files (comments allowed, one file per thing); hybrid talk (written dialogue trees that read the simulation, plus generated small talk from memories, gossip and needs); a full visual graph editor in the Editor plus F5 hot reload; build M7, M8, M9 first and hold kill gate 2 (X-M6) afterwards | M7 | US-150..US-175 | Decided (owner, 2026-10-01) |
| D-35 | Assembly of M7-M9 (owner, one chat round with Anima, 2026-10-01): design questions go to the owner at each kickoff (D-22); P-009 pays the test debt (the M2d-M6 tests were never run), then every story runs the full verification and needs green CI on qa; US-155 builds the seven proposed world objects (fire pit, knapping stone, food store, shelter, flint nodule, water source, sleeping furs); the dialogue panel pauses the game | M7 | P-009, US-155, US-161 | Decided (owner, 2026-10-01) |
| D-40 | Authoring tools and Politics (owner, five chat rounds with Dominus, 2026-10-01): M10 Quests and story authoring (authored quests only, one JSON file per quest + quest graph, flat with prerequisites, journal + tracker + markers, the tutorial becomes a quest, crossroads become story events, play-in-editor debugger), M11 Data editors (schema-driven forms for every data file, Game Rules page with system switches, daily routines), M12 World editing (all generator settings with live preview, hand edits as overrides on the seed, per clan and person social, economic, political and daily setup, actions and properties per kind and per placed thing), M13 Politics (third pillar with victory: alliances and vassal oaths, elders' council, marriage ties, leadership challenges, economic and technological levers) | M10 | US-180..US-216 | Decided (owner, 2026-10-01) |
| D-41 | Technology and assembly of M10-M14 (owner, two chat rounds with Anima, 2026-10-01): Technology joins as the fourth pillar in M14 (tech tree as data, research by doing, workshops and inventors, espionage and theft; victory = Ember Strand held with a 60% Technology share, or all four pillars averaging 50%); Politics victory 60% vassals confirmed; Game Rules default assets/data/rules/standard.json, New Game and levels may name another; in M10-M14 Dominus decides design questions (delegated) and tests run at exit reviews; kill gate 2 after M14 | M10 | P-010, US-180..US-226 | Decided (owner, 2026-10-01) |
| D-42 | Resolution, lighting and buildings (owner, three chat rounds with Dominus, 2026-10-01): 960x540 virtual resolution with whole-step scaling or Fill, windowed, borderless and full screen, camera zoom (default 2x) and UI scale; Luna moves to SDL_GPU shaders (ADR-021); sun and moon cycle, fire, torch and effect lights, seasonal day length, weather dimming and lightning; shadows from the sun, the moon and nearby fires for characters, plants and buildings; normal maps generated from the art, placeholder building art; buildings from whole blueprints and from pieces, clan members help, rival clans build, wear and repair, damage and fire; interiors per building (roof fade or interior map, set in the Editor); prefabs composed from pieces in the Editor; all right after M8 | M8b | US-230..US-257 | Decided (owner, 2026-10-01) |
| D-43 | Assembly of M8b-M8e (owner, one chat round with Anima, 2026-10-01): M8d split into M8d Buildings (US-250, US-251, US-252, US-256) and M8e Building life (US-253, US-254, US-255, US-257); US-256 no longer waits for US-254; the M7-M9 rules (D-35) apply; D-06 answered | M8b | P-011, US-230..US-257 | Decided (owner, 2026-10-01) |
| D-44 | M8b window and scale settings: windowed sizes 1280x720, 1600x900, 1920x1080, 2560x1440; borderless and exclusive full screen; Whole scaling with black bars by default on non-whole sizes, Fill as a Settings option; camera zoom 1x/2x and UI scale 1x/2x in Settings, zoom also on the mouse wheel and keys in play; first start without settings.json uses windowed 1280x720, zoom 2x, UI scale 1x and lighting Medium | M8b | US-231..US-233 | Decided (owner, 2026-10-01) |
| D-47 | Housekeeping (owner, one chat round, 2026-10-01): D-07, D-09 and D-11 closed by what M4-M6 built under delegated decisions; D-10 superseded by the four pillars; STO-01 and SDC-01 absorbed, OPEN-08, OPEN-10 and OPEN-15 answered; every MVP scope item Decided; on GitHub's GPU-less runners the first-frame limit is 10 s, and every exit review runs the strict 3 s check on the owner's PC (`tools/verify.ps1 -Config Release`); the reading PDFs on Drive are not mirrored | all | X-M8b..X-M14 | Decided (owner, 2026-10-01) |
| D-48 | Remaining open items, one by one (owner, 2026-10-01): ARC-01..ARC-08 confirmed as built (ARC-01 renamed Six-layer architecture); the kill gate 2 playtest is planned now in docs/plans/M6-playtest-plan.md and recruiting starts at K-M13; the Anima Prompt Catalog becomes a Google Doc on Drive | M13 | K-M13, X-M6 | Decided (owner, 2026-10-01) |
| D-49 | Lighting design answers (K-M8c; owner, 2026-10-02) | M8c | US-240..US-247 | Decided (owner, 2026-10-02) |
| D-50 | Celestial bodies: sun and moon as placeable light-source objects (owner, 2026-10-04); shadows by ground-plane projection from the body's position; sprites in the sky; eclipses as data events; new story US-248 before US-244 | M8c | US-248, US-244 | Decided (owner, 2026-10-04) |
| D-52 | NPC foundation, trade economy and NPC life (owner, 2026-10-04, six chat rounds): 24 design answers and 4 follow-ups in docs/decision-requests/D-52.md: owner-defined NPC Classes (one or more per NPC), kind files, placed NPCs as full persons, up to 100,000 persons with detail by distance, nine attitudes per pair, talk only with a dialogue, a Confront button, hidden actions plus an Actions pop-up, barter and owner-defined currency with supply and demand and reputation, schedules and NPC-to-NPC interactions, forms before the graph editor, test level npc-test.json; three milestones right after M8c | M9a-M9c | US-260..US-270, US-280..US-284, US-290..US-294 | Decided (owner, 2026-10-04) |
| D-58 | Editor help and live data (owner, 2026-10-05, three chat rounds, docs/decision-requests/D-58.md): tooltips with purpose, range and example from assets/data/editor/help.json on every field of the Level, Building and Graph editors (a test enforces it); a dropdown suggestion list on every field (numbers: default, min, max, last 5 typed); hot reload of every data file on save and by a file watch within 1 s, all or nothing, placed things follow, a missing kind shows a red marker; level edits win over an older run save by id; new milestone M10b after X-M10; exit = tests plus a 10-minute owner walkthrough | M10b | US-300..US-305 | Decided (owner, 2026-10-05) |

## 6. Assembly prompts

### A-000 Start assembly (owner pastes this once)
```text
Dominus Avengers Assemble.
You are Mraw, the Dominus Full Team, assembling Project Odyssey with Codex v2.11 written by Anima.
Read Codex.md in this folder completely. Execute prompt P-000. Then, acting as mraw-orchestrator, execute the Codex prompts strictly in order (K-M0, then the M0 story prompts, X-M0, K-M1, ...), each through the build loop L-01.
Stop only where the Charter's human_gates say so. End every session with an assembly report.
```

### A-001 Continue assembly (owner pastes this to start each new session)
```text
Mraw, continue assembly.
Read CLAUDE.md, Limit.md, docs/Codex.md and docs/status.md. Check docs/decisions.md for decisions the owner has answered since the last session and unblock those stories. Then continue with the first prompt in Codex order that is To do or newly unblocked, through the build loop L-01, without waiting for the owner except at the Charter's human gates. Save a Milestone-<n>.md after every story. End with an assembly report.
```

### A-002 Take codex issues to Anima (owner pastes this in a Dominus session)
```text
Anima, amend the Codex.
Read docs/Codex.md and docs/codex-issues.md. For each open issue: decide the fix with the owner if it changes design, update the affected prompts, bump the Codex minor version, add a line per change to the amendment log, and mark the issue Resolved with the new version. Hand the new Codex back to Mraw.
```

## 7. Prompts by milestone
### P-000 Bootstrap
```xml
<prompt id="P-000" codex="1.8" name="Bootstrap the Mraw workspace">
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
<prompt id="P-001" codex="1.8" name="Adopt Codex v1.2 in an existing workspace">
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

### P-002 Adopt Codex v1.3 (autonomous assembly)
```xml
<prompt id="P-002" codex="1.8" name="Adopt Codex v1.3 in an existing workspace">
<context>
Codex v1.3 writes the owner's standing instructions of 2026-09-30 into the Charter: delegated design decisions, the qa integration branch, CHANGELOG.md per change set, and a Milestone-<n>.md snapshot after every story. A workspace bootstrapped with v1.3 needs nothing: mark this prompt Done with the note "not needed".
</context>
<instructions>
1. Make sure the qa branch exists on the remote; create it from main if not.
2. docs/status.md: add P-002 after P-001 and set it Done.
3. docs/codex-issues.md: mark CI-004 and CI-005 Resolved in v1.3.
4. AGENTS.md: keep the owner's changelog rule and add one line telling any AI agent to follow CLAUDE.md (the Charter) and docs/Codex.md.
5. Commit on qa "P-002: adopt Codex v1.3 (autonomous assembly)" and push.
</instructions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

### P-003 Adopt Codex v1.4 (Luna Physics)
```xml
<prompt id="P-003" codex="1.8" name="Adopt Codex v1.4 in an existing workspace">
<context>
Codex v1.4 adds milestone M1b Luna Physics (requirements v1.5: PHY-01..PHY-06, ARC-10, ADR-017) right after M1, before the rest of M2. A workspace bootstrapped with v1.4 needs nothing: mark this prompt Done with the note "not needed".
</context>
<instructions>
1. docs/status.md: add P-003 after P-002 (Done) and K-M1b, S-US-025..S-US-029, X-M1b after X-M1 (To do), keeping every existing status. A story that was In progress in M2 goes back to To do with the note "paused for M1b; work in progress on its story branch".
2. docs/adr/: add ADR-017-luna-physics.md (own deterministic fixed-point 3D physics) and index it.
3. docs/decisions.md: D-15 reads "M0 > M1 > M1b > M2 > ... > M6".
4. Commit on qa "P-003: adopt Codex v1.4 (Luna Physics)", push, set P-003 Done.
</instructions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

### P-004 Adopt Codex v1.5 (assembly instructions)
```xml
<prompt id="P-004" codex="1.8" name="Adopt Codex v1.5 in an existing workspace">
<context>
Codex v1.5 writes down how the team already works: Limit.md as the resume point, tools/verify.ps1 as the standard verification, continuing paused story branches, continuous assembly, and who does what. A workspace bootstrapped with v1.5 needs nothing: mark this prompt Done with the note "not needed".
</context>
<instructions>
1. Make sure Limit.md and tools/verify.ps1 exist; create them from the descriptions in section 8 if not.
2. docs/status.md: add P-004 after P-003 and set it Done.
3. Commit on qa "P-004: adopt Codex v1.5 (assembly instructions)" and push.
</instructions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

### P-005 Adopt Codex v1.6 (Kill Gate 1 pivot)
```xml
<prompt id="P-005" codex="1.8" name="Adopt Codex v1.6 in an existing workspace">
<context>
Kill Gate 1 failed on 2026-09-30: the owner read the M2 chronicle and found "not really a story" (D-GATE-M2: Pivot). The Codex kill rule applies: no new content until the simulation is redesigned. The owner decided the redesign (D-18): new milestone M2b Story engine (requirements v1.6: STO-02, STO-03, SDC-02; epic E11; US-110..US-115), and the gate is repeated at the end of M2b, judged by the owner alone. Codex issue CI-006 (the US-020 first-frame check fails now and then on the CI Debug runner) is resolved here too.
</context>
<instructions>
1. docs/status.md: add P-005 after P-004; set X-M2 to "Failed (Kill Gate 1: Pivot, D-GATE-M2)"; add K-M2b, S-US-110..S-US-115 and X-M2b (To do) between X-M2 and K-M3.
2. docs/decisions.md: record D-GATE-M2 (Decided: Pivot, with the owner's words), D-18 (Decided, the owner's choices) and D-GATE-M2b (Open).
3. CI-006: in tests/luna/run_game_window.cmake, keep the 3-second first-frame limit for Release builds (what a player runs); for Debug builds, which AddressSanitizer slows down, require only that the first frame appears within 15 seconds. The window must still open and close cleanly in both. Pass the configuration to the script from CMakeLists.txt. Mark CI-006 Resolved in Codex v1.6 in docs/codex-issues.md.
4. Update Limit.md (next prompt K-M2b) and commit on qa "P-005: adopt Codex v1.6 (Kill Gate 1 pivot)", push, and wait for green CI.
</instructions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

### P-006 Adopt Codex v1.7 (Level editor)
```xml
<prompt id="P-006" codex="1.8" name="Adopt Codex v1.7 in an existing workspace">
<context>
Kill Gate 1 passed on 2026-09-30 (D-GATE-M2b: Go, "it is a story"); M2 and M2b are done (`main` tagged m2-done and m2b-done). Before M3 the owner asked for a level editor (requirements v1.7: epic E12, US-120..US-126; decisions D-05 and D-19; Mraw's brief docs/plans/M2c-editor-brief.md). The owner added his own art to assets/sprites/: labelled sprite sheets (heroes, monsters, ground tiles) and landscape pictures, all generated images of placeholder quality.
</context>
<instructions>
1. CLAUDE.md: update the Charter's version references to Codex v1.7 and requirements v1.7; nothing else in the Charter changes.
2. docs/status.md: add P-006 after P-005; set X-M2b to Done (Go); add K-M2c, S-US-120..S-US-126 and X-M2c (To do) between X-M2b and K-M3.
3. docs/decisions.md: D-05, D-GATE-M2b and D-19 as in section 5.
4. Commit the owner's sprite sheets in assets/sprites/ unchanged (they are the originals the atlases are cut from; the repository ignores nothing there).
5. Update Limit.md (next prompt K-M2c) and commit on qa "P-006: adopt Codex v1.7 (level editor)", push, and wait for green CI.
</instructions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

### P-007 Adopt Codex v1.8 (Content and combat)
```xml
<prompt id="P-007" codex="1.8" name="Adopt Codex v1.8 in an existing workspace">
<context>
M2c is done (`main` tagged m2c-done). The owner added seven sprite sheets (150 weapons, 150 plants, 50 animals, 100 animated effects, 100 elemental effects, 100 weather types) and, in six question rounds in chat, decided M2d Content and combat (requirements v1.8: epic E13, US-130..US-138; D-21) and that design decisions are the owner's from now on (D-22). Mraw's brief: docs/plans/M2d-content-brief.md.
</context>
<instructions>
1. CLAUDE.md: regenerate from the Charter C-01 (tools/sync-codex.ps1 does it): version references v1.8 and human gate 3 (design decisions are the owner's).
2. docs/status.md: add P-007 after P-006; set X-M2c to Done; add K-M2d, S-US-130..S-US-138 and X-M2d (To do) between X-M2c and K-M3.
3. docs/decisions.md: D-21 and D-22 as in section 5; add a note that delegated decisions made before D-22 stay valid unless the owner overrides them.
4. Commit the seven new sheets in assets/sprites/ unchanged.
5. Update Limit.md (next prompt K-M2d) and commit on qa "P-007: adopt Codex v1.8 (content and combat)", push, and wait for green CI.
</instructions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
### P-008 Adopt Codex v1.9 (Aiming and ballistics)
```xml
<prompt id="P-008" codex="1.9" name="Adopt Codex v1.9 in an existing workspace">
<context>
US-130..US-135 are done. Before plants the owner asked to aim weapons with the mouse, to have ballistics and several shootable ranged weapons (requirements v1.9: US-139..US-141 in epic E13; D-25, two question rounds in chat). Mraw's brief: docs/plans/M2d-aiming-brief.md.
</context>
<instructions>
1. CLAUDE.md: regenerate from the Charter C-01 (tools/sync-codex.ps1 does it): version references v1.9.
2. docs/status.md: add P-008 after P-007 (Done); add S-US-139, S-US-140 and S-US-141 (To do) between S-US-135 and S-US-136.
3. docs/decisions.md: D-25 as in section 5.
4. Update Limit.md (next prompt S-US-139) and commit on qa "P-008: adopt Codex v1.9 (aiming and ballistics)", push, and wait for green CI.
</instructions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

### P-009 Adopt Codex v2.0 (World interactions, dialogue and their editor)
```xml
<prompt id="P-009" codex="2.0" name="Adopt Codex v2.0 and pay the test debt">
<context>
M2d, M3, M4, M5 and M6 are built; M6 waits at kill gate 2 (D-GATE-M6). Since M2d the owner asked for no tests or CI, so the M2d-M6 tests were compiled but never run and nothing has been pushed since 5125364. The owner has now decided three new milestones built before kill gate 2 (requirements v2.0, Round 13; D-34): M7 World interactions, M8 Speak to NPCs, M9 Interaction and dialogue editor; and how they are assembled (D-35). Mraw's brief: docs/plans/M7-M9-interactions-brief.md. M7 starts with a large refactor (US-152 moves every hard-coded action into data), which is only safe on a test suite that is known to pass.
</context>
<instructions>
1. CLAUDE.md and .claude/agents/: regenerate from the Charter C-01 and roles (tools/sync-codex.ps1 does it): version references v2.0.
2. docs/status.md: add P-009 after P-008; add K-M7, S-US-150..S-US-156, X-M7, K-M8, S-US-160..S-US-165, X-M8, K-M9, S-US-170..S-US-175, X-M9 (To do) between X-M5 and K-M6; X-M6 stays Blocked until X-M9 is done.
3. docs/decisions.md: D-08 Decided (answered by D-34); D-34 and D-35 as in section 5.
4. Pay the test debt on qa: run `pwsh tools/verify.ps1 -Story P-009` (Debug and Release, every test). Fix every failure in the code the failing test covers, one commit per fix ("P-009: fix <test>"). Then push qa and wait for green CI. Keep each test's intent: change a test only when it is provably wrong about the requirements, and list every such change in the report with the reason.
5. Record the result in docs/gates/test-debt.md: tests run, failures found, each fix, CI run link. Complete the exit checks that M2d-M5 left owed (docs/gates/M2d.md..M5.md) from this run, and mark them done there.
6. Update Limit.md (next prompt K-M7), save a Milestone-<n>.md, and commit on qa "P-009: adopt Codex v2.0 (interactions, dialogue, editor)", push, and wait for green CI.
</instructions>
<stop_conditions>
A failure whose fix would change behaviour the requirements describe (write a decision request for the owner); more than 3 attempts on one failure (codex issue); push needs authentication that is not configured.
</stop_conditions>
<output_format>Assembly report (Charter report_format), with the test-debt table.</output_format>
</prompt>
```

### P-010 Adopt Codex v2.1 (Authoring tools, Politics, Technology)
```xml
<prompt id="P-010" codex="2.1" name="Adopt Codex v2.1">
<context>
M7 is done and M8 is under way. The owner planned five more milestones before kill gate 2 (requirements v2.2, Rounds 14 and 15): M10 Quests and story authoring, M11 Data editors, M12 World editing, M13 Politics and M14 Technology (D-40, D-41). For M10-M14 the owner delegates design questions to Dominus and runs tests at exit reviews (D-41); M8 and M9 keep the D-35 rules. Mraw's brief: docs/plans/M10-M13-authoring-brief.md (sections 9 and 10 cover M14). Run this prompt as soon as the Codex sync reports v2.1, before the next story; a story in progress is finished first.
</context>
<instructions>
1. CLAUDE.md and .claude/agents/: regenerate from the Charter C-01 and roles (tools/sync-codex.ps1 does it): version references v2.1 and the M10-M14 exceptions.
2. docs/decisions.md: add D-40 and D-41 as in section 5. These two IDs are taken by the Codex; if a story already used D-40 or D-41 for something else, renumber that story's decision to the next free ID and update its references (decision request file, Milestone file, CHANGELOG line).
3. docs/status.md: add P-010 after P-009; add K-M10..X-M10, K-M11..X-M11, K-M12..X-M12, K-M13..X-M13, K-M14..X-M14 (To do) in the execution order of section 7, between X-M9 and K-M6.
4. docs/codex-issues.md: mark CI-007 "Resolved in Codex v2.1 (catalog hot reload joins US-191; dialogue reload in M8 as built)" and CI-008 "Resolved in Codex v2.1 (objects ride on the plant machinery; noted in the M12 design notes)".
5. Mirror requirements v2.2 and the backlog with tools/sync-workspace.ps1 if the session hook has not already done it.
6. Update Limit.md (next prompt in Codex order) and commit on qa "P-010: adopt Codex v2.1 (authoring tools, Politics, Technology)", push, and wait for green CI.
</instructions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

### P-011 Adopt Codex v2.2 (Resolution, lighting and buildings)
```xml
<prompt id="P-011" codex="2.2" name="Adopt Codex v2.2">
<context>
M8 is under way. The owner added four milestones built right after M8 and before M9 (requirements v2.4, Rounds 16 and 17; D-42, D-43): M8b Resolution and GPU renderer, M8c Lighting and shadows, M8d Buildings, M8e Building life; and answered D-06 (a mid-range target PC). They follow the M7-M9 rules (D-35). Mraw's brief: docs/plans/M8b-M8d-render-light-build-brief.md (section 9 records the split and D-06). Run this prompt as soon as the Codex sync reports v2.2; a story in progress is finished first.
</context>
<instructions>
1. CLAUDE.md and .claude/agents/: regenerate from the Charter C-01 and roles (tools/sync-codex.ps1 does it): version references v2.2 and architecture rule 11 (rendering).
2. docs/decisions.md: D-06 Decided with the owner's answer; D-42 and D-43 as in section 5. These IDs are taken by the Codex; if a story already used D-42 or D-43, renumber that story's decision to the next free ID and update its references.
3. docs/status.md: add P-011 after P-010; add K-M8b..X-M8b, K-M8c..X-M8c, K-M8d..X-M8d, K-M8e..X-M8e (To do) in the execution order of section 7, between X-M8 and K-M9.
4. Mirror requirements v2.4 and the backlog with tools/sync-workspace.ps1 if the session hook has not already done it, and commit docs/plans/M8b-M8d-render-light-build-brief.md if it is not committed yet.
5. Update Limit.md (next prompt in Codex order) and commit on qa "P-011: adopt Codex v2.2 (resolution, lighting, buildings)", push, and wait for green CI.
</instructions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

### P-012 Adopt Codex v2.11 (NPC foundation, trade economy, NPC life)
```xml
<prompt id="P-012" codex="2.11" name="Adopt Codex v2.11">
<context>
The owner asked on 2026-10-04 to talk, confront and trade with every NPC, to set every NPC by hand in the Editor with owner-defined NPC Classes, and to make placed NPCs full persons of a world of up to 100,000 (D-52, Decided in six chat rounds). Three new milestones M9a, M9b, M9c come right after M8c and before M8d. Mraw's brief: docs/plans/M9a-npc-roles-brief.md. Run this prompt as soon as the Codex sync reports v2.11; a story in progress (for example US-248) is finished first.
</context>
<instructions>
1. CLAUDE.md and .claude/agents/: regenerate from the Charter C-01 and roles (tools/sync-codex.ps1 does it): version references v2.11.
2. docs/decisions.md: add D-52 as in section 5 (Decided, owner, 2026-10-04); D-15 reads "... > M8c > M9a > M9b > M9c > M8d > ...". If a story already used D-52, renumber that story's decision to the next free ID and update its references. Commit docs/decision-requests/D-52.md.
3. docs/status.md: add P-012 after P-011; add K-M9a..X-M9a, K-M9b..X-M9b, K-M9c..X-M9c (To do) in the execution order of section 7, between X-M8c and K-M8d.
4. Requirements v2.10 and the backlog (epics E26-E28, Round 23) are on Drive; mirror them with tools/sync-workspace.ps1 if the session hook has not, and commit them with "Docs: sync workspace files from Drive". The brief and D-52.md are already committed (6c9d395).
5. Update Limit.md (next prompt in Codex order) and commit on qa "P-012: adopt Codex v2.11 (NPC foundation, trade economy, NPC life)" (explicit paths only), push.
</instructions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

### P-013 Adopt Codex v2.13 (Editor help and live data)
```xml
<prompt id="P-013" codex="2.13" name="Adopt Codex v2.13">
<context>
The owner asked on 2026-10-05 for tooltips in the Game Editor, auto-suggestions on every field, and Editor changes reflected in the game after save (D-58, Decided in three chat rounds). A new milestone M10b Editor help and live data (epic E29, US-300..US-305) is queued after X-M10 and before K-M11. Mraw's brief: docs/plans/M10b-editor-help-live-data-brief.md. Run this prompt as soon as the Codex sync reports v2.13; a story in progress (for example an M10 story) is finished and merged first, then M10 continues in Codex order.
</context>
<instructions>
1. CLAUDE.md and .claude/agents/: regenerate from the Charter C-01 and roles (tools/sync-codex.ps1 does it): version references v2.13.
2. docs/decisions.md: D-58 is already there (Decided, owner, 2026-10-05) with docs/decision-requests/D-58.md, committed on qa by Mraw with the brief. Make D-15 read "... > M10 > M10b > M11 > ...". If a story already used D-58, renumber that story's decision to the next free ID and update its references.
3. docs/status.md: add P-013 after P-012; add K-M10b, S-US-300, S-US-301, S-US-302, S-US-303, S-US-304, S-US-305, X-M10b (To do) between X-M10 and K-M11.
4. Requirements v2.11 and the backlog (epic E29, Round 24) are on Drive; mirror them with tools/sync-workspace.ps1 if the session hook has not, and commit them with "Docs: sync workspace files from Drive".
5. Update Limit.md (next prompt in Codex order) and commit on qa "P-013: adopt Codex v2.13 (Editor help and live data)" (explicit paths only), push.
</instructions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

### M0 Tooling ready
Exit criteria: A clean checkout builds in Visual Studio; you pause the program on a breakpoint; CI runs on push.

```xml
<prompt id="K-M0" codex="1.8" name="Kick off M0 Tooling ready">
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
<prompt id="S-US-001" codex="1.8" milestone="M0" story="US-001" priority="Must" size="S">
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
<prompt id="S-US-002" codex="1.8" milestone="M0" story="US-002" priority="Must" size="S">
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
<prompt id="S-US-003" codex="1.8" milestone="M0" story="US-003" priority="Must" size="S">
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
<prompt id="S-US-004" codex="1.8" milestone="M0" story="US-004" priority="Must" size="S">
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
<prompt id="X-M0" codex="1.8" name="Exit review M0">
<instructions>
1. Demonstrate the exit criteria: A clean checkout builds in Visual Studio; you pause the program on a breakpoint; CI runs on push.
2. Collect evidence (test output, headless run logs, FPS logs, screenshots) into docs/gates/M0.md, one section per criterion, each marked met or not met.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m0-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the gate result.</output_format>
</prompt>
```

### M1 Luna engine: walking skeleton
Exit criteria: Luna opens a window; a demo character walks around a tile map at 60 FPS with crisp pixels at any window size; the build proves Luna contains no Odysseus code.

```xml
<prompt id="K-M1" codex="1.8" name="Kick off M1 Luna engine: walking skeleton">
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
<prompt id="S-US-020" codex="1.8" milestone="M1" story="US-020" priority="Must" size="M">
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
The first frame must appear within 3 seconds in Release; the Debug build (AddressSanitizer) only has to show it within 15 seconds (CI-006).
Manual checks in docs/plans/US-020.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: RAII wrapper around SDL_Window; unique_ptr with a custom deleter.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-021 Control the game through intents
```xml
<prompt id="S-US-021" codex="1.8" milestone="M1" story="US-021" priority="Must" size="S">
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
<prompt id="S-US-022" codex="1.8" milestone="M1" story="US-022" priority="Must" size="M">
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
<prompt id="S-US-023" codex="1.8" milestone="M1" story="US-023" priority="Must" size="M">
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
<prompt id="S-US-024" codex="1.8" milestone="M1" story="US-024" priority="Must" size="M">
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
<prompt id="X-M1" codex="1.8" name="Exit review M1">
<instructions>
1. Demonstrate the exit criteria: Luna opens a window; a demo character walks around a tile map at 60 FPS with crisp pixels at any window size; the build proves Luna contains no Odysseus code. For the last criterion, show the US-003 "Luna stays game-agnostic" check passing on the current code.
2. Collect evidence (test output, headless run logs, FPS logs, screenshots) into docs/gates/M1.md, one section per criterion, each marked met or not met.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m1-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the gate result.</output_format>
</prompt>
```

### M1b Luna Physics (deterministic 3D)
Exit criteria: Luna Physics passes its math, hit-detection, ballistics and rigid-body tests identically on every build; in the demo the hero throws a spear that flies in an arc with a shadow and hits a target.

```xml
<prompt id="K-M1b" codex="1.8" name="Kick off M1b Luna Physics">
<instructions>
1. Confirm the previous milestone's exit review (docs/gates/M1.md) exists and passed.
2. Read docs/decisions.md. This milestone needs no open owner decision; design questions that come up are delegated to Dominus (Charter human_gates).
3. Architect: write docs/plans/M1b-physics-design.md (fixed-point format and overflow rules, units, the shapes and their tests, the integrator, how the Engine draws 3D positions top-down) before US-025.
4. Set this milestone's stories to To do in docs/status.md in this order: US-025, US-026, US-027, US-028, US-029.
5. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, decisions requested.</output_format>
</prompt>
```

#### S-US-025 Build deterministic 3D math
```xml
<prompt id="S-US-025" codex="1.8" milestone="M1b" story="US-025" priority="Must" size="M">
<context>
Story US-025: Build deterministic 3D math.
As a developer, I want fixed-point numbers, 3D vectors and rotations in Luna Physics, so that physics gives identical results on every computer and every run.
Epic E10 Luna physics: Luna simulates hits, throws and bodies with deterministic 3D physics.
Traces to: PHY-01, ARC-10, ADR-017.
</context>
<dependencies>
Stories that must be Done: US-003.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Luna Physics: src/luna/physics/ (target luna_physics, namespace luna::physics), headless, Core only (Charter rules 9 and 10); tests in a Physics-identity test program. This story also creates the layer: add luna_physics to CMake and extend the ADR-016 enforcement (boundary.h, the include validator's layer list, and US-003-style probes proving Physics cannot include Platform, Engine, Simulation, Game or SDL3, and that Engine and Simulation may include Physics).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Exact arithmetic">
Given two fixed-point values
When they are added, multiplied and divided
Then the results equal the expected values bit for bit in Debug, Release and CI
</scenario>
<scenario name="Rotations">
Given a vector rotated by 90 degrees about the up axis four times
When the rotations are applied as quaternions
Then it returns to the starting vector within 1/65536 of a unit
</scenario>
<scenario name="Determinism">
Given two runs of 1,000,000 mixed math operations with the same inputs
When both finish
Then both produce the same hash
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-025 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-025.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Fixed-point arithmetic; operator overloading; quaternions.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-026 Detect hits between shapes
```xml
<prompt id="S-US-026" codex="1.8" milestone="M1b" story="US-026" priority="Must" size="L">
<context>
Story US-026: Detect hits between shapes.
As a developer, I want spheres, capsules and boxes to report overlaps, ray hits and swept hits, so that every hit is found and a fast spear never passes through its target.
Epic E10 Luna physics: Luna simulates hits, throws and bodies with deterministic 3D physics.
Traces to: PHY-02, ARC-10.
</context>
<dependencies>
Stories that must be Done: US-025.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Luna Physics: src/luna/physics/ (target luna_physics, namespace luna::physics), headless, Core only (Charter rules 9 and 10); tests in a Physics-identity test program.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Overlap">
Given a sphere and a box that touch
When their overlap is tested
Then a hit is reported with the contact point and normal
</scenario>
<scenario name="No tunnelling">
Given a projectile moving 10 m per tick towards a 0.2 m target
When its path is swept
Then the hit is found at the correct time of impact
</scenario>
<scenario name="Many bodies">
Given 1,000 bodies in a region
When one physics step checks them through the spatial grid
Then only nearby pairs are tested and the step takes under 2 ms on the development PC
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-026 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-026.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Structs and pure functions; geometry; spatial grids.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-027 Fly projectiles with real ballistics
```xml
<prompt id="S-US-027" codex="1.8" milestone="M1b" story="US-027" priority="Must" size="M">
<context>
Story US-027: Fly projectiles with real ballistics.
As a hunter, I want thrown spears and darts to fly in true arcs under gravity, drag and wind, so that aim, strength and distance matter.
Epic E10 Luna physics: Luna simulates hits, throws and bodies with deterministic 3D physics.
Traces to: PHY-03, ADR-017.
</context>
<dependencies>
Stories that must be Done: US-026.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Luna Physics: src/luna/physics/ (target luna_physics, namespace luna::physics), headless, Core only (Charter rules 9 and 10); tests in a Physics-identity test program.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Arc">
Given a spear thrown at 45 degrees and 20 m/s over flat ground without drag
When it lands
Then its range is v^2/g = 40.8 m within 1%
</scenario>
<scenario name="Drag and wind">
Given the same throw with air drag and a crosswind
When it lands
Then it falls short and drifts downwind by the amounts the formulas predict
</scenario>
<scenario name="Aim">
Given a target 25 m away
When the aim solver computes the launch angle for a given speed
Then the thrown projectile hits the target
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-027 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-027.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Numerical integration; units.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-028 Push and bounce bodies
```xml
<prompt id="S-US-028" codex="1.8" milestone="M1b" story="US-028" priority="Must" size="M">
<context>
Story US-028: Push and bounce bodies.
As a player, I want bodies to have mass, be pushed, slide, bounce and come to rest, so that knockback and thrown or falling objects feel physical.
Epic E10 Luna physics: Luna simulates hits, throws and bodies with deterministic 3D physics.
Traces to: PHY-04, ADR-017.
</context>
<dependencies>
Stories that must be Done: US-026.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Luna Physics: src/luna/physics/ (target luna_physics, namespace luna::physics), headless, Core only (Charter rules 9 and 10); tests in a Physics-identity test program.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Impulse">
Given a 70 kg body at rest
When an impulse of 140 N s pushes it
Then its velocity becomes 2 m/s in the impulse direction
</scenario>
<scenario name="Bounce and rest">
Given a ball dropped onto the ground with restitution 0.5
When it bounces
Then each bounce reaches a quarter of the previous height and it comes to rest
</scenario>
<scenario name="Friction">
Given a crate sliding on grass
When no force pushes it any more
Then it stops within the distance friction predicts
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-028 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-028.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Classes with invariants; fixed-timestep integration.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-029 Throw a spear in the demo
```xml
<prompt id="S-US-029" codex="1.8" milestone="M1b" story="US-029" priority="Must" size="M">
<context>
Story US-029: Throw a spear in the demo.
As a player, I want to throw a spear at a target in the walking skeleton, so that Luna Physics is proven end to end in the real game.
Epic E10 Luna physics: Luna simulates hits, throws and bodies with deterministic 3D physics.
Traces to: PHY-02, PHY-03, PHY-05.
</context>
<dependencies>
Stories that must be Done: US-024, US-027, US-028.
Owner decisions that must be Decided: none.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game layer using Luna Physics and Luna Engine: src/game/, apps/odysseus/; materials as data in assets/data/materials.json (validated, Charter rule 7); the Engine draws 3D positions top-down (height lifts the sprite, a shadow stays on the ground).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Throw">
Given the hero facing a straw target 8 tiles away
When I press Interact
Then a spear flies in a visible arc with a shadow and hits the target
</scenario>
<scenario name="Material">
Given a flint-tipped and a wooden spear (materials.json)
When each hits the target
Then the damage differs by the materials' hardness and density as configured
</scenario>
<scenario name="Blocked">
Given a target behind a rock
When I throw at it
Then the spear hits the rock and stops; the target is unharmed
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-029 ..." cases and the determinism hash test.
Manual checks in docs/plans/US-029.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Putting it together; content as data (materials.json).</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M1b" codex="1.8" name="Exit review M1b">
<instructions>
1. Demonstrate the exit criteria: Luna Physics passes its math, hit-detection, ballistics and rigid-body tests identically on every build; in the demo the hero throws a spear that flies in an arc with a shadow and hits a target. Include the textbook comparisons (range, bounce heights, friction distance) and a screenshot of the spear in flight.
2. Collect evidence into docs/gates/M1b.md, one section per criterion, each marked met or not met.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m1b-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the gate result.</output_format>
</prompt>
```

### M2 Console clan simulator (KILL GATE 1)
Exit criteria: The headless runner simulates a 20-person clan for 100 years without crashing; the printed chronicle is shown to 3 people and at least 2 find a story in it; the determinism test passes.

```xml
<prompt id="K-M2" codex="1.8" name="Kick off M2 Console clan simulator (KILL GATE 1)">
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
<prompt id="S-US-010" codex="1.8" milestone="M2" story="US-010" priority="Must" size="M">
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
<prompt id="S-US-011" codex="1.8" milestone="M2" story="US-011" priority="Must" size="M">
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
<prompt id="S-US-012" codex="1.8" milestone="M2" story="US-012" priority="Must" size="L">
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
<prompt id="S-US-013" codex="1.8" milestone="M2" story="US-013" priority="Must" size="M">
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
<prompt id="S-US-014" codex="1.8" milestone="M2" story="US-014" priority="Must" size="M">
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
<prompt id="S-US-015" codex="1.8" milestone="M2" story="US-015" priority="Must" size="S">
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
<prompt id="S-US-016" codex="1.8" milestone="M2" story="US-016" priority="Must" size="M">
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
<prompt id="X-M2" codex="1.8" name="Exit review M2">
<instructions>
1. Demonstrate the exit criteria: The headless runner simulates a 20-person clan for 100 years without crashing; the printed chronicle is shown to 3 people and at least 2 find a story in it; the determinism test passes.
2. Collect evidence (test output, headless run logs, FPS logs, screenshots) into docs/gates/M2.md, one section per criterion, each marked met or not met.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m2-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
4. This is a kill gate. Evaluate each success criterion below with evidence and write a recommendation (go / pivot / stop) with reasons. Criteria that need people (readers, playtesters) cannot be measured by agents: write a decision request D-GATE-M2 asking the owner for the result, and treat his answer as the gate decision. Until the owner answers, do not start the next milestone: end the session with the assembly report and a Milestone file.
- Chronicle interest (early): 2 of 3 readers find a story in the console chronicle (measured by: Show printed chronicle)
- Determinism: Same seed + inputs give the same world hash after 10,000 ticks (measured by: Automated test in CI)
- Kill / pivot rule: If M2 or M6 fails: stop adding content; redesign the simulation or the loop first (measured by: Owner decision, recorded)
</instructions>
<output_format>Assembly report with the gate result.</output_format>
</prompt>
```

### M2b Story engine (KILL GATE 1 retry)
Exit criteria: The chronicle tells the clan's story in fewer, bigger episodes with reasons; every death and feud says why; the 100-year soak still runs without crashing and the determinism test passes; the owner reads the printed story and judges whether it is a story (Kill Gate 1 retry).

Why this milestone exists: Kill Gate 1 failed on 2026-09-30. The owner found the M2 chronicle "not really a story" and asked for reasons behind deaths and feuds, more kinds of interaction between people, and fewer, more complex events (D-GATE-M2, D-18). By the Codex kill rule, no content is added before the simulation is redesigned; M2b is that redesign, and the gate is repeated at its end.

```xml
<prompt id="K-M2b" codex="1.8" name="Kick off M2b Story engine">
<instructions>
1. Confirm docs/gates/M2.md exists and records the failed gate, and that D-GATE-M2 (Pivot) and D-18 are recorded in docs/decisions.md.
2. Architect: write docs/plans/M2b-story-design.md before US-110. It covers: an event log in which every notable event has an id, its people and the ids of the events that caused it; how causes are chosen (a starvation death points at the empty store and any theft that emptied it; a feud points at the thefts, quarrels or blame that soured it); health (sick, injured) and its causes; the new interactions of D-18 and the data files that tune them; how episodes are recognised from linked events (a beginning, a turn, an end) and named; how the story is printed (`--story`, episodes first, then lines with reasons); the save-version upgrade path; and how the chronicle stays readable (at most about 40 episodes a century).
3. Set this milestone's stories to To do in docs/status.md in this order: US-110, US-111, US-112, US-113, US-114, US-115.
4. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, decisions requested.</output_format>
</prompt>
```

#### S-US-110 Give every death and feud a reason
```xml
<prompt id="S-US-110" codex="1.8" milestone="M2b" story="US-110" priority="Must" size="M">
<context>
Story US-110: Give every death and feud a reason.
As a reader, I want every death and feud in the chronicle to say why it happened, so that events connect into a story.
Epic E11 Story engine: The clan's life becomes a readable story: episodes with causes, people and consequences.
Traces to: STO-03, NA-02. Owner decision D-18 (the story pivot after Kill Gate 1).
</context>
<dependencies>
Stories that must be Done: US-014.
Owner decisions that must be Decided: D-18.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation layer only: src/sim/ and apps/headless/, content numbers as data in assets/data/sim/ (Charter rule 7). No SDL3, no rendering (ADR-004). Follow docs/plans/M2b-story-design.md.
The simulation stays deterministic (Charter rule 6): new randomness comes from seeded streams, new state goes into the world hash, and the save format gains it with a new save version that upgrades older saves (ADR-010, US-016).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Death with a cause">
Given a person who starves after the food store was robbed or ran empty
When the death is recorded
Then the sentence names the cause and the event behind it, e.g. 'Winter, year 74: Hano died of hunger in the hard winter, after Brak stole from the store.'
</scenario>
<scenario name="Feud with a reason">
Given two people whose opinions fell through thefts, quarrels or blame
When their feud begins
Then the sentence names what turned them, e.g. 'A feud broke out between Tok and Brak over stolen meat.'
</scenario>
<scenario name="Traceable">
Given any death or feud in the chronicle
When I inspect it in the headless runner
Then it lists the earlier events that caused it
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-110 ..." cases, the determinism hash test and the 100-year soak (US-015 Run).
Manual checks in docs/plans/US-110.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Small structs that point to other data (event ids); enums.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-111 Quarrel, blame and take revenge
```xml
<prompt id="S-US-111" codex="1.8" milestone="M2b" story="US-111" priority="Must" size="L">
<context>
Story US-111: Quarrel, blame and take revenge.
As a player, I want people to quarrel, blame each other for losses and seek revenge, so that conflicts have causes and consequences.
Epic E11 Story engine: The clan's life becomes a readable story: episodes with causes, people and consequences.
Traces to: SDC-02, GD-03. Owner decision D-18 (the story pivot after Kill Gate 1).
</context>
<dependencies>
Stories that must be Done: US-110.
Owner decisions that must be Decided: D-18.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation layer only: src/sim/ and apps/headless/, content numbers as data in assets/data/sim/ (Charter rule 7). No SDL3, no rendering (ADR-004). Follow docs/plans/M2b-story-design.md.
The simulation stays deterministic (Charter rule 6): new randomness comes from seeded streams, new state goes into the world hash, and the save format gains it with a new save version that upgrades older saves (ADR-010, US-016).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Quarrel">
Given two tired or hungry people who dislike each other
When they meet
Then they may quarrel; both remember it and think less of each other
</scenario>
<scenario name="Blame">
Given a person whose close kin died after a theft they know of, or on a hunt someone led
When they grieve
Then they blame the person linked to the cause and remember it for life
</scenario>
<scenario name="Revenge">
Given a feud that keeps worsening
When revenge is taken
Then it ends in a fight (injury or death) or the clan exiles the aggressor, and the chronicle tells which and why
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-111 ..." cases, the determinism hash test and the 100-year soak (US-015 Run).
Manual checks in docs/plans/US-111.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: State machines with enums; std::optional.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-112 Share food and nurse the sick
```xml
<prompt id="S-US-112" codex="1.8" milestone="M2b" story="US-112" priority="Must" size="L">
<context>
Story US-112: Share food and nurse the sick.
As a player, I want people to share food in hard times and care for the sick and injured, so that kindness has weight and debts of gratitude form.
Epic E11 Story engine: The clan's life becomes a readable story: episodes with causes, people and consequences.
Traces to: SDC-02. Owner decision D-18 (the story pivot after Kill Gate 1).
</context>
<dependencies>
Stories that must be Done: US-110.
Owner decisions that must be Decided: D-18.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation layer only: src/sim/ and apps/headless/, content numbers as data in assets/data/sim/ (Charter rule 7). No SDL3, no rendering (ADR-004). Follow docs/plans/M2b-story-design.md.
The simulation stays deterministic (Charter rule 6): new randomness comes from seeded streams, new state goes into the world hash, and the save format gains it with a new save version that upgrades older saves (ADR-010, US-016).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Sickness">
Given a person weakened by cold, hunger or a hunting wound
When sickness or injury strikes
Then they are sick or injured for some days and may die of it
</scenario>
<scenario name="Nursing">
Given a sick person and a kind or close person nearby
When they are nursed
Then recovery is likelier and the patient remembers the carer with gratitude
</scenario>
<scenario name="Sharing and adoption">
Given a hungry child or an orphan
When a relative or friend has food or room
Then they share or adopt, and the one helped remembers it
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-112 ..." cases, the determinism hash test and the 100-year soak (US-015 Run).
Manual checks in docs/plans/US-112.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Standard algorithms: std::find_if, std::max_element, std::sort.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-113 Court and compete for a partner
```xml
<prompt id="S-US-113" codex="1.8" milestone="M2b" story="US-113" priority="Must" size="M">
<context>
Story US-113: Court and compete for a partner.
As a player, I want courtship with rivals and broken pairings, so that love brings drama as well as children.
Epic E11 Story engine: The clan's life becomes a readable story: episodes with causes, people and consequences.
Traces to: SDC-02. Owner decision D-18 (the story pivot after Kill Gate 1).
</context>
<dependencies>
Stories that must be Done: US-111.
Owner decisions that must be Decided: D-18.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation layer only: src/sim/ and apps/headless/, content numbers as data in assets/data/sim/ (Charter rule 7). No SDL3, no rendering (ADR-004). Follow docs/plans/M2b-story-design.md.
The simulation stays deterministic (Charter rule 6): new randomness comes from seeded streams, new state goes into the world hash, and the save format gains it with a new save version that upgrades older saves (ADR-010, US-016).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Courtship">
Given an unpaired adult who favours another
When they court (gifts, time together)
Then the other's opinion rises, and they pair only when both agree
</scenario>
<scenario name="Rivals">
Given two suitors courting the same person
When one is chosen
Then the rivals become jealous and may quarrel, and the one not chosen remembers it
</scenario>
<scenario name="Parting">
Given partners whose opinions of each other fall below the parting threshold
When a day ends
Then they part, and the chronicle records why
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-113 ..." cases, the determinism hash test and the 100-year soak (US-015 Run).
Manual checks in docs/plans/US-113.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Comparators and ranking (std::sort with a lambda).</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-114 Teach the young and hunt together
```xml
<prompt id="S-US-114" codex="1.8" milestone="M2b" story="US-114" priority="Must" size="L">
<context>
Story US-114: Teach the young and hunt together.
As a player, I want masters to teach apprentices and hunters to go out in parties, so that skills pass down and hunts create heroes and cowards.
Epic E11 Story engine: The clan's life becomes a readable story: episodes with causes, people and consequences.
Traces to: SDC-02. Owner decision D-18 (the story pivot after Kill Gate 1).
</context>
<dependencies>
Stories that must be Done: US-111.
Owner decisions that must be Decided: D-18.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation layer only: src/sim/ and apps/headless/, content numbers as data in assets/data/sim/ (Charter rule 7). No SDL3, no rendering (ADR-004). Follow docs/plans/M2b-story-design.md.
The simulation stays deterministic (Charter rule 6): new randomness comes from seeded streams, new state goes into the world hash, and the save format gains it with a new save version that upgrades older saves (ADR-010, US-016).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Apprenticeship">
Given a skilled adult and a youth of working age
When the adult takes the youth as apprentice
Then the youth's skill grows faster and they grow close
</scenario>
<scenario name="Hunting party">
Given a mammoth sighting
When a party of 3 to 5 hunters forms and hunts
Then roles emerge (leader, hero, coward), the outcome depends on the party, and each member remembers what the others did
</scenario>
<scenario name="Rescue">
Given a hunter in danger
When another hunter saves them
Then the rescued owes a debt of gratitude and the chronicle tells of it
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-114 ..." cases, the determinism hash test and the 100-year soak (US-015 Run).
Manual checks in docs/plans/US-114.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: A class that coordinates others (the hunting party).</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-115 Tell the clan's story in episodes
```xml
<prompt id="S-US-115" codex="1.8" milestone="M2b" story="US-115" priority="Must" size="L">
<context>
Story US-115: Tell the clan's story in episodes.
As a reader, I want the chronicle to tell fewer, bigger episodes with a beginning, a turn and an end, so that I can follow the clan's story.
Epic E11 Story engine: The clan's life becomes a readable story: episodes with causes, people and consequences.
Traces to: STO-02, STO-03, NA-02. Owner decision D-18 (the story pivot after Kill Gate 1).
</context>
<dependencies>
Stories that must be Done: US-110, US-111, US-112, US-113, US-114.
Owner decisions that must be Decided: D-18.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation layer only: src/sim/ and apps/headless/, content numbers as data in assets/data/sim/ (Charter rule 7). No SDL3, no rendering (ADR-004). Follow docs/plans/M2b-story-design.md.
The simulation stays deterministic (Charter rule 6): new randomness comes from seeded streams, new state goes into the world hash, and the save format gains it with a new save version that upgrades older saves (ADR-010, US-016).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Episode">
Given linked events (a lean autumn, thefts, blame, a feud, deaths)
When the episode ends
Then the chronicle writes it as one short named paragraph ('The Hard Winter of year 74') saying who, why and what came of it
</scenario>
<scenario name="Fewer, bigger">
Given a 100-year run
When I print the story
Then it tells at most 40 episodes, each naming its people and causes, followed by the births, deaths, pairings and feuds with their reasons
</scenario>
<scenario name="Both views">
Given the headless runner
When I ask for the story or for the full chronicle
Then --story prints the episodes and --chronicle still prints every event
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-115 ..." cases, the determinism hash test and the 100-year soak (US-015 Run).
Manual checks in docs/plans/US-115.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Designing with data: grouping and summarising.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M2b" codex="1.8" name="Exit review M2b = Kill Gate 1 retry">
<instructions>
1. Demonstrate the exit criteria: The chronicle tells the clan's story in fewer, bigger episodes with reasons; every death and feud says why; the 100-year soak still runs without crashing and the determinism test passes; the owner reads the printed story and judges whether it is a story (Kill Gate 1 retry).
2. Collect evidence into docs/gates/M2b.md, one section per criterion, each marked met or not met: the 100-year soak on several seeds, the determinism tests, and the printed story of the soak seed (`odysseus_headless --seed 7 --years 100 --story`), also saved as a reader packet in docs/gates/M2b-reader-packet.md.
3. This is the Kill Gate 1 retry, judged by the owner alone (D-18). Agents cannot judge it: write docs/decision-requests/D-GATE-M2b.md asking the owner whether the printed story is a story (options: Go; Go with notes; Pivot again; Stop), with the team's recommendation and reasons. Until the owner answers, do not start M3: end the session with the assembly report and a Milestone file.
4. When the owner answers Go: merge qa into main, push, confirm CI on main is green, tag the repository m2-done and m2b-done and push the tags, and save a Milestone-<n>.md snapshot. When the owner answers Pivot or Stop: stop adding content and raise codex issues for Anima with the owner's notes.
- Chronicle interest (early): the owner finds a story in the printed chronicle (measured by: the owner reads it, D-GATE-M2b)
- Determinism: Same seed + inputs give the same world hash after 10,000 ticks (measured by: Automated test in CI)
- Kill / pivot rule: If M2 or M6 fails: stop adding content; redesign the simulation or the loop first (measured by: Owner decision, recorded)
</instructions>
<output_format>Assembly report with the gate result.</output_format>
</prompt>
```

### M2c Level editor (Game mode and Editor mode)
Exit criteria: The owner switches between Game mode and Editor mode; in the Editor he paints ground tiles, places characters with properties and sets the level's name, size, default ground and hero start; he saves, reloads and plays the level; the game draws his own art; the spear and sword demos still work.

Why this milestone exists: after Kill Gate 1 passed, the owner asked for an editor before M3 so he can build the world himself with his own art (D-19, D-05). M3 then puts the living clan into levels made with it.

Design notes for every M2c prompt (Anima, from the brief docs/plans/M2c-editor-brief.md):
- Layers (Charter rules 1, 2, 4, 9): mouse events are translated in Platform; the Engine turns them into a game-agnostic **pointer** (position in virtual pixels, buttons, wheel) next to the intents, with scripted pointer input for tests; the **UI toolkit** (bitmap font, panel, button, list, number field) is Luna Engine and knows nothing of Odysseus; the **Editor**, **Level** and **art library** are Game code in src/game/ (src/game/editor/ for the editor). The simulation is not touched.
- Art (D-05, D-04, D-16): the owner's sheets in assets/sprites/ are labelled pictures (dark or white backgrounds, text labels, no fixed grid, 60-125 px per figure or tile). They are never edited. A cutter program cuts figures and tiles by rectangles listed in `assets/sprites/cuts.json`, removes the background, scales them (box filter) to 32x48 characters (feet at the bottom centre, transparent padding) and 32x32 tiles, and writes atlases plus an atlas index JSON to `assets/sprites/atlas/`. The game loads only the atlases. The landscape pictures are not tiles and are not used in M2c.
- Libraries: stb_image and stb_image_write (single headers in third_party/, public domain or MIT) read and write PNG. Record ADR-018 (why, and why the editor UI is our own toolkit and not Dear ImGui: ImGui's platform backend would bring SDL3 into Game code).
- Data (Charter rules 7 and 8, ADR-010): `assets/data/tiles.json` (tile kinds: name, atlas cell, solid), `assets/data/characters.json` (character kinds: name, atlas cells, defaults: HP, sword damage), and level files `assets/levels/<name>.json` (versioned: name, width, height, default ground, ground grid, placed characters with id, kind, position, facing, name, HP, sword damage; hero start). Levels save like the world saves: temp file then rename, 3 backups, errors name file and field.
- Tests: logic headless in `odysseus_game_tests` (Level model, JSON round trips, validation errors, brush, fills, undo and redo, character edits, the cutter's rectangles and scaling); the window end to end with scripted input (`--hold`, a new `--click x:y:time[:button]` and `--key`), screenshots by `--screenshot`, and pixel checks.
- UX at 480x270: icon buttons with the name shown on hover, palettes at the screen edges, never covering the map's centre; the mode is always shown in a corner ("GAME" / "EDITOR").

```xml
<prompt id="K-M2c" codex="1.8" name="Kick off M2c Level editor">
<instructions>
1. Confirm docs/gates/M2b.md records the passed gate (D-GATE-M2b: Go) and that D-05 and D-19 are Decided in docs/decisions.md.
2. Architect: write docs/plans/M2c-editor-design.md before US-120, following the design notes above: the cutter and the atlas format (review the owner's sheets and choose, per character, the frames that exist: the best 8-direction hero sheet, mirroring where a direction is missing, front-view monsters with one frame); the pointer and the widget toolkit (how widgets receive the pointer and draw text); the Level format and its version; the editor's tools and its command history (undo and redo); how Game mode starts from a level; and which of today's demo objects (test map, straw targets, enemy, sword) become level data.
3. Set this milestone's stories to To do in docs/status.md in this order: US-120, US-121, US-122, US-123, US-124, US-125, US-126.
4. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, decisions requested.</output_format>
</prompt>
```

#### S-US-120 Real art in the game
```xml
<prompt id="S-US-120" codex="1.8" milestone="M2c" story="US-120" priority="Must" size="L">
<context>
Story US-120: Real art in the game.
As a player, I want the heroes, monsters and ground drawn from the new art, so that the world looks like the game I imagine.
Epic E12 Level editor: The owner builds levels by hand (ground tiles, characters and settings) and plays them at once.
Traces to: D-05, D-19, D-04, D-16.
</context>
<dependencies>
Stories that must be Done: none in M2c.
Owner decisions that must be Decided: D-05, D-19.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: PNG reading and writing in Luna Engine (an Image loader and writer over stb_image / stb_image_write, ADR-018); the cutting logic in Game code (src/game/art/); the cutter program apps/atlas/ (target odysseus_atlas, Game identity) with `--preview` writing a contact sheet of every cut frame for review; the rectangles in assets/sprites/cuts.json; the atlases and atlas index in assets/sprites/atlas/ (committed). Follow the M2c design notes and docs/plans/M2c-editor-design.md.
Cover at least: one hero with 8 directions and a walk cycle (mirrored where the sheets lack a direction), 10 monsters, and 12 ground tiles including grass, dirt, path, stone, sand, snow, water and mud. The game keeps its programmer art as the fallback when an atlas is missing or damaged, and logs which file and why.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Atlas">
Given the owner's sprite sheets in assets/sprites
When the cutter tool runs
Then it writes clean atlases (transparent background, 32x48 characters, 32x32 tiles) from rectangles kept in JSON
</scenario>
<scenario name="Heroes and ground">
Given the game starts
When the hero walks the test map
Then the hero, the ground tiles and the enemy are drawn from the atlases, not programmer art
</scenario>
<scenario name="Missing art">
Given an atlas file that is missing or damaged
When the game starts
Then it names the file and falls back to programmer art
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-120 ..." cases and the existing US-024 and US-029 end-to-end tests.
Manual checks in docs/plans/US-120.md done, with results recorded there: the contact sheet (docs/evidence/US-120/) and a game screenshot reviewed; every cut frame is a whole figure or tile with no label text or background left.
</verification>
<teach_back>C++ concept for the owner: Reading binary files; structs of rectangles.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-121 Point, click and read on screen
```xml
<prompt id="S-US-121" codex="1.8" milestone="M2c" story="US-121" priority="Must" size="M">
<context>
Story US-121: Point, click and read on screen.
As a level designer, I want to point, click and read labels on screen, so that I can use editor tools.
Epic E12 Level editor: The owner builds levels by hand (ground tiles, characters and settings) and plays them at once.
Traces to: D-19, ARC-03.
</context>
<dependencies>
Stories that must be Done: none in M2c.
Owner decisions that must be Decided: D-19.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: mouse and text-entry events in Luna Platform (src/luna/platform/, SDL3 only there); the pointer, keyboard shortcuts (Ctrl+Z, Ctrl+Y, Ctrl+S, F1, F2, Delete, digits and letters for text fields) and scripted `--click` / `--key` input in Luna Engine; the bitmap font (printable ASCII, drawn by code like the programmer art, at least 5x7 pixels per glyph) and the widgets (panel, button with icon and hover label, scrolling list, number field, text field) in Luna Engine, game-agnostic. Follow the M2c design notes and docs/plans/M2c-editor-design.md.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Pointer">
Given the mouse moves, clicks or scrolls
When the events arrive
Then the game reads a pointer (position in virtual pixels, buttons, wheel), never raw SDL3 events
</scenario>
<scenario name="Text">
Given any label in printable ASCII
When it is drawn
Then it is readable and pixel-exact at 480x270
</scenario>
<scenario name="Widgets">
Given a panel with buttons, a list and a number field
When they are clicked or typed into
Then the right action runs (tested headless)
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-121 ..." cases, the platform event-translation tests and "US-021 Game reads only intents" (extended to the pointer).
Manual checks in docs/plans/US-121.md done, with results recorded there: a screenshot of a test panel with every glyph and each widget.
</verification>
<teach_back>C++ concept for the owner: Classes with virtual functions (widgets).</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-122 Levels as data
```xml
<prompt id="S-US-122" codex="1.8" milestone="M2c" story="US-122" priority="Must" size="M">
<context>
Story US-122: Levels as data.
As a level designer, I want levels saved as files, so that what I build stays and can be shared.
Epic E12 Level editor: The owner builds levels by hand (ground tiles, characters and settings) and plays them at once.
Traces to: D-19, ARC-08.
</context>
<dependencies>
Stories that must be Done: US-120.
Owner decisions that must be Decided: D-19.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game code (src/game/level.{h,cpp} and the definitions loaders); data in assets/data/tiles.json, assets/data/characters.json and assets/levels/ (Charter rules 7 and 8). The built-in test map, the straw targets' positions and the demo enemy become the first level file, `assets/levels/valley.json`, and the game loads it at start (a `--level <file>` option chooses another). Follow the M2c design notes and docs/plans/M2c-editor-design.md.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Load">
Given a level file
When the game starts
Then the map, ground, characters and hero start come from the file; the demo enemy is a placed character
</scenario>
<scenario name="Round trip">
Given a level
When it is saved and loaded
Then it is exactly the same
</scenario>
<scenario name="Damaged">
Given a damaged level file
When it is loaded
Then the error names the file and the field, and the last good backup is used
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-122 ..." cases and the existing US-024 and US-029 end-to-end tests (now on valley.json).
Manual checks in docs/plans/US-122.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Reading and writing JSON with validation.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-123 Game mode and Editor mode
```xml
<prompt id="S-US-123" codex="1.8" milestone="M2c" story="US-123" priority="Must" size="S">
<context>
Story US-123: Game mode and Editor mode.
As a level designer, I want to switch between playing and editing, so that I can try what I build at once.
Epic E12 Level editor: The owner builds levels by hand (ground tiles, characters and settings) and plays them at once.
Traces to: D-19.
</context>
<dependencies>
Stories that must be Done: US-121, US-122.
Owner decisions that must be Decided: D-19.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game code (src/game/ and src/game/editor/): a mode state (enum class) in OdysseyGame; F1 = Game, F2 = Editor (also `--editor` to start in it); the mode name always shown in a corner; in the Editor the world is paused, the camera pans with the arrow keys, WASD or a dragged right button and zooms not at all (pixel art stays 1:1); going back to Game rebuilds the play state from the edited level (hero at the hero start). Follow the M2c design notes and docs/plans/M2c-editor-design.md.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Switch">
Given the game running
When I press F2
Then Editor mode is shown, the world pauses and the camera pans freely
</scenario>
<scenario name="Back to play">
Given the Editor with changes
When I press F1
Then the game plays the edited level from the hero start
</scenario>
<scenario name="Game untouched">
Given Game mode
When I play
Then the spear and sword demos behave as before
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-123 ..." cases, an end-to-end run that presses F2 and F1 by script with screenshots, and the existing US-024 and US-029 end-to-end tests.
Manual checks in docs/plans/US-123.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: State machines (enum class).</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-124 Paint ground tiles
```xml
<prompt id="S-US-124" codex="1.8" milestone="M2c" story="US-124" priority="Must" size="M">
<context>
Story US-124: Paint ground tiles.
As a level designer, I want to paint ground tiles by hand, so that I shape the world.
Epic E12 Level editor: The owner builds levels by hand (ground tiles, characters and settings) and plays them at once.
Traces to: D-19.
</context>
<dependencies>
Stories that must be Done: US-123.
Owner decisions that must be Decided: D-19.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game code (src/game/editor/): a tile palette (every tile kind of tiles.json, with its name on hover); tools brush (click and drag), rectangle fill, flood fill (4-neighbour, iterative, never recursive) and eraser (paints the level's default ground); a grid overlay (toggle G); a command history with undo (Ctrl+Z) and redo (Ctrl+Y), one command per stroke or fill, at least 100 steps; Ctrl+S saves the level (US-122's safe save). Tiles marked solid block walking in Game mode. Follow the M2c design notes and docs/plans/M2c-editor-design.md.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Paint">
Given a tile chosen in the palette
When I click or drag on the map
Then the cells change; solid tiles block walking in Game mode
</scenario>
<scenario name="Fill">
Given an area
When I use the rectangle or flood fill
Then the whole area is painted in one step
</scenario>
<scenario name="Undo">
Given a series of edits
When I press Ctrl+Z or Ctrl+Y
Then the level steps back and forward exactly
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-124 ..." cases (among them random edit sequences whose undo and redo match a simple model) and an end-to-end paint, save and play run by script.
Manual checks in docs/plans/US-124.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: The command pattern (undo and redo).</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-125 Place characters
```xml
<prompt id="S-US-125" codex="1.8" milestone="M2c" story="US-125" priority="Must" size="M">
<context>
Story US-125: Place characters.
As a level designer, I want to place heroes and monsters in the world, so that the game has someone to meet.
Epic E12 Level editor: The owner builds levels by hand (ground tiles, characters and settings) and plays them at once.
Traces to: D-19.
</context>
<dependencies>
Stories that must be Done: US-123, US-124.
Owner decisions that must be Decided: D-19.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game code (src/game/editor/ and the level's character list): a character palette (every kind of characters.json); place by click; select by click (outline), move by dragging, turn with R (8 facings; front-view monsters show their single frame), delete with Delete; a properties panel for the selected character (name, HP, sword damage) with number and text fields; every edit is an undoable command (US-124's history). Placed characters get stable ids (never reused) and are stored by value in the level; nothing keeps raw pointers into the list. In Game mode placed characters stand still (D-19); the hero's sword hits any placed enemy in reach, with the red flash and the HP label of today's demo enemy. Follow the M2c design notes and docs/plans/M2c-editor-design.md.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Place">
Given a character chosen in the palette
When I click on the map
Then it appears there, facing south, with default properties
</scenario>
<scenario name="Edit">
Given a placed character
When I select, move, turn or delete it, or change its name, HP or sword damage
Then the change is shown and saved
</scenario>
<scenario name="Play">
Given a placed enemy
When I hit it with the sword in Game mode
Then it takes damage, flashes red and is defeated at 0 HP
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-125 ..." cases and an end-to-end place, save, play and strike run by script with screenshots.
Manual checks in docs/plans/US-125.md done, with results recorded there.
</verification>
<teach_back>C++ concept for the owner: Owning objects in a std::vector; ids instead of pointers.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-126 Level and character settings
```xml
<prompt id="S-US-126" codex="1.8" milestone="M2c" story="US-126" priority="Must" size="S">
<context>
Story US-126: Level and character settings.
As a level designer, I want to set the level's name, size, default ground and the hero's start, so that each level is my own.
Epic E12 Level editor: The owner builds levels by hand (ground tiles, characters and settings) and plays them at once.
Traces to: D-19.
</context>
<dependencies>
Stories that must be Done: US-125.
Owner decisions that must be Decided: D-19.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game code (src/game/editor/): a level settings panel (name; width and height from 8 to 256 tiles; default ground); a resize keeps the painted tiles where they are, fills new cells with the default ground, and drops characters that fall outside (undoable); the hero start is a marker the owner drags; new level and open level (from assets/levels/) with a prompt to save unsaved changes. The owner-facing guide docs/guides/editor.md explains every control in plain words with screenshots. Follow the M2c design notes and docs/plans/M2c-editor-design.md.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Settings">
Given a level
When I change its name, size or default ground
Then it is saved and reloaded; a resize keeps the painted tiles
</scenario>
<scenario name="Hero start">
Given the hero start marker
When I move it
Then Game mode starts the hero there
</scenario>
<scenario name="Guide">
Given the owner
When he opens docs/guides/editor.md
Then every control is explained in plain words
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
cmake --preset windows-x64-debug, then cmake --build --preset windows-x64-debug: zero warnings.
cmake --build --preset windows-x64-release: zero warnings.
ctest --preset windows-x64-debug and windows-x64-release: all tests pass, including the "US-126 ..." cases.
Manual checks in docs/plans/US-126.md done, with results recorded there: the guide read against the running editor, control by control.
</verification>
<teach_back>C++ concept for the owner: Resizing a 2D grid stored in one vector.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M2c" codex="1.8" name="Exit review M2c">
<instructions>
1. Demonstrate the exit criteria: The owner switches between Game mode and Editor mode; in the Editor he paints ground tiles, places characters with properties and sets the level's name, size, default ground and hero start; he saves, reloads and plays the level; the game draws his own art; the spear and sword demos still work.
2. Collect evidence into docs/gates/M2c.md, one section per criterion, each marked met or not met: a scripted end-to-end session (paint, place, set, save, restart, load, play, strike) with screenshots in docs/evidence/X-M2c/; the full test run; the guide.
3. No kill gate: invite the owner to try the editor with docs/guides/editor.md and record his notes when he gives them; they become new stories through Anima, never silent changes.
4. Merge qa into main, push, confirm CI on main is green, tag the repository m2c-done and push the tag, and save a Milestone-<n>.md snapshot. Then continue with K-M3.
</instructions>
<output_format>Assembly report with the review result.</output_format>
</prompt>
```

### M2d Content and combat (weapons, nature, effects, weather)
Exit criteria: The owner places weapon pickups, plants, animals and looping effects with the Editor, saves and plays the level; the hero picks weapons into a 9-slot hotbar and fights with the 16 starter weapons of 8 classes, elements included, aiming them with the mouse and shooting arcs with bows, crossbows, thrown weapons and staffs; enemies (goblins, predators and boars) strike back when hit and in reach, and die; the hero respawns at 0 HP; plants block, are inspected, chopped and eaten and regrow; effects play on hits, deaths and plant actions; the weather changes by itself; every M2c feature and the spear and sword demos still work.

Why this milestone exists: after M2c the owner added seven sprite sheets and asked for them to be processed and added to the game with full gameplay, before M3 (D-21). Every design choice below was taken by the owner in chat (D-22); none is open.

Design notes for every M2d prompt (Anima, from the brief docs/plans/M2d-content-brief.md):
- Decisions (D-22): the owner takes every design decision. When a prompt meets a design or scope question that this Codex does not answer, Mraw stops that story, asks the owner in chat (AskUserQuestion rounds, recommended option first), records the answer in docs/decisions.md as "Decided (owner, date)", and continues. Technical choices stay with Dominus and are recorded in ADRs or the design document.
- Art (D-05): the seven new sheets in assets/sprites/ are never edited. They are cut by the existing cutter (odysseus_atlas, cuts.json) into new atlases in assets/sprites/atlas/: weapons 32x32 icons; plants by size class (small 32x32, tall 32x64, tree 64x96, feet at the bottom centre); animals scaled to fit 64x48 (feet at the bottom centre; side view, the other side mirrored); effects as 4-frame strips (the elemental grid as single frames); weather as 4-frame screen overlays (at least 160x90 per frame, tiled or scaled to the 480x270 virtual screen). Labelled sheets: the label strip is excluded from each rectangle, and the painted background is removed with a per-cut background sample and opaque-bounds trimming; a cut may carry its own background colour and tolerance in cuts.json. Printed numbers on the sheets are unreliable; catalogs use names.
- Data (Charter rules 7 and 8): `assets/data/weapons.json` (all 150: name, atlas frame, class, element or none, era fantasy or future, starter yes or no, damage, speed, range; the class gives the attack), `assets/data/plants.json` (all 150: name, frame, size class, blocks, edible, inspect text), `assets/data/characters.json` extended with the 50 animals (enemy flag, strike damage, reach; defaults HP), `assets/data/effects.json` (name, frames, frame time, loop or one-shot, anchor), `assets/data/weather.json` (name, frames, frame time, weight; "clear" carries about a third of the total weight). Every file is validated with errors naming file and field.
- Levels: format version 2 adds weapon pickups, plants and placed effects; version 1 files still load and are saved as version 2. demo.json stays the fixture every test uses (D-20).
- Combat (owner numbers, all in JSON): hero 100 HP; enemies strike back once per hit after a 0.5 s telegraphed wind-up if the hero is within their reach (1.5 m by default); goblins and small predators 5-10 damage, big predators 15-20; the hero at 0 HP fades and respawns at the hero start with 100 HP, enemies keep their HP; enemies at 0 HP die (smoke effect) and are gone until the level restarts; nothing drops. Enemies = goblins plus wolf, fox, bear, cougar, lynx, leopard, jaguar, cheetah, lion, tiger, snow leopard, hyena, jackal, boar, wild pig, rhino, hippopotamus, bull, buffalo, water buffalo; every other animal is a bystander that cannot be hit.
- Weapons: 8 classes, each with its own attack: sword (arc in front), axe (heavy, slow arc), spear (long thrust), bow and crossbow (arrow projectile), thrown (spinning projectile), whip and flail (long reach arc), staff (magic bolt), gun (fast projectile). The starter set is one plain and one elemental weapon per class (16), chosen by Dominus in US-133 and shown to the owner on a numbered contact sheet; swapping is editing `starter` in weapons.json. Elements: fire burns over time, ice slows, lightning chains to one more enemy nearby, poison damages over time, void and dark heal the hero by part of the damage.
- Plants: big plants (trees, big bushes) block walking like solid tiles; small ones are walk-over. Interact next to a plant shows its name and inspect text. Any weapon hit destroys a plant (leaf burst); an edible one heals the hero a flat 10 HP (healing glow). 15 s after a plant is destroyed, the same kind regrows at a random free spot inside the current camera view (growth effect).
- Effects: a Luna Engine effect player (game-agnostic: frames, frame time, loop or once, world or screen space) draws effects; the Game decides which plays when: hit sparks, element effects, death smoke, projectile trails, leaf burst, healing glow, growth, and Editor-placed looping effects saved in the level.
- Weather: visual only. A random weather every 60-120 s of play, cross-fading over 3 s, from weather.json weights; drawn as a screen overlay above the world and below the UI; its random stream is a seeded PCG32 stream (Charter rule 6), so a test with a seed sees the same weathers in the same order.
- Hotbar and intents (Charter rule 4): new intents Slot1..Slot9 (keys 1-9); Shift keeps cycling weapons; walking over a pickup puts it in the first free slot (a full hotbar leaves the pickup lying). The hotbar and the hero's HP are drawn with the Luna UI toolkit.
- Aiming and ballistics (D-25, stories US-139..US-141, before plants): free aim at the mouse cursor; the hero faces the pointer (8 facings) and melee arcs are centred on the exact angle to it; shots are Luna Physics arcs (fixed-point, height, gravity) whose launch angle is solved to land at the pointer, clamped to the weapon's range; a shot hits the first enemy whose body (feet to about 1.5 m) it passes through, is blocked by rocks and trees when low and clears them when high, and a miss sticks in the ground and vanishes; a ground shadow and a lifted sprite show the height. Bows, crossbows and thrown weapons arc, staff bolts fly flat and fast, guns are not part of this scope; no ammo, only the rate-of-fire cooldown.
- Tests: logic headless in odysseus_game_tests (catalog validation, level v1 to v2, strike-back timing, death and respawn, each weapon class, each element, plant block, chop, heal and regrow placement, weather sequence per seed); the window end to end with scripted input and screenshots, as in M2c.

```xml
<prompt id="K-M2d" codex="1.8" name="Kick off M2d Content and combat">
<instructions>
1. Confirm docs/gates/M2c.md records M2c done and that D-21 and D-22 are Decided in docs/decisions.md.
2. Architect: write docs/plans/M2d-content-design.md before US-130, following the design notes above: the cut plan per sheet (grid or labelled, frame sizes, background handling), the catalog formats, level format version 2, the combat model (HP, strike-back state machine, death and respawn), the weapon class interface and projectiles, status effects, plants in the world (blocking, interaction, regrow), the effect player, and the weather cycle.
3. Set this milestone's stories to To do in docs/status.md in this order: US-130, US-131, US-132, US-133, US-134, US-135, US-139, US-140, US-141, US-136, US-137, US-138.
4. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, decisions requested.</output_format>
</prompt>
```

#### S-US-130 Content catalogs from the new sheets
```xml
<prompt id="S-US-130" codex="1.8" milestone="M2d" story="US-130" priority="Must" size="L">
<context>
Story US-130: Content catalogs from the new sheets.
As the owner, I want my seven new sprite sheets cut into game atlases and catalogs, so that weapons, plants, animals, effects and weather can be used in the game.
Epic E13 Content and combat: The owner's weapons, plants, animals, effects and weather are in the game and playable.
Traces to: D-05, D-21, ARC-08.
</context>
<dependencies>
Stories that must be Done: US-120.
Owner decisions that must be Decided: D-05, D-21.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: new cuts in assets/sprites/cuts.json; any new cutting logic (label exclusion, per-cut background, grids) in the existing Game art code and odysseus_atlas; atlases in assets/sprites/atlas/; catalogs in assets/data/ (weapons.json, plants.json, effects.json, weather.json; the animals added to characters.json with bystanders and enemies as in the design notes). Loading and validating the catalogs is Game code with tests. Commit the seven sheets unchanged.
`odysseus_atlas --preview` writes one numbered contact sheet per catalog to docs/evidence/US-130/ with each item's catalog name, for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Cut">
Given the seven sheets and cuts.json
When the atlas tool runs
Then atlases and JSON catalogs exist for weapons (150), plants (150), animals (50), effects (200) and weather (100), with backgrounds removed
</scenario>
<scenario name="Review">
Given the contact sheets in docs/evidence/US-130/
When the owner opens them
Then every item is shown with its catalog name and number
</scenario>
<scenario name="Valid">
Given a catalog with a wrong field
When the game loads it
Then the error names the file and the field
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
Debug and Release builds with zero warnings; ctest in both presets passes, including the "US-130 ..." cases.
Manual checks in docs/plans/US-130.md done, with results recorded there: every contact sheet reviewed; no label text, halo or cut-off part in any frame (fix the cut, not the sheet).
</verification>
<teach_back>C++ concept for the owner: Reading data tables with std::map and validating them.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-131 Hero HP, fighting back and death
```xml
<prompt id="S-US-131" codex="1.8" milestone="M2d" story="US-131" priority="Must" size="M">
<context>
Story US-131: Hero HP, fighting back and death.
As the player, I want enemies to strike back when I hit them, so that fighting has risk.
Epic E13 Content and combat. Traces to: D-21.
</context>
<dependencies>
Stories that must be Done: US-125.
Owner decisions that must be Decided: D-21.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game code (hero, enemy and a small combat model with an enum-class state machine: idle, wind-up, strike); numbers from characters.json (strike damage, reach) with defaults 10 damage and 1.5 m; the hero's HP drawn on screen with the Luna UI toolkit. The wind-up shows a telegraph (the enemy flashes) for 0.5 s; the strike lands only if the hero is still within reach when it ends. Hero at 0 HP: a short fade, respawn at the level's hero start with 100 HP; enemies keep their HP. Enemies at 0 HP die and are removed until the level restarts (the death effect arrives with US-132; until then they simply vanish).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Strike back">
Given an enemy the hero hits while standing within its reach
When 0.5 s pass with a warning flash
Then the hero loses the enemy kind's strike damage
</scenario>
<scenario name="Out of reach">
Given the hero hits an enemy from afar with the spear throw, or steps away during the wind-up
When the wind-up ends
Then the hero loses nothing
</scenario>
<scenario name="Death and respawn">
Given the hero or an enemy reaching 0 HP
When it happens
Then the hero respawns at the hero start with 100 HP; the enemy is removed until the level restarts
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
Debug and Release builds with zero warnings; ctest in both presets passes, including the "US-131 ..." cases and the M2c tests.
Manual checks in docs/plans/US-131.md done, with results recorded there: a scripted fight screenshot showing the hero's HP and a telegraph flash.
</verification>
<teach_back>C++ concept for the owner: A small state machine with enum class.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-132 Effect player
```xml
<prompt id="S-US-132" codex="1.8" milestone="M2d" story="US-132" priority="Must" size="M">
<context>
Story US-132: Effect player.
As the player, I want hits, deaths and projectiles to show effects, so that combat reads clearly.
Epic E13 Content and combat. Traces to: D-21, ARC-09.
</context>
<dependencies>
Stories that must be Done: US-130.
Owner decisions that must be Decided: D-21.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: the effect player in Luna Engine (game-agnostic: frames from an atlas, frame time in ticks, one-shot or looping, world or screen position, drawn in depth order with the world); which effect plays when is Game code, named in effects.json. Wire in: a hit spark on every weapon hit, a smoke puff when a character dies, a trail behind the thrown spear and arrows.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="One-shot">
Given an effect started in the world
When its frames have played
Then it disappears
</scenario>
<scenario name="Looping">
Given a looping effect
When time passes
Then it keeps playing until it is removed
</scenario>
<scenario name="Combat">
Given a sword hit, a death or a spear in flight
When it happens
Then a hit spark, a smoke puff or a trail plays
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
Debug and Release builds with zero warnings; ctest in both presets passes, including the "US-132 ..." cases.
Manual checks in docs/plans/US-132.md done, with results recorded there: screenshots of each wired effect.
</verification>
<teach_back>C++ concept for the owner: Timers and animation frames in a fixed timestep.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-133 Weapon classes and the starter set
```xml
<prompt id="S-US-133" codex="1.8" milestone="M2d" story="US-133" priority="Must" size="L">
<context>
Story US-133: Weapon classes and the starter set.
As the player, I want each kind of weapon to fight differently, so that choosing a weapon matters.
Epic E13 Content and combat. Traces to: D-21.
</context>
<dependencies>
Stories that must be Done: US-130, US-132.
Owner decisions that must be Decided: D-21.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game code. One weapon-class interface with the 8 classes of the design notes (sword, axe, spear, bow and crossbow, thrown, whip and flail, staff, gun); projectiles for bow, thrown, staff and gun; damage, speed (attacks per second) and range from weapons.json. Today's sword and spear become the sword and spear classes, with their tests kept. Pick the 16 starters (one plain and one elemental per class), set `starter` in weapons.json, and write their numbered contact sheet to docs/evidence/US-133/ for the owner to review; until US-134, the starters are cycled with Shift for testing. The held weapon is drawn in the hero's hand from its icon, rotated and mirrored with the facing (reuse the US-029 sword hand point).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Classes">
Given each of the 8 classes
When the hero attacks
Then its own attack shape, speed and range are used, from weapons.json
</scenario>
<scenario name="Starter set">
Given the 16 starter weapons
When the owner reviews their contact sheet
Then any starter can be swapped by editing weapons.json, with no code change
</scenario>
<scenario name="In hand">
Given a weapon selected
When the hero walks in any of the 8 directions
Then the weapon's icon is drawn in the hero's hand
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
Debug and Release builds with zero warnings; ctest in both presets passes, including the "US-133 ..." cases and the US-029 sword and spear tests.
Manual checks in docs/plans/US-133.md done, with results recorded there: a screenshot per class attacking a goblin; the starter contact sheet.
</verification>
<teach_back>C++ concept for the owner: Virtual functions or std::variant for weapon classes.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-134 Pickups and the hotbar
```xml
<prompt id="S-US-134" codex="1.8" milestone="M2d" story="US-134" priority="Must" size="M">
<context>
Story US-134: Pickups and the hotbar.
As a level designer, I want to place weapons in my levels for the hero to find, so that levels have rewards.
Epic E13 Content and combat. Traces to: D-21, ADR-010.
</context>
<dependencies>
Stories that must be Done: US-133.
Owner decisions that must be Decided: D-21.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: level format version 2 (pickups: weapon name and position), reading version 1 unchanged; a weapon palette in the Editor listing the starter weapons (placing, moving and deleting pickups works like characters, with undo and redo); new intents Slot1..Slot9 in Platform and Engine; the 9-slot hotbar in Game code, drawn at the bottom centre with the Luna UI toolkit. Walking over a pickup puts the weapon in the first free slot and removes the pickup until the level restarts; a full hotbar leaves it lying. 1-9 select a slot, Shift cycles through the filled slots. The hero starts with an empty hotbar unless demo levels place pickups (demo.json gets the sword and the spear as pickups next to the hero start so the M1b and M2c tests keep working).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Place">
Given the Editor
When I place a weapon pickup and save
Then the level file (version 2) keeps it, and a version 1 file still loads
</scenario>
<scenario name="Pick up">
Given the hero walks over a pickup
When it happens
Then the weapon goes into the first free of 9 hotbar slots
</scenario>
<scenario name="Select">
Given weapons in the hotbar
When I press 1-9 or Shift
Then that weapon, or the next one, is held
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
Debug and Release builds with zero warnings; ctest in both presets passes, including the "US-134 ..." cases and every earlier end-to-end test.
Manual checks in docs/plans/US-134.md done, with results recorded there: a screenshot of the hotbar with three weapons; docs/guides/editor.md explains pickups and the hotbar.
</verification>
<teach_back>C++ concept for the owner: Upgrading a saved file format (version 1 to 2).</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-135 Elements
```xml
<prompt id="S-US-135" codex="1.8" milestone="M2d" story="US-135" priority="Should" size="S">
<context>
Story US-135: Elements.
As the player, I want elemental weapons to do something special, so that they feel different.
Epic E13 Content and combat. Traces to: D-21.
</context>
<dependencies>
Stories that must be Done: US-133.
Owner decisions that must be Decided: D-21.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game code: status effects as small structs on characters (burn, slow, poison; a chain jump and a drain happen at the hit). Numbers in weapons.json per element (for example burn 2 HP per second for 3 s, slow to 50% for 2 s, chain to the nearest enemy within 3 m for half damage, poison 1 HP per second for 5 s, drain 25% of the damage to the hero); each shows its effect from effects.json while it lasts.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Over time">
Given a hit with a fire or poison weapon
When seconds pass
Then the target loses HP over time with its effect showing
</scenario>
<scenario name="Ice and void">
Given a hit with an ice or a void weapon
When it happens
Then the target is slowed, or part of the damage heals the hero
</scenario>
<scenario name="Lightning">
Given a hit with a lightning weapon and a second enemy nearby
When it happens
Then the lightning jumps to that enemy
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
Debug and Release builds with zero warnings; ctest in both presets passes, including the "US-135 ..." cases (one per element).
Manual checks in docs/plans/US-135.md done, with results recorded there: a screenshot per element.
</verification>
<teach_back>C++ concept for the owner: Components: small structs attached to characters.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-139 Mouse aiming
```xml
<prompt id="S-US-139" codex="1.9" milestone="M2d" story="US-139" priority="Must" size="M">
<context>
Story US-139: Mouse aiming.
As the player, I want to aim my weapon with the mouse, so that I can attack exactly where I point.
Epic E13 Content and combat. Traces to: D-25 (owner, 2026-10-01). Brief: docs/plans/M2d-aiming-brief.md.
</context>
<dependencies>
Stories that must be Done: US-133, US-134.
Owner decisions that must be Decided: D-25.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Luna Engine gives the Game the pointer's world position (pointer plus camera; Luna stays game-agnostic) and an Intent for the attack button (left mouse button; Charter rule 4: Game code reads intents, never raw buttons). Game code: in Game mode, the hero faces the pointer (the nearest of the 8 facings) and keeps walking with the keys; the attack intent uses the held catalog weapon toward the pointer (the Interact key keeps attacking toward the facing, so today's tests and scripts still work); melee arcs (MeleeBehaviour) are centred on the exact angle to the pointer, not on the facing; a small aim line from the hero and a crosshair at the pointer are drawn while a catalog weapon is held; the Editor keeps using the pointer for its tools, unchanged. Add a scripted `--aim <x>:<y>:<from>:<to>` flag to apps/odysseus/main.cpp (like --point) so tests and screenshots can aim. Do not change projectile flight yet (US-140).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Face the cursor">
Given a weapon is held in Game mode
When the mouse pointer moves around the hero
Then the hero faces the pointer's nearest of the 8 directions and an aim line and crosshair follow the pointer
</scenario>
<scenario name="Swing toward the cursor">
Given a melee weapon is held and an enemy stands to the north-east
When the pointer is north-east of the hero and the attack button is pressed
Then the swing hits the enemy; with the pointer to the south-west the same swing misses it
</scenario>
<scenario name="Keys still work">
Given the Interact key
When it is pressed with no pointer movement
Then the attack goes toward the hero's facing as before, and every earlier test still passes
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
Debug and Release builds with zero warnings; ctest in both presets passes, including the "US-139 ..." cases and all earlier weapon, combat and spear tests.
Manual checks in docs/plans/US-139.md done, with results recorded there: screenshots of the hero facing two different pointer positions with the aim line, and a melee swing toward the pointer hitting a goblin.
</verification>
<teach_back>C++ concept for the owner: Converting between coordinate spaces (screen, world) and angles with atan2.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-140 Arc ballistics for shots
```xml
<prompt id="S-US-140" codex="1.9" milestone="M2d" story="US-140" priority="Must" size="L">
<context>
Story US-140: Arc ballistics for shots.
As the player, I want shots to fly as real arcs that land where I aim, so that aiming and distance matter.
Epic E13 Content and combat. Traces to: D-25 (owner, 2026-10-01), ARC-10, ADR-017.
</context>
<dependencies>
Stories that must be Done: US-139.
Owner decisions that must be Decided: D-25.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game code using Luna Physics (fixed-point 32.32, SI units, 1 tile = 1 m; Charter rule 10). Reuse the functions of the US-027 and US-029 work (ballistics flight per tick, aimLaunchAngle, launchVelocity, the world collision and material hits) instead of copying them; add a Game-side `ArcShot` that holds a Luna Physics projectile with height. At launch the angle is solved so the shot lands at the pointer, with the distance clamped to the weapon's range (an unreachable pointer lands at the range limit). Each tick the shot advances one physics tick; it hits the first enemy whose body it passes through (feet to about 1.5 m high; the enemy's position is its feet, as today), hits solids (rock, tree, wall tiles) when it is below their height and flies over them when above, and otherwise ends at the ground: a miss sticks in the ground briefly (an effect) and is removed. Drawing: the sprite is lifted by its height and a small shadow is drawn on the ground; the conversion from physics position to screen stays in one place (luna/engine/physics_view). Damage, elements and the provoke rule reuse the existing strike path. The flat projectile code from US-133 stays for staff bolts (US-141). Keep the M1b spear-throw demo and its tests working.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Lands at the cursor">
Given a launch speed and a pointer within range on level ground
When a shot is fired
Then it follows a ballistic arc and lands within half a metre of the pointer, and two runs with the same inputs land on the same spot
</scenario>
<scenario name="Hits in its path">
Given an enemy between the hero and the pointer, and a second enemy behind a rock
When a low arc passes through the first enemy's body height
Then the first enemy is hit; the rock blocks a low shot to the second, and a high arc clears the rock
</scenario>
<scenario name="Range and misses">
Given a pointer farther than the weapon's range
When the shot is fired
Then it lands at the range limit, and a miss sticks in the ground briefly and is removed
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
Debug and Release builds with zero warnings; ctest in both presets passes, including the "US-140 ..." cases, the physics tests and the US-029 spear tests; the determinism check still passes.
Manual checks in docs/plans/US-140.md done, with results recorded there: a screenshot of a shot in the air with its shadow, one of a shot blocked by a rock, and the landing distance measured against the pointer.
</verification>
<teach_back>C++ concept for the owner: Fixed-point numbers versus floating point, and why the game uses them for replays.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-141 Bows, crossbows, thrown weapons and staff bolts
```xml
<prompt id="S-US-141" codex="1.9" milestone="M2d" story="US-141" priority="Must" size="M">
<context>
Story US-141: Bows, crossbows, thrown weapons and staff bolts.
As the player, I want several ranged weapons I can aim and shoot, so that I can defeat enemies from a distance.
Epic E13 Content and combat. Traces to: D-25 (owner, 2026-10-01).
</context>
<dependencies>
Stories that must be Done: US-140.
Owner decisions that must be Decided: D-25.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game code. The bow, crossbow and thrown classes use the US-140 arcs (bows and crossbows: light arrows, long range, the fastest arcs; thrown: heavy, slower arcs with spin, short range and high damage); the staff fires its magic bolt flat and fast toward the pointer with its element effects (the US-133 projectile path, now aimed at the pointer). Guns stay on the flat path and are not part of this story. Speed per class and range, damage and rate of fire per weapon come from weapons.json (add the launch speed per class to a `classes` section of that file with validation naming file and field); there is no ammo, only the rate-of-fire cooldown (D-25). Elements (US-135) apply on hit. The starter set's ranged weapons are all shootable from the hotbar. Add `assets/levels/range.json`, a shooting-range level (straw targets, goblins in the open and goblins behind rocks, weapon pickups of the ranged starters at the start) for the owner and for the evidence. The M1b spear-throw demo still works.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Each ranged class shoots">
Given a bow, a crossbow, a thrown weapon and a staff from the starter set
When each is fired at a goblin with the pointer on it
Then each hits with its own speed and path (arcs for the first three, a flat bolt for the staff) and the goblin loses the weapon's damage
</scenario>
<scenario name="Elements on shots">
Given an elemental bow or staff
When its shot hits a goblin
Then the element's effect applies as for melee weapons
</scenario>
<scenario name="Shooting range">
Given the range level
When the owner plays it
Then goblins in the open and behind rocks can be shot with the ranged weapons, and the numbers can be changed in weapons.json without code
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
Debug and Release builds with zero warnings; ctest in both presets passes, including the "US-141 ..." cases (one per ranged class) and all earlier weapon, element and spear tests.
Manual checks in docs/plans/US-141.md done, with results recorded there: a screenshot per ranged class shooting on the range level.
</verification>
<teach_back>C++ concept for the owner: Data-driven tuning: why speeds and ranges live in JSON and not in code.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-136 Plants
```xml
<prompt id="S-US-136" codex="1.8" milestone="M2d" story="US-136" priority="Must" size="L">
<context>
Story US-136: Plants.
As a level designer and player, I want plants in the world that block, can be looked at, chopped and eaten, so that the world feels alive.
Epic E13 Content and combat. Traces to: D-21.
</context>
<dependencies>
Stories that must be Done: US-130, US-132, US-134.
Owner decisions that must be Decided: D-21.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: plants in level format version 2 (kind, position); a plant palette in the Editor (all 150, with place, move, delete, undo and redo like characters); Game code for plants in the world: big plants block walking, spears and arrows; Interact next to a plant shows its name and inspect text for a few seconds; any weapon hit destroys it with a leaf burst; an edible plant heals the hero 10 HP with a healing glow; 15 s later the same kind regrows at a random free spot (not blocked, not on the hero, not on another plant) inside the current camera view, with the growth effect. The random spot comes from a seeded PCG32 stream.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Place and block">
Given the Editor's plant palette
When I place a tree and a flower and play
Then the tree blocks walking and the flower does not
</scenario>
<scenario name="Inspect, chop and eat">
Given the hero next to a plant
When the player presses Interact, or hits the plant with a weapon
Then its name and inspect text show, or it is destroyed with a leaf burst; an edible plant heals the hero 10 HP
</scenario>
<scenario name="Regrow">
Given a destroyed plant
When 15 s pass
Then the same plant grows back at a random free spot inside the camera view
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
Debug and Release builds with zero warnings; ctest in both presets passes, including the "US-136 ..." cases.
Manual checks in docs/plans/US-136.md done, with results recorded there: screenshots of a planted level, an inspect text, a chop and a regrow; docs/guides/editor.md explains plants.
</verification>
<teach_back>C++ concept for the owner: Random numbers from a seeded stream; spatial queries.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-137 Animals in the Editor
```xml
<prompt id="S-US-137" codex="1.8" milestone="M2d" story="US-137" priority="Must" size="S">
<context>
Story US-137: Animals in the Editor.
As a level designer, I want to place animals, so that my levels have wildlife and dangers.
Epic E13 Content and combat. Traces to: D-21.
</context>
<dependencies>
Stories that must be Done: US-130, US-131.
Owner decisions that must be Decided: D-21.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: the 50 animals appear in the Editor's character palette (a tab or a scrolling list, readable at 480x270); placed animals use the properties panel like characters (name, HP, facing: east or west for side views, strike damage for enemies); enemies from the design notes take damage, strike back and die like goblins; bystanders cannot be hit and weapons pass them.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Place">
Given the character palette
When I place any of the 50 animals
Then it stands in the level with its name and HP
</scenario>
<scenario name="Enemies">
Given a predator or a boar
When the hero hits it
Then it takes damage and strikes back like a goblin
</scenario>
<scenario name="Bystanders">
Given a deer, a cow or a rabbit
When the hero attacks next to it
Then it takes no damage
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
Debug and Release builds with zero warnings; ctest in both presets passes, including the "US-137 ..." cases.
Manual checks in docs/plans/US-137.md done, with results recorded there: a screenshot of a level with animals; docs/guides/editor.md lists which animals are enemies.
</verification>
<teach_back>C++ concept for the owner: Data-driven behaviour flags.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-138 Placed effects and random weather
```xml
<prompt id="S-US-138" codex="1.8" milestone="M2d" story="US-138" priority="Should" size="M">
<context>
Story US-138: Placed effects and random weather.
As the owner, I want ambient effects and changing weather, so that the world has atmosphere.
Epic E13 Content and combat. Traces to: D-21, ADR-011.
</context>
<dependencies>
Stories that must be Done: US-132.
Owner decisions that must be Decided: D-21.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: placed effects in level format version 2 and an effect palette in the Editor (the looping effects from effects.json: fireflies, campfire, portal, magic circle and others), with undo and redo; Game code for the weather cycle (a seeded PCG32 stream picks by weight every 60-120 s of play; 3 s cross-fade) drawn as a screen overlay above the world and below the UI; the Editor shows no weather. `--seed` fixes the weather sequence for tests and screenshots.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Placed effects">
Given the Editor
When I place a campfire, fireflies or a portal and save
Then it loops in Game mode after a reload
</scenario>
<scenario name="Weather">
Given Game mode
When 60-120 s pass
Then a random weather fades in over 3 s; clear sky comes about 1 in 3
</scenario>
<scenario name="Repeatable">
Given the same seed
When the game runs twice
Then the same weathers come in the same order
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done.</definition_of_done>
<verification>
Debug and Release builds with zero warnings; ctest in both presets passes, including the "US-138 ..." cases.
Manual checks in docs/plans/US-138.md done, with results recorded there: screenshots of three weathers and of placed effects; docs/guides/editor.md explains effects and weather.
</verification>
<teach_back>C++ concept for the owner: Blending two overlays with alpha.</teach_back>
<stop_conditions>Build loop L-01 stop conditions.</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M2d" codex="1.8" name="Exit review M2d">
<instructions>
1. Demonstrate the exit criteria: The owner places weapon pickups, plants, animals and looping effects with the Editor, saves and plays the level; the hero picks weapons into a 9-slot hotbar and fights with the 16 starter weapons of 8 classes, elements included; enemies strike back when hit and in reach, and die; the hero respawns at 0 HP; plants block, are inspected, chopped and eaten and regrow; effects play; the weather changes by itself; every M2c feature and the spear and sword demos still work.
2. Collect evidence into docs/gates/M2d.md, one section per criterion, each marked met or not met: a scripted end-to-end session with screenshots in docs/evidence/X-M2d/; the full test run; the updated guide.
3. No kill gate: invite the owner to play with docs/guides/editor.md and record the notes when given; they become new stories through Anima, never silent changes.
4. Merge qa into main, push, confirm CI on main is green, tag the repository m2d-done and push the tag, and save a Milestone-<n>.md snapshot. Then stop: K-M3 starts only when the owner says so.
</instructions>
<output_format>Assembly report with the review result.</output_format>
</prompt>
```

### M3 Living clan on screen
Exit criteria: The clan from M2 runs inside the game; NPCs are visible, dressed in layered outfits, and act on their needs.

```xml
<prompt id="K-M3" codex="1.8" name="Kick off M3 Living clan on screen">
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
<prompt id="S-US-030" codex="1.8" milestone="M3" story="US-030" priority="Must" size="M">
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
<prompt id="S-US-032" codex="1.8" milestone="M3" story="US-032" priority="Must" size="L">
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
<prompt id="S-US-031" codex="1.8" milestone="M3" story="US-031" priority="Should" size="S">
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
<prompt id="X-M3" codex="1.8" name="Exit review M3">
<instructions>
1. Demonstrate the exit criteria: The clan from M2 runs inside the game; NPCs are visible, dressed in layered outfits, and act on their needs.
2. Collect evidence (test output, headless run logs, FPS logs, screenshots) into docs/gates/M3.md, one section per criterion, each marked met or not met.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m3-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the gate result.</output_format>
</prompt>
```

### M4 Region, tools and saves
Exit criteria: A region is generated from a seed with biomes, resources and two rival clans; any NPC can be inspected; the game saves and loads.

```xml
<prompt id="K-M4" codex="1.8" name="Kick off M4 Region, tools and saves">
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
<prompt id="S-US-040" codex="1.8" milestone="M4" story="US-040" priority="Must" size="L">
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
<prompt id="S-US-041" codex="1.8" milestone="M4" story="US-041" priority="Must" size="M">
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
<prompt id="S-US-042" codex="1.8" milestone="M4" story="US-042" priority="Must" size="M">
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
<prompt id="S-US-043" codex="1.8" milestone="M4" story="US-043" priority="Should" size="M">
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
<prompt id="S-US-080" codex="1.8" milestone="M4" story="US-080" priority="Must" size="M">
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
<prompt id="S-US-083" codex="1.8" milestone="M4" story="US-083" priority="Should" size="M">
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
<prompt id="X-M4" codex="1.8" name="Exit review M4">
<instructions>
1. Demonstrate the exit criteria: A region is generated from a seed with biomes, resources and two rival clans; any NPC can be inspected; the game saves and loads.
2. Collect evidence (test output, headless run logs, FPS logs, screenshots) into docs/gates/M4.md, one section per criterion, each marked met or not met.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m4-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the gate result.</output_format>
</prompt>
```

### M5 Vertical slice feature-complete
Exit criteria: A player can start a new game, live the Growing Period, take a profession, pursue Trade or Religion, and win or lose.

```xml
<prompt id="K-M5" codex="1.8" name="Kick off M5 Vertical slice feature-complete">
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
<prompt id="S-US-050" codex="1.8" milestone="M5" story="US-050" priority="Must" size="M">
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
<prompt id="S-US-053" codex="1.8" milestone="M5" story="US-053" priority="Must" size="S">
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
<prompt id="S-US-051" codex="1.8" milestone="M5" story="US-051" priority="Must" size="M">
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
<prompt id="S-US-052" codex="1.8" milestone="M5" story="US-052" priority="Must" size="L">
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
<prompt id="S-US-054" codex="1.8" milestone="M5" story="US-054" priority="Must" size="S">
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
<prompt id="S-US-060" codex="1.8" milestone="M5" story="US-060" priority="Must" size="S">
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
<prompt id="S-US-061" codex="1.8" milestone="M5" story="US-061" priority="Must" size="L">
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
<prompt id="S-US-062" codex="1.8" milestone="M5" story="US-062" priority="Must" size="M">
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
<prompt id="S-US-063" codex="1.8" milestone="M5" story="US-063" priority="Should" size="M">
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
<prompt id="S-US-070" codex="1.8" milestone="M5" story="US-070" priority="Must" size="M">
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
<prompt id="S-US-055" codex="1.8" milestone="M5" story="US-055" priority="Must" size="M">
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
<prompt id="S-US-071" codex="1.8" milestone="M5" story="US-071" priority="Must" size="L">
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
<prompt id="S-US-072" codex="1.8" milestone="M5" story="US-072" priority="Must" size="L">
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
<prompt id="S-US-073" codex="1.8" milestone="M5" story="US-073" priority="Must" size="S">
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
<prompt id="S-US-081" codex="1.8" milestone="M5" story="US-081" priority="Should" size="S">
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
<prompt id="S-US-082" codex="1.8" milestone="M5" story="US-082" priority="Must" size="S">
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
<prompt id="X-M5" codex="1.8" name="Exit review M5">
<instructions>
1. Demonstrate the exit criteria: A player can start a new game, live the Growing Period, take a profession, pursue Trade or Religion, and win or lose.
2. Collect evidence (test output, headless run logs, FPS logs, screenshots) into docs/gates/M5.md, one section per criterion, each marked met or not met.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m5-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the gate result.</output_format>
</prompt>
```

### M7 World interactions
Exit criteria: Every action in today's game comes from data files; the player, NPCs and animals act on things with timed, visible, saved results; files edited offline reload with F5 and errors name file and line.

Why this milestone exists: the owner wants game entities to interact with each other and with the world, configured offline in readable files and in the Editor (D-34). Today every world action is C++ in RunFlow::openContext and the clan's actions are only rates; M7 makes one data-driven system that the player, NPCs and animals share. Order: the language and loader first (US-150, US-151), hot reload early so the owner can iterate (US-156), then the refactor (US-152), time and state (US-153), objects (US-155), and NPCs last (US-154).

Design notes for every M7, M8 and M9 prompt (Anima, from the brief docs/plans/M7-M9-interactions-brief.md):
- Decisions (D-22, D-35): the owner takes every design decision. K-M7, K-M8 and K-M9 each ask the owner, in one chat round, every design question their stories leave open; a question that appears mid-story stops that story until the owner answers. Technical choices stay with Dominus and go into ADRs or the milestone design document.
- Formats (D-34, ADR-019): interactions are JSON in assets/data/interactions/, one file per interaction, `//` comments allowed (parse with comments ignored); dialogue is `.dlg` plain text in assets/data/dialogue/; the graph layout lives in `<name>.dlg.layout.json`. The brief's section 4 is the contract: field names, the condition functions (`has`, `need`, `skill`, `trait`, `opinion`, `kin`, `flag`, `tag`, `time`, `season`, `distance`) and the effect verbs (`give`, `take`, `set`, `flag`, `opinion`, `remember`, `start`, `talk`, `say`, `fx`, `sound`, `after`, `chronicle`). Adding a function or verb is allowed when a story needs it: document it in the guide in the same commit.
- One language: interactions and dialogue share one condition and effect language, parsed once into expression trees (std::variant nodes), evaluated against a read-only view of the world. Errors always name file, line and field (Charter rule 7); bad data never crashes the game: the last good data stays loaded.
- Layers (Charter rules 1, 3, 9): the language, the registry, the action runner, dialogue parsing, selection and validation live in src/sim/ (no Engine, Platform or SDL); panels and Editor tabs live in src/game/; the node-graph widget lives in Luna Engine and knows nothing about dialogue.
- Determinism (Charter rule 6): NPC choices and small-talk picks use seeded PCG32 streams; things are referred to by id, never by pointer; the determinism hash covers interaction state, flags and conversation memories.
- Saves (Charter rule 8): thing states, running actions, flags and conversation memories are saved; every save or level format change bumps its version with a migration from the previous one.
- Comments: Editor saves keep `note` fields and `.dlg` `#` notes; JSON `//` comments are lost on an Editor save, and the guide says so.
- Tests: every shipped JSON and .dlg file is validated in a CI test; round-trip tests (load, save, load) for every format; the M5 and M6 behaviour is the regression baseline for US-152.

```xml
<prompt id="K-M7" codex="2.0" name="Kick off M7 World interactions">
<instructions>
1. Confirm that M5 (X-M5) and P-009 (docs/gates/test-debt.md) are done, and that D-34 and D-35 are Decided in docs/decisions.md.
2. Ask the owner, in one chat round (AskUserQuestion, 2-4 options each, recommended first), every design question the M7 stories leave open after the brief and D-34/D-35 (for example: the progress-ring look, which actions an NPC may interrupt, whether animals flee from the hero). Record the answers in docs/decisions.md and in the milestone design document.
3. Architect: write docs/plans/M7-interactions-design.md before US-150, following the design notes: the grammar of the language (tokens and precedence), the registry and matching, the action runner and timers, state storage and save migrations, the NPC scoring loop, the hot-reload swap, and the test plan.
4. Set this milestone's stories to To do in docs/status.md in this order: US-150, US-151, US-156, US-152, US-153, US-155, US-154.
5. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, the owner's answers.</output_format>
</prompt>
```

#### S-US-150 Interaction data and the rule language
```xml
<prompt id="S-US-150" codex="2.0" milestone="M7" story="US-150" priority="Must" size="L">
<context>
Story US-150: Interaction data and the rule language.
As the owner, I want every action and interaction described in JSON files I can edit offline, with conditions and effects in plain words, so that I can change how the world works without code.
Epic E14 World interactions: Every thing in the world offers actions from data; the player, NPCs and animals act on things with timed, visible, saved results.
Traces to: INT-01, INT-03, ADR-019.
</context>
<dependencies>
Stories that must be Done: US-060.
Owner decisions that must be Decided: D-34.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation layer: src/sim/rules/ (lexer, parser and evaluator of the condition and effect language; the interaction registry and its JSON loader with nlohmann/json, comments allowed). Data: assets/data/interactions/. Guide: docs/guides/interaction-data.md (mraw-writer), one example per condition function and effect verb. Ship one example interaction (gather.json) so the tests have real data.
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Load">
Given gather.json with a range, a duration, two conditions and three effects
When the game starts
Then the interaction is registered and offered on edible plants
</scenario>
<scenario name="Error">
Given a file with an unknown effect verb on line 12
When the game starts
Then the error reads 'interactions/gather.json:12: unknown effect verb "giv"' and the rest of the data loads
</scenario>
<scenario name="Guide">
Given every condition function and effect verb
When the owner opens docs/guides/interaction-data.md
Then each has a one-line meaning and an example
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-150`: Debug and Release builds with zero warnings; every test passes in both, including the "US-150 ..." cases and the determinism hash test.
Green CI on qa after the merge.
Manual checks in docs/plans/US-150.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-150/.
</verification>
<teach_back>C++ concept for the owner: A small expression parser (tokens, recursive descent).</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-151 Tags and smart objects
```xml
<prompt id="S-US-151" codex="2.0" milestone="M7" story="US-151" priority="Must" size="M">
<context>
Story US-151: Tags and smart objects.
As the owner, I want things to carry tags and states and to advertise the interactions that fit them, so that a new plant or object gets its actions just by its tags.
Epic E14 World interactions: Every thing in the world offers actions from data; the player, NPCs and animals act on things with timed, visible, saved results.
Traces to: INT-01, INT-02, GD-04.
</context>
<dependencies>
Stories that must be Done: US-150.
Owner decisions that must be Decided: D-34.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Catalogs in assets/data/ gain `tags` and `states` (with a default) for plants, animals, characters and items; the loaders validate them. Matching (actor kind, target tags, state) lives in src/sim/rules/; the Game asks it which interactions a thing offers.
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Advertise">
Given a new plant kind tagged edible and plant
When the hero right-clicks it
Then Gather and Inspect are offered without any code change
</scenario>
<scenario name="State">
Given a bush in state picked
When the hero right-clicks it
Then Gather is shown disabled with 'Nothing to pick yet'
</scenario>
<scenario name="Unknown tag">
Given an interaction targeting a tag no catalog uses
When the data loads
Then a warning names the file and the tag
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-151`: Debug and Release builds with zero warnings; every test passes in both, including the "US-151 ..." cases and the determinism hash test.
Green CI on qa after the merge.
Manual checks in docs/plans/US-151.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-151/.
</verification>
<teach_back>C++ concept for the owner: Sets of tags and matching with std::ranges.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-156 Hot reload and the validation panel
```xml
<prompt id="S-US-156" codex="2.0" milestone="M7" story="US-156" priority="Must" size="M">
<context>
Story US-156: Hot reload and the validation panel.
As the owner, I want to press F5 after editing a file offline and see my change, or a clear list of mistakes, so that I can iterate fast without restarting.
Epic E14 World interactions: Every thing in the world offers actions from data; the player, NPCs and animals act on things with timed, visible, saved results.
Traces to: INT-03.
</context>
<dependencies>
Stories that must be Done: US-150.
Owner decisions that must be Decided: D-34.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: F5 (new intent Reload) reloads assets/data/ catalogs, interactions and dialogue: load into new structures, validate, swap only when valid. Errors collect into a list shown by a Game panel (Luna UI) with file:line: message; the last good data stays live. Works in Game and Editor modes.
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Reload">
Given the game running and gather.json edited in Notepad
When the owner presses F5
Then the change is live within a second
</scenario>
<scenario name="Bad file">
Given a file with a syntax error
When the owner presses F5
Then a panel lists 'file:line: message' and the last good data stays in use
</scenario>
<scenario name="Fix">
Given the panel showing an error
When the owner fixes the file and presses F5
Then the panel closes and the new data is used
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-156`: Debug and Release builds with zero warnings; every test passes in both, including the "US-156 ..." cases and the determinism hash test.
Green CI on qa after the merge.
Manual checks in docs/plans/US-156.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-156/.
</verification>
<teach_back>C++ concept for the owner: Watching files and swapping data safely.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-152 The context menu from data
```xml
<prompt id="S-US-152" codex="2.0" milestone="M7" story="US-152" priority="Must" size="L">
<context>
Story US-152: The context menu from data.
As the player, I want right-click actions to come from the data, so that the owner's changes appear in the game; nothing I could do before is lost.
Epic E14 World interactions: Every thing in the world offers actions from data; the player, NPCs and animals act on things with timed, visible, saved results.
Traces to: INT-01, INT-03.
</context>
<dependencies>
Stories that must be Done: US-151, US-061.
Owner decisions that must be Decided: D-34.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Move every action built in RunFlow::openContext (src/game/run_flow.cpp: Talk, Give berries, Ask to teach, Craft, Craft at the fire, Eat berries, Tend the fire, Tend the sacred fire) and the plant Inspect and chop actions into assets/data/interactions/*.json. Effects that need Game code (open the craft screen, start a dialogue) are named effect verbs the Game registers. The context menu is built from the registry; a failed condition's `else` text is the disabled reason. Run every M5 and M6 test before and after; behaviour must not change.
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Same actions">
Given the M5 camp level
When the hero right-clicks a person, the fire and the knapping stone
Then Talk, Give berries, Ask to teach, Craft, Eat berries and Tend the fire are offered as before
</scenario>
<scenario name="Data change">
Given the owner renames 'Tend the fire' in tend-fire.json
When the game reloads the data
Then the menu shows the new name
</scenario>
<scenario name="Regression">
Given the M5 and M6 tests
When they run
Then they pass unchanged
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-152`: Debug and Release builds with zero warnings; every test passes in both, including the "US-152 ..." cases and the determinism hash test.
Green CI on qa after the merge.
Manual checks in docs/plans/US-152.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-152/.
</verification>
<teach_back>C++ concept for the owner: Replacing hard-coded behaviour behind a stable interface (refactoring with tests).</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-153 Timed actions and world state
```xml
<prompt id="S-US-153" codex="2.0" milestone="M7" story="US-153" priority="Must" size="M">
<context>
Story US-153: Timed actions and world state.
As the player, I want actions to take time, show progress and change the thing I act on, so that the world reacts and remembers.
Epic E14 World interactions: Every thing in the world offers actions from data; the player, NPCs and animals act on things with timed, visible, saved results.
Traces to: INT-04.
</context>
<dependencies>
Stories that must be Done: US-152.
Owner decisions that must be Decided: D-34.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: An action runner in src/sim/rules/: the running action per actor (interaction id, target id, elapsed ticks), interruption by Move or Attack intents, effects applied at the end, `after` effects queued on the world timer; target state stored per thing id and written into saves (region and level save versions bumped with a migration). Progress ring and effects drawn by the Game.
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Progress">
Given Gather with a duration of 3 s
When the hero starts it
Then a progress ring fills over 3 s and the berries arrive at the end
</scenario>
<scenario name="Interrupt">
Given a timed action under way
When the player moves or attacks
Then the action stops and gives nothing
</scenario>
<scenario name="Saved">
Given a bush picked and a fire lit
When the game saves and loads
Then the bush is still picked and the fire still burns
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-153`: Debug and Release builds with zero warnings; every test passes in both, including the "US-153 ..." cases and the determinism hash test.
Green CI on qa after the merge.
Manual checks in docs/plans/US-153.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-153/.
</verification>
<teach_back>C++ concept for the owner: Timers in the fixed timestep; saving component state.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-155 Age 1 world objects
```xml
<prompt id="S-US-155" codex="2.0" milestone="M7" story="US-155" priority="Should" size="M">
<context>
Story US-155: Age 1 world objects.
As a level designer, I want fire pits, knapping stones, stores, shelters, flint nodules, water and sleeping furs as placeable objects with their own actions, so that a camp is something to interact with.
Epic E14 World interactions: Every thing in the world offers actions from data; the player, NPCs and animals act on things with timed, visible, saved results.
Traces to: INT-02, INT-04.
</context>
<dependencies>
Stories that must be Done: US-151.
Owner decisions that must be Decided: D-34, D-35.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: assets/data/objects.json with the seven Age 1 objects (D-35): fire pit, knapping stone, food store, shelter, flint nodule, water source, sleeping furs, each with tags, states, atlas frame (programmer art allowed) and its interactions in assets/data/interactions/. Objects are a new Editor palette page and are saved in levels (level format version bump with migration).
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Place">
Given the Editor's object palette
When the owner places a fire pit
Then it appears in the level and is saved
</scenario>
<scenario name="Use">
Given a fire pit with no fire
When the hero chooses Light fire with a fire drill
Then the fire burns and warms people nearby
</scenario>
<scenario name="Data">
Given a new object kind added to objects.json
When the game reloads
Then it appears in the palette with its tags
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-155`: Debug and Release builds with zero warnings; every test passes in both, including the "US-155 ..." cases and the determinism hash test.
Green CI on qa after the merge.
Manual checks in docs/plans/US-155.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-155/.
</verification>
<teach_back>C++ concept for the owner: Data-driven object kinds.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-154 NPCs and animals use interactions
```xml
<prompt id="S-US-154" codex="2.0" milestone="M7" story="US-154" priority="Must" size="L">
<context>
Story US-154: NPCs and animals use interactions.
As the player, I want clan members and animals to do the same things I can do, chosen by their needs and traits, so that I see them live in the world.
Epic E14 World interactions: Every thing in the world offers actions from data; the player, NPCs and animals act on things with timed, visible, saved results.
Traces to: INT-05, SDC-02.
</context>
<dependencies>
Stories that must be Done: US-153, US-030.
Owner decisions that must be Decided: D-34.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: NPC choice in the Simulation: each idle clan member and animal scores the interactions it can do with the `npc.score` expression and `cooldown`, picks the best with the clan's seeded PCG32 stream for ties, walks to the target and runs it with the same action runner. The M3 view places actions on real targets instead of fixed spots. Extend the determinism hash test to cover interaction state. assets/data/sim/actions.json keeps only tuning numbers that are not interactions.
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Hungry">
Given a hungry clan member near a ripe bush
When time passes
Then they walk to the bush and gather, and the bush shows as picked
</scenario>
<scenario name="Animals">
Given a deer near grass and a wolf nearby
When time passes
Then the deer grazes and flees from the wolf
</scenario>
<scenario name="Deterministic">
Given the same seed and inputs
When two runs simulate 10,000 ticks
Then the world hashes match
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-154`: Debug and Release builds with zero warnings; every test passes in both, including the "US-154 ..." cases and the determinism hash test.
Green CI on qa after the merge.
Manual checks in docs/plans/US-154.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-154/.
</verification>
<teach_back>C++ concept for the owner: Utility scoring; sharing one system between player and AI.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M7" codex="2.0" name="Exit review M7">
<instructions>
1. Demonstrate the exit criteria: Every action in today's game comes from data files; the player, NPCs and animals act on things with timed, visible, saved results; files edited offline reload with F5 and errors name file and line.
2. Collect evidence (test output, CI run, screenshots, data files) into docs/gates/M7.md, one section per criterion, each marked met or not met. Also: the hand-written actions are gone from RunFlow::openContext; a clip or screenshot sequence shows a clan member gathering and an animal fleeing; the owner's offline edit loop (edit a file, F5, see the change) is shown in the evidence.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m7-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the exit result.</output_format>
</prompt>
```

### M8 Speak to NPCs
Exit criteria: The player talks to any clan member; written conversations branch on the simulation; without a script NPCs make small talk from their memories; talk changes opinions, items and memories; NPCs talk to each other in bubbles.

Why this milestone exists: the owner wants to speak to NPCs (SDC-03). Hybrid talk (D-34): written `.dlg` trees that read the simulation, and generated small talk from memories, gossip and needs when no written line fits. The design notes under M7 apply to every M8 prompt; the dialogue panel pauses the game (D-35).

```xml
<prompt id="K-M8" codex="2.0" name="Kick off M8 Speak to NPCs">
<instructions>
1. Confirm that M7 (docs/gates/M7.md) and its stories are done, and that D-34 and D-35 are Decided in docs/decisions.md.
2. Ask the owner, in one chat round, the open design questions of the M8 stories (for example: the mood words, how many choices fit the panel, which small-talk topics come first). Record the answers.
3. Architect: write docs/plans/M8-dialogue-design.md before US-160: the .dlg grammar, the runtime state machine, selection rules, the small-talk generator, the memory and chronicle links, bubbles, and the test plan.
4. Set this milestone's stories to To do in docs/status.md in this order: US-160, US-161, US-162, US-163, US-164, US-165.
5. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, the owner's answers.</output_format>
</prompt>
```

#### S-US-160 The dialogue script format
```xml
<prompt id="S-US-160" codex="2.0" milestone="M8" story="US-160" priority="Must" size="L">
<context>
Story US-160: The dialogue script format.
As the owner, I want to write conversations as plain text with speakers, choices, conditions and effects, so that I can write dialogue in any text editor.
Epic E15 Speak to NPCs: The player talks to any clan member; written and generated conversations read the simulation and change it; NPCs talk to each other.
Traces to: SDC-03, SDC-04, ADR-019.
</context>
<dependencies>
Stories that must be Done: US-150.
Owner decisions that must be Decided: D-34.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: src/sim/dialogue/: line-based parser for .dlg (headers @who, @when, @priority; nodes `=== id`; speaker lines with optional [if ...]; choices `-> text [if ...] {effects} => node`; END; `#` notes kept and attached to the next element), using the US-150 condition and effect language; a canonical writer. Format guide docs/guides/dialogue-format.md (mraw-writer) with the brief's example. Ship assets/data/dialogue/elder-fire.dlg.
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Parse">
Given elder-fire.dlg with three nodes, a conditional line and three choices
When the game loads it
Then the conversation has three nodes and the choices link to them
</scenario>
<scenario name="Error">
Given a choice that points to a missing node on line 9
When the game loads it
Then the error reads 'dialogue/elder-fire.dlg:9: unknown node "hunts"'
</scenario>
<scenario name="Round trip">
Given every shipped .dlg file
When it is loaded and written back
Then the text is the same, # notes included
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-160`: Debug and Release builds with zero warnings; every test passes in both, including the "US-160 ..." cases and the determinism hash test.
Green CI on qa after the merge.
Manual checks in docs/plans/US-160.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-160/.
</verification>
<teach_back>C++ concept for the owner: Writing a line-based parser and a printer that round-trip.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-161 Conversations and the dialogue panel
```xml
<prompt id="S-US-161" codex="2.0" milestone="M8" story="US-161" priority="Must" size="L">
<context>
Story US-161: Conversations and the dialogue panel.
As the player, I want to talk to an NPC in a panel with their name, mood, words and my choices, so that speaking to people is part of play.
Epic E15 Speak to NPCs: The player talks to any clan member; written and generated conversations read the simulation and change it; NPCs talk to each other.
Traces to: SDC-03.
</context>
<dependencies>
Stories that must be Done: US-160.
Owner decisions that must be Decided: D-34, D-35.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Conversation runtime in src/sim/dialogue/ (current node, visible lines and choices, effects applied through the effect runner); Talk opens it. The dialogue panel in src/game/ (Luna UI): speaker name, mood word from opinion and needs, text, numbered choices (mouse or keys 1-9), Esc leaves. The game pauses while the panel is open (D-35).
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Talk">
Given the hero next to the elder
When the player chooses Talk
Then the game pauses and the panel shows the elder's name, mood, first line and numbered choices
</scenario>
<scenario name="Choose">
Given the choice 'Offer berries' with its effects
When the player picks it with the mouse or the key 2
Then a berry leaves the bag, the elder's opinion rises by 5 and the next node is shown
</scenario>
<scenario name="Hidden">
Given a choice whose condition is false
When the node is shown
Then the choice is hidden, or shown disabled when the script marks it so
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-161`: Debug and Release builds with zero warnings; every test passes in both, including the "US-161 ..." cases and the determinism hash test.
Green CI on qa after the merge.
Manual checks in docs/plans/US-161.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-161/.
</verification>
<teach_back>C++ concept for the owner: A small state machine over data; immediate-mode UI.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-162 Who says what
```xml
<prompt id="S-US-162" codex="2.0" milestone="M8" story="US-162" priority="Must" size="M">
<context>
Story US-162: Who says what.
As the owner, I want conversations chosen by character, role, kind, opinion and priority, with short greetings, so that each NPC sounds like themselves.
Epic E15 Speak to NPCs: The player talks to any clan member; written and generated conversations read the simulation and change it; NPCs talk to each other.
Traces to: SDC-03.
</context>
<dependencies>
Stories that must be Done: US-161.
Owner decisions that must be Decided: D-34.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Selection in src/sim/dialogue/: candidates whose @who matches (placed id > role > kind) and whose @when holds, highest @priority wins, ties by the seeded stream. Greetings and barks are short scripts with @bark; a per-NPC cooldown of 60 s.
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Specific first">
Given a script for the placed character 'Ama' and one for all elders
When the hero talks to Ama
Then Ama's own script is used
</scenario>
<scenario name="Opinion">
Given a script with @when opinion(npc, hero) < -30
When an NPC who dislikes the hero is spoken to
Then that script is used
</scenario>
<scenario name="Greeting">
Given the hero passing within 3 m of a friendly NPC
When it happens
Then a short greeting bubble appears, at most once a minute per NPC
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-162`: Debug and Release builds with zero warnings; every test passes in both, including the "US-162 ..." cases and the determinism hash test.
Green CI on qa after the merge.
Manual checks in docs/plans/US-162.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-162/.
</verification>
<teach_back>C++ concept for the owner: Ranking candidates with std::ranges::max_element.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-163 Generated small talk
```xml
<prompt id="S-US-163" codex="2.0" milestone="M8" story="US-163" priority="Must" size="M">
<context>
Story US-163: Generated small talk.
As the player, I want NPCs without a written line to talk about what they remember, what they heard and how they feel, so that every clan member has something to say.
Epic E15 Speak to NPCs: The player talks to any clan member; written and generated conversations read the simulation and change it; NPCs talk to each other.
Traces to: SDC-03, STO-02.
</context>
<dependencies>
Stories that must be Done: US-161, US-012.
Owner decisions that must be Decided: D-34.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: assets/data/dialogue/smalltalk.json: topics (memories, people/gossip, needs, season, the hero) with at least three templates each and tokens filled from the NPC's memories, gossip, needs and opinion (US-012 memory system). Used when no script fits and through `{smalltalk.<topic>}` tokens. mraw-designer writes the templates; the owner reads 50 samples at X-M8.
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Memory">
Given an NPC who saw a wolf at the fire yesterday
When the hero talks to them with no script
Then they mention the wolf in their own words
</scenario>
<scenario name="Gossip">
Given an NPC who heard that Bo blamed Ama
When the topic 'people' comes up
Then they repeat the gossip and say how they feel about it
</scenario>
<scenario name="Variety">
Given 50 small-talk lines from one seed
When the owner reads them
Then no line repeats more than twice
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-163`: Debug and Release builds with zero warnings; every test passes in both, including the "US-163 ..." cases and the determinism hash test.
Green CI on qa after the merge.
Manual checks in docs/plans/US-163.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-163/.
</verification>
<teach_back>C++ concept for the owner: Text templates with tokens.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-164 Conversations are remembered
```xml
<prompt id="S-US-164" codex="2.0" milestone="M8" story="US-164" priority="Must" size="M">
<context>
Story US-164: Conversations are remembered.
As the player, I want what I say to matter later, so that people remember kindness and insults and talk about them.
Epic E15 Speak to NPCs: The player talks to any clan member; written and generated conversations read the simulation and change it; NPCs talk to each other.
Traces to: SDC-03, SDC-02, STO-03.
</context>
<dependencies>
Stories that must be Done: US-161.
Owner decisions that must be Decided: D-34.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Dialogue effects `remember`, `opinion` and `flag` write into the Simulation's memory, opinion and flag stores so gossip (half strength) and the chronicle pick them up; notable conversations (marked `chronicle` in the script) get a chronicle line with its reason (STO-03). Flags and conversation memories are saved.
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Memory">
Given the hero insults Bo in a conversation
When it ends
Then Bo has a memory with a bad feeling and his opinion of the hero falls
</scenario>
<scenario name="Gossip">
Given that memory
When two days pass
Then Bo's friends have heard it at half strength
</scenario>
<scenario name="Saved">
Given flags set in conversations
When the game saves and loads
Then the flags and the conversation memories are still there
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-164`: Debug and Release builds with zero warnings; every test passes in both, including the "US-164 ..." cases and the determinism hash test.
Green CI on qa after the merge.
Manual checks in docs/plans/US-164.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-164/.
</verification>
<teach_back>C++ concept for the owner: Linking dialogue effects to the memory system.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-165 NPCs talk to each other
```xml
<prompt id="S-US-165" codex="2.0" milestone="M8" story="US-165" priority="Should" size="M">
<context>
Story US-165: NPCs talk to each other.
As the player, I want to see clan members talk in speech bubbles, so that quarrels, courtship and sharing happen in front of me.
Epic E15 Speak to NPCs: The player talks to any clan member; written and generated conversations read the simulation and change it; NPCs talk to each other.
Traces to: SDC-05, SDC-02.
</context>
<dependencies>
Stories that must be Done: US-162, US-154.
Owner decisions that must be Decided: D-34.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: When the social simulation (M2b: quarrels, courtship, sharing) fires between two people near the camera, play an exchange of bubbles from scripts with @who pairs or @bark topics; bubbles drawn by the Game, 3 s each, one at a time per speaker. Outcomes still come from the simulation; the bubbles show them.
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Bubbles">
Given two friends at the fire in the evening
When they talk
Then bubbles with short lines appear over their heads in turn
</scenario>
<scenario name="Quarrel">
Given a quarrel started by the social simulation
When it happens near the hero
Then the quarrel is shown as angry lines and both opinions fall
</scenario>
<scenario name="Readable">
Given a bubble on screen
When 3 s pass or the next line starts
Then it disappears
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-165`: Debug and Release builds with zero warnings; every test passes in both, including the "US-165 ..." cases and the determinism hash test.
Green CI on qa after the merge.
Manual checks in docs/plans/US-165.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-165/.
</verification>
<teach_back>C++ concept for the owner: Scheduling short timed events.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M8" codex="2.0" name="Exit review M8">
<instructions>
1. Demonstrate the exit criteria: The player talks to any clan member; written conversations branch on the simulation; without a script NPCs make small talk from their memories; talk changes opinions, items and memories; NPCs talk to each other in bubbles.
2. Collect evidence (test output, CI run, screenshots, data files) into docs/gates/M8.md, one section per criterion, each marked met or not met. Also: 50 small-talk samples from one seed are saved to docs/gates/M8-smalltalk.md for the owner to read, with the count of repeated lines.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m8-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the exit result.</output_format>
</prompt>
```

### M8b Resolution and GPU renderer
Exit criteria: The game renders at 960x540 through Luna's SDL_GPU renderer (with the old renderer as fallback) and looks the same at 2x camera zoom; windowed sizes, borderless and exclusive full screen work; every screen and editor panel is laid out for the new size; 60 FPS at 1080p on the target mid-range PC (D-06).

Why this milestone exists: the owner wants a higher resolution and window, global lighting with shadows, and buildings (D-42), right after M8 so that every editor from M9 on is built at the final size on the final renderer. M8b changes the renderer and the resolution without changing what the game shows.

Design notes for every M8b-M8e prompt (Anima, from the brief docs/plans/M8b-M8d-render-light-build-brief.md):
- Decisions (D-35, D-43): the owner answers each kickoff's design questions in one chat round; a design question that appears mid-story stops that story until the owner answers. Technical choices (the shader compiler, data layouts, algorithms) are Dominus's, recorded in ADRs or the milestone design document.
- Renderer (Charter rule 11, ADR-021): SDL_GPU only in Luna Platform and Engine; the Renderer interface grows the calls lighting needs, with no graphics-API type in it; the SDL_Renderer path stays as fallback and for tests; game code does not change because of the switch.
- Tests: CI runners have no GPU. Headless tests use the RecordingRenderer as today; GPU pictures are checked with screenshots on the owner's PC, saved in docs/evidence/US-xxx/ and listed in the story's manual checks.
- Determinism (Charter rule 6): lighting, shadows and flicker are presentation only; buildings, construction, wear and fire are Simulation, seeded, saved and in the world hash.
- Data (Charter rules 7, 8): the brief's section 4 is the contract (settings, sky.json, lights.json, light fields, height, pieces.json, kinds.json, prefabs); every format change bumps its version with a migration; guides docs/guides/lighting.md and buildings.md.
- Performance (D-06): 60 FPS at 1080p on High lighting on a mid-range PC (6-core CPU, 16 GB RAM, RX 6600 / RTX 3060 class GPU, DirectX 12); Low lighting for weaker PCs.

```xml
<prompt id="K-M8b" codex="2.2" name="Kick off M8b Resolution and GPU renderer">
<instructions>
1. Confirm that M8 is done (docs/gates/M8.md, tag m8-done), and that D-42, D-43 and D-06 are Decided in docs/decisions.md.
2. Ask the owner, in one chat round (AskUserQuestion, 2-4 options each, recommended first), every design question the M8b stories leave open after the brief and D-42/D-43 (for example: the window sizes offered, whether Fill is the default on odd screens, where the UI scale and camera zoom live in the settings screen). Record the answers in docs/decisions.md as "Decided (owner, <date>)".
3. Architect: write docs/plans/M8b-renderer-design.md (GPU device and swapchain ownership, the Renderer interface changes, batching, shader build, fallback, scaling and window modes, zoom and UI scale, layout rules, performance method) before the first story.
4. Set this milestone's stories to To do in docs/status.md in this order: US-230, US-231, US-232, US-233, US-234.
5. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, the owner's answers.</output_format>
</prompt>
```

#### S-US-230 Luna's SDL_GPU renderer
```xml
<prompt id="S-US-230" codex="2.2" milestone="M8b" story="US-230" priority="Must" size="L">
<context>
Story US-230: Luna's SDL_GPU renderer.
As the developer, I want Luna to draw through SDL_GPU with shaders, keeping the old renderer as a fallback, so that lighting and shadows become possible without changing game code.
Epic E22 Resolution and GPU renderer: The game renders at 960x540 on Luna's new SDL_GPU renderer, in any window size or full screen, and every screen and editor has room to breathe.
Traces to: ARC-11, ADR-021, ADR-003.
</context>
<dependencies>
Stories that must be Done: US-020.
Owner decisions that must be Decided: D-42.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Luna: a GpuRenderer in src/luna/engine/ (with the SDL_GPU device, swapchain and command buffers in src/luna/platform/) implementing the existing Renderer interface: textures, sprite batching, Normal and Add blending, the 480x270 picture first. Shaders in src/luna/engine/shaders/ (HLSL), compiled at build time; choose the compiler (for example SDL_shadercross or DXC from vcpkg) and record it with its trade-offs in docs/adr/ADR-021-sdl-gpu-renderer.md. The window falls back to the SDL_Renderer path when SDL_GPU fails and logs why; a --renderer gpu|sdl switch for tests. Pixel comparison of both renderers on the demo and camp levels (owner's PC; CI runs the RecordingRenderer tests).
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Same picture">
Given the demo level and the camp
When they are drawn with the GPU and the old renderer
Then the screenshots match pixel for pixel at 1x lighting
</scenario>
<scenario name="Fallback">
Given a PC where SDL_GPU cannot start
When the game starts
Then it uses the old renderer, says so in the log and plays
</scenario>
<scenario name="Game code">
Given every draw call in src/game/
When the switch is made
Then no game file changed its drawing code
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-230/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-230`: the Debug build with zero warnings and every Debug test passes, including the "US-230 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-230.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-230/.
</verification>
<teach_back>C++ concept for the owner: Graphics pipelines, shaders and sprite batching.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-231 960x540 and window modes
```xml
<prompt id="S-US-231" codex="2.4" milestone="M8b" story="US-231" priority="Must" size="M">
<context>
Story US-231: 960x540 and window modes.
As the player, I want the game at 960x540 that fills my screen crisply in a window or full screen, so that it looks sharp on any monitor.
Epic E22 Resolution and GPU renderer: The game renders at 960x540 on Luna's new SDL_GPU renderer, in any window size or full screen, and every screen and editor has room to breathe.
Traces to: ENV-18, US-081.
</context>
<dependencies>
Stories that must be Done: US-230, US-081.
Owner decisions that must be Decided: D-42, D-44.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Luna Platform and Engine: virtual size 960x540 in ApplicationSettings; Whole scaling (largest whole multiple that fits, centred with black bars) by default, or Fill (scale to fit); window modes (windowed sizes 1280x720, 1600x900, 1920x1080, 2560x1440; borderless; exclusive full screen) applied at once and saved in settings.json (US-081 settings screen extended); high-DPI aware. On first start without settings.json, use windowed 1280x720, camera zoom 2x, UI scale 1x and lighting Medium. Follow D-44 and docs/plans/M8b-renderer-design.md for these settings and migration from version 1 settings.
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Scale">
Given a 1920x1080 and a 3840x2160 screen
When the game runs full screen
Then it scales by 2 and by 4 with no blur and no border
</scenario>
<scenario name="Modes">
Given the settings screen
When the player picks 1280x720, 1600x900, 1920x1080 or 2560x1440 windowed, borderless or exclusive full screen
Then the mode and size change at once and are saved
</scenario>
<scenario name="Odd sizes">
Given a 2560x1440 screen
When the game runs
Then Whole centres the 960x540 image at 2x with black bars; Fill uses the screen area when selected
</scenario>
<scenario name="First start">
Given no settings.json exists
When the game launches
Then it opens windowed at 1280x720 with Whole scaling, and defaults to camera zoom 2x, UI scale 1x and lighting Medium
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-231/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-231`: the Debug build with zero warnings and every Debug test passes, including the "US-231 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-231.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-231/.
</verification>
<teach_back>C++ concept for the owner: Viewports and scaling maths.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-232 Camera zoom and UI scale
```xml
<prompt id="S-US-232" codex="2.4" milestone="M8b" story="US-232" priority="Must" size="M">
<context>
Story US-232: Camera zoom and UI scale.
As the player, I want to zoom the world and size the interface, so that I can see more of the land or read more easily.
Epic E22 Resolution and GPU renderer: The game renders at 960x540 on Luna's new SDL_GPU renderer, in any window size or full screen, and every screen and editor has room to breathe.
Traces to: ENV-18.
</context>
<dependencies>
Stories that must be Done: US-231.
Owner decisions that must be Decided: D-42, D-44.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Engine: the camera gains a zoom (1x or 2x, default 2x so the world looks as before); the UI draws in its own pass with a UI scale (1x or 2x, default 1x); the 5x7 font is drawn crisp at both scales; pointer mapping (US-121) goes through zoom and UI scale; scripted input tests updated. Put camera zoom and UI scale in the Settings screen; allow camera zoom through the mouse wheel and keys in play, while UI scale stays in Settings (D-44).
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Zoom">
Given the default camera zoom of 2x
When the player zooms out in Settings or with the mouse wheel or zoom keys in play
Then the view shows 1x (30 x 17 tiles) and back, around the hero
</scenario>
<scenario name="UI">
Given UI scale 1x and 2x
When the player switches UI scale in Settings
Then panels, the font and the hotbar resize and stay crisp
</scenario>
<scenario name="Pointer">
Given any zoom
When the player aims and clicks
Then the pointer hits the same world spot as before
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-232/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-232`: the Debug build with zero warnings and every Debug test passes, including the "US-232 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-232.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-232/.
</verification>
<teach_back>C++ concept for the owner: Two coordinate systems: world and screen.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-233 Every screen at the new size
```xml
<prompt id="S-US-233" codex="2.2" milestone="M8b" story="US-233" priority="Must" size="L">
<context>
Story US-233: Every screen at the new size.
As the owner, I want every screen, panel and editor laid out for 960x540, so that nothing is cramped or out of place.
Epic E22 Resolution and GPU renderer: The game renders at 960x540 on Luna's new SDL_GPU renderer, in any window size or full screen, and every screen and editor has room to breathe.
Traces to: ENV-18, EDT-01.
</context>
<dependencies>
Stories that must be Done: US-232.
Owner decisions that must be Decided: D-42.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game: every screen and panel (HUD, hotbar, dialogue panel, run screens, menus, settings, Editor tool bar, palettes, properties, the Interactions and Dialogue work of M8) laid out from the virtual size and UI scale, never fixed 480x270 numbers; screenshot references regenerated and shown to the owner on a contact sheet in docs/evidence/US-233/ for his approval.
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Screens">
Given the HUD, hotbar, menus, the run screens, the dialogue panel and the Editor's tool bar, palettes and properties
When they are opened at 960x540
Then they use the space, nothing overlaps and the text is legible
</scenario>
<scenario name="Tests">
Given the screenshot tests
When they run
Then they pass with the new reference pictures, reviewed by the owner
</scenario>
<scenario name="Old saves">
Given a save and a level from before
When they load
Then they play the same
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-233/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-233`: the Debug build with zero warnings and every Debug test passes, including the "US-233 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-233.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-233/.
</verification>
<teach_back>C++ concept for the owner: Layout from data instead of fixed numbers.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-234 Frame budget at the new size
```xml
<prompt id="S-US-234" codex="2.2" milestone="M8b" story="US-234" priority="Must" size="S">
<context>
Story US-234: Frame budget at the new size.
As the player, I want the game smooth at 960x540, so that the bigger picture costs nothing in feel.
Epic E22 Resolution and GPU renderer: The game renders at 960x540 on Luna's new SDL_GPU renderer, in any window size or full screen, and every screen and editor has room to breathe.
Traces to: NFR-03, MVP success criteria.
</context>
<dependencies>
Stories that must be Done: US-233.
Owner decisions that must be Decided: D-42, D-06.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Performance: a scripted 10-minute run with 500 simulated people at 1080p, recording CPU and GPU frame times (F3 overlay gains GPU time from SDL_GPU timestamps); target 60 FPS on the D-06 mid-range PC; measured on the owner's development PC (RX 7900 XTX) with the numbers scaled by the documented ratio, and the method written in docs/plans/US-234.md.
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Budget">
Given 500 simulated people in the region on the minimum PC (D-06)
When the game runs for 10 minutes
Then it holds 60 FPS
</scenario>
<scenario name="Overlay">
Given F3
When it is pressed
Then the overlay shows CPU and GPU frame times
</scenario>
<scenario name="Record">
Given the performance run
When it ends
Then its numbers are saved in docs/evidence/
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-234/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-234`: the Debug build with zero warnings and every Debug test passes, including the "US-234 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-234.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-234/.
</verification>
<teach_back>C++ concept for the owner: Measuring GPU and CPU time.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M8b" codex="2.2" name="Exit review M8b">
<instructions>
1. Demonstrate the exit criteria: The game renders at 960x540 through Luna's SDL_GPU renderer (with the old renderer as fallback) and looks the same at 2x camera zoom; windowed sizes, borderless and exclusive full screen work; every screen and editor panel is laid out for the new size; 60 FPS at 1080p on the target mid-range PC (D-06).
   Also run `pwsh tools/verify.ps1 -Story X-M8b -Config Release` on the owner's PC: every Release test with the strict 3-second first-frame limit, which GitHub's GPU-less runners only check at 10 seconds (D-47); record the result in docs/gates/M8b.md.
2. Collect evidence (test output, CI run, screenshots, frame-time tables) into docs/gates/M8b.md, one section per criterion, each marked met or not met. Also: side-by-side screenshots (old 480x270 and new 960x540 at 2x zoom) and the frame-time table from US-234.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m8b-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the exit result.</output_format>
</prompt>
```

### M8c Lighting and shadows
Exit criteria: The world is lit by the sun and moon through the day and the seasons, by fires, torches and effects at night, and dimmed by weather; sprites are shaded with generated normal maps; characters, plants and buildings cast shadows from the sun, the moon and nearby fires; the Editor previews any time of day; High lighting holds 60 FPS on the target PC.

Why this milestone exists: light that follows the day, the seasons and the weather, fires that glow, and shadows from characters, plants and buildings (D-42, ENV-19..ENV-21). The design notes under M8b apply.

```xml
<prompt id="K-M8c" codex="2.2" name="Kick off M8c Lighting and shadows">
<instructions>
1. Confirm that M8b (docs/gates/M8b.md) and its stories are done, and that D-42, D-43 and D-06 are Decided in docs/decisions.md.
2. Ask the owner, in one chat round (AskUserQuestion, 2-4 options each, recommended first), every design question the M8c stories leave open after the brief and D-42/D-43 (for example: how dark nights are, the colour of dawn and dusk, the torch's look, how long shadows may get). Record the answers in docs/decisions.md as "Decided (owner, <date>)".
3. Architect: write docs/plans/M8c-lighting-design.md (lit pass, light buffer, normal-map generation, sky curves and day length, shadow projection and budgets, weather light, quality presets) before the first story.
4. Set this milestone's stories to To do in docs/status.md in this order: US-240, US-241, US-242, US-243, US-248, US-244, US-245, US-246, US-247.
5. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, the owner's answers.</output_format>
</prompt>
```

#### S-US-240 The lighting pipeline
```xml
<prompt id="S-US-240" codex="2.2" milestone="M8c" story="US-240" priority="Must" size="L">
<context>
Story US-240: The lighting pipeline.
As the developer, I want a lit pass with ambient light, point lights and normal maps in the GPU renderer, so that the world can be lit from data.
Epic E23 Lighting and shadows: Days and nights, seasons and weather light the world; fires glow; characters, plants and buildings cast shadows that move with the sun, the moon and nearby fires.
Traces to: ENV-19, ADR-021.
</context>
<dependencies>
Stories that must be Done: US-230.
Owner decisions that must be Decided: D-42.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Engine: a lit pass in the GpuRenderer: ambient colour, up to 64 point lights per frame in a buffer, per-sprite normal-map sampling; the Renderer interface gains light and normal-map calls (no SDL types in the interface); assets/data/light/lights.json with a schema-ready layout; the SDL_Renderer fallback draws unlit with the ambient tint only.
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Ambient">
Given an ambient colour in lights.json
When the scene is drawn
Then every sprite is tinted by it
</scenario>
<scenario name="Point light">
Given a light with a colour, radius and strength
When it is placed near a sprite
Then the sprite's lit side faces the light, using its normal map
</scenario>
<scenario name="Budget">
Given 64 lights on screen
When the scene is drawn
Then the frame budget of US-234 still holds
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-240/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-240`: the Debug build with zero warnings and every Debug test passes, including the "US-240 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-240.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-240/.
</verification>
<teach_back>C++ concept for the owner: Shader inputs, uniform data and lighting maths.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-241 Generated normal maps
```xml
<prompt id="S-US-241" codex="2.2" milestone="M8c" story="US-241" priority="Must" size="M">
<context>
Story US-241: Generated normal maps.
As the owner, I want normal maps made from my sprites by a tool, so that lights shade my art without painting a second image.
Epic E23 Lighting and shadows: Days and nights, seasons and weather light the world; fires glow; characters, plants and buildings cast shadows that move with the sun, the moon and nearby fires.
Traces to: ENV-19, D-05.
</context>
<dependencies>
Stories that must be Done: US-240, US-120.
Owner decisions that must be Decided: D-42.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Tools: odysseus_atlas --normals generates a normal-map atlas per atlas (height from alpha distance and luminance, Sobel slopes); a hand-made <frame>_n.png next to a cut wins; normal atlases committed; missing maps light flat.
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Generate">
Given the atlases
When the atlas tool runs with normal maps on
Then every atlas gets a matching normal-map atlas
</scenario>
<scenario name="Missing">
Given a sprite without a normal map
When it is lit
Then it is lit flat, with no error
</scenario>
<scenario name="Own map">
Given a hand-made normal map named like the sprite
When the tool runs
Then the hand-made one is kept
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-241/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-241`: the Debug build with zero warnings and every Debug test passes, including the "US-241 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-241.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-241/.
</verification>
<teach_back>C++ concept for the owner: Image processing: height and slopes from pixels.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-242 Day, night and seasons
```xml
<prompt id="S-US-242" codex="2.2" milestone="M8c" story="US-242" priority="Must" size="M">
<context>
Story US-242: Day, night and seasons.
As the player, I want dawn, day, dusk and night to follow the game clock, with longer summer days and short winter days, so that time feels real.
Epic E23 Lighting and shadows: Days and nights, seasons and weather light the world; fires glow; characters, plants and buildings cast shadows that move with the sun, the moon and nearby fires.
Traces to: ENV-19, ENV-20.
</context>
<dependencies>
Stories that must be Done: US-240, US-010.
Owner decisions that must be Decided: D-42.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game: assets/data/light/sky.json keyframes per hour (sun and moon direction and elevation, ambient colour, shadow strength), blended from the simulation's game clock; day length per season from calendar.json (sunrise and sunset hours); the simulation's night hours stay as they are (no simulation change).
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Cycle">
Given the game clock running
When a day passes
Then the light goes through dawn, day, dusk and night colours from sky.json
</scenario>
<scenario name="Seasons">
Given midsummer and midwinter
When the days are compared
Then the summer day is longer, as calendar.json says
</scenario>
<scenario name="Moon">
Given night
When it is drawn
Then the moon gives a dim, blue light and a direction for shadows
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-242/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-242`: the Debug build with zero warnings and every Debug test passes, including the "US-242 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-242.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-242/.
</verification>
<teach_back>C++ concept for the owner: Interpolating curves over time.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-243 Fires, torches and glowing effects
```xml
<prompt id="S-US-243" codex="2.2" milestone="M8c" story="US-243" priority="Must" size="M">
<context>
Story US-243: Fires, torches and glowing effects.
As the player, I want fires, torches, the sacred fire and elemental effects to light the night with a flicker, so that camps glow in the dark.
Epic E23 Lighting and shadows: Days and nights, seasons and weather light the world; fires glow; characters, plants and buildings cast shadows that move with the sun, the moon and nearby fires.
Traces to: ENV-19.
</context>
<dependencies>
Stories that must be Done: US-240, US-132.
Owner decisions that must be Decided: D-42.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Data: a `light` field on objects, effects, weapons and items (colour, radius, strength, flicker); a torch item for clan members at night (carried light follows its holder); flicker from a seeded noise in the Game, never the simulation stream.
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Fire">
Given a lit fire pit at night
When the scene is drawn
Then it lights a circle around it with a flicker
</scenario>
<scenario name="Data">
Given a catalog entry with a light field
When the thing is placed
Then it emits that light; without the field, none
</scenario>
<scenario name="Torch">
Given a clan member carrying a torch
When they walk at night
Then the light moves with them
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-243/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-243`: the Debug build with zero warnings and every Debug test passes, including the "US-243 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-243.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-243/.
</verification>
<teach_back>C++ concept for the owner: Noise for natural flicker.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-248 Celestial bodies: the sun and the moon as objects
```xml
<prompt id="S-US-248" codex="2.7" milestone="M8c" story="US-248" priority="Must" size="L">
<context>
Story US-248: Celestial bodies.
As the owner, I want the sun and the moon to be objects I can place in a level, so that the light and the shadows come from where they really are.
Epic E23 Lighting and shadows: Days and nights, seasons and weather light the world; fires glow; characters, plants and buildings cast shadows that move with the sun, the moon and nearby fires.
Traces to: ENV-22, ENV-21.
Decision D-50 (owner, 2026-10-04): sun and moon are light-source objects placed in the Editor; their position decides the light direction and the shadows; every level gets a default pair on an orbit that follows the game clock; both are drawn as sprites in the sky band or at the edge of the picture; eclipses are scripted data events; moon phases, shadow maps and height-map shadows are not part of this milestone. Brief: docs/plans/US-248-celestial-brief.md.
</context>
<dependencies>
Stories that must be Done: US-242, US-243.
Owner decisions that must be Decided: D-42, D-49, D-50.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Data: a `celestial` kind of world object in the object catalog (sprite, a light kind from `light/lights.json`, orbit radius, tilt and height in metres where 1 tile = 1 m, and `follows`: `"clock"` or a fixed position); `assets/data/light/celestial-events.json` for eclipses (which body, start day and hour, length, depth of the dimming). Game: `src/game/celestial.*` gives, every frame, the current light (direction away from the body, elevation above the ground, strength, source body) for the sun and for the moon; the default pair is used when a level places none; the sky ambient colour and the shadow strength still come from `sky.json` (US-242). Draw the bodies as sprites in the sky band or at the edge of the picture. Editor: place, move and delete the bodies, with the time-of-day preview that already exists. Presentation only: the simulation never reads any of it (ADR-016, determinism hash unchanged).
Rules: clamp the elevation to a minimum so a body on the horizon gives a long but finite shadow (the D-49 caps of 2.5 x the object's height for the sun and 1.5 x for the moon stay); with several suns the strongest is used and nothing is summed. Hand US-244 one function that returns the current light, so that US-244 only draws shadows.
Follow the brief docs/plans/US-248-celestial-brief.md, docs/plans/M8c-lighting-design.md and docs/guides/lighting.md; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else (moon phases, shadows themselves) is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Default pair">
Given a level that places no sun or moon
When the game runs through a day
Then a default sun and moon travel their orbit by the clock, and the light direction follows the sun by day and the moon by night
</scenario>
<scenario name="Placed body">
Given a sun placed at another position in the Editor
When the level is played
Then the light direction and elevation come from that position, not from the default orbit
</scenario>
<scenario name="Seen">
Given the sun or the moon above the horizon
When the scene is drawn
Then its sprite appears in the sky band or at the edge of the picture and moves with the clock
</scenario>
<scenario name="Eclipse">
Given an eclipse event for the sun in celestial-events.json
When its time comes
Then the sun's light dims by the stated depth for the stated length and then returns
</scenario>
<scenario name="Data">
Given a mistake in a celestial entry or an event
When the file loads
Then the problem names the file and the field, and the game keeps the default pair
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/lighting.md with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-248/; the determinism hash test passes unchanged.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-248`: the Debug build with zero warnings and every Debug test passes, including the "US-248 ..." cases and the determinism hash test. After a failure, rerun only the failing cases with the doctest filter on the test exe, then run the full check once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-248.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-248/.
</verification>
<teach_back>C++ concept for the owner: Turning a position into a direction with a vector and an angle (atan2).</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-244 Sun and moon shadows
```xml
<prompt id="S-US-244" codex="2.2" milestone="M8c" story="US-244" priority="Must" size="L">
<context>
Story US-244: Sun and moon shadows.
As the player, I want characters, plants and buildings to cast shadows that turn and stretch with the sun and the moon, so that the world has depth.
Epic E23 Lighting and shadows: Days and nights, seasons and weather light the world; fires glow; characters, plants and buildings cast shadows that move with the sun, the moon and nearby fires.
Traces to: ENV-21.
</context>
<dependencies>
Stories that must be Done: US-242, US-248.
Owner decisions that must be Decided: D-42, D-50.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Engine: shadow casters (sprite silhouette, ground point, height from the catalog's `height`) drawn before the lit pass as sheared, darkened silhouettes along the light direction given by the celestial bodies of US-248 (the body's true position), length from the elevation, fading with shadow strength; catalogs gain `height` and optional `shadow: false`.
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Day">
Given a tree at 9:00 and at 17:00
When it is drawn
Then its shadow points away from the sun and is longer in the morning and evening
</scenario>
<scenario name="Casters">
Given characters, plants and buildings
When the sun shines
Then each casts a shadow sized by its height in the catalog
</scenario>
<scenario name="Overcast">
Given fog or heavy cloud
When it sets in
Then the shadows fade
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-244/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-244`: the Debug build with zero warnings and every Debug test passes, including the "US-244 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-244.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-244/.
</verification>
<teach_back>C++ concept for the owner: Projecting silhouettes with a shear transform.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-245 Shadows from fires
```xml
<prompt id="S-US-245" codex="2.2" milestone="M8c" story="US-245" priority="Must" size="M">
<context>
Story US-245: Shadows from fires.
As the player, I want a fire to throw shadows of the people and things near it at night, so that night scenes feel alive.
Epic E23 Lighting and shadows: Days and nights, seasons and weather light the world; fires glow; characters, plants and buildings cast shadows that move with the sun, the moon and nearby fires.
Traces to: ENV-21.
</context>
<dependencies>
Stories that must be Done: US-244, US-243.
Owner decisions that must be Decided: D-42.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Engine: for each caster, the nearest fires within their radius (max per object from lights.json) add faint shadows away from the fire at night; the Low preset turns them off.
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Fire shadow">
Given a person standing near a fire at night
When it is drawn
Then their shadow points away from the fire
</scenario>
<scenario name="Two fires">
Given a person between two fires
When it is drawn
Then they cast two faint shadows
</scenario>
<scenario name="Budget">
Given many fires
When the scene is drawn
Then only the nearest fires (from lights.json) cast shadows and the frame budget holds
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-245/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-245`: the Debug build with zero warnings and every Debug test passes, including the "US-245 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-245.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-245/.
</verification>
<teach_back>C++ concept for the owner: Choosing the nearest lights per object within a budget.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-246 Weather and light
```xml
<prompt id="S-US-246" codex="2.2" milestone="M8c" story="US-246" priority="Must" size="S">
<context>
Story US-246: Weather and light.
As the player, I want clouds, rain, fog and storms to darken and tint the world, with lightning flashes, so that weather changes the mood.
Epic E23 Lighting and shadows: Days and nights, seasons and weather light the world; fires glow; characters, plants and buildings cast shadows that move with the sun, the moon and nearby fires.
Traces to: ENV-19.
</context>
<dependencies>
Stories that must be Done: US-242, US-138.
Owner decisions that must be Decided: D-42.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Data: weather.json entries gain `light` (dim, tint) and `flash` (lightning frequency); the weather fade (US-138) blends the light the same 3 s; lightning flashes the ambient for a few frames.
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Dim">
Given rain starting at noon
When it fades in
Then the light dims and cools over the same 3 s
</scenario>
<scenario name="Lightning">
Given a storm
When lightning strikes
Then the whole scene flashes for a moment
</scenario>
<scenario name="Data">
Given weather.json
When the owner sets a weather's light
Then the game uses it
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-246/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-246`: the Debug build with zero warnings and every Debug test passes, including the "US-246 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-246.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-246/.
</verification>
<teach_back>C++ concept for the owner: Blending light settings.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-247 Lighting in the Editor and quality settings
```xml
<prompt id="S-US-247" codex="2.2" milestone="M8c" story="US-247" priority="Must" size="M">
<context>
Story US-247: Lighting in the Editor and quality settings.
As the owner, I want to preview any time of day and place lights in the Editor, and players to choose lighting quality, so that levels look right and run everywhere.
Epic E23 Lighting and shadows: Days and nights, seasons and weather light the world; fires glow; characters, plants and buildings cast shadows that move with the sun, the moon and nearby fires.
Traces to: ENV-19, EDT-01.
</context>
<dependencies>
Stories that must be Done: US-243, US-244, US-248.
Owner decisions that must be Decided: D-42, D-06.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Editor: a time-of-day slider in the tool bar (preview only, not saved), a Light tool that places light points saved in the level (level version bump with migration); Settings: lighting Low (no normal maps, no fire shadows), Medium, High; the D-06 target holds High at 60 FPS.
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Preview">
Given the Editor
When the owner drags the time-of-day slider
Then the level is lit as at that hour
</scenario>
<scenario name="Place">
Given the light tool
When the owner places a light and sets it
Then it is saved in the level and shines in the game
</scenario>
<scenario name="Quality">
Given Low, Medium and High lighting
When the player picks Low
Then fire shadows and normal maps switch off and the frame time drops
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-247/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-247`: the Debug build with zero warnings and every Debug test passes, including the "US-247 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-247.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-247/.
</verification>
<teach_back>C++ concept for the owner: Settings that change a pipeline.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M8c" codex="2.2" name="Exit review M8c">
<instructions>
1. Demonstrate the exit criteria: The world is lit by the sun and moon through the day and the seasons, by fires, torches and effects at night, and dimmed by weather; sprites are shaded with generated normal maps; characters, plants and buildings cast shadows from the sun, the moon and nearby fires; the Editor previews any time of day; High lighting holds 60 FPS on the target PC.
   Also run `pwsh tools/verify.ps1 -Story X-M8c -Config Release` on the owner's PC: every Release test with the strict 3-second first-frame limit, which GitHub's GPU-less runners only check at 10 seconds (D-47); record the result in docs/gates/M8c.md.
2. Collect evidence (test output, CI run, screenshots, frame-time tables) into docs/gates/M8c.md, one section per criterion, each marked met or not met. Also: a time-lapse screenshot sheet (dawn, noon, dusk, night with fires, rain) and the frame-time table on High and Low.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m8c-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the exit result.</output_format>
</prompt>
```

### M9a NPC foundation
Exit criteria: Every placed NPC is a person with one or more classes, an attitude and opinions; the player talks to and confronts NPCs and sees their actions in a pop-up; the owner edits classes, kinds and NPCs in the Editor; the NPC test level shows all of it; 100,000 persons run within the ADR-022 budget.

Why this milestone exists: the owner asked on 2026-10-04 to talk and trade with every NPC, to set each NPC's classes, attitude and actions in the Editor, and to treat placed NPCs as full persons of a world of up to 100,000 (D-52). Today only clan members talk and only rival camps barter; placed characters are combat figures. It is built right after M8c (D-52 Q-01).

Design notes for M9a-M9c (owner decisions D-52, docs/decision-requests/D-52.md; brief docs/plans/M9a-npc-roles-brief.md):
- NPC Class is one concept for the owner's "categories" and "classes": owner-defined in the Editor (assets/data/npc-classes/<id>.json), with colour, icon, tags, default dialogues and default actions; an NPC has one or more classes.
- Layers of data, each overriding the one before: class, kind file (assets/data/npcs/<kind>.json), placed NPC in the level (only differences). Allow and deny lists merge in that order; a later deny wins.
- Placed NPCs other than animals and monsters are full persons of the simulation. Up to 100,000 region-loaded persons: compact store, spatial grid, detail by distance (full near the hero, a daily summary far away), opinions only for pairs that met (ADR-022, written in US-263).
- Attitude words: friendly, neutral, wary, hostile, scared, suspicious, enchanted, lovingly, enviously; derived from an integer opinion per pair; no factions.
- Talk only for NPCs with a dialogue for that partner; Confront is a separate button (taunt, insult, ask for peace, antagonise, de-escalate) with opinion consequences. Denied actions are hidden; the Actions pop-up shows every action and its requirements.
- Everything in the simulation is deterministic, integer and saved; Editor forms, menus, pop-ups and markers are Game code; nothing new touches SDL. Never edit assets/levels/valley.json (the owner's work); the test level is assets/levels/npc-test.json.
- Source of truth: Project Odyssey.docx v2.10 holds epics E26-E28, requirements SDC-08..SDC-12, INT-09..INT-12, EDT-08, NFR-08 and MVP-17 (Round 23); SDC-12 builds the schedules that SDC-07 (M11, US-196) reuses.

```xml
<prompt id="K-M9a" codex="2.9" name="Kick off M9a NPC foundation">
<instructions>
1. Confirm that M8c (docs/gates/M8c.md) and its stories are done, and that D-34, D-35 and D-52 are Decided in docs/decisions.md.
2. Confirm that the mirrored requirements (docs/project/requirements/Project Odyssey.docx) are v2.10 or later and contain epics E26-E28; if not, run tools/sync-workspace.ps1, and stop with a codex issue if they still do not.
3. D-52 is Decided (docs/decision-requests/D-52.md). Ask the owner, in one chat round (2-4 options each, recommended first), only the questions the M9a stories still leave open after D-52 (for example: the Confront key, the Actions pop-up key, the icon set for classes). Record the answers.
4. Architect: write docs/plans/M9-npc-design.md before US-260: class, kind and override formats; the person store and ADR-022 outline; opinion events and thresholds; talk and confront; the Actions pop-up; Editor forms; save format; test plan for M9a-M9c.
5. Set this milestone's stories to To do in docs/status.md in this order: US-260, US-261, US-262, US-263, US-264, US-265, US-266, US-267, US-268, US-269, US-270.
6. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, the owner's answers.</output_format>
</prompt>
```

#### S-US-260 NPC Classes
```xml
<prompt id="S-US-260" codex="2.9" milestone="M9a" story="US-260" priority="Must" size="M">
<context>
Story US-260: NPC Classes.
As the owner, I want to create, edit, delete and assign NPC Classes in the Editor, each with a colour, an icon, tags, default dialogues and default actions, so that I can say what kinds of people my world has.
Epic E26 NPC foundation: Every placed NPC is a person with classes, an attitude and opinions; the player talks to, confronts and acts on any NPC; the owner sets every NPC in the Editor.
Traces to: SDC-08, EDT-08.
</context>
<dependencies>
Stories that must be Done: US-150, US-126.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation: a class catalog loaded from assets/data/npc-classes/<id>.json (loader and validator next to src/sim/interaction.*, errors as `file:line: message`, a bad file is skipped, F5 reloads it). Game (Editor): an NPC Classes tab: list, New, Delete (refused with the names of the NPCs that still use the class), and a form for label, colour, icon (from the icon atlas), tags, default dialogues by partner type and default allow/deny actions; saves readable JSON in the guide's field order. Ship classes for the test cast: trader, talker, hunter, elder, guard, monster, animal.
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Create">
Given the NPC Classes tab
When the owner creates the class healer with a green colour and the tag healer and saves
Then assets/data/npc-classes/healer.json exists and the class is offered in every class picker
</scenario>
<scenario name="Delete in use">
Given a class used by two placed NPCs
When the owner deletes it
Then the Editor refuses and names both NPCs
</scenario>
<scenario name="Mistake">
Given a class file with an unknown icon
When the game starts
Then the error names the file, the line and the field, and the other classes load
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-260`: the Debug build with zero warnings and every Debug test passes, including the "US-260 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-260.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-260/.
</verification>
<teach_back>C++ concept for the owner: Catalogs keyed by id; refusing a delete that would leave dangling references.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-261 Kind defaults and placed-NPC overrides
```xml
<prompt id="S-US-261" codex="2.9" milestone="M9a" story="US-261" priority="Must" size="M">
<context>
Story US-261: Kind defaults and placed-NPC overrides.
As the owner, I want defaults per NPC kind and per-NPC changes in the level, so that a new wanderer starts sensible and each placed one can differ.
Epic E26 NPC foundation: Every placed NPC is a person with classes, an attitude and opinions; the player talks to, confronts and acts on any NPC; the owner sets every NPC in the Editor.
Traces to: SDC-09, EDT-08.
</context>
<dependencies>
Stories that must be Done: US-260, US-122.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation: kind files assets/data/npcs/<kind>.json (classes, attitude, tags, dialogues, actions); placed characters in the level gain the same fields, saved only where they differ. Precedence: classes, then kind, then the placed NPC; allow and deny lists merge in that order and a later deny wins. A level without the new fields loads as before. Kind files for every shipped character kind keep today's behaviour (goblin, skeleton, wolf, tiger hostile). F5 reloads kind files.
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Precedence">
Given the class trader allows trade and a placed NPC of that class denies it
When the game resolves the NPC's actions
Then trade is denied for that NPC only
</scenario>
<scenario name="Old level">
Given a level saved before this story
When it is loaded and saved
Then the file is unchanged
</scenario>
<scenario name="Reload">
Given a kind file changed on disk while playing
When the owner presses F5
Then the change applies to every NPC of that kind without an override
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-261`: the Debug build with zero warnings and every Debug test passes, including the "US-261 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-261.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-261/.
</verification>
<teach_back>C++ concept for the owner: Layered defaults; std::optional for 'not set'; merging lists with a precedence rule.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-262 Placed NPCs are full persons
```xml
<prompt id="S-US-262" codex="2.9" milestone="M9a" story="US-262" priority="Must" size="L">
<context>
Story US-262: Placed NPCs are full persons.
As the player, I want the people placed in a level to be real members of the world, with needs, memories, ageing and families, so that they feel alive like my clan.
Epic E26 NPC foundation: Every placed NPC is a person with classes, an attitude and opinions; the player talks to, confronts and acts on any NPC; the owner sets every NPC in the Editor.
Traces to: SDC-09.
</context>
<dependencies>
Stories that must be Done: US-261, US-154.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation: every placed NPC that is not an animal or monster class becomes a person record of the simulation at level load (needs, memories, age, family; a family id may be set in the Editor later by US-268), driven by the same daily rules as clan members; its figure on the map follows the person. Animals and monsters stay creatures. Persons are saved and loaded with the game; the determinism hash test includes them.
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Person">
Given a placed wanderer
When a day passes
Then its needs change and a memory of the day exists, as for a clan member
</scenario>
<scenario name="Saved">
Given a placed person who aged one day and met the hero
When the game is saved and loaded
Then age, needs and memories are the same
</scenario>
<scenario name="Creatures">
Given a placed goblin of class monster
When the level loads
Then it is a creature, not a person, and fights as before
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-262`: the Debug build with zero warnings and every Debug test passes, including the "US-262 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-262.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-262/.
</verification>
<teach_back>C++ concept for the owner: Identity and lifetime: one id from the level file to the save.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-263 The NPC store and detail by distance
```xml
<prompt id="S-US-263" codex="2.9" milestone="M9a" story="US-263" priority="Must" size="L">
<context>
Story US-263: The NPC store and detail by distance.
As the owner, I want the world to hold up to 100,000 region-loaded NPCs while the game stays smooth, so that the end game can be crowded.
Epic E26 NPC foundation: Every placed NPC is a person with classes, an attitude and opinions; the player talks to, confronts and acts on any NPC; the owner sets every NPC in the Editor.
Traces to: NFR-08, SDC-09.
</context>
<dependencies>
Stories that must be Done: US-262.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation: a compact person store (struct of arrays, ids, no per-person heap objects in hot loops) and a spatial grid for 'who is near'. Detail by distance (D-52 S-02): persons within the near radius tick fully; far persons get one coarse summary per in-game day; moving in or out of the radius changes their level without losing state. Write ADR-022 (store layout, radius, daily budget, numbers measured). A headless load test with 100,000 generated persons.
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Load">
Given 100,000 generated persons in a headless run
When one in-game day is simulated
Then it finishes within the budget of ADR-022 on the D-06 PC and the determinism hash is stable over two runs
</scenario>
<scenario name="Near and far">
Given a person who walks out of the near radius and back
When the days pass
Then its needs and memories continue without a jump or a loss
</scenario>
<scenario name="Frame">
Given the test level with 100,000 far persons loaded
When the game runs for one minute
Then the frame time stays within the D-06 target
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-263`: the Debug build with zero warnings and every Debug test passes, including the "US-263 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-263.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-263/.
</verification>
<teach_back>C++ concept for the owner: Struct of arrays and cache lines; spatial hashing; why O(n^2) breaks at 100,000.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-264 Attitudes and opinions
```xml
<prompt id="S-US-264" codex="2.9" milestone="M9a" story="US-264" priority="Must" size="M">
<context>
Story US-264: Attitudes and opinions.
As the player, I want every NPC to have its own attitude to me and to others, changed by what we do, so that my choices matter person by person.
Epic E26 NPC foundation: Every placed NPC is a person with classes, an attitude and opinions; the player talks to, confronts and acts on any NPC; the owner sets every NPC in the Editor.
Traces to: SDC-10.
</context>
<dependencies>
Stories that must be Done: US-263.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation: nine attitude words (friendly, neutral, wary, hostile, scared, suspicious, enchanted, lovingly, enviously) derived from an integer opinion per (holder, target) pair, stored only for pairs that met. Events that change it (D-52 Q-05): dialogue quality and frequency, gifts, trade, help events, a marriage in the family, being in the same family; each with an amount in data (assets/data/sim/opinions.json). Hostile replaces the old `enemy` switch for NPCs with a kind file. The attitude word shows in the NPC's menu title.
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Gift">
Given a neutral NPC
When the hero gives it a gift worth the 'gift' amount
Then its opinion of the hero rises by that amount and its word changes when a threshold is crossed
</scenario>
<scenario name="Family">
Given two NPCs of the same family
When the level loads
Then each starts with the 'same family' opinion of the other
</scenario>
<scenario name="Sparse">
Given 100,000 persons where 10 pairs have met
When the store is inspected
Then exactly 10 opinion entries exist
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-264`: the Debug build with zero warnings and every Debug test passes, including the "US-264 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-264.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-264/.
</verification>
<teach_back>C++ concept for the owner: Sparse maps keyed by pairs; deriving words from numbers with thresholds.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-265 Talk with placed NPCs
```xml
<prompt id="S-US-265" codex="2.9" milestone="M9a" story="US-265" priority="Must" size="M">
<context>
Story US-265: Talk with placed NPCs.
As the player, I want to talk to any NPC that has something to say, so that every person in a level can speak.
Epic E26 NPC foundation: Every placed NPC is a person with classes, an attitude and opinions; the player talks to, confronts and acts on any NPC; the owner sets every NPC in the Editor.
Traces to: SDC-11, SDC-03.
</context>
<dependencies>
Stories that must be Done: US-264, US-161.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game: placed persons become subjects with their resolved tags; `talk.json` targets a tag every NPC with a dialogue for the player carries; the conversation opens the NPC's `player` dialogue (class default, kind or the NPC's own); an NPC with no dialogue has no Talk option (D-52 Q-11). Dialogues may read and change opinions (conditions and effects added to the .dlg guide). The panel pauses the game (D-35). Clan members keep their M8 talk.
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Talk">
Given a placed NPC with a player dialogue, 2 m from the hero
When the player right-clicks it and chooses Talk
Then its dialogue opens and the game pauses
</scenario>
<scenario name="No script">
Given a placed NPC with no player dialogue
When the player right-clicks it
Then Talk is not offered
</scenario>
<scenario name="Clan unchanged">
Given a clan member
When the player chooses Talk
Then the conversation is the same as before this story
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-265`: the Debug build with zero warnings and every Debug test passes, including the "US-265 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-265.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-265/.
</verification>
<teach_back>C++ concept for the owner: One interface for many kinds of things; data-driven dispatch by tags.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-266 Confront
```xml
<prompt id="S-US-266" codex="2.9" milestone="M9a" story="US-266" priority="Must" size="M">
<context>
Story US-266: Confront.
As the player, I want a separate Confront button with taunt, insult, ask for peace, antagonise and de-escalate, so that I can deal with hostile NPCs by words and live with the consequences.
Epic E26 NPC foundation: Every placed NPC is a person with classes, an attitude and opinions; the player talks to, confronts and acts on any NPC; the owner sets every NPC in the Editor.
Traces to: SDC-11.
</context>
<dependencies>
Stories that must be Done: US-265.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game: a Confront intent (its own key and a right-click menu entry) offered for any NPC, hostile ones included; five interaction files (taunt, insult, ask-for-peace, antagonise, de-escalate) whose effects change opinions of the target and of every NPC within hearing range who knows the target, by amounts in data; the outcome can start or stop a fight. NPCs may confront each other later (US-292).
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Calm down">
Given a hostile NPC about to attack
When the player chooses De-escalate and the check succeeds
Then it stops attacking and its opinion of the hero rises
</scenario>
<scenario name="Insult">
Given an NPC with two friends within hearing range
When the player insults it
Then its opinion and its friends' opinions of the hero fall by the data amounts
</scenario>
<scenario name="Separate">
Given a friendly NPC
When the player presses the Confront key
Then the five confront actions are offered and Talk is not among them
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-266`: the Debug build with zero warnings and every Debug test passes, including the "US-266 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-266.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-266/.
</verification>
<teach_back>C++ concept for the owner: Intents mapped from keys; effects that spread to bystanders.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-267 Actions and the Actions pop-up
```xml
<prompt id="S-US-267" codex="2.9" milestone="M9a" story="US-267" priority="Must" size="S">
<context>
Story US-267: Actions and the Actions pop-up.
As the player, I want to see every action an NPC could offer and what each needs, while the menu shows only what I can do now, so that I know how to unlock the rest.
Epic E26 NPC foundation: Every placed NPC is a person with classes, an attitude and opinions; the player talks to, confronts and acts on any NPC; the owner sets every NPC in the Editor.
Traces to: INT-09.
</context>
<dependencies>
Stories that must be Done: US-265.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation: the offer check applies tags, then the resolved allow and deny lists (D-52 Q-08). Game: denied or unmet actions are hidden in the right-click menu (Q-09); an Actions pop-up (a menu entry and a key) lists every action the NPC has, each with its unmet requirements in plain words (for example 'needs: friendly or better').
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Hidden">
Given a trader whose trade needs the attitude friendly and who is wary of the hero
When the player right-clicks it
Then Trade is not in the menu
</scenario>
<scenario name="Pop-up">
Given the same trader
When the player opens the Actions pop-up
Then Trade is listed with 'needs: friendly or better'
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-267`: the Debug build with zero warnings and every Debug test passes, including the "US-267 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-267.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-267/.
</verification>
<teach_back>C++ concept for the owner: Filtering with std::ranges; turning conditions into readable text.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-268 Editor NPC panel
```xml
<prompt id="S-US-268" codex="2.9" milestone="M9a" story="US-268" priority="Must" size="L">
<context>
Story US-268: Editor NPC panel.
As the owner, I want to select a placed NPC in the Editor and set its name, classes, attitude, family, dialogues by partner type and actions, so that I set every NPC by hand.
Epic E26 NPC foundation: Every placed NPC is a person with classes, an attitude and opinions; the player talks to, confronts and acts on any NPC; the owner sets every NPC in the Editor.
Traces to: EDT-08, SDC-08.
</context>
<dependencies>
Stories that must be Done: US-267, US-125.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game (Editor): the Select tool's properties panel for a placed character: name, classes (several), starting attitude, family, dialogues by partner type (Player, each NPC Class, Animals, Environment; the list comes from data so it can grow), allow/deny action checkboxes built from the registry, and 'Reset to defaults'. Only differences from class and kind are saved. Each change is one Undo step (US-126).
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Edit">
Given a placed wanderer selected
When the owner gives it the classes trader and elder, the attitude friendly and its own player dialogue, and saves
Then the level holds only those differences and F1 plays with them
</scenario>
<scenario name="Actions">
Given the actions list of a placed NPC
When the owner unticks Give gift and plays
Then that NPC's menu does not offer Give gift
</scenario>
<scenario name="Undo">
Given five panel changes
When the owner presses Ctrl+Z five times
Then the NPC is as it was
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-268`: the Debug build with zero warnings and every Debug test passes, including the "US-268 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-268.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-268/.
</verification>
<teach_back>C++ concept for the owner: Forms bound to data; saving only the differences.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-269 Editor kinds tab and map markers
```xml
<prompt id="S-US-269" codex="2.9" milestone="M9a" story="US-269" priority="Must" size="M">
<context>
Story US-269: Editor kinds tab and map markers.
As the owner, I want a tab for each NPC kind's defaults and a colour ring with an icon under each placed NPC, so that I can change a whole kind at once and read a level at a glance.
Epic E26 NPC foundation: Every placed NPC is a person with classes, an attitude and opinions; the player talks to, confronts and acts on any NPC; the owner sets every NPC in the Editor.
Traces to: EDT-08.
</context>
<dependencies>
Stories that must be Done: US-268.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game (Editor): an NPC kinds tab reusing the US-268 form, writing assets/data/npcs/<kind>.json; under every placed NPC a ring in its class colour with the class icon (several classes: the ring split in equal arcs, the first class's icon); drawn only in the Editor.
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Kind">
Given the kinds tab
When the owner sets goblin to neutral and saves
Then every placed goblin without an override is neutral in play
</scenario>
<scenario name="Markers">
Given a level with a trader, a guard and a goblin
When the owner opens it in the Editor
Then each shows its ring and icon, and none shows in Game mode
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-269`: the Debug build with zero warnings and every Debug test passes, including the "US-269 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-269.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-269/.
</verification>
<teach_back>C++ concept for the owner: Reusing a widget for two data sources; stable JSON field order.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-270 NPC test level
```xml
<prompt id="S-US-270" codex="2.9" milestone="M9a" story="US-270" priority="Must" size="S">
<context>
Story US-270: NPC test level.
As the owner, I want a test level with seven NPCs that shows every M9a feature, so that I can try it all in one walk.
Epic E26 NPC foundation: Every placed NPC is a person with classes, an attitude and opinions; the player talks to, confronts and acts on any NPC; the owner sets every NPC in the Editor.
Traces to: SDC-08..SDC-11, INT-09.
</context>
<dependencies>
Stories that must be Done: US-269.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Data: assets/levels/npc-test.json built in the Editor with the D-52 cast: trader, talker, wary hunter, elder (with a written dialogue), guard, goblin, deer; each talking NPC with its own .dlg; a walk-through checklist in docs/guides/npc-data.md. Never edit assets/levels/valley.json.
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Walk-through">
Given `odysseus.exe --level assets/levels/npc-test.json`
When the owner follows the checklist
Then every NPC offers exactly the actions its classes and overrides allow, the Actions pop-up explains the rest, confront changes opinions, the goblin attacks
</scenario>
<scenario name="Loads clean">
Given the test level
When the game loads it
Then the log has no class, kind, interaction or dialogue error
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-270`: the Debug build with zero warnings and every Debug test passes, including the "US-270 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-270.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-270/.
</verification>
<teach_back>C++ concept for the owner: Smoke tests that load every shipped file.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
```xml
<prompt id="X-M9a" codex="2.9" name="Exit review M9a">
<instructions>
1. Demonstrate the exit criteria: Every placed NPC is a person with one or more classes, an attitude and opinions; the player talks to and confronts NPCs and sees their actions in a pop-up; the owner edits classes, kinds and NPCs in the Editor; the NPC test level shows all of it; 100,000 persons run within the ADR-022 budget.
   Also run `pwsh tools/verify.ps1 -Story X-M9a -Config Release` on the owner's PC: every Release test with the strict 3-second first-frame limit, which GitHub's GPU-less runners only check at 10 seconds (D-47); record the result in docs/gates/M9a.md.
2. Collect evidence (test output, CI run, screenshots) into docs/gates/M9a.md, one section per criterion, each marked met or not met. Also: the US-270 walk-through; one NPC changed in the panel and one kind and one class changed in their tabs, each shown in play; the ADR-022 load test result.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m9a-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the exit result.</output_format>
</prompt>
```

### M9b Trade economy
Exit criteria: The player trades with any trader by barter or the region's currency; prices follow supply and demand and the trader's opinion; stock is limited and restocks daily; the owner sets currencies and stock in the Editor.

Why this milestone exists: D-52 Q-13..Q-17 and S-03: barter and owner-defined currency, supply and demand, reputation, limited daily stock.

```xml
<prompt id="K-M9b" codex="2.9" name="Kick off M9b Trade economy">
<instructions>
1. Confirm that M9a (docs/gates/M9a.md) and its stories are done, and that D-34, D-35 and D-52 are Decided in docs/decisions.md.
2. Ask the owner, in one chat round (2-4 options each, recommended first), only the questions the M9b stories leave open after D-52. Record the answers.
3. Architect: extend docs/plans/M9-npc-design.md for M9b before its first story.
4. Set this milestone's stories to To do in docs/status.md in this order: US-280, US-281, US-282, US-283, US-284.
5. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, the owner's answers.</output_format>
</prompt>
```

#### S-US-280 Currencies per region
```xml
<prompt id="S-US-280" codex="2.9" milestone="M9b" story="US-280" priority="Must" size="S">
<context>
Story US-280: Currencies per region.
As the owner, I want to choose in the Editor which items are currency in a region, so that trade can use money where I want it.
Epic E27 Trade economy: The player trades with any trader by barter or currency; prices follow supply and demand and opinion; the owner sets currencies and stock in the Editor.
Traces to: INT-11, EDT-08.
</context>
<dependencies>
Prompts that must be Done: X-M9a.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation: a region's currencies (item ids with a base value) in the level or region data. Game (Editor): a Currencies section in the level settings with an item picker. A region with none trades by barter only.
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Set">
Given the test level's settings
When the owner marks shells as currency with value 1 and saves
Then trades in that level accept shells
</scenario>
<scenario name="None">
Given a level with no currency
When the player trades
Then only barter is offered
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-280`: the Debug build with zero warnings and every Debug test passes, including the "US-280 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-280.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-280/.
</verification>
<teach_back>C++ concept for the owner: Value types; why money is an integer.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-281 Trader stock
```xml
<prompt id="S-US-281" codex="2.9" milestone="M9b" story="US-281" priority="Must" size="M">
<context>
Story US-281: Trader stock.
As the player, I want traders to have limited goods that come back each day and to pay more for what they want, so that trade is a choice.
Epic E27 Trade economy: The player trades with any trader by barter or currency; prices follow supply and demand and opinion; the owner sets currencies and stock in the Editor.
Traces to: INT-11.
</context>
<dependencies>
Stories that must be Done: US-280.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation: per-trader stock, restock per day, wants; wants are bought at full value, other goods at half (D-52 Q-17); integers only; saved; far traders restock in their daily summary (US-263).
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Restock">
Given a trader with 0 flint and a restock of 2 per day
When a day passes
Then it has 2 flint
</scenario>
<scenario name="Wants">
Given a trader who wants berries
When the hero sells berries and then furs
Then berries count at full value and furs at half
</scenario>
<scenario name="Saved">
Given stock after a trade
When save and load
Then the stock is the same
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-281`: the Debug build with zero warnings and every Debug test passes, including the "US-281 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-281.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-281/.
</verification>
<teach_back>C++ concept for the owner: Integer arithmetic and rounding rules written down.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-282 Supply and demand, and reputation
```xml
<prompt id="S-US-282" codex="2.9" milestone="M9b" story="US-282" priority="Must" size="M">
<context>
Story US-282: Supply and demand, and reputation.
As the player, I want prices to rise when goods are scarce and fall when they are plentiful, and better deals from people who like me, so that the economy feels real.
Epic E27 Trade economy: The player trades with any trader by barter or currency; prices follow supply and demand and opinion; the owner sets currencies and stock in the Editor.
Traces to: INT-12, SDC-10.
</context>
<dependencies>
Stories that must be Done: US-281, US-264.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation: price = base value adjusted by the region's supply and demand of that item (formula and limits in an ADR or the design doc, data-tunable) and by the trader's opinion of the buyer; opinion thresholds gate access to some goods or to trade at all (D-52 Q-15). Deterministic; no floats in the simulation.
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Scarce">
Given flint scarce in the region
When the hero asks its price twice a day apart while it gets scarcer
Then the second price is higher
</scenario>
<scenario name="Reputation">
Given two traders with equal stock, one friendly and one suspicious
When the hero asks the same item's price
Then the friendly one is cheaper
</scenario>
<scenario name="Gate">
Given a trader whose rare goods need friendly
When the wary hero opens trade
Then the rare goods are not offered and the Actions pop-up says why
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-282`: the Debug build with zero warnings and every Debug test passes, including the "US-282 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-282.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-282/.
</verification>
<teach_back>C++ concept for the owner: Fixed-point formulas; clamping; tuning through data.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-283 Trade screen for any trader
```xml
<prompt id="S-US-283" codex="2.9" milestone="M9b" story="US-283" priority="Must" size="L">
<context>
Story US-283: Trade screen for any trader.
As the player, I want one trade screen for every trader, with barter and currency, so that I can trade with a passing NPC as with a rival camp.
Epic E27 Trade economy: The player trades with any trader by barter or currency; prices follow supply and demand and opinion; the owner sets currencies and stock in the Editor.
Traces to: INT-11.
</context>
<dependencies>
Stories that must be Done: US-282.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game: `RunFlow::openBarter` takes a trader reference (rival camp or NPC); `trade.json` targets the `trader` tag; the screen shows both sides' goods, their prices and the currency balance; rival-camp barter keeps its counter-offer and pay-later behaviour and its tests.
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Trade">
Given a friendly trader with 6 flint and the hero with 10 berries
When the player gives berries worth 2 flint
Then the hero has 2 more flint and the trader 2 fewer, the berries moved
</scenario>
<scenario name="Currency">
Given a region with shells as currency
When the player buys 1 fur for shells
Then the shells move at the shown price
</scenario>
<scenario name="Rivals unchanged">
Given a rival camp
When the player chooses Barter
Then it works as before this story
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-283`: the Debug build with zero warnings and every Debug test passes, including the "US-283 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-283.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-283/.
</verification>
<teach_back>C++ concept for the owner: std::variant for 'one of several kinds'.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-284 Editor trade panel
```xml
<prompt id="S-US-284" codex="2.9" milestone="M9b" story="US-284" priority="Must" size="M">
<context>
Story US-284: Editor trade panel.
As the owner, I want to set a trader's stock, restock, wants and price settings in the Editor, so that each trader is my own.
Epic E27 Trade economy: The player trades with any trader by barter or currency; prices follow supply and demand and opinion; the owner sets currencies and stock in the Editor.
Traces to: EDT-08, INT-11.
</context>
<dependencies>
Stories that must be Done: US-283.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game (Editor): a Trade section in the NPC panel (US-268) and in the kinds and classes forms: stock table, restock per day, wants list; undo per change. Set up the test level's trader and wary hunter and extend the walk-through checklist.
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Edit">
Given the test level's trader selected
When the owner adds 3 furs, a restock of 1 per day and the want berries, and saves
Then play shows the furs and restocks one a day
</scenario>
<scenario name="Walk-through">
Given the extended checklist
When the owner follows the trade steps
Then each step behaves as written
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-284`: the Debug build with zero warnings and every Debug test passes, including the "US-284 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-284.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-284/.
</verification>
<teach_back>C++ concept for the owner: Table widgets bound to maps.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
```xml
<prompt id="X-M9b" codex="2.9" name="Exit review M9b">
<instructions>
1. Demonstrate the exit criteria: The player trades with any trader by barter or the region's currency; prices follow supply and demand and the trader's opinion; stock is limited and restocks daily; the owner sets currencies and stock in the Editor.
   Also run `pwsh tools/verify.ps1 -Story X-M9b -Config Release` on the owner's PC: every Release test with the strict 3-second first-frame limit, which GitHub's GPU-less runners only check at 10 seconds (D-47); record the result in docs/gates/M9b.md.
2. Collect evidence (test output, CI run, screenshots) into docs/gates/M9b.md, one section per criterion, each marked met or not met. Also: a trade by barter and one by currency with the test level's trader; a price before and after a scarcity; a refused trade explained in the Actions pop-up.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m9b-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the exit result.</output_format>
</prompt>
```

### M9c NPC life
Exit criteria: NPCs follow day and night schedules, act from their classes, custom actions and events, and do everything to each other with the same interaction files; the owner edits it all in the Editor; the test level shows a living day and the 100,000-person soak passes.

Why this milestone exists: D-52 Q-10, Q-18, Q-19: schedules, class, quest, custom and event actions (quest actions are a hook that M10 fills), NPC-to-NPC interactions.

```xml
<prompt id="K-M9c" codex="2.9" name="Kick off M9c NPC life">
<instructions>
1. Confirm that M9b (docs/gates/M9b.md) and its stories are done, and that D-34, D-35 and D-52 are Decided in docs/decisions.md.
2. Ask the owner, in one chat round (2-4 options each, recommended first), only the questions the M9c stories leave open after D-52. Record the answers.
3. Architect: extend docs/plans/M9-npc-design.md for M9c before its first story.
4. Set this milestone's stories to To do in docs/status.md in this order: US-290, US-291, US-292, US-293, US-294.
5. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, the owner's answers.</output_format>
</prompt>
```

#### S-US-290 Day and night schedules
```xml
<prompt id="S-US-290" codex="2.9" milestone="M9c" story="US-290" priority="Must" size="L">
<context>
Story US-290: Day and night schedules.
As the owner, I want to give every NPC a daily schedule in the Editor, so that people work, eat, meet and sleep at their own hours.
Epic E28 NPC life: NPCs follow schedules, do their class, custom and event actions and do everything to each other with the same files, all edited in the Editor.
Traces to: SDC-12, SDC-07, EDT-08.
</context>
<dependencies>
Prompts that must be Done: X-M9b.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation: a schedule is a list of (from time, activity, place) entries per class, kind or NPC (same precedence as US-261); activities are interactions or 'go to'; needs and danger can interrupt and the schedule resumes; far persons follow it in their daily summary. Game (Editor): a schedule form in the NPC panel with times, activity and place pickers.
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Follow">
Given an NPC scheduled to work at the market from 06:00 and sleep at home from 21:00
When the day passes
Then it is at the market at 10:00 and at home at 22:00
</scenario>
<scenario name="Interrupt">
Given the same NPC very hungry at 10:00
When it eats
Then it returns to the market afterwards
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-290`: the Debug build with zero warnings and every Debug test passes, including the "US-290 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-290.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-290/.
</verification>
<teach_back>C++ concept for the owner: Time-based state machines; interruptions and resumption.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-291 Action sources
```xml
<prompt id="S-US-291" codex="2.9" milestone="M9c" story="US-291" priority="Must" size="M">
<context>
Story US-291: Action sources.
As the owner, I want NPCs to act from their classes, from my custom actions and from world events, all set in the Editor, so that I control what people do. (v2.12, D-54 Q11: no quest source in M9c; M10 adds the quest hook and changes the action schema.)
Epic E28 NPC life: NPCs follow schedules, do their class, custom and event actions and do everything to each other with the same files, all edited in the Editor.
Traces to: SDC-12, EDT-08.
</context>
<dependencies>
Stories that must be Done: US-290.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation: the NPC chooser (US-154) takes candidate actions from class actions, custom actions on the NPC, and event actions (an event in data offers actions to NPCs that match it, for example a fire nearby); there is no quest-action source in M9c (D-54 Q11, Codex v2.12): M10 adds the quest hook and changes the action schema. Game (Editor): sections for custom and event actions in the NPC and class forms.
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Class">
Given an NPC of class guard whose class action is patrol
When it is idle on duty
Then it patrols
</scenario>
<scenario name="Event">
Given an event action 'help put out fire' for class villager
When a fire starts within 12 m of a villager
Then the villager goes to help
</scenario>
<scenario name="Sources">
Given the NPC chooser
When it collects the candidate actions of an NPC
Then they come only from class, custom and event actions, and no quest source exists yet
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-291`: the Debug build with zero warnings and every Debug test passes, including the "US-291 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-291.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-291/.
</verification>
<teach_back>C++ concept for the owner: Strategy objects as sources; extension points.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-292 NPCs act on each other
```xml
<prompt id="S-US-292" codex="2.9" milestone="M9c" story="US-292" priority="Must" size="L">
<context>
Story US-292: NPCs act on each other.
As the player, I want NPCs to talk, fight, trade, give gifts and confront each other with the same rules as me, so that the world lives without me.
Epic E28 NPC life: NPCs follow schedules, do their class, custom and event actions and do everything to each other with the same files, all edited in the Editor.
Traces to: INT-10.
</context>
<dependencies>
Stories that must be Done: US-291.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation: every interaction with an `npc` block may have an NPC as actor and another NPC as target (D-52 Q-19): talk (bubbles), fight, trade (both stocks), gift, confront; opinions change as for the hero; deterministic; bounded per tick by the US-263 budget.
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Trade">
Given two traders near each other where one wants what the other has
When they are idle
Then they trade and both stocks change
</scenario>
<scenario name="Fight">
Given two NPCs who are hostile to each other
When they meet
Then they fight and the bystanders' opinions change
</scenario>
<scenario name="Budget">
Given the 100,000-person load test
When a day is simulated
Then it still meets ADR-022
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-292`: the Debug build with zero warnings and every Debug test passes, including the "US-292 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-292.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-292/.
</verification>
<teach_back>C++ concept for the owner: Symmetric actor and target; budgets per tick.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-293 Default interactions by partner type
```xml
<prompt id="S-US-293" codex="2.9" milestone="M9c" story="US-293" priority="Must" size="M">
<context>
Story US-293: Default interactions by partner type.
As the owner, I want each class and NPC to have default dialogues and actions for meeting the player, other classes, animals and the environment, with a list I can extend, so that every meeting has a sensible reaction.
Epic E28 NPC life: NPCs follow schedules, do their class, custom and event actions and do everything to each other with the same files, all edited in the Editor.
Traces to: INT-10.
</context>
<dependencies>
Stories that must be Done: US-292.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation and Editor: the partner types (Player, each NPC Class, Animals, Environment) come from data (assets/data/sim/partner-types.json) so the owner can add more; for each, a class or NPC lists default dialogues and actions; the chooser prefers them when that partner is the target. Defaults per partner type are files assets/data/interactions/defaults-<type>.json (class, animal, environment) that classes and NPCs override; the Environment partner offers gather and forage, shelter and rest, hunt and fish, pray and ritual at a place (D-54 Q14, Q16, v2.12).
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Animals">
Given a hunter class whose Animals default is hunt
When a deer is near an idle hunter
Then the hunter hunts it
</scenario>
<scenario name="Extend">
Given a new partner type 'buildings' added to the data file
When the Editor opens
Then the NPC panel offers defaults for buildings
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-293`: the Debug build with zero warnings and every Debug test passes, including the "US-293 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-293.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-293/.
</verification>
<teach_back>C++ concept for the owner: Data-driven enumerations.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
#### S-US-294 Living test level
```xml
<prompt id="S-US-294" codex="2.9" milestone="M9c" story="US-294" priority="Must" size="S">
<context>
Story US-294: Living test level.
As the owner, I want the test level to show a day of NPC life, and the 100,000-person soak to pass, so that I can judge M9c in one sitting.
Epic E28 NPC life: NPCs follow schedules, do their class, custom and event actions and do everything to each other with the same files, all edited in the Editor.
Traces to: SDC-12, INT-10, NFR-08.
</context>
<dependencies>
Stories that must be Done: US-293.
Owner decisions that must be Decided: D-34, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Data: npc-test.json gains schedules, class, custom and event actions and NPC-to-NPC trade and talk for the seven NPCs; the walk-through checklist covers one full day; a headless soak of 100,000 persons for 30 in-game days with the same seed giving the same save hash on two runs (D-54 Q15, v2.12).
Follow the formats in the M9a-M9c design notes and docs/plans/M9-npc-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Day">
Given the test level
When the owner watches one in-game day
Then every NPC follows its schedule and at least one NPC-to-NPC trade and one conversation happen
</scenario>
<scenario name="Soak">
Given 100,000 persons
When 30 in-game days run headless
Then the budget of ADR-022 holds and the determinism hash is stable
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in docs/guides/npc-data.md (new in US-260) with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-294`: the Debug build with zero warnings and every Debug test passes, including the "US-294 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-294.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-294/.
</verification>
<teach_back>C++ concept for the owner: Soak tests and what they catch.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3); a performance budget of ADR-022 that cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```
```xml
<prompt id="X-M9c" codex="2.9" name="Exit review M9c">
<instructions>
1. Demonstrate the exit criteria: NPCs follow day and night schedules, act from their classes, custom actions and events, and do everything to each other with the same interaction files; the owner edits it all in the Editor; the test level shows a living day and the 100,000-person soak passes.
   Also run `pwsh tools/verify.ps1 -Story X-M9c -Config Release` on the owner's PC: every Release test with the strict 3-second first-frame limit, which GitHub's GPU-less runners only check at 10 seconds (D-47); record the result in docs/gates/M9c.md.
2. Collect evidence (test output, CI run, screenshots) into docs/gates/M9c.md, one section per criterion, each marked met or not met. Also: one in-game day of the test level recorded; one NPC-to-NPC trade and one conversation; the 30-day soak result.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m9c-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the exit result.</output_format>
</prompt>
```

### M8d Buildings
Exit criteria: The hero builds from blueprints and piece by piece on a building grid, and rooms form; the owner composes prefabs from pieces in the Editor and places them in levels; prefabs marked buildable are offered as blueprints.

Why this milestone exists: the hero builds from blueprints and piece by piece, and the owner composes and places pre-made buildings (D-42, INT-07, EDT-07); split from M8e by D-43. The design notes under M8b apply.

```xml
<prompt id="K-M8d" codex="2.2" name="Kick off M8d Buildings">
<instructions>
1. Confirm that M8c (docs/gates/M8c.md) and, since v2.9, M9a, M9b and M9c (docs/gates/M9c.md) and their stories are done, and that D-42, D-43 and D-06 are Decided in docs/decisions.md.
2. Ask the owner, in one chat round (AskUserQuestion, 2-4 options each, recommended first), every design question the M8d stories leave open after the brief and D-42/D-43 (for example: the Build menu's layout, which blueprints are known at the start, how blueprints look before materials arrive). Record the answers in docs/decisions.md as "Decided (owner, <date>)".
3. Architect: write docs/plans/M8d-buildings-design.md (building data and grid, placement validity, construction as interactions, room detection, prefab format and editor, save migrations) before the first story.
4. Set this milestone's stories to To do in docs/status.md in this order: US-250, US-251, US-252, US-256.
5. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, the owner's answers.</output_format>
</prompt>
```

#### S-US-250 Building pieces and building kinds as data
```xml
<prompt id="S-US-250" codex="2.2" milestone="M8d" story="US-250" priority="Must" size="L">
<context>
Story US-250: Building pieces and building kinds as data.
As the owner, I want building pieces (walls, floors, roofs, doors, posts) and building kinds (layout, materials, build time, interior mode, uses) in data files, so that I can design buildings without code.
Epic E24 Buildings: The hero builds from whole blueprints and piece by piece; the owner composes pre-made buildings in the Editor and places them.
Traces to: INT-07, EDT-07, ARC-08.
</context>
<dependencies>
Stories that must be Done: US-150, US-155.
Owner decisions that must be Decided: D-42, D-43.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Data: assets/data/buildings/pieces.json and kinds.json (brief section 4), validated with file, line and field; buildings get their own list in levels, world files and saves (versions bumped with migrations), not the plant machinery (CI-008); Simulation: src/sim/buildings/ (kinds, pieces, placed buildings by id, condition, owner, contents); five starter kinds: hut, windbreak, storage pit, drying rack, palisade.
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Load">
Given pieces.json and buildings.json with a hut, a windbreak, a storage pit, a drying rack and a palisade
When the game starts
Then every kind and piece is known, with errors naming file, line and field
</scenario>
<scenario name="Own list">
Given buildings in a level
When it is saved and loaded
Then they are kept in their own list, not as plants (CI-008)
</scenario>
<scenario name="Uses">
Given a hut with shelter and sleep, a storage pit with storage
When the data loads
Then their interactions are offered
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-250/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-250`: the Debug build with zero warnings and every Debug test passes, including the "US-250 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-250.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-250/.
</verification>
<teach_back>C++ concept for the owner: Composite data: a whole made of parts.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-251 Building from blueprints
```xml
<prompt id="S-US-251" codex="2.2" milestone="M8d" story="US-251" priority="Must" size="L">
<context>
Story US-251: Building from blueprints.
As the player, I want to choose a building, place its blueprint, bring materials and build it over time, so that I can shape my camp.
Epic E24 Buildings: The hero builds from whole blueprints and piece by piece; the owner composes pre-made buildings in the Editor and places them.
Traces to: INT-07.
</context>
<dependencies>
Stories that must be Done: US-250, US-153.
Owner decisions that must be Decided: D-42, D-43.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game and Simulation: a Build menu (B, new intent Build) of known blueprints; a ghost on a 1 m building grid with rotation and red where blocked; construction as an interaction (M7) with material delivery and stages; cancel drops delivered materials.
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Place">
Given known blueprints
When the player picks a hut and places it
Then a see-through blueprint shows, red where it cannot stand
</scenario>
<scenario name="Build">
Given a placed blueprint
When materials are brought and the hero works on it
Then it rises in stages and is finished after its build time
</scenario>
<scenario name="Cancel">
Given a blueprint under way
When the player cancels it
Then the delivered materials are dropped on the site
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-251/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-251`: the Debug build with zero warnings and every Debug test passes, including the "US-251 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-251.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-251/.
</verification>
<teach_back>C++ concept for the owner: Placement validity on a grid.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-252 Building piece by piece
```xml
<prompt id="S-US-252" codex="2.2" milestone="M8d" story="US-252" priority="Must" size="L">
<context>
Story US-252: Building piece by piece.
As the player, I want to place walls, floors, roofs and doors one by one, so that I can build any shape.
Epic E24 Buildings: The hero builds from whole blueprints and piece by piece; the owner composes pre-made buildings in the Editor and places them.
Traces to: INT-07.
</context>
<dependencies>
Stories that must be Done: US-251.
Owner decisions that must be Decided: D-42, D-43.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Pieces placed one by one as small blueprints; room detection by flood fill over walls, doors and roof cover (enclosed + roofed + door = room, warm and sheltered); walking paths go through doors only.
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Pieces">
Given the build menu's pieces
When the player places walls in a ring, a door and a roof
Then each piece is built like a small blueprint
</scenario>
<scenario name="Room">
Given an enclosed, roofed space with a door
When it is finished
Then it counts as a room: warm, sheltered, with its own light
</scenario>
<scenario name="Paths">
Given a door
When people walk to the room
Then they go through the door, never through walls
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-252/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-252`: the Debug build with zero warnings and every Debug test passes, including the "US-252 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-252.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-252/.
</verification>
<teach_back>C++ concept for the owner: Flood fill to find enclosed rooms.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-256 Prefab editor and placing buildings
```xml
<prompt id="S-US-256" codex="2.2" milestone="M8d" story="US-256" priority="Must" size="L">
<context>
Story US-256: Prefab editor and placing buildings.
As the owner, I want to compose buildings from pieces in the Editor, save them as prefabs with costs and interior mode, and place them whole, so that levels start with real camps.
Epic E24 Buildings: The hero builds from whole blueprints and piece by piece; the owner composes pre-made buildings in the Editor and places them.
Traces to: EDT-07.
</context>
<dependencies>
Stories that must be Done: US-252, US-124.
Owner decisions that must be Decided: D-42, D-43.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Editor: a Prefab tab: compose pieces on the grid, set materials, build time, interior mode (and interior level name) and uses, save assets/data/buildings/prefabs/<id>.json; prefabs on a palette page placed whole in levels (finished); `buildable` prefabs appear in the Build menu. The interior mode works in the game from US-254 (M8e).
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Compose">
Given the Prefab tab
When the owner places pieces, sets materials, build time and interior mode, and saves
Then a prefab file is written and appears in the palette
</scenario>
<scenario name="Place">
Given a prefab
When the owner places it in a level
Then the finished building stands there in the game
</scenario>
<scenario name="Blueprint">
Given a prefab marked buildable
When a game starts
Then the player can build it from the build menu
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-256/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-256`: the Debug build with zero warnings and every Debug test passes, including the "US-256 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-256.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-256/.
</verification>
<teach_back>C++ concept for the owner: Saving a group of parts as one reusable asset.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M8d" codex="2.2" name="Exit review M8d">
<instructions>
1. Demonstrate the exit criteria: The hero builds from blueprints and piece by piece on a building grid, and rooms form; the owner composes prefabs from pieces in the Editor and places them in levels; prefabs marked buildable are offered as blueprints.
   Also run `pwsh tools/verify.ps1 -Story X-M8d -Config Release` on the owner's PC: every Release test with the strict 3-second first-frame limit, which GitHub's GPU-less runners only check at 10 seconds (D-47); record the result in docs/gates/M8d.md.
2. Collect evidence (test output, CI run, screenshots, frame-time tables) into docs/gates/M8d.md, one section per criterion, each marked met or not met. Also: a camp built in the game from blueprints and pieces, and a prefab composed in the Editor and placed, with screenshots.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m8d-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the exit result.</output_format>
</prompt>
```

### M8e Building life
Exit criteria: Clan members help build and rival clans build; each building fades its roof or opens an interior map as set in the Editor; buildings wear, are repaired, take damage, burn and fall; they shelter, warm and store for the clan and are saved.

Why this milestone exists: buildings become part of clan life: clans build together, interiors open, buildings wear, burn and fall, shelter and store (D-42, D-43, INT-07, INT-08, ENV-20). The design notes under M8b apply.

```xml
<prompt id="K-M8e" codex="2.2" name="Kick off M8e Building life">
<instructions>
1. Confirm that M8d (docs/gates/M8d.md) and its stories are done, and that D-42, D-43 and D-06 are Decided in docs/decisions.md.
2. Ask the owner, in one chat round (AskUserQuestion, 2-4 options each, recommended first), every design question the M8e stories leave open after the brief and D-42/D-43 (for example: how fast buildings wear, whether rivals may burn buildings, which buildings open an interior map by default). Record the answers in docs/decisions.md as "Decided (owner, <date>)".
3. Architect: write docs/plans/M8e-building-life-design.md (construction jobs for NPCs and rivals, roof fade and interior maps, wear, damage and fire spread, rooms in the simulation's needs and stores) before the first story.
4. Set this milestone's stories to To do in docs/status.md in this order: US-253, US-254, US-255, US-257.
5. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, the owner's answers.</output_format>
</prompt>
```

#### S-US-253 Clans build together
```xml
<prompt id="S-US-253" codex="2.2" milestone="M8e" story="US-253" priority="Must" size="M">
<context>
Story US-253: Clans build together.
As the player, I want my clan members to help build and rival clans to grow their own camps, so that building is a shared life.
Epic E25 Building life: Clans build together; interiors by roof fade or interior map; wear, repair, damage and fire; shelter, warmth and storage for the clan.
Traces to: INT-07, INT-05.
</context>
<dependencies>
Stories that must be Done: US-251, US-154.
Owner decisions that must be Decided: D-42, D-43.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation: construction jobs (blueprint, materials missing, work left) that clan members score with the M7 utility AI (bring material, build); rival clans pick blueprints by their needs and build at their camp over seasons, seeded and in the world hash.
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Help">
Given a blueprint and idle clan members
When time passes
Then they bring materials and build, through the interaction system
</scenario>
<scenario name="Rivals">
Given a rival camp
When a season passes
Then it has built at least one building from its own blueprints
</scenario>
<scenario name="Deterministic">
Given the same seed and inputs
When two runs
Then the same buildings stand in the same places
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-253/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-253`: the Debug build with zero warnings and every Debug test passes, including the "US-253 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-253.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-253/.
</verification>
<teach_back>C++ concept for the owner: Job queues for many workers.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-254 Interiors: roof fade or interior map
```xml
<prompt id="S-US-254" codex="2.2" milestone="M8e" story="US-254" priority="Must" size="L">
<context>
Story US-254: Interiors: roof fade or interior map.
As the player, I want to see inside buildings, either by the roof fading or by entering an interior, as the building says, so that houses are places to be.
Epic E25 Building life: Clans build together; interiors by roof fade or interior map; wear, repair, damage and fire; shelter, warmth and storage for the clan.
Traces to: ENV-20, EDT-07.
</context>
<dependencies>
Stories that must be Done: US-252, US-122.
Owner decisions that must be Decided: D-42, D-43.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game: roof fade (roof and front-wall pieces fade when the hero is inside, by the room map from US-252); interior map mode: the door loads the named interior level (made in the Editor) and its exit door returns; per placed building override set in the Editor.
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Fade">
Given a hut set to roof fade
When the hero walks in
Then the roof and the front walls fade and the inside shows, lit by its fire and door
</scenario>
<scenario name="Interior">
Given a lodge set to interior map
When the hero uses its door
Then the interior level loads and its door leads back outside
</scenario>
<scenario name="Setting">
Given a placed building
When the owner switches its interior mode in the Editor
Then the game follows it
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-254/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-254`: the Debug build with zero warnings and every Debug test passes, including the "US-254 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-254.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-254/.
</verification>
<teach_back>C++ concept for the owner: Linking two maps by doors.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-255 Wear, repair, damage and fire
```xml
<prompt id="S-US-255" codex="2.2" milestone="M8e" story="US-255" priority="Must" size="L">
<context>
Story US-255: Wear, repair, damage and fire.
As the player, I want buildings to wear with weather, to be repaired, damaged and to burn, so that they need care.
Epic E25 Building life: Clans build together; interiors by roof fade or interior map; wear, repair, damage and fire; shelter, warmth and storage for the clan.
Traces to: INT-08, PHY-04.
</context>
<dependencies>
Stories that must be Done: US-251, US-029.
Owner decisions that must be Decided: D-42, D-43.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation: condition per piece falling by season and weather (data), Repair interaction; damage from attacks and falling to rubble at zero; fire on wooden pieces spreading to neighbours by Luna material flammability and burning down over time (presentation via M2d fire effects and M8c fire light); general heat and fire simulation stays out of scope.
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Wear">
Given a hut through a winter
When the season passes
Then its condition falls as the data says and Repair is offered
</scenario>
<scenario name="Fire">
Given a burning torch dropped by a wooden wall
When time passes
Then the wall catches fire, it spreads to wooden neighbours by their materials and ends in rubble
</scenario>
<scenario name="Damage">
Given an attack on a wall
When it is hit
Then the wall loses condition and falls at zero
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-255/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-255`: the Debug build with zero warnings and every Debug test passes, including the "US-255 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-255.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-255/.
</verification>
<teach_back>C++ concept for the owner: Spreading state across neighbours (cellular rules).</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-257 Buildings in the clan's life
```xml
<prompt id="S-US-257" codex="2.2" milestone="M8e" story="US-257" priority="Must" size="M">
<context>
Story US-257: Buildings in the clan's life.
As the player, I want buildings to matter to the clan: shelter and warmth, sleeping, storage and ownership, saved with the world, so that building changes how people live.
Epic E25 Building life: Clans build together; interiors by roof fade or interior map; wear, repair, damage and fire; shelter, warmth and storage for the clan.
Traces to: INT-07, SDC-02.
</context>
<dependencies>
Stories that must be Done: US-253, US-011.
Owner decisions that must be Decided: D-42, D-43.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Simulation: rooms reduce Warmth loss for sleepers, storage buildings hold clan food (counts in the store, slower spoil), owners per building; everything saved and in the determinism hash.
Follow the brief docs/plans/M8b-M8d-render-light-build-brief.md and the milestone design document; a design question they do not answer goes to the owner (Charter human gate 3).
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Warmth">
Given a cold night
When clan members sleep in a hut
Then their Warmth need falls slower than outside
</scenario>
<scenario name="Storage">
Given a storage pit
When food is put in
Then it counts in the clan store and spoils slower
</scenario>
<scenario name="Saved">
Given buildings with owners, condition and contents
When the game saves and loads
Then everything is kept
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; round-trip tests for every new format; GPU screenshots for any visual change saved in docs/evidence/US-257/.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-257`: the Debug build with zero warnings and every Debug test passes, including the "US-257 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-257.md done on the owner's PC with the GPU renderer, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-257/.
</verification>
<teach_back>C++ concept for the owner: Linking world objects to simulation needs.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M8e" codex="2.2" name="Exit review M8e">
<instructions>
1. Demonstrate the exit criteria: Clan members help build and rival clans build; each building fades its roof or opens an interior map as set in the Editor; buildings wear, are repaired, take damage, burn and fall; they shelter, warm and store for the clan and are saved.
   Also run `pwsh tools/verify.ps1 -Story X-M8e -Config Release` on the owner's PC: every Release test with the strict 3-second first-frame limit, which GitHub's GPU-less runners only check at 10 seconds (D-47); record the result in docs/gates/M8e.md.
2. Collect evidence (test output, CI run, screenshots, frame-time tables) into docs/gates/M8e.md, one section per criterion, each marked met or not met. Also: a season of play where clan members and a rival clan build, a hut is repaired, a wall burns, and the hero enters a roof-fade hut and an interior-map lodge.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m8e-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the exit result.</output_format>
</prompt>
```

### M9 Interaction and dialogue editor
Exit criteria: The owner opens any dialogue or interaction in the Editor as a graph, edits it, test-plays it and saves it, and the file still reads well in a text editor.

Why this milestone exists: the owner chose a full visual graph editor (D-34) for dialogue and interactions. Risk (brief section 8): the graph widget (US-170) is the largest UI work so far; if it passes 4 weeks of effort, Mraw writes a decision request offering a form-based editor instead, and the owner decides. The design notes under M7 apply to every M9 prompt.

```xml
<prompt id="K-M9" codex="2.0" name="Kick off M9 Interaction and dialogue editor">
<instructions>
1. Confirm that M8e (docs/gates/M8e.md) and its stories are done (since v2.2 M8b-M8e come between M8 and M9), and that D-34 and D-35 are Decided in docs/decisions.md.
2. Ask the owner, in one chat round, the open design questions of the M9 stories (for example: node look, where the Dialogue and Interactions tabs sit in the tool bar, which values Test-play can set). Record the answers.
3. Architect: write docs/plans/M9-graph-editor-design.md before US-170: the graph model and commands, text-graph mapping for .dlg, the interaction graph's layout, overrides in levels, Test-play isolation, validation, and the test plan.
4. Set this milestone's stories to To do in docs/status.md in this order: US-170, US-171, US-172, US-175, US-173, US-174.
5. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, the owner's answers.</output_format>
</prompt>
```

#### S-US-170 Node-graph widget
```xml
<prompt id="S-US-170" codex="2.0" milestone="M9" story="US-170" priority="Must" size="L">
<context>
Story US-170: Node-graph widget.
As the owner, I want a canvas with nodes and wires that I can pan, zoom, select, drag and undo, so that rules and conversations can be edited visually.
Epic E16 Interaction and dialogue editor: The owner edits every conversation and interaction as a graph in the Editor, test-plays it and saves readable files.
Traces to: INT-06, SDC-06, ARC-09.
</context>
<dependencies>
Stories that must be Done: US-121, US-126.
Owner decisions that must be Decided: D-34.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Luna Engine (game-agnostic): src/luna/engine/ui/node_graph.{h,cpp}: graph model (nodes with ports, wires by node and port id), canvas with pan (right drag), zoom (wheel, 50-200%), selection box, drag, connect and delete; every edit is a Command in the existing History (US-126). Tested headless with the RecordingRenderer.
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Edit">
Given an empty graph
When the owner adds two nodes and drags a wire between their ports
Then the wire connects them
</scenario>
<scenario name="Navigate">
Given a graph larger than the screen
When the owner drags with the right button and turns the wheel
Then the view pans and zooms around the cursor
</scenario>
<scenario name="Undo">
Given ten graph edits
When the owner presses Ctrl+Z ten times
Then the graph is as it was
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-170`: the Debug build with zero warnings and every Debug test passes, including the "US-170 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-170.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-170/.
</verification>
<teach_back>C++ concept for the owner: Graph data structures; hit testing; the Command pattern.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-171 Dialogue graph editor
```xml
<prompt id="S-US-171" codex="2.0" milestone="M9" story="US-171" priority="Must" size="L">
<context>
Story US-171: Dialogue graph editor.
As the owner, I want to open a conversation as a graph, edit lines, choices, conditions and effects, and save it, so that I can write dialogue in the game.
Epic E16 Interaction and dialogue editor: The owner edits every conversation and interaction as a graph in the Editor, test-plays it and saves readable files.
Traces to: SDC-06, INT-03.
</context>
<dependencies>
Stories that must be Done: US-170, US-160.
Owner decisions that must be Decided: D-34.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game: a Dialogue tab in the Editor (src/game/editor_dialogue.*): .dlg to graph and back through the US-160 parser and writer; node boxes show speaker lines; choices are wires with their text, condition and effects edited in a side panel; positions saved in <name>.dlg.layout.json beside the script; Ctrl+S saves.
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Open">
Given elder-fire.dlg
When the owner opens it in the Editor
Then each node is a box with its lines and each choice is a wire to its target
</scenario>
<scenario name="Save">
Given a node added and a choice rewired
When the owner presses Ctrl+S
Then the .dlg text shows the change in canonical form and the positions are in elder-fire.dlg.layout.json
</scenario>
<scenario name="Hand edits">
Given a file edited in Notepad with # notes
When it is opened, changed in the graph and saved
Then the notes are still there
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-171`: the Debug build with zero warnings and every Debug test passes, including the "US-171 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-171.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-171/.
</verification>
<teach_back>C++ concept for the owner: Mapping between a text format and a graph.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-172 Interaction graph editor
```xml
<prompt id="S-US-172" codex="2.0" milestone="M9" story="US-172" priority="Must" size="L">
<context>
Story US-172: Interaction graph editor.
As the owner, I want to see and edit interactions as actor, verb and target nodes with condition and effect blocks, so that I can design how things act on each other.
Epic E16 Interaction and dialogue editor: The owner edits every conversation and interaction as a graph in the Editor, test-plays it and saves readable files.
Traces to: INT-06.
</context>
<dependencies>
Stories that must be Done: US-170, US-150.
Owner decisions that must be Decided: D-34.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game: an Interactions tab in the Editor: actor, verb and target-tag nodes with each interaction a path between them; selecting an interaction shows its fields as condition and effect blocks (add, remove, reorder) with pickers filled from the catalogs; saves the interaction's JSON (fields in the guide's order; `note` fields kept).
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="View">
Given the interactions folder
When the owner opens the interaction graph
Then actors, verbs and target tags are nodes and each interaction is a path between them
</scenario>
<scenario name="Edit">
Given Gather selected
When the owner changes its duration to 2 s and adds the effect 'fx leaves'
Then gather.json is saved with the change
</scenario>
<scenario name="Pickers">
Given a condition or effect field
When the owner types
Then items, tags, needs and nodes are offered from the catalogs
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-172`: the Debug build with zero warnings and every Debug test passes, including the "US-172 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-172.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-172/.
</verification>
<teach_back>C++ concept for the owner: Forms built from a schema; pickers over catalogs.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-175 Graph validation
```xml
<prompt id="S-US-175" codex="2.0" milestone="M9" story="US-175" priority="Must" size="S">
<context>
Story US-175: Graph validation.
As the owner, I want unreachable nodes, dead ends and unknown names listed and clickable, so that broken conversations never reach players.
Epic E16 Interaction and dialogue editor: The owner edits every conversation and interaction as a graph in the Editor, test-plays it and saves readable files.
Traces to: INT-03, SDC-06.
</context>
<dependencies>
Stories that must be Done: US-171, US-172.
Owner decisions that must be Decided: D-34.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Validation in src/sim/ (headless): breadth-first search from `start` for unreachable nodes, nodes with no choice and no END, unknown nodes, items, tags, needs and effect verbs; the Editor lists the findings and clicking one selects the node or block. The same check runs in CI over every shipped file.
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Unreachable">
Given a node no choice leads to
When validation runs
Then it is listed and clicking it selects the node
</scenario>
<scenario name="Dead end">
Given a node with no choice and no END
When validation runs
Then it is listed as a dead end
</scenario>
<scenario name="Unknown">
Given an effect giving an item no catalog has
When validation runs
Then the item name and the node are listed
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-175`: the Debug build with zero warnings and every Debug test passes, including the "US-175 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-175.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-175/.
</verification>
<teach_back>C++ concept for the owner: Graph traversal (breadth-first search).</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-173 Attach to placed things
```xml
<prompt id="S-US-173" codex="2.0" milestone="M9" story="US-173" priority="Must" size="M">
<context>
Story US-173: Attach to placed things.
As a level designer, I want to give a placed character a conversation and a placed object its own interactions, so that each level can have its own people and places.
Epic E16 Interaction and dialogue editor: The owner edits every conversation and interaction as a graph in the Editor, test-plays it and saves readable files.
Traces to: SDC-06, INT-06.
</context>
<dependencies>
Stories that must be Done: US-171, US-172.
Owner decisions that must be Decided: D-34.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Level format: placed characters gain `dialogue`, placed things gain `overrides` (interaction id -> changed fields). The Editor's properties panel gets a dialogue picker and an overrides list; the registry applies overrides per thing id.
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Attach">
Given a placed character selected
When the owner picks 'elder-fire' as its dialogue
Then talking to that character in Game mode starts elder-fire
</scenario>
<scenario name="Override">
Given a placed bush selected
When the owner sets its regrow time to 60 s
Then only that bush regrows in 60 s
</scenario>
<scenario name="Saved">
Given attached dialogues and overrides
When the level is saved and loaded
Then they are still attached
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-173`: the Debug build with zero warnings and every Debug test passes, including the "US-173 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-173.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-173/.
</verification>
<teach_back>C++ concept for the owner: Per-instance overrides of shared data.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-174 Test-play from the Editor
```xml
<prompt id="S-US-174" codex="2.0" milestone="M9" story="US-174" priority="Should" size="M">
<context>
Story US-174: Test-play from the Editor.
As the owner, I want to play a conversation or an interaction from the Editor with conditions I choose, so that I can check every branch without playing the game for hours.
Epic E16 Interaction and dialogue editor: The owner edits every conversation and interaction as a graph in the Editor, test-plays it and saves readable files.
Traces to: SDC-06, INT-06.
</context>
<dependencies>
Stories that must be Done: US-171, US-172.
Owner decisions that must be Decided: D-34.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Editor: a Test-play panel that runs the dialogue runtime or the action runner on a copy of the world with chosen values (opinion, needs, items, flags, time, season); Play from here starts at the selected node; nothing is written to the level or saves.
Follow the formats in the milestone's design notes and the brief docs/plans/M7-M9-interactions-brief.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Branch">
Given a choice that needs opinion >= 20
When the owner sets the opinion to 25 and test-plays
Then the choice is offered
</scenario>
<scenario name="Start anywhere">
Given a node selected
When the owner chooses Play from here
Then the conversation starts at that node
</scenario>
<scenario name="No side effects">
Given a test-play that gives items
When it ends
Then the level and the save are unchanged
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: every new data field is in the guide with an example; every shipped JSON and .dlg file passes the validator; load-save-load gives the same data for every shipped file this story touches.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-174`: the Debug build with zero warnings and every Debug test passes, including the "US-174 ..." cases and the determinism hash test.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-174.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-174/.
</verification>
<teach_back>C++ concept for the owner: Faking inputs for tests (test doubles).</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M9" codex="2.0" name="Exit review M9">
<instructions>
1. Demonstrate the exit criteria: The owner opens any dialogue or interaction in the Editor as a graph, edits it, test-plays it and saves it, and the file still reads well in a text editor.
   Also run `pwsh tools/verify.ps1 -Story X-M9 -Config Release` on the owner's PC: every Release test with the strict 3-second first-frame limit, which GitHub's GPU-less runners only check at 10 seconds (D-47); record the result in docs/gates/M9.md.
2. Collect evidence (test output, CI run, screenshots, data files) into docs/gates/M9.md, one section per criterion, each marked met or not met. Also: one conversation and one interaction are built from scratch in the Editor, test-played, saved, and shown as text beside the graph in the evidence. Then X-M6 (kill gate 2) is next: end the session after this report, because it needs people.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m9-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the exit result.</output_format>
</prompt>
```

### M10 Quests and story authoring
Exit criteria: The owner writes a quest in the Editor's graph (or offline in JSON), test-plays it with the debugger and saves it; a player gets it from an NPC, follows it in the journal, tracker and markers, and finishes it; the elder tutorial is a quest and crossroads events are edited as story events.

Why this milestone exists: the owner wants the Editor to be the game's authoring tool for story, entities, world, mechanics and their configuration, and to create quests (D-40). M10 brings authored quests, story events and the play-in-editor debugger; M11-M12 the data and world editors; M13-M14 the Politics and Technology pillars built on them.

Design notes for every M10-M14 prompt (Anima, from the brief docs/plans/M10-M13-authoring-brief.md):
- Decisions (D-41): Dominus decides design questions in these milestones with the recommended option; each goes into docs/decisions.md as "Decided by Dominus (delegated)" with docs/decision-requests/<ID>.md (options, choice, reasoning) and into the next Milestone file. The owner may override any of them later. Scope changes (new stories, new systems) are not design questions: they become codex issues.
- Tests (D-41): every story writes its tests as before, and they must compile; stories are Done after a zero-warning Debug build and the merge into qa (CI on qa builds Release, D-46). Each X-Mx runs `pwsh tools/verify.ps1` and CI, fixes every failure, and only then merges into main.
- Formats: the brief's section 4 (quests, story events, schemas, Game Rules, routines, world file, politics) and section 10 (technologies, research) are the contract. One rule language for everything (ADR-019); one schema validator for loading, CI and the Editor (ADR-020); errors name file, line and field; bad data never crashes the game.
- Layers (Charter rules 1, 3, 9): runtimes, validators, routines, region overrides, politics and research in src/sim/ (headless, deterministic, saved); tabs, forms, graphs, the region view and the debugger in src/game/; any new widget (form fields, minimap, timeline) in Luna Engine, game-agnostic.
- Determinism and saves (Charter rules 6, 8): seeded streams for every new random choice; ids, never pointers (this also enables catalog reload, CI-007); every save, level and world format change bumps its version with a migration.
- Editor tools never write data or saves unless the owner saves; Play here and the debugger work on copies.
- Guides: docs/guides/quests.md, schemas.md, world-editing.md, politics.md, technology.md, each with an example per field, written in the story that adds the format.

```xml
<prompt id="K-M10" codex="2.1" name="Kick off M10 Quests and story authoring">
<instructions>
1. Confirm that M9 (docs/gates/M9.md) and its stories are done (for M10: X-M9 also ran its full verification and CI), and that D-40 and D-41 are Decided in docs/decisions.md.
2. Architect: write docs/plans/M10-quests-design.md (quest runtime and save format, event bus, giver resolution, journal and markers, quest graph mapping, tutorial migration, debugger) before the first story. Every design question it meets, Dominus decides (D-41) and records as delegated.
3. Set this milestone's stories to To do in docs/status.md in this order: US-180, US-181, US-182, US-183, US-186, US-184, US-187, US-185.
4. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, delegated decisions.</output_format>
</prompt>
```

#### S-US-180 Quest data and runtime
```xml
<prompt id="S-US-180" codex="2.1" milestone="M10" story="US-180" priority="Must" size="L">
<context>
Story US-180: Quest data and runtime.
As the owner, I want quests as JSON files with steps, objectives, conditions, rewards and prerequisites, so that I can write quests offline and the game runs them.
Epic E17 Quests: The owner writes quests in the Editor and offline; players get, follow and finish them with a journal, a tracker and markers.
Traces to: STO-04, ADR-019.
</context>
<dependencies>
Stories that must be Done: US-150, US-164.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Simulation: src/sim/quests/ (quest loader with comments allowed, the quest state machine per quest: locked, available, active, done, failed; steps, branches, hints, fail rules, rewards through the effect runner; quest state in saves with a save version bump and migration). Data: assets/data/quests/ in the brief's section 4.1 format. Guide: docs/guides/quests.md.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Load">
Given a quest file with two steps, a branch and a reward
When the game starts
Then the quest is available once its prerequisites hold
</scenario>
<scenario name="Progress">
Given an active quest
When its step's objective is met
Then the next step starts and the reward is given at the end
</scenario>
<scenario name="Error">
Given a quest whose step points to a missing step on line 14
When the data loads
Then the error reads 'quests/<file>.json:14: unknown step' and the rest loads
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-180/. The "US-180 ..." tests exist and compile; they run at X-M10.
Manual checks in docs/plans/US-180.md listed, to be run at X-M10.
</verification>
<teach_back>C++ concept for the owner: State machines over data; saving progress.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-181 Objectives from world events
```xml
<prompt id="S-US-181" codex="2.1" milestone="M10" story="US-181" priority="Must" size="M">
<context>
Story US-181: Objectives from world events.
As the player, I want my actions in the world to count toward quests, so that quests follow what I really do.
Epic E17 Quests: The owner writes quests in the Editor and offline; players get, follow and finish them with a journal, a tracker and markers.
Traces to: STO-04, INT-01.
</context>
<dependencies>
Stories that must be Done: US-180.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Simulation: a world event bus (interaction finished, dialogue node reached, item gained, given, crafted, enemy defeated, place entered, flag set, time passed) published by the existing systems with the actor id; the quest runtime subscribes and counts only the hero's events.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Count">
Given an objective 'gather 5 berries'
When the hero gathers 5 berries
Then the objective is complete
</scenario>
<scenario name="Kinds">
Given objectives talk, go to, gather, give, craft, interact, defeat, wait and flag
When each happens
Then each is detected
</scenario>
<scenario name="Not mine">
Given an NPC gathering berries
When it happens
Then the hero's quest does not count it
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-181/. The "US-181 ..." tests exist and compile; they run at X-M10.
Manual checks in docs/plans/US-181.md listed, to be run at X-M10.
</verification>
<teach_back>C++ concept for the owner: An event bus (observer pattern).</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-182 Getting and handing in quests
```xml
<prompt id="S-US-182" codex="2.1" milestone="M10" story="US-182" priority="Must" size="M">
<context>
Story US-182: Getting and handing in quests.
As the player, I want NPCs to offer quests in conversation and take them back when done, so that quests belong to people.
Epic E17 Quests: The owner writes quests in the Editor and offline; players get, follow and finish them with a journal, a tracker and markers.
Traces to: STO-04, SDC-03.
</context>
<dependencies>
Stories that must be Done: US-180, US-161.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Dialogue (src/sim/dialogue/) gains the effects `quest start|complete|fail <id>` and the conditions `quest(<id>)`, `step(<id>)`; giver resolution (person id, role:<role>); the Game draws a quest sign over NPCs with an available quest or a finished one to hand in.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Offer">
Given an NPC with an available quest
When the hero is near
Then a quest sign shows over the NPC and Talk offers the quest
</scenario>
<scenario name="Accept">
Given the offer in a .dlg choice with {quest start <id>}
When the player picks it
Then the quest is active
</scenario>
<scenario name="Hand in">
Given a finished quest and its giver
When the player talks to the giver
Then the turn-in line plays and the reward is given
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-182/. The "US-182 ..." tests exist and compile; they run at X-M10.
Manual checks in docs/plans/US-182.md listed, to be run at X-M10.
</verification>
<teach_back>C++ concept for the owner: Linking two data systems through ids.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-183 Journal, tracker and markers
```xml
<prompt id="S-US-183" codex="2.1" milestone="M10" story="US-183" priority="Must" size="M">
<context>
Story US-183: Journal, tracker and markers.
As the player, I want a journal, an on-screen tracker and markers over targets, so that I always know what to do next.
Epic E17 Quests: The owner writes quests in the Editor and offline; players get, follow and finish them with a journal, a tracker and markers.
Traces to: STO-04.
</context>
<dependencies>
Stories that must be Done: US-180.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Game: Journal screen (Luna UI, J key via a new intent), tracker in a screen corner, markers over target things, people and places; a Markers switch in settings.json (default on).
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Journal">
Given two active, one done and one failed quest
When the player opens the journal
Then they are listed in their groups with each step's text and log
</scenario>
<scenario name="Tracker">
Given an active quest
When its step changes
Then the tracker shows the new step at once
</scenario>
<scenario name="Markers">
Given markers on in settings
When a step targets a person or place
Then a marker shows over it; with markers off, none shows
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-183/. The "US-183 ..." tests exist and compile; they run at X-M10.
Manual checks in docs/plans/US-183.md listed, to be run at X-M10.
</verification>
<teach_back>C++ concept for the owner: Immediate-mode UI lists.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-186 Play-in-editor debugger
```xml
<prompt id="S-US-186" codex="2.1" milestone="M10" story="US-186" priority="Must" size="M">
<context>
Story US-186: Play-in-editor debugger.
As the owner, I want to play from any spot in the Editor with a debug panel, so that I can test quests and content in minutes.
Epic E17 Quests: The owner writes quests in the Editor and offline; players get, follow and finish them with a journal, a tracker and markers.
Traces to: EDT-06.
</context>
<dependencies>
Stories that must be Done: US-180.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Game: Play here (Editor) starts Game mode at the cursor on a copy of the level or region; F10 debug panel (Editor builds only): quest controls, flags, items, opinions, time and season jump, teleport, and a 'why not' view that evaluates the selected step's conditions and shows each value.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Play here">
Given the Editor open
When the owner chooses Play here
Then the hero starts at the cursor and Esc returns to the Editor unchanged
</scenario>
<scenario name="Control">
Given the debug panel
When the owner starts, completes or resets a quest, sets a flag, an item or an opinion, jumps time or teleports
Then the game reflects it at once
</scenario>
<scenario name="Why not">
Given a quest step that does not complete
When the owner selects it in the panel
Then the failing condition and its current values are shown
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-186/. The "US-186 ..." tests exist and compile; they run at X-M10.
Manual checks in docs/plans/US-186.md listed, to be run at X-M10.
</verification>
<teach_back>C++ concept for the owner: Debug-only code paths.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-184 Quest graph editor
```xml
<prompt id="S-US-184" codex="2.1" milestone="M10" story="US-184" priority="Must" size="L">
<context>
Story US-184: Quest graph editor.
As the owner, I want to build quests as graphs in the Editor, so that I can see and change their steps, branches and prerequisites.
Epic E17 Quests: The owner writes quests in the Editor and offline; players get, follow and finish them with a journal, a tracker and markers.
Traces to: STO-04, EDT-01.
</context>
<dependencies>
Stories that must be Done: US-180, US-170.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Game: a Quests tab in the Editor using the M9 node graph (steps = nodes, next and branches = wires), step blocks for objective, conditions, hint, effects with pickers from catalogs, people and places; a quest list view drawing prerequisite links; saves <id>.json and <id>.quest.layout.json.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Steps">
Given a quest open in the Editor
When the owner adds a step and wires a branch
Then the graph and the saved JSON show them
</scenario>
<scenario name="Blocks">
Given a step selected
When the owner adds an objective, a condition and a reward
Then pickers offer items, people, places, tags and quests
</scenario>
<scenario name="Prerequisites">
Given the quest list view
When the owner links quest B after quest A
Then B's prerequisites name A and the view draws the link
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-184/. The "US-184 ..." tests exist and compile; they run at X-M10.
Manual checks in docs/plans/US-184.md listed, to be run at X-M10.
</verification>
<teach_back>C++ concept for the owner: Reusing a generic widget for a new data type.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-187 Quest validation
```xml
<prompt id="S-US-187" codex="2.1" milestone="M10" story="US-187" priority="Must" size="S">
<context>
Story US-187: Quest validation.
As the owner, I want broken quests found before players see them, so that no quest can get stuck.
Epic E17 Quests: The owner writes quests in the Editor and offline; players get, follow and finish them with a journal, a tracker and markers.
Traces to: STO-04, EDT-01.
</context>
<dependencies>
Stories that must be Done: US-184.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Simulation (headless) validator over quests: unreachable steps, unknown items, people, places, interactions and quests, prerequisite cycles (depth-first search); listed in the Quests tab, clickable; a CI test runs it on every shipped quest.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Unreachable">
Given a step no branch leads to
When validation runs
Then it is listed and clicking it selects it
</scenario>
<scenario name="Impossible">
Given an objective naming an unknown item, person or place
When validation runs
Then it is listed
</scenario>
<scenario name="Cycle">
Given quests that require each other
When validation runs
Then the prerequisite cycle is listed
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-187/. The "US-187 ..." tests exist and compile; they run at X-M10.
Manual checks in docs/plans/US-187.md listed, to be run at X-M10.
</verification>
<teach_back>C++ concept for the owner: Cycle detection in a graph.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-185 The tutorial as a quest; story events
```xml
<prompt id="S-US-185" codex="2.1" milestone="M10" story="US-185" priority="Must" size="M">
<context>
Story US-185: The tutorial as a quest; story events.
As the owner, I want the elder tutorial rebuilt as the first quest and the crossroads events edited as story events, so that all story content is authored in one place.
Epic E17 Quests: The owner writes quests in the Editor and offline; players get, follow and finish them with a journal, a tracker and markers.
Traces to: STO-05, EDT-01.
</context>
<dependencies>
Stories that must be Done: US-184, US-090.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Rebuild assets/data/hero/tutorial.json as assets/data/quests/first-day.json and remove the tutorial code path once the quest reproduces it (the US-090 tests are the baseline); crossroads.json becomes assets/data/story/events/*.json with a `trigger`, loaded by the existing crossroads code; a Story events list in the Editor edits them as forms.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Tutorial">
Given a new game with the tutorial on
When the player starts
Then the elder quest guides gather, eat and tend the fire as before, with the hint after 2 minutes
</scenario>
<scenario name="Story events">
Given a crossroads event
When the owner opens it in the Editor
Then its trigger, text and options with effects are edited and saved
</scenario>
<scenario name="Same game">
Given the M5 and M6 tests
When they run
Then they pass
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-185/. The "US-185 ..." tests exist and compile; they run at X-M10.
Manual checks in docs/plans/US-185.md listed, to be run at X-M10.
</verification>
<teach_back>C++ concept for the owner: Migrating data without changing behaviour.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M10" codex="2.1" name="Exit review M10">
<instructions>
1. Run the deferred tests (D-41): `pwsh tools/verify.ps1 -Story X-M10` on qa (Debug, every test), then push qa and wait for CI (Release, every test). Fix every failure in the code the failing test covers, one commit per fix; change a test only when it is provably wrong about the requirements, and list each such change with its reason. Run every story's manual checks from docs/plans/US-xxx.md.
   Also run `pwsh tools/verify.ps1 -Story X-M10 -Config Release` on the owner's PC: every Release test with the strict 3-second first-frame limit, which GitHub's GPU-less runners only check at 10 seconds (D-47); record the result in docs/gates/M10.md.
2. Demonstrate the exit criteria: The owner writes a quest in the Editor's graph (or offline in JSON), test-plays it with the debugger and saves it; a player gets it from an NPC, follows it in the journal, tracker and markers, and finishes it; the elder tutorial is a quest and crossroads events are edited as story events.
3. Collect evidence into docs/gates/M10.md, one section per criterion, met or not met. Also: the owner's quest loop is shown in the evidence: a quest built in the graph, test-played with the debugger, saved, then played from an NPC to the reward. List every decision Dominus took as delegated in this milestone, for the owner to review.
4. If all are met and CI on qa is green: merge qa into main, push, confirm CI on main is green, tag the repository m10-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the exit result and the test-run table.</output_format>
</prompt>
```

### M10b Editor help and live data
Exit criteria: The owner rests the pointer on any field of the Level, Building and Graph editors and reads what it does, its range and an example; picks values from the suggestion list of every field; saves a light kind, an object and an interaction (in the Editor and in a text editor) and sees each change in the running game within a second; and a bush moved in the Editor shows in its new place in a loaded run.

Why this milestone exists: the owner asked on 2026-10-05 for tooltips in the Game Editor, auto-suggestions on every field, and Editor changes reflected in the game after save (D-58, 11 answers in three chat rounds). Today only buttons have hover hints, no field suggests values, objects.json and lights.json need a restart, outside edits need F5, and the run save restored at launch can hide level edits. It is built after X-M10 and before K-M11 (D-58 Q1).

Design notes for M10b (owner decisions D-58, docs/decision-requests/D-58.md; brief docs/plans/M10b-editor-help-live-data-brief.md):
- Rules: the owner takes design decisions (D-22, Charter human gate 3); the M10-M14 delegation of D-41 does **not** apply to M10b. Every story runs the full verification with green CI on qa (D-46), like M9a.
- Tooltip (D-58 Q2, Q5): purpose, range and example, one or two short lines, drawn like today's `Button::hint`; on every field of the Level Editor panels, the Building editor and the Graph editor. A coverage test fails by name when a field has no help entry, so later fields (M11 forms included) get one too.
- Help source (Q7): `assets/data/editor/help.json`, one entry per field id `<panel>.<field>` with `purpose`, `range`, `example`, `suggest` and optional `list`. A missing or broken file never stops the Editor.
- Suggestions (Q3, Q6): a dropdown under the focused field (above it near the screen bottom), up to 8 rows, prefix matches first then substring, case-insensitive; Up/Down, Tab/Enter accept, Esc closes, a click accepts; closed, it takes no keys. Sources: catalogs, file names, fixed lists; number fields: the kind or class default, the min, the max and the last 5 values typed in that field this session.
- Live data (Q4, Q8, Q9): every data file the Editor saves, and every file under assets/data/ and the open level changed outside, applies in the running game within 1 s, all or nothing (like F5 today: a broken file keeps the last good data and the mistakes panel names file and line). Placed things follow the new data unless they have their own value; a thing whose kind is gone shows a red "?" marker, is skipped by play, and a warning names the level entry. F5 stays and reloads everything.
- Run save vs level (Q10): when the level was saved after the run's save, every placed thing changed, added or removed in the level comes from the level, matched by id through a per-id hash baseline stored in the run save; untouched run state stays. Save format version bump with a migration (Charter rule 8).
- Layers (Charter rules 1, 9): `TextField`/`NumberField` hint and suggest and the new `SuggestList` are Luna Engine widgets, game-agnostic; help loading, suggestion sources, the reload registry and the watcher are Game code (`src/game/editor_help.*`, `src/game/data_reload.*`); the baseline merge of saved state belongs where each saved system lives. The watcher polls file times (no new library); tests and headless runs turn it off (`--no-watch`), so determinism tests never see a reload.
- Never edit assets/levels/valley.json (the owner's work); tests use their own copies.
- Source of truth: Project Odyssey.docx v2.11 holds epic E29, requirements EDT-09, EDT-10, EDT-11, NFR-09 and MVP-18 (Round 24).

```xml
<prompt id="K-M10b" codex="2.13" name="Kick off M10b Editor help and live data">
<instructions>
1. Confirm that M10 (docs/gates/M10.md) and its stories are done, and that D-58 is Decided in docs/decisions.md.
2. Confirm that the mirrored requirements (docs/project/requirements/Project Odyssey.docx) are v2.11 or later and contain epic E29; if not, run tools/sync-workspace.ps1, and stop with a codex issue if they still do not.
3. D-58 is Decided (docs/decision-requests/D-58.md). Ask the owner, in one chat round (2-4 options each, recommended first), only the questions the M10b stories still leave open after D-58 and the design document draft (for example: the tooltip hover delay, whether the suggestion list opens on focus or on the first typed letter, the colour of the missing-kind marker). Record the answers as D-59 in docs/decisions.md (or the next free ID).
4. Architect: write docs/plans/M10b-editor-help-design.md before US-300: the full list of fields and their help ids (count them per editor; the brief found about 41), the help.json format and its validator, the SuggestList widget and key handling, every suggestion source, the reload registry (data sets, files, all-or-nothing swap, what follows each), the watcher (poll interval, debounce, own-write filter, `--no-watch`), the run-save baseline and its migration, and the test plan. Technical choices are the architect's; record them there.
5. Set this milestone's stories to To do in docs/status.md in this order: US-300, US-301, US-302, US-303, US-304, US-305.
6. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, the owner's answers.</output_format>
</prompt>
```

#### S-US-300 Field tooltips from help.json
```xml
<prompt id="S-US-300" codex="2.13" milestone="M10b" story="US-300" priority="Must" size="M">
<context>
Story US-300: Field tooltips from help.json.
As the owner, I want every Editor field to say what it does, its range and an example, so that I never need the guide to fill a form.
Epic E29 Editor help and live data: Every Editor field explains itself and suggests its values; whatever the owner saves shows in the running game at once, and level edits win over an older run save.
Traces to: EDT-09.
</context>
<dependencies>
Stories that must be Done: US-123, US-173.
Owner decisions that must be Decided: D-58.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Luna Engine (src/luna/engine/ui.*): `TextField` and `NumberField` get a `hint` drawn in `drawOverlay` exactly like `Button::hint`. Game: src/game/editor_help.* loads assets/data/editor/help.json (comments allowed; errors as `file:line: message`) and gives every field its help id and hint when a panel is built in editor.cpp, building_editor.cpp and graph_editor.cpp (small additive edits there). Write the entries for every field listed in docs/plans/M10b-editor-help-design.md. A coverage test builds every panel headless and fails naming each field without an entry.
Follow the M10b design notes and docs/plans/M10b-editor-help-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Hover">
Given the NPC panel
When the pointer rests on Sword
Then a tooltip shows purpose, range and example from help.json
</scenario>
<scenario name="Coverage">
Given all panels of the Level, Building and Graph editors
When the coverage test builds them
Then every field has a help entry, and a missing one fails the test by name
</scenario>
<scenario name="Broken file">
Given a help.json with a syntax error
When the Editor opens
Then fields show no tooltip, the status line names the line, and nothing crashes
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: docs/guides/editor.md gains a "Tooltips and suggestions" section and a line on help.json with an example entry; the shipped help.json passes its validator.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-300`: the Debug build with zero warnings and every Debug test passes, including the "US-300 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-300.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-300/.
</verification>
<teach_back>C++ concept for the owner: Loading text data once and looking it up by key; tests that walk every widget.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-301 Suggestion list widget
```xml
<prompt id="S-US-301" codex="2.13" milestone="M10b" story="US-301" priority="Must" size="M">
<context>
Story US-301: Suggestion list widget.
As the owner, I want a list of matching values under the field I type in, so that I pick instead of remembering names.
Epic E29 Editor help and live data: Every Editor field explains itself and suggests its values; whatever the owner saves shows in the running game at once, and level edits win over an older run save.
Traces to: EDT-10.
</context>
<dependencies>
Stories that must be Done: US-300.
Owner decisions that must be Decided: D-58.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Luna Engine only (src/luna/engine/ui.*, game-agnostic): `TextField` and `NumberField` get a `suggest` function (typed text to a list of strings); a new `SuggestList` overlay under the focused field (above it when it would leave the screen), up to 8 rows with scrolling, prefix matches first then substring, case-insensitive; Up/Down move, Tab/Enter accept, Esc closes and keeps the typed text, a click on a row accepts. Closed, it consumes no input, so every existing key and shortcut behaves as before. Unit tests drive it through `UiInput` without a window. No game field is wired yet (US-302).
Follow the M10b design notes and docs/plans/M10b-editor-help-design.md.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Filter">
Given a field with suggestions trader, trapper, elder
When the owner types "tr"
Then the list shows trader and trapper
</scenario>
<scenario name="Keys">
Given the list open
When the owner presses Down then Tab
Then the second row is the field's text; Esc closes the list and keeps what was typed
</scenario>
<scenario name="Quiet">
Given the list closed
When the owner presses Tab, Enter or the arrows
Then they do what they did before M10b
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: Luna stays game-agnostic (the ADR-016 boundary check passes).</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-301`: the Debug build with zero warnings and every Debug test passes, including the "US-301 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-301.md done, with results in docs/evidence/US-301/.
</verification>
<teach_back>C++ concept for the owner: A reusable overlay widget; keyboard focus and who gets a key.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-302 Suggestions on every field
```xml
<prompt id="S-US-302" codex="2.13" milestone="M10b" story="US-302" priority="Must" size="L">
<context>
Story US-302: Suggestions on every field.
As the owner, I want every field to suggest the values it accepts, so that I never type an id that does not exist.
Epic E29 Editor help and live data: Every Editor field explains itself and suggests its values; whatever the owner saves shows in the running game at once, and level edits win over an older run save.
Traces to: EDT-10.
</context>
<dependencies>
Stories that must be Done: US-301, US-260, US-170.
Owner decisions that must be Decided: D-58.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game (src/game/editor_help.*): resolve each help entry's `suggest` source against the data in use: `number` (kind or class default, min, max, last 5 typed in that field this session), `files:<folder>/<glob>`, `catalog:<name>` (every catalog the design document lists: NPC classes and kinds, partner types, interactions and their fields, light kinds, objects, plants, characters, items, building kinds, prefabs, quests, levels), `values:a|b|c`, `none`; `list: true` completes the item after the last comma and keeps the rest. Sources read the current data each time the list opens, so a catalog saved a moment ago is offered at once. Wire every field of the three editors. The coverage test of US-300 also fails for a field whose `suggest` is missing or names an unknown source.
Follow the M10b design notes and docs/plans/M10b-editor-help-design.md; a format change is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Text">
Given the NPC Script field
When the owner types "el"
Then the .dlg files of the dialogue folder starting with "el" are listed
</scenario>
<scenario name="Lists">
Given the Classes field holding "trader, "
When the owner types "e"
Then the classes starting with "e" are listed and accepting one keeps "trader, "
</scenario>
<scenario name="Numbers">
Given the HP field of a wolf
When it gets focus
Then the list shows the kind default, the min, the max and the last 5 values typed there
</scenario>
<scenario name="Fresh">
Given a new NPC Class saved
When the owner opens the Classes field
Then the new class is listed without a restart
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: docs/guides/editor.md lists every suggestion source with an example.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-302`: the Debug build with zero warnings and every Debug test passes, including the "US-302 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-302.md done, with results and screenshots (`odysseus.exe --screenshot`) in docs/evidence/US-302/.
</verification>
<teach_back>C++ concept for the owner: Strategy functions as data sources; completing one item of a comma list.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-303 Live reload of every data file
```xml
<prompt id="S-US-303" codex="2.13" milestone="M10b" story="US-303" priority="Must" size="L">
<context>
Story US-303: Live reload of every data file.
As the owner, I want everything I save to show in the running game at once, so that I test changes without a restart.
Epic E29 Editor help and live data: Every Editor field explains itself and suggests its values; whatever the owner saves shows in the running game at once, and level edits win over an older run save.
Traces to: EDT-11, NFR-09.
</context>
<dependencies>
Stories that must be Done: US-156, US-260.
Owner decisions that must be Decided: D-58.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game (src/game/data_reload.*): one registry of reloadable data sets, each with its files, an all-or-nothing reload (read into a side copy; only a clean copy replaces the data in use, as `OdysseyGame::reloadInteractions` does today) and what must follow it (placed things, Editor palettes, help suggestions). Move the existing reloads (interactions, dialogues, NPC classes and kinds, partner defaults, building kinds and prefabs, quests) into it and add objects.json, lights.json, characters, plants, items, help.json and every other catalog under assets/data/ that today needs a restart. Every Editor save calls the registry for the file it wrote; F5 reloads every set. Placed things without their own value follow the new data; a placed thing whose kind is gone draws a red "?" marker, is skipped by play, and a warning names the level entry. Simulation catalogs reload through ids, never pointers (CI-007). Remove every "restart the game" line from the guides.
Follow the M10b design notes and docs/plans/M10b-editor-help-design.md.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Catalog">
Given a running game
When the owner changes a light kind's colour in lights.json and saves
Then placed lights of that kind shine in the new colour within 1 s
</scenario>
<scenario name="Object">
Given objects.json
When the owner adds an object and saves
Then it is on the object page of the palette without a restart
</scenario>
<scenario name="All or nothing">
Given a broken interaction file saved
When the reload runs
Then the last good data stays in use and the mistakes panel names the file and line
</scenario>
<scenario name="Missing kind">
Given a placed thing whose kind the owner deleted
When the data reloads
Then it shows a red marker and a warning naming the level entry, and the game runs on
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: a timing test shows each data set reloads in under 100 ms on the D-06 PC (NFR-09) and records the numbers in docs/evidence/US-303/; the determinism hash test runs with no reload.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-303`: the Debug build with zero warnings and every Debug test passes, including the "US-303 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-303.md done, with results and screenshots in docs/evidence/US-303/.
</verification>
<teach_back>C++ concept for the owner: A registry of reloadable data sets; swapping data only when the new copy is clean.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3); the NFR-09 budget cannot be met (write a decision request for the owner).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-304 File watch for outside edits
```xml
<prompt id="S-US-304" codex="2.13" milestone="M10b" story="US-304" priority="Must" size="M">
<context>
Story US-304: File watch for outside edits.
As the owner, I want files I edit in a text editor to reload by themselves, so that I can work outside the Editor too.
Epic E29 Editor help and live data: Every Editor field explains itself and suggests its values; whatever the owner saves shows in the running game at once, and level edits win over an older run save.
Traces to: EDT-11, NFR-09.
</context>
<dependencies>
Stories that must be Done: US-303.
Owner decisions that must be Decided: D-58.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: Game (src/game/data_reload.*): a watcher that polls the `last_write_time` of every registered file and folder (std::filesystem; no new library, no thread unless the design document justifies one) at the design document's interval with a debounce, so a saved change is live within 1 s; it ignores, once, the files the game wrote itself. The open level changed outside: reloaded when the Editor has no unsaved changes; otherwise the status line says the file changed and nothing is overwritten. `--no-watch` turns it off; tests and headless runs use it.
Follow the M10b design notes and docs/plans/M10b-editor-help-design.md.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Outside">
Given the game running
When the owner saves an interaction file in a text editor
Then the change is live within 1 s, without F5
</scenario>
<scenario name="Own writes">
Given the Editor saves a file
When the watcher sees it
Then it does not reload it a second time
</scenario>
<scenario name="Open level">
Given the Editor open with unsaved changes
When the level file changes on disk
Then nothing is overwritten and the status line says the file changed
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: the watcher costs under 0.5 ms per frame on the D-06 PC with the shipped data (recorded in docs/evidence/US-304/); docs/guides/editor.md explains live data and `--no-watch`.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-304`: the Debug build with zero warnings and every Debug test passes, including the "US-304 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-304.md done, with results in docs/evidence/US-304/.
</verification>
<teach_back>C++ concept for the owner: Polling file times; debouncing; ignoring your own writes.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-305 Level edits win over the run save
```xml
<prompt id="S-US-305" codex="2.13" milestone="M10b" story="US-305" priority="Must" size="L">
<context>
Story US-305: Level edits win over the run save.
As the owner, I want my level edits to win over an older run save, so that what I saved is what I play.
Epic E29 Editor help and live data: Every Editor field explains itself and suggests its values; whatever the owner saves shows in the running game at once, and level edits win over an older run save.
Traces to: EDT-11, ADR-010.
</context>
<dependencies>
Stories that must be Done: US-303, US-153.
Owner decisions that must be Decided: D-58.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only.
Where the work belongs: every run save (things.json, buildings.json, the placed people) also stores the level baseline: for each placed thing (plants and objects, characters and NPCs, pickups, effects, lights, level buildings) its id and a hash of its level entry. On load, and after every Editor save while a run is loaded, each id is compared with the level now: the same hash keeps the run state; changed or new comes fresh from the level; gone from the level is removed from the run. The status line says how many things the level updated. Bump each save format's version with a migration: a save without a baseline loads as before once and is written with one. Saved state stays in the layer that owns it (Simulation state in src/sim/, Game state in src/game/).
Follow the M10b design notes and docs/plans/M10b-editor-help-design.md; a save format change beyond the baseline is a design question for the owner.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete only when every scenario passes with evidence, the Definition of Done holds, and it is merged into qa with green CI; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Edited">
Given a run saved
When the owner moves a berry bush in the Editor and saves
Then the next start (and F1) shows it in its new place, fresh
</scenario>
<scenario name="Untouched">
Given the same run
When the owner starts again
Then the berries picked on another bush still ripen, and the clan and hero are as saved
</scenario>
<scenario name="Removed">
Given an NPC deleted from the level
When the run loads
Then it is not in the run and the status line counts the update
</scenario>
<scenario name="Old save">
Given a save from before M10b
When it loads
Then it behaves as before once and is written with a baseline
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done, plus: load-save-load round-trip tests for every changed save format; docs/guides/editor.md says how level edits and run saves combine.</definition_of_done>
<verification>
`pwsh tools/verify.ps1 -Story US-305`: the Debug build with zero warnings and every Debug test passes, including the "US-305 ..." cases and the determinism hash test. After a failure rerun only the failing cases; run the full verification once at the end.
Green CI on qa after the merge (Release build and every Release test, D-46).
Manual checks in docs/plans/US-305.md done, with results and screenshots in docs/evidence/US-305/.
</verification>
<teach_back>C++ concept for the owner: Content hashes as a baseline; merging by id; save versions and migrations.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a design or save-format question this Codex does not answer (ask the owner, Charter human gate 3).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M10b" codex="2.13" name="Exit review M10b">
<instructions>
1. Run `pwsh tools/verify.ps1 -Story X-M10b` on qa (Debug, every test) and confirm CI on qa is green. Also run `pwsh tools/verify.ps1 -Story X-M10b -Config Release` on the owner's PC: every Release test with the strict 3-second first-frame limit, which GitHub's GPU-less runners only check at 10 seconds (D-47); record the result in docs/gates/M10b.md.
2. Write docs/gates/M10b-walkthrough.md: a 10-minute script for the owner (D-58 Q11) that covers every exit criterion: hover five fields in each editor; pick a class, a dialogue file and a number from suggestion lists; change a light kind's colour and add an object in a text editor and watch the running game; save a broken interaction and see the mistakes panel; move a bush, save, start the run again.
3. Demonstrate the exit criteria: The owner rests the pointer on any field of the Level, Building and Graph editors and reads what it does, its range and an example; picks values from the suggestion list of every field; saves a light kind, an object and an interaction (in the Editor and in a text editor) and sees each change in the running game within a second; and a bush moved in the Editor shows in its new place in a loaded run.
4. Collect evidence into docs/gates/M10b.md, one section per criterion, met or not met, with the coverage test output (fields per editor) and the NFR-09 timings.
5. Owner gate: ask the owner in chat to run the walkthrough and answer Pass or Fail with notes; record the answer in docs/gates/M10b.md. This is the only stop of M10b.
6. If all are met, the walkthrough passed and CI on qa is green: merge qa into main, push, confirm CI on main is green, tag the repository m10b-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the exit result, the walkthrough answer and the test-run table.</output_format>
</prompt>
```

### M11 Data editors
Exit criteria: Every data file has a schema; the owner edits any entity, mechanic, story tuning value, game rule and daily routine in Editor forms with pickers and validation, and the running game reloads it.

Why this milestone exists: every data file becomes editable in the Editor through forms built from schemas (D-40, EDT-02, EDT-03, SDC-07). The design notes under M10 apply.

```xml
<prompt id="K-M11" codex="2.13" name="Kick off M11 Data editors">
<instructions>
1. Confirm that M10 and M10b (docs/gates/M10.md, docs/gates/M10b.md) and their stories are done (X-M10 and X-M10b ran their full verification and CI), and that D-40, D-41 and D-58 are Decided in docs/decisions.md. The M11 schema forms reuse the M10b help ids, tooltips and suggestion lists: a schema field's description and examples feed its entry in assets/data/editor/help.json (one help source), and every new form field passes the M10b coverage test.
2. Architect: write docs/plans/M11-data-editors-design.md (schema subset and validator, form widgets, reference index, catalog reload by id (CI-007), quick check, Game Rules loading, routines as role-level weights on the M9c schedules of US-290) before the first story. Every design question it meets, Dominus decides (D-41) and records as delegated.
3. Set this milestone's stories to To do in docs/status.md in this order: US-190, US-191, US-193, US-194, US-195, US-196, US-192.
4. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, delegated decisions.</output_format>
</prompt>
```

#### S-US-190 Schemas for every data file
```xml
<prompt id="S-US-190" codex="2.1" milestone="M11" story="US-190" priority="Must" size="L">
<context>
Story US-190: Schemas for every data file.
As the owner, I want every data file described by a schema with types, ranges, choices, links and help text, so that the Editor and the validator know what each field means.
Epic E18 Data editors: Every data file (entities, mechanics, story tuning, game rules, daily routines) is edited in the Editor through forms built from schemas.
Traces to: EDT-02, ADR-020.
</context>
<dependencies>
Stories that must be Done: US-150.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Simulation: src/sim/schema/ (schema loader and validator for the brief's section 4.3 subset); assets/data/schemas/<file>.schema.json for every file under assets/data/; every loader validates against its schema; a CI test checks every data file and compares each schema's fields with its loader's. Guide: docs/guides/schemas.md.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Coverage">
Given every file under assets/data/
When the schema check runs in CI
Then each file has a schema and passes it
</scenario>
<scenario name="Errors">
Given a value out of its range
When the game loads it
Then the error names file, line, field and the allowed range
</scenario>
<scenario name="Links">
Given a recipe naming an item no catalog has
When the check runs
Then the broken link is listed
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-190/. The "US-190 ..." tests exist and compile; they run at X-M11.
Manual checks in docs/plans/US-190.md listed, to be run at X-M11.
</verification>
<teach_back>C++ concept for the owner: Describing data with data (a schema interpreter).</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-191 Schema-driven form editor
```xml
<prompt id="S-US-191" codex="2.1" milestone="M11" story="US-191" priority="Must" size="L">
<context>
Story US-191: Schema-driven form editor.
As the owner, I want a Data tab where any file opens as forms built from its schema, so that I can edit everything without touching JSON.
Epic E18 Data editors: Every data file (entities, mechanics, story tuning, game rules, daily routines) is edited in the Editor through forms built from schemas.
Traces to: EDT-02.
</context>
<dependencies>
Stories that must be Done: US-190, US-121.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Game: a Data tab in the Editor: file list, entry list with search, forms built from the schema (number with range, text, enum, bool, list, nested object, reference picker), help text, undo through History, Ctrl+S writes canonical JSON (note fields kept). Saving reloads the file into the running game. This resolves CI-007: catalogs (plants, animals, weapons, characters, objects) must be swappable, so every holder of a catalog definition (WorldPlant::def, the hotbar's weapons, starters_, enemies' kinds and the like) keeps the kind's id and looks the definition up, never a raw pointer; F5 then reloads catalogs too.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Open">
Given weapons.json
When the owner opens it in the Data tab
Then entries are listed with search, and a form shows the selected entry's fields with help text
</scenario>
<scenario name="Edit">
Given a damage value changed and Ctrl+S
When the file is saved
Then only that value changes in the file, note fields stay, and the running game uses it
</scenario>
<scenario name="Undo">
Given five form edits
When Ctrl+Z five times
Then the data is as before
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-191/. The "US-191 ..." tests exist and compile; they run at X-M11.
Manual checks in docs/plans/US-191.md listed, to be run at X-M11.
</verification>
<teach_back>C++ concept for the owner: Building UI from a description at run time.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-193 Entity editor
```xml
<prompt id="S-US-193" codex="2.1" milestone="M11" story="US-193" priority="Must" size="M">
<context>
Story US-193: Entity editor.
As the owner, I want to create, copy, rename and delete kinds of weapons, plants, animals, characters, objects and items, with their tags, states and interactions, so that new entities need no code.
Epic E18 Data editors: Every data file (entities, mechanics, story tuning, game rules, daily routines) is edited in the Editor through forms built from schemas.
Traces to: EDT-02, INT-02.
</context>
<dependencies>
Stories that must be Done: US-191, US-172.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Game: entity actions in the Data tab for catalogs (create, copy, rename, delete) with a reference index built from the schemas' `ref` fields, so a rename updates every reference across data, quests, dialogue and levels and a delete lists uses and asks to confirm; tags, states and the kind's interactions (opens the M9 interaction graph).
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Create">
Given the plants catalog
When the owner copies a bush, renames it and changes its tags
Then the new plant appears in the Editor palette and offers its interactions
</scenario>
<scenario name="Rename">
Given an item used by recipes and quests
When the owner renames it
Then every reference is updated and listed
</scenario>
<scenario name="Delete">
Given a kind that is still used
When the owner deletes it
Then the uses are listed and the delete waits for confirmation
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-193/. The "US-193 ..." tests exist and compile; they run at X-M11.
Manual checks in docs/plans/US-193.md listed, to be run at X-M11.
</verification>
<teach_back>C++ concept for the owner: Keeping references consistent (rename refactoring).</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-194 Mechanics and story tuning with a quick check
```xml
<prompt id="S-US-194" codex="2.1" milestone="M11" story="US-194" priority="Must" size="M">
<context>
Story US-194: Mechanics and story tuning with a quick check.
As the owner, I want needs, actions, life, social, story, calendar, professions, recipes and hero data in forms, and a quick headless run to see the effect, so that I can tune mechanics safely.
Epic E18 Data editors: Every data file (entities, mechanics, story tuning, game rules, daily routines) is edited in the Editor through forms built from schemas.
Traces to: EDT-02, STO-02.
</context>
<dependencies>
Stories that must be Done: US-191.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Game + headless: forms for every mechanics and story file (sim/*.json, hero/*.json); a Quick check button runs odysseus_headless for 20 years with the edited data and the current seed and shows population, deaths by cause and episodes beside the previous run.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Forms">
Given every mechanics and story file
When the owner opens it
Then it shows as forms
</scenario>
<scenario name="Quick check">
Given a changed hunger rate
When the owner runs the 20-year check
Then a summary shows population, deaths by cause and episodes, next to the last run
</scenario>
<scenario name="Determinism">
Given the same data and seed
When the check runs twice
Then the summaries are the same
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-194/. The "US-194 ..." tests exist and compile; they run at X-M11.
Manual checks in docs/plans/US-194.md listed, to be run at X-M11.
</verification>
<teach_back>C++ concept for the owner: Running the simulation headless from the game.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-195 Game Rules page
```xml
<prompt id="S-US-195" codex="2.1" milestone="M11" story="US-195" priority="Must" size="M">
<context>
Story US-195: Game Rules page.
As the owner, I want one page for New Game presets, comfort, victory thresholds and switches for whole systems, so that I can set up how a game plays.
Epic E18 Data editors: Every data file (entities, mechanics, story tuning, game rules, daily routines) is edited in the Editor through forms built from schemas.
Traces to: EDT-03, MVP-09.
</context>
<dependencies>
Stories that must be Done: US-191.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Simulation: assets/data/rules/standard.json (brief section 4.4) loaded by every system that has a switch; New Game screen gets a rules picker; a level may name `rules`; the Game Rules page in the Data tab. Victory thresholds move here from code or other files.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Switches">
Given weather, combat, rival clans, tutorial, markers, chronicle and politics switches
When the owner turns weather off and starts a game
Then no weather happens
</scenario>
<scenario name="Rules sets">
Given two rules files
When a new game or a level picks one
Then its values are used
</scenario>
<scenario name="Thresholds">
Given a victory threshold changed
When a game reaches it
Then victory follows the new value
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-195/. The "US-195 ..." tests exist and compile; they run at X-M11.
Manual checks in docs/plans/US-195.md listed, to be run at X-M11.
</verification>
<teach_back>C++ concept for the owner: Feature flags read once at start.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-196 Daily routines as data
```xml
<prompt id="S-US-196" codex="2.11" milestone="M11" story="US-196" priority="Must" size="M">
<context>
Story US-196: Daily routines as data.
As the owner, I want daily routines per role and per person (sleep, work, meals, gatherings at the fire), so that clan life has a rhythm I can shape.
Epic E18 Data editors: Every data file (entities, mechanics, story tuning, game rules, daily routines) is edited in the Editor through forms built from schemas.
Traces to: SDC-07, SDC-12, INT-05.
Since v2.11: M9c already built day and night schedules (US-290, SDC-12) in one format with the precedence class, kind, NPC, an Editor schedule form, interruption by needs and danger, and a daily summary for far persons. This story reuses them; it never adds a second schedule format or a second scheduler.
</context>
<dependencies>
Stories that must be Done: US-154, US-191, US-290, US-291.
Owner decisions that must be Decided: D-40, D-41, D-52.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Simulation: a routine is the US-290 schedule format given per role (a profession or an NPC Class) and per person, stored where US-290 stores schedules (no assets/data/sim/routines.json; if the M11 design document wants one file per role, it holds US-290 schedule entries). What this story adds: each block may carry weights for interaction tags, and the M7 utility scoring multiplies an interaction's score by the active block's weight for matching tags, so a block guides choices instead of only naming one activity; clan members (who had no schedule in M9c) get their profession's routine; needs below their danger level still win (US-290 interruption rule). Game: a 24-hour timeline view (drag blocks) added to the US-290 schedule form, and the schema of the schedule format added to the M11 schema set so the Data editors open it; the timeline and the form edit the same data.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Routine">
Given the hunters' routine with work from 6 to 14
When a day passes
Then hunters choose work interactions in that block more often than at other times
</scenario>
<scenario name="Edit">
Given the day timeline of the hunters' routine
When the owner drags a block
Then the schedule data is saved in the US-290 format, the schedule form shows the same change, and play uses it
</scenario>
<scenario name="One format">
Given an NPC Class schedule made in M9c and a profession routine made in this story
When both files are loaded
Then both are read by the same loader and validator, and no routines-only format exists
</scenario>
<scenario name="Needs win">
Given a starving hunter in a work block
When time passes
Then they eat first
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-196/. The "US-196 ..." tests exist and compile; they run at X-M11.
Manual checks in docs/plans/US-196.md listed, to be run at X-M11.
</verification>
<teach_back>C++ concept for the owner: Time blocks and weighting a utility AI; extending one data format instead of adding a second.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-192 Picture pickers and cutting frames
```xml
<prompt id="S-US-192" codex="2.1" milestone="M11" story="US-192" priority="Should" size="M">
<context>
Story US-192: Picture pickers and cutting frames.
As the owner, I want to pick sprites, animations and effects by picture and cut new frames from my sheets in the Editor, so that new content gets its art without the command line.
Epic E18 Data editors: Every data file (entities, mechanics, story tuning, game rules, daily routines) is edited in the Editor through forms built from schemas.
Traces to: EDT-02, D-05.
</context>
<dependencies>
Stories that must be Done: US-191, US-120.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Game: picture pickers for atlas-frame fields, animation and effect previews in the form; a Cut tool on a sheet in assets/sprites/ that adds a named rectangle to cuts.json and rebuilds the atlas through the existing odysseus_atlas code.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Pick">
Given a field holding an atlas frame
When the owner opens its picker
Then frames are shown as pictures and the choice is saved
</scenario>
<scenario name="Preview">
Given an effect or an animation
When it is selected
Then it plays in the form
</scenario>
<scenario name="Cut">
Given a sheet in assets/sprites/
When the owner draws a rectangle and names it
Then cuts.json gains the cut and the atlas is rebuilt
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-192/. The "US-192 ..." tests exist and compile; they run at X-M11.
Manual checks in docs/plans/US-192.md listed, to be run at X-M11.
</verification>
<teach_back>C++ concept for the owner: Image regions and previews.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M11" codex="2.1" name="Exit review M11">
<instructions>
1. Run the deferred tests (D-41): `pwsh tools/verify.ps1 -Story X-M11` on qa (Debug, every test), then push qa and wait for CI (Release, every test). Fix every failure in the code the failing test covers, one commit per fix; change a test only when it is provably wrong about the requirements, and list each such change with its reason. Run every story's manual checks from docs/plans/US-xxx.md.
   Also run `pwsh tools/verify.ps1 -Story X-M11 -Config Release` on the owner's PC: every Release test with the strict 3-second first-frame limit, which GitHub's GPU-less runners only check at 10 seconds (D-47); record the result in docs/gates/M11.md.
2. Demonstrate the exit criteria: Every data file has a schema; the owner edits any entity, mechanic, story tuning value, game rule and daily routine in Editor forms with pickers and validation, and the running game reloads it.
3. Collect evidence into docs/gates/M11.md, one section per criterion, met or not met. Also: one new plant kind and one changed mechanic are made in the Data tab only, saved and seen in the running game; the schema CI test is green. List every decision Dominus took as delegated in this milestone, for the owner to review.
4. If all are met and CI on qa is green: merge qa into main, push, confirm CI on main is green, tag the repository m11-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the exit result and the test-run table.</output_format>
</prompt>
```

### M12 World editing
Exit criteria: The owner opens the procedural region in the Editor, tunes the generator with a live preview, paints terrain, water and mountains, places things, people and camps, and edits each clan's and person's relations, economy, routines and actions; edits are saved on top of the seed and a new game plays them.

Why this milestone exists: the owner edits the procedural region and its clans and people in the Editor, on top of the seed (D-40, EDT-04, EDT-05). World objects ride on the plant machinery (CI-008): they are placed, saved and found like plants, flagged `object`; split them into their own list only when an object needs data a plant cannot hold. The design notes under M10 apply.

```xml
<prompt id="K-M12" codex="2.1" name="Kick off M12 World editing">
<instructions>
1. Confirm that M11 (docs/gates/M11.md) and its stories are done (for M10: X-M9 also ran its full verification and CI), and that D-40 and D-41 are Decided in docs/decisions.md.
2. Architect: write docs/plans/M12-world-editing-design.md (chunk streaming and minimap budget, world file and overrides, regeneration conflicts, region tools, inspector data, save migrations) before the first story. Every design question it meets, Dominus decides (D-41) and records as delegated.
3. Set this milestone's stories to To do in docs/status.md in this order: US-200, US-201, US-202, US-203, US-204, US-205, US-206, US-207.
4. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, delegated decisions.</output_format>
</prompt>
```

#### S-US-200 The region in the Editor
```xml
<prompt id="S-US-200" codex="2.1" milestone="M12" story="US-200" priority="Must" size="L">
<context>
Story US-200: The region in the Editor.
As the owner, I want to open the procedural region in the Editor with a zoomable map, so that I can work on the whole world.
Epic E19 World editing: The owner shapes the procedural region and its clans, people, economy and relations in the Editor, on top of the seed.
Traces to: EDT-04.
</context>
<dependencies>
Stories that must be Done: US-040, US-123.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Game: a Region view in the Editor that streams the 32-tile chunks around the camera from the existing generator (US-040), a cached minimap for zoomed-out views, layer switches (terrain, water, plants, things, people, places, camps). Performance budget in the design document.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Open">
Given a seed
When the owner opens the region
Then it shows at any zoom from the whole 256 x 256 map to single tiles, at a smooth frame rate
</scenario>
<scenario name="Layers">
Given the layer switches
When the owner hides plants and people
Then only terrain shows
</scenario>
<scenario name="Same world">
Given the region opened in the Editor
When it is compared to the game's
Then they are identical
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-200/. The "US-200 ..." tests exist and compile; they run at X-M12.
Manual checks in docs/plans/US-200.md listed, to be run at X-M12.
</verification>
<teach_back>C++ concept for the owner: Streaming chunks and level of detail.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-201 Generator settings with live preview
```xml
<prompt id="S-US-201" codex="2.1" milestone="M12" story="US-201" priority="Must" size="M">
<context>
Story US-201: Generator settings with live preview.
As the owner, I want every generator setting in forms with a preview map, so that I can shape a region before committing to it.
Epic E19 World editing: The owner shapes the procedural region and its clans, people, economy and relations in the Editor, on top of the seed.
Traces to: EDT-04.
</context>
<dependencies>
Stories that must be Done: US-200, US-191.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Generator settings moved to assets/data/sim/region.json with a schema (all biome, noise, river, resource and camp settings); the Region view's Settings form with Preview (regenerate a low-resolution map off the main world) and Apply; conflicts with hand edits listed.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Preview">
Given the biome mix changed
When the owner presses Preview
Then a new map shows within a few seconds beside the old one
</scenario>
<scenario name="Apply">
Given a preview the owner likes
When Apply
Then the region and region.json use the new settings
</scenario>
<scenario name="Keep edits">
Given hand edits on the region
When the settings change
Then the edits stay and conflicts are listed
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-201/. The "US-201 ..." tests exist and compile; they run at X-M12.
Manual checks in docs/plans/US-201.md listed, to be run at X-M12.
</verification>
<teach_back>C++ concept for the owner: Pure functions of a seed.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-202 Hand edits on top of the seed
```xml
<prompt id="S-US-202" codex="2.1" milestone="M12" story="US-202" priority="Must" size="L">
<context>
Story US-202: Hand edits on top of the seed.
As the owner, I want to paint terrain and biomes on the region and keep them on top of the seed, so that the world is generated and hand-made at once.
Epic E19 World editing: The owner shapes the procedural region and its clans, people, economy and relations in the Editor, on top of the seed.
Traces to: EDT-04, ADR-010.
</context>
<dependencies>
Stories that must be Done: US-200.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Simulation: world file assets/worlds/<name>.json (brief section 4.6): seed + generator settings + overrides per chunk; the region loader applies overrides after generation; brush, rectangle and fill tools from the level editor work on the region and record Commands for undo.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Paint">
Given the brush on the region
When the owner paints a meadow into a forest
Then the change shows and is saved as an override of those chunks
</scenario>
<scenario name="Small saves">
Given 100 painted tiles
When the region is saved
Then the file holds the seed plus the changed tiles, not the whole map
</scenario>
<scenario name="Undo">
Given region edits
When Ctrl+Z
Then they are undone as in the level editor
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-202/. The "US-202 ..." tests exist and compile; they run at X-M12.
Manual checks in docs/plans/US-202.md listed, to be run at X-M12.
</verification>
<teach_back>C++ concept for the owner: Storing differences (overlays) instead of copies.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-203 Water and mountains
```xml
<prompt id="S-US-203" codex="2.1" milestone="M12" story="US-203" priority="Must" size="M">
<context>
Story US-203: Water and mountains.
As the owner, I want to draw and remove rivers, lakes, cliffs and caves, so that I can shape the land.
Epic E19 World editing: The owner shapes the procedural region and its clans, people, economy and relations in the Editor, on top of the seed.
Traces to: EDT-04.
</context>
<dependencies>
Stories that must be Done: US-202.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Region tools for rivers (path with width, fords), lakes (area), cliffs and cave mouths, stored as overrides; walking and shot blocking follow the existing solid rules.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="River">
Given the river tool
When the owner draws a river
Then it flows from start to end and blocks walking except at fords
</scenario>
<scenario name="Cave">
Given a cliff
When the owner places a cave mouth
Then the cave can be entered in the game
</scenario>
<scenario name="Remove">
Given a generated lake
When the owner removes it
Then land replaces it and the change is an override
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-203/. The "US-203 ..." tests exist and compile; they run at X-M12.
Manual checks in docs/plans/US-203.md listed, to be run at X-M12.
</verification>
<teach_back>C++ concept for the owner: Path tools and connected areas.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-204 Things, people and places in the region
```xml
<prompt id="S-US-204" codex="2.1" milestone="M12" story="US-204" priority="Must" size="M">
<context>
Story US-204: Things, people and places in the region.
As the owner, I want to place plants, animals, objects, people and named places in the region, so that quests and stories have their spots.
Epic E19 World editing: The owner shapes the procedural region and its clans, people, economy and relations in the Editor, on top of the seed.
Traces to: EDT-04, STO-04.
</context>
<dependencies>
Stories that must be Done: US-202, US-155.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Region placement of plants, animals, world objects (objects ride on the plant machinery, CI-008), NPCs and named places, with ids kept across regeneration; places become pickers for quests and dialogue.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Place">
Given the palettes
When the owner places a fire pit and an NPC
Then they are saved as region overrides and appear in the game
</scenario>
<scenario name="Places">
Given a named place 'Red Cliff'
When a quest step targets it
Then the quest marker points to it
</scenario>
<scenario name="Pickers">
Given a quest or dialogue field
When the owner picks a person or place
Then region people and places are offered
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-204/. The "US-204 ..." tests exist and compile; they run at X-M12.
Manual checks in docs/plans/US-204.md listed, to be run at X-M12.
</verification>
<teach_back>C++ concept for the owner: Ids that survive regeneration.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-205 Camps and resources
```xml
<prompt id="S-US-205" codex="2.1" milestone="M12" story="US-205" priority="Must" size="M">
<context>
Story US-205: Camps and resources.
As the owner, I want to move the player and rival camps and set resource spots and amounts, so that I control the region's balance.
Epic E19 World editing: The owner shapes the procedural region and its clans, people, economy and relations in the Editor, on top of the seed.
Traces to: EDT-04, MVP-10.
</context>
<dependencies>
Stories that must be Done: US-202, US-041.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Camps (player and rivals) and resource spots with amounts as world-file entries; rival clan worlds (US-041) start at the placed camps; placement rules validated with reasons.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Camps">
Given a rival camp
When the owner moves it
Then the rival clan lives there in the game
</scenario>
<scenario name="Resources">
Given a flint spot
When the owner sets it to 50
Then 50 flint can be taken there
</scenario>
<scenario name="Rules">
Given a camp placed in water
When the owner tries
Then it is refused with the reason
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-205/. The "US-205 ..." tests exist and compile; they run at X-M12.
Manual checks in docs/plans/US-205.md listed, to be run at X-M12.
</verification>
<teach_back>C++ concept for the owner: Validating placements against rules.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-206 Clans and people inspector
```xml
<prompt id="S-US-206" codex="2.1" milestone="M12" story="US-206" priority="Must" size="L">
<context>
Story US-206: Clans and people inspector.
As the owner, I want to edit each clan and person in the world: members, kinship, leader, opinions and grudges, rival stance, owned items, stores, debts, trade partners, routine and allowed actions, so that I set up the social, economic and political world.
Epic E19 World editing: The owner shapes the procedural region and its clans, people, economy and relations in the Editor, on top of the seed.
Traces to: EDT-05, SDC-07, PIL-08.
</context>
<dependencies>
Stories that must be Done: US-204, US-196, US-173.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Game: Inspector panels for a clan and a person in the Region view: members, kinship, leader, opinions and grudges (with reasons), rival stance, owned items, store contents, debts, trade partners, routine, allowed actions and property overrides (M9 overrides extended to region things); stored in the world file's clans and people sections and applied when a game starts.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Social">
Given two people selected
When the owner sets their opinion to -50 and adds a grudge with a reason
Then the game starts with that grudge and the chronicle can tell it
</scenario>
<scenario name="Economy">
Given a clan selected
When the owner fills its store and adds a debt to a rival
Then the game starts with that store and debt
</scenario>
<scenario name="Overrides">
Given a person selected
When the owner removes Barter from their actions and changes their HP
Then only that person changes
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-206/. The "US-206 ..." tests exist and compile; they run at X-M12.
Manual checks in docs/plans/US-206.md listed, to be run at X-M12.
</verification>
<teach_back>C++ concept for the owner: Editing a graph of relations safely.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-207 Play the edited region
```xml
<prompt id="S-US-207" codex="2.1" milestone="M12" story="US-207" priority="Must" size="S">
<context>
Story US-207: Play the edited region.
As the owner, I want to start a new game on my edited region or play it from the Editor, so that my world is the game.
Epic E19 World editing: The owner shapes the procedural region and its clans, people, economy and relations in the Editor, on top of the seed.
Traces to: EDT-04, EDT-06.
</context>
<dependencies>
Stories that must be Done: US-206, US-186.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: New Game picks a world file (default: generated from a seed); Play here works on the region; save and world versions bumped with migrations so older saves load.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="New game">
Given an edited region saved as a world file
When the player starts a new game from it
Then every edit is there
</scenario>
<scenario name="Play here">
Given the region in the Editor
When the owner chooses Play here
Then the game starts on the edited region at the cursor
</scenario>
<scenario name="Old saves">
Given a save from before region editing
When it loads
Then it still works
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-207/. The "US-207 ..." tests exist and compile; they run at X-M12.
Manual checks in docs/plans/US-207.md listed, to be run at X-M12.
</verification>
<teach_back>C++ concept for the owner: Versioned save formats.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M12" codex="2.1" name="Exit review M12">
<instructions>
1. Run the deferred tests (D-41): `pwsh tools/verify.ps1 -Story X-M12` on qa (Debug, every test), then push qa and wait for CI (Release, every test). Fix every failure in the code the failing test covers, one commit per fix; change a test only when it is provably wrong about the requirements, and list each such change with its reason. Run every story's manual checks from docs/plans/US-xxx.md.
   Also run `pwsh tools/verify.ps1 -Story X-M12 -Config Release` on the owner's PC: every Release test with the strict 3-second first-frame limit, which GitHub's GPU-less runners only check at 10 seconds (D-47); record the result in docs/gates/M12.md.
2. Demonstrate the exit criteria: The owner opens the procedural region in the Editor, tunes the generator with a live preview, paints terrain, water and mountains, places things, people and camps, and edits each clan's and person's relations, economy, routines and actions; edits are saved on top of the seed and a new game plays them.
3. Collect evidence into docs/gates/M12.md, one section per criterion, met or not met. Also: an edited region (terrain, a river, a camp moved, a clan's store and a grudge set) is saved as a small world file and a new game starts on it. List every decision Dominus took as delegated in this milestone, for the owner to review.
4. If all are met and CI on qa is green: merge qa into main, push, confirm CI on main is green, tag the repository m12-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the exit result and the test-run table.</output_format>
</prompt>
```

### M13 Politics
Exit criteria: Clans form alliances and vassal oaths, hold elders' councils, bind themselves by marriage, challenge leaders and use trade and craft knowledge as levers; the player can win the region through Politics; every political rule is editable in the Editor.

Why this milestone exists: Politics returns to the MVP as the third playable pillar with a victory (D-40, PIL-08). The economic and technological levers in M13 cover trade pacts, tribute, embargoes and teaching recipes; technologies themselves arrive in M14. The design notes under M10 apply.

```xml
<prompt id="K-M13" codex="2.1" name="Kick off M13 Politics">
<instructions>
1. Confirm that M12 (docs/gates/M12.md) and its stories are done (for M10: X-M9 also ran its full verification and CI), and that D-40 and D-41 are Decided in docs/decisions.md.
2. Architect: write docs/plans/M13-politics-design.md (stance model and reasons, alliances, oaths and tribute, council voting, marriage ties, leadership, levers, victory and balance check) before the first story. Every design question it meets, Dominus decides (D-41) and records as delegated.
3. Set this milestone's stories to To do in docs/status.md in this order: US-210, US-211, US-212, US-213, US-214, US-215, US-216.
4. Recruiting for kill gate 2 starts now (D-48): remind the owner in the assembly report to recruit the eight playtesters as docs/plans/M6-playtest-plan.md section 3 describes. Recruiting needs people, so it is the owner's task; agents never contact anyone.
5. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, delegated decisions.</output_format>
</prompt>
```

#### S-US-210 The political model
```xml
<prompt id="S-US-210" codex="2.1" milestone="M13" story="US-210" priority="Must" size="L">
<context>
Story US-210: The political model.
As the owner, I want clans as political actors with relations, alliances, vassal ties and a Politics share of the region, all as editable data, so that politics is part of the simulation.
Epic E20 Politics: Clans ally, swear and break oaths, marry, vote and challenge leaders; the player can win the region through Politics.
Traces to: PIL-03, PIL-08.
</context>
<dependencies>
Stories that must be Done: US-206, US-195.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Simulation: src/sim/politics/ with clan-to-clan stance (-100..100, moved by events with reasons), alliance and vassal relations, and the Politics share; assets/data/sim/politics.json (brief section 4.7) with a schema; politics on/off follows the Game Rules switch.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Relations">
Given three clans
When the simulation runs
Then each pair has a stance from -100 to 100 that moves with events, with reasons
</scenario>
<scenario name="Share">
Given one rival a vassal of the player's clan
When the dominion screen opens
Then Politics shows that clan's share of the region's people
</scenario>
<scenario name="Data">
Given politics.json
When the owner opens it in the Data tab
Then every political rule is a form
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-210/. The "US-210 ..." tests exist and compile; they run at X-M13.
Manual checks in docs/plans/US-210.md listed, to be run at X-M13.
</verification>
<teach_back>C++ concept for the owner: Modelling relations between groups.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-211 Alliances and vassal oaths
```xml
<prompt id="S-US-211" codex="2.1" milestone="M13" story="US-211" priority="Must" size="L">
<context>
Story US-211: Alliances and vassal oaths.
As the player, I want to ally with clans, take tribute and make them swear vassal oaths, which can break, so that I can bind the region to my clan.
Epic E20 Politics: Clans ally, swear and break oaths, marry, vote and challenge leaders; the player can win the region through Politics.
Traces to: PIL-03, PIL-08.
</context>
<dependencies>
Stories that must be Done: US-210.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Alliance and oath proposals between clans (AI and hero), tribute each season in goods, oath breaking by stance and chance from the seeded stream, chronicle lines with reasons.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Alliance">
Given a rival with a good stance
When the hero proposes an alliance
Then it is accepted or refused with a reason
</scenario>
<scenario name="Oath">
Given an allied clan that is weaker and indebted
When the hero asks for a vassal oath
Then it swears and pays tribute each season
</scenario>
<scenario name="Break">
Given a vassal whose stance falls below its limit
When a season passes
Then it may break the oath, and the chronicle says why
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-211/. The "US-211 ..." tests exist and compile; they run at X-M13.
Manual checks in docs/plans/US-211.md listed, to be run at X-M13.
</verification>
<teach_back>C++ concept for the owner: Contracts as data with conditions.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-212 Elders' council
```xml
<prompt id="S-US-212" codex="2.1" milestone="M13" story="US-212" priority="Must" size="M">
<context>
Story US-212: Elders' council.
As the player, I want clan decisions voted by the elders and to sway them, so that leading a clan means persuading people.
Epic E20 Politics: Clans ally, swear and break oaths, marry, vote and challenge leaders; the player can win the region through Politics.
Traces to: PIL-08, SDC-02.
</context>
<dependencies>
Stories that must be Done: US-210.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Elders' council for clan decisions (move camp, war, sharing, alliances): votes from needs, traits and opinions; the hero sways votes through dialogue choices and gifts; results in the chronicle.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Vote">
Given a decision to move camp
When the council meets
Then each elder votes by needs, traits and opinion, and the result is shown
</scenario>
<scenario name="Sway">
Given the hero talks to an elder before the vote
When a persuasion choice succeeds
Then that elder's vote changes
</scenario>
<scenario name="Record">
Given a council decision
When it is taken
Then the chronicle records it with the votes
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-212/. The "US-212 ..." tests exist and compile; they run at X-M13.
Manual checks in docs/plans/US-212.md listed, to be run at X-M13.
</verification>
<teach_back>C++ concept for the owner: Simple voting rules and weights.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-213 Marriage ties
```xml
<prompt id="S-US-213" codex="2.1" milestone="M13" story="US-213" priority="Must" size="M">
<context>
Story US-213: Marriage ties.
As the player, I want marriages between clans to bind them, so that love and politics meet.
Epic E20 Politics: Clans ally, swear and break oaths, marry, vote and challenge leaders; the player can win the region through Politics.
Traces to: PIL-08, SDC-02.
</context>
<dependencies>
Stories that must be Done: US-210, US-113.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Inter-clan marriages from the M2b courtship system create marriage ties that raise stance and weigh alliance answers; parting weakens them.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Tie">
Given a courtship between two clans' members
When they marry
Then the clans' stance rises and a marriage tie is recorded
</scenario>
<scenario name="Alliance">
Given a marriage tie
When an alliance is proposed
Then acceptance is more likely
</scenario>
<scenario name="Parting">
Given a married couple parts
When it happens
Then the tie weakens and the stance falls
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-213/. The "US-213 ..." tests exist and compile; they run at X-M13.
Manual checks in docs/plans/US-213.md listed, to be run at X-M13.
</verification>
<teach_back>C++ concept for the owner: Events that affect two systems.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-214 Leadership challenges
```xml
<prompt id="S-US-214" codex="2.1" milestone="M13" story="US-214" priority="Must" size="M">
<context>
Story US-214: Leadership challenges.
As the player, I want leaders to be challenged when their people turn against them, and to challenge or back a leader myself, so that power can change hands.
Epic E20 Politics: Clans ally, swear and break oaths, marry, vote and challenge leaders; the player can win the region through Politics.
Traces to: PIL-08.
</context>
<dependencies>
Stories that must be Done: US-212.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Leadership: challenges when the clan's opinion of the leader falls below the limit, decided by council or contest rules from politics.json; contested succession on a leader's death; the hero can challenge or back a leader.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Challenge">
Given a leader whose clan opinion is below the limit
When time passes
Then a challenger rises and the council or a contest decides
</scenario>
<scenario name="Hero">
Given the hero with enough support
When the hero challenges the leader
Then the outcome follows the rules and the chronicle tells it
</scenario>
<scenario name="Succession">
Given a leader dies
When it happens
Then succession is contested between candidates with reasons
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-214/. The "US-214 ..." tests exist and compile; they run at X-M13.
Manual checks in docs/plans/US-214.md listed, to be run at X-M13.
</verification>
<teach_back>C++ concept for the owner: State transitions with guards.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-215 Economic and technological levers
```xml
<prompt id="S-US-215" codex="2.1" milestone="M13" story="US-215" priority="Must" size="M">
<context>
Story US-215: Economic and technological levers.
As the player, I want trade pacts, tribute in goods, embargoes and sharing crafts with allies as political tools, so that wealth and know-how buy influence.
Epic E20 Politics: Clans ally, swear and break oaths, marry, vote and challenge leaders; the player can win the region through Politics.
Traces to: PIL-08, PIL-02.
</context>
<dependencies>
Stories that must be Done: US-211, US-062.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Levers: trade pacts (barter terms), tribute in goods, embargoes, and teaching crafts and recipes to allies (uses the M5 recipes and the US-222 teaching once M14 lands; here, recipes only), each moving stance and dependence as politics.json says.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Pact">
Given two allied clans
When a trade pact is made
Then barter between them is cheaper and stance rises
</scenario>
<scenario name="Embargo">
Given a rival under embargo
When it tries to trade with the hero's allies
Then it is refused and stance falls
</scenario>
<scenario name="Know-how">
Given the hero teaches an ally a recipe
When it happens
Then the ally can craft it and its stance and dependence rise
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-215/. The "US-215 ..." tests exist and compile; they run at X-M13.
Manual checks in docs/plans/US-215.md listed, to be run at X-M13.
</verification>
<teach_back>C++ concept for the owner: Interfaces between subsystems.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-216 Political victory and diplomacy
```xml
<prompt id="S-US-216" codex="2.1" milestone="M13" story="US-216" priority="Must" size="M">
<context>
Story US-216: Political victory and diplomacy.
As the player, I want to win the region through Politics and to do diplomacy in conversation, so that Politics is a full path to victory.
Epic E20 Politics: Clans ally, swear and break oaths, marry, vote and challenge leaders; the player can win the region through Politics.
Traces to: PIL-03, PIL-08, MVP-08, MVP-09.
</context>
<dependencies>
Stories that must be Done: US-211, US-195, US-161.
Owner decisions that must be Decided: D-40, D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Politics in the dominion screen and the end-of-game rules (thresholds from Game Rules); diplomacy choices (alliance, oath, pact, embargo) offered in conversations with clan leaders through dialogue effects.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Victory">
Given vassals holding 60% of the region's people (Game Rules value)
When the share is reached
Then the game ends in a Political victory with the chronicle
</scenario>
<scenario name="Combined">
Given Trade, Religion and Politics shares
When they reach the combined threshold
Then victory follows the Game Rules
</scenario>
<scenario name="Diplomacy">
Given a clan leader
When the hero talks to them
Then alliance, oath, pact and embargo choices are offered in the conversation
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-216/. The "US-216 ..." tests exist and compile; they run at X-M13.
Manual checks in docs/plans/US-216.md listed, to be run at X-M13.
</verification>
<teach_back>C++ concept for the owner: Combining scores from several systems.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M13" codex="2.1" name="Exit review M13">
<instructions>
1. Run the deferred tests (D-41): `pwsh tools/verify.ps1 -Story X-M13` on qa (Debug, every test), then push qa and wait for CI (Release, every test). Fix every failure in the code the failing test covers, one commit per fix; change a test only when it is provably wrong about the requirements, and list each such change with its reason. Run every story's manual checks from docs/plans/US-xxx.md.
   Also run `pwsh tools/verify.ps1 -Story X-M13 -Config Release` on the owner's PC: every Release test with the strict 3-second first-frame limit, which GitHub's GPU-less runners only check at 10 seconds (D-47); record the result in docs/gates/M13.md.
2. Demonstrate the exit criteria: Clans form alliances and vassal oaths, hold elders' councils, bind themselves by marriage, challenge leaders and use trade and craft knowledge as levers; the player can win the region through Politics; every political rule is editable in the Editor.
3. Collect evidence into docs/gates/M13.md, one section per criterion, met or not met. Also: a 10-seed headless balance check reports how often and how fast each victory happens with Politics on; the owner gets the table. List every decision Dominus took as delegated in this milestone, for the owner to review.
4. If all are met and CI on qa is green: merge qa into main, push, confirm CI on main is green, tag the repository m13-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the exit result and the test-run table.</output_format>
</prompt>
```

### M14 Technology
Exit criteria: Clans research by doing in workshops with inventors, pass knowledge by teaching, steal and guard secrets, and unlock recipes, interactions and professions from an editable tech tree; the player can win by forging the Ember Strand while leading in known technologies.

Why this milestone exists: the owner added Technology as the fourth pillar (D-41, PIL-09): an editable tech tree, research by doing, workshops, inventors, teaching, espionage and the Ember Strand. The design notes under M10 apply.

```xml
<prompt id="K-M14" codex="2.1" name="Kick off M14 Technology">
<instructions>
1. Confirm that M13 (docs/gates/M13.md) and its stories are done (for M10: X-M9 also ran its full verification and CI), and that D-40 and D-41 are Decided in docs/decisions.md.
2. Architect: write docs/plans/M14-technology-design.md (tech registry and unlocks, research accumulation, workshops and teaching, theft and secrets, rival priorities, Ember chain, victory and balance check) before the first story. Every design question it meets, Dominus decides (D-41) and records as delegated.
3. Set this milestone's stories to To do in docs/status.md in this order: US-220, US-221, US-226, US-222, US-224, US-223, US-225.
4. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, delegated decisions.</output_format>
</prompt>
```

#### S-US-220 The tech tree as data
```xml
<prompt id="S-US-220" codex="2.1" milestone="M14" story="US-220" priority="Must" size="L">
<context>
Story US-220: The tech tree as data.
As the owner, I want Age 1 technologies with prerequisites, discovery rules and unlocks in a data file shown as a tree in the Editor, so that I can design how know-how grows.
Epic E21 Technology: Clans discover, teach, guard and steal technologies; the player can win the region by forging the Ember Strand while leading in know-how.
Traces to: PIL-05, PIL-09, EDT-02.
</context>
<dependencies>
Stories that must be Done: US-191, US-170.
Owner decisions that must be Decided: D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Simulation: assets/data/sim/technologies.json and research.json (brief section 10) with schemas; the tech registry with unlocks applied to recipes, interactions, professions and objects; Game: a Tech tree view in the Editor on the M9 node graph.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Load">
Given technologies.json with fire mastery, hafted spears, sewing and ochre
When the game starts
Then each technology's prerequisites and unlocks (recipes, interactions, professions, objects) are known
</scenario>
<scenario name="Editor">
Given the tech tree view
When the owner adds a technology and links it after another
Then the tree and the saved file show it
</scenario>
<scenario name="Cycle">
Given two technologies that require each other
When validation runs
Then the cycle is listed
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-220/. The "US-220 ..." tests exist and compile; they run at X-M14.
Manual checks in docs/plans/US-220.md listed, to be run at X-M14.
</verification>
<teach_back>C++ concept for the owner: Directed acyclic graphs and topological order.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-221 Research by doing
```xml
<prompt id="S-US-221" codex="2.1" milestone="M14" story="US-221" priority="Must" size="M">
<context>
Story US-221: Research by doing.
As the player, I want my clan to discover technologies by using skills and materials, so that progress grows out of daily life.
Epic E21 Technology: Clans discover, teach, guard and steal technologies; the player can win the region by forging the Ember Strand while leading in know-how.
Traces to: PIL-09.
</context>
<dependencies>
Stories that must be Done: US-220.
Owner decisions that must be Decided: D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Simulation: research points per clan and technology from interactions and skills tagged in `discover.by`; discovery when points reach the threshold and prerequisites are known; chronicle lines naming the discoverer.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Discover">
Given a clan that knaps flint every day
When enough research has built up
Then the next flint technology is discovered and its recipes unlock
</scenario>
<scenario name="Chronicle">
Given a discovery
When it happens
Then the chronicle says who discovered it and how
</scenario>
<scenario name="Locked">
Given a technology whose prerequisite is unknown
When the work happens
Then it is not discovered
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-221/. The "US-221 ..." tests exist and compile; they run at X-M14.
Manual checks in docs/plans/US-221.md listed, to be run at X-M14.
</verification>
<teach_back>C++ concept for the owner: Accumulators and thresholds.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-226 The technology screen
```xml
<prompt id="S-US-226" codex="2.1" milestone="M14" story="US-226" priority="Must" size="M">
<context>
Story US-226: The technology screen.
As the player, I want a screen with what my clan knows, what is next and what each discovery unlocks, so that I can plan research.
Epic E21 Technology: Clans discover, teach, guard and steal technologies; the player can win the region by forging the Ember Strand while leading in know-how.
Traces to: PIL-09, PIL-07.
</context>
<dependencies>
Stories that must be Done: US-220.
Owner decisions that must be Decided: D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Game: Technology screen (Luna UI) with known, reachable and locked technologies as a tree, details, progress, unlocks and who else knows a secret.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Tree">
Given a clan with some technologies
When the player opens the screen
Then known, reachable and locked technologies are shown as a tree
</scenario>
<scenario name="Detail">
Given a technology selected
When it is shown
Then its discovery rule, progress and unlocks are listed
</scenario>
<scenario name="Secrets">
Given a technology marked secret
When it is shown
Then who else is known to have it is listed
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-226/. The "US-226 ..." tests exist and compile; they run at X-M14.
Manual checks in docs/plans/US-226.md listed, to be run at X-M14.
</verification>
<teach_back>C++ concept for the owner: Laying out a tree for reading.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-222 Workshops, inventors and teaching
```xml
<prompt id="S-US-222" codex="2.1" milestone="M14" story="US-222" priority="Must" size="M">
<context>
Story US-222: Workshops, inventors and teaching.
As the player, I want workshops and inventive people to speed research and teaching to spread knowledge, so that who works where matters.
Epic E21 Technology: Clans discover, teach, guard and steal technologies; the player can win the region by forging the Ember Strand while leading in know-how.
Traces to: PIL-09, US-215.
</context>
<dependencies>
Stories that must be Done: US-221, US-155.
Owner decisions that must be Decided: D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Workshop factor per object kind, inventive trait bonus and solo discovery, teaching as an interaction between members of allied clans (completes the US-215 craft-sharing lever for technologies).
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Workshop">
Given knapping at a knapping stone versus in the open
When a day passes
Then the workshop gives more research, by the data's factor
</scenario>
<scenario name="Inventor">
Given an NPC with the inventive trait
When they work
Then they add research faster and can discover alone
</scenario>
<scenario name="Teach">
Given a known technology
When a member teaches another clan's member (an ally)
Then that clan knows it after the teaching time
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-222/. The "US-222 ..." tests exist and compile; they run at X-M14.
Manual checks in docs/plans/US-222.md listed, to be run at X-M14.
</verification>
<teach_back>C++ concept for the owner: Modifiers that stack in a defined order.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-224 Rival research and the Technology share
```xml
<prompt id="S-US-224" codex="2.1" milestone="M14" story="US-224" priority="Must" size="M">
<context>
Story US-224: Rival research and the Technology share.
As the player, I want rival clans to research too and to see who leads in know-how, so that Technology is a race.
Epic E21 Technology: Clans discover, teach, guard and steal technologies; the player can win the region by forging the Ember Strand while leading in know-how.
Traces to: PIL-09, ENV-10.
</context>
<dependencies>
Stories that must be Done: US-221.
Owner decisions that must be Decided: D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Rival clans research by their priorities from research.json; the Technology share per clan; Technology in the dominion screen.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Rivals">
Given two rival clans
When a year passes
Then each has discovered technologies by its own priorities
</scenario>
<scenario name="Share">
Given the dominion screen
When the player opens it
Then Technology shows each clan's share of the Age 1 technologies known
</scenario>
<scenario name="Data">
Given rival priorities
When the owner edits them in forms
Then rivals follow the new priorities
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-224/. The "US-224 ..." tests exist and compile; they run at X-M14.
Manual checks in docs/plans/US-224.md listed, to be run at X-M14.
</verification>
<teach_back>C++ concept for the owner: Comparing scores across actors.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-223 Espionage, theft and secrets
```xml
<prompt id="S-US-223" codex="2.1" milestone="M14" story="US-223" priority="Must" size="M">
<context>
Story US-223: Espionage, theft and secrets.
As the player, I want rivals to steal or copy know-how and to guard my own, so that knowledge is power to protect.
Epic E21 Technology: Clans discover, teach, guard and steal technologies; the player can win the region by forging the Ember Strand while leading in know-how.
Traces to: PIL-09, PIL-08.
</context>
<dependencies>
Stories that must be Done: US-222, US-210.
Owner decisions that must be Decided: D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: Theft by observation and trade with chances from research.json and the seeded stream; secret technologies and secret-keepers; caught spies lower stance with reasons.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Steal">
Given a rival watching our workshop or trading with us
When time passes
Then it may learn one of our technologies, and stance falls if caught
</scenario>
<scenario name="Guard">
Given a technology marked secret
When a rival tries to learn it
Then the chance is lower and secret-keepers can catch the spy
</scenario>
<scenario name="Repeat">
Given the same seed and inputs
When two runs
Then the same thefts happen
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-223/. The "US-223 ..." tests exist and compile; they run at X-M14.
Manual checks in docs/plans/US-223.md listed, to be run at X-M14.
</verification>
<teach_back>C++ concept for the owner: Probabilities from a seeded stream.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

#### S-US-225 The Ember Strand and Technology victory
```xml
<prompt id="S-US-225" codex="2.1" milestone="M14" story="US-225" priority="Must" size="L">
<context>
Story US-225: The Ember Strand and Technology victory.
As the player, I want to forge the Ember Strand at the end of a long chain of discoveries and win when I hold it and lead in know-how, so that Technology is a full path to victory.
Epic E21 Technology: Clans discover, teach, guard and steal technologies; the player can win the region by forging the Ember Strand while leading in know-how.
Traces to: PIL-05, PIL-09, MVP-08, MVP-09.
</context>
<dependencies>
Stories that must be Done: US-224, US-195, US-062.
Owner decisions that must be Decided: D-41.
</dependencies>
<instructions>
Run the Mraw build loop L-01 for this story only (with the M10-M14 exceptions, D-41).
Where the work belongs: The Ember chain (technologies + a recipe forged at the sacred fire), the Technology victory and the four-pillar combined rule in Game Rules; chronicle for a rival forging Ember.
Follow the formats in the brief docs/plans/M10-M13-authoring-brief.md and the milestone design document; Dominus decides any open design detail and records it as delegated.
Scope is exactly the acceptance criteria below; anything else is a new story, not part of this one.
Completion: the story is complete when its code and tests are written, the Debug build has zero warnings, the Definition of Done holds as the Charter's M10-M14 exception states, and it is merged into qa; a progress summary is not completion.
</instructions>
<acceptance_criteria>
<scenario name="Chain">
Given the Ember chain in technologies.json and recipes.json
When the clan discovers every step and gathers the materials
Then the Ember Strand can be forged at the sacred fire
</scenario>
<scenario name="Victory">
Given the Ember Strand held and a Technology share at or above the Game Rules value (60%)
When both are true
Then the game ends in a Technology victory with the chronicle
</scenario>
<scenario name="Rival forges">
Given a rival forges Ember first
When it happens
Then the chronicle tells it and the player can still win another way or take it
</scenario>
</acceptance_criteria>
<definition_of_done>Charter definition_of_done (M10-M14 exception), plus: every new data field has a schema entry and a guide line with an example; round-trip tests for every new format.</definition_of_done>
<verification>
The Debug build with zero warnings (CI builds Release after the merge, D-46); build log in docs/evidence/US-225/. The "US-225 ..." tests exist and compile; they run at X-M14.
Manual checks in docs/plans/US-225.md listed, to be run at X-M14.
</verification>
<teach_back>C++ concept for the owner: Long multi-step goals as data.</teach_back>
<stop_conditions>Build loop L-01 stop conditions; a scope change this Codex does not cover (codex issue).</stop_conditions>
<output_format>Assembly report (Charter report_format).</output_format>
</prompt>
```

```xml
<prompt id="X-M14" codex="2.1" name="Exit review M14">
<instructions>
1. Run the deferred tests (D-41): `pwsh tools/verify.ps1 -Story X-M14` on qa (Debug, every test), then push qa and wait for CI (Release, every test). Fix every failure in the code the failing test covers, one commit per fix; change a test only when it is provably wrong about the requirements, and list each such change with its reason. Run every story's manual checks from docs/plans/US-xxx.md.
   Also run `pwsh tools/verify.ps1 -Story X-M14 -Config Release` on the owner's PC: every Release test with the strict 3-second first-frame limit, which GitHub's GPU-less runners only check at 10 seconds (D-47); record the result in docs/gates/M14.md.
2. Demonstrate the exit criteria: Clans research by doing in workshops with inventors, pass knowledge by teaching, steal and guard secrets, and unlock recipes, interactions and professions from an editable tech tree; the player can win by forging the Ember Strand while leading in known technologies.
3. Collect evidence into docs/gates/M14.md, one section per criterion, met or not met. Also: a 10-seed headless balance check covers all four pillars. Then X-M6 (kill gate 2) is next: end the session after this report, because it needs people. List every decision Dominus took as delegated in this milestone, for the owner to review.
4. If all are met and CI on qa is green: merge qa into main, push, confirm CI on main is green, tag the repository m14-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the exit result and the test-run table.</output_format>
</prompt>
```

### M6 Playtest and go/no-go (KILL GATE 2)
Exit criteria: 8 outside playtesters play; success criteria measured; go/no-go decision recorded.

```xml
<prompt id="K-M6" codex="2.0" name="Kick off M6 Playtest and go/no-go (KILL GATE 2)">
<instructions>
1. Confirm the previous milestone's exit review exists in docs/gates/ and passed: since v2.1 that is M14 (docs/gates/M14.md, D-41). M6's stories US-090..US-092 are already Done; if so, go straight to X-M6.
2. Read docs/decisions.md. For every decision this milestone needs (D-11) that is not Decided, write its decision request now, all at once, so the owner can answer them in one sitting.
3. Set this milestone's stories to To do in docs/status.md in this order: US-090, US-091, US-092.
4. Continue with the first story prompt.
</instructions>
<output_format>Short kickoff note in the assembly report: milestone goal, stories, decisions requested.</output_format>
</prompt>
```

#### S-US-090 Learn the game in the first ten minutes
```xml
<prompt id="S-US-090" codex="1.8" milestone="M6" story="US-090" priority="Should" size="M">
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
<prompt id="S-US-091" codex="1.8" milestone="M6" story="US-091" priority="Must" size="S">
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
<prompt id="S-US-092" codex="1.8" milestone="M6" story="US-092" priority="Should" size="S">
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
<prompt id="X-M6" codex="1.8" name="Exit review M6">
<instructions>
0. Run the playtest exactly as docs/plans/M6-playtest-plan.md says (session script, interview, evidence table, privacy); agents prepare the package and the evidence template, the owner runs the sessions.
1. Demonstrate the exit criteria: 8 outside playtesters play; success criteria measured; go/no-go decision recorded.
2. Collect evidence (test output, headless run logs, FPS logs, screenshots) into docs/gates/M6.md, one section per criterion, each marked met or not met.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m6-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
4. This is a kill gate. Evaluate each success criterion below with evidence and write a recommendation (go / pivot / stop) with reasons. Criteria that need people (readers, playtesters) cannot be measured by agents: write a decision request D-GATE-M6 asking the owner for the result, and treat his answer as the gate decision. Until the owner answers, do not start the next milestone: end the session with the assembly report and a Milestone file.
- Voluntary play time: 5 of 8 playtesters play 30+ minutes without being asked to continue (measured by: Observation + local session log (US-092))
- Emergent story: 3 of 8 can retell a story that came from the simulation, not from a script (measured by: Post-play interview)
- Stability: No crash in a 2-hour play session or a 100-year soak test (measured by: Soak test + crash log)
- Kill / pivot rule: If M2 or M6 fails: stop adding content; redesign the simulation or the loop first (measured by: Owner decision, recorded)
</instructions>
<output_format>Assembly report with the gate result.</output_format>
</prompt>
```

### Execution order
P-000 -> P-001 -> P-002 -> P-003 -> P-004 -> P-005 -> P-006 -> P-007 -> P-008 -> P-009 -> P-010 -> P-011 -> P-012 -> P-013 -> K-M0 -> S-US-001 -> S-US-002 -> S-US-003 -> S-US-004 -> X-M0 -> K-M1 -> S-US-020 -> S-US-021 -> S-US-022 -> S-US-023 -> S-US-024 -> X-M1 -> K-M1b -> S-US-025 -> S-US-026 -> S-US-027 -> S-US-028 -> S-US-029 -> X-M1b -> K-M2 -> S-US-010 -> S-US-011 -> S-US-012 -> S-US-013 -> S-US-014 -> S-US-015 -> S-US-016 -> X-M2 -> K-M2b -> S-US-110 -> S-US-111 -> S-US-112 -> S-US-113 -> S-US-114 -> S-US-115 -> X-M2b -> K-M2c -> S-US-120 -> S-US-121 -> S-US-122 -> S-US-123 -> S-US-124 -> S-US-125 -> S-US-126 -> X-M2c -> K-M2d -> S-US-130 -> S-US-131 -> S-US-132 -> S-US-133 -> S-US-134 -> S-US-135 -> S-US-139 -> S-US-140 -> S-US-141 -> S-US-136 -> S-US-137 -> S-US-138 -> X-M2d -> K-M3 -> S-US-030 -> S-US-032 -> S-US-031 -> X-M3 -> K-M4 -> S-US-040 -> S-US-041 -> S-US-042 -> S-US-043 -> S-US-080 -> S-US-083 -> X-M4 -> K-M5 -> S-US-050 -> S-US-053 -> S-US-051 -> S-US-052 -> S-US-054 -> S-US-060 -> S-US-061 -> S-US-062 -> S-US-063 -> S-US-070 -> S-US-055 -> S-US-071 -> S-US-072 -> S-US-073 -> S-US-081 -> S-US-082 -> X-M5 -> K-M7 -> S-US-150 -> S-US-151 -> S-US-156 -> S-US-152 -> S-US-153 -> S-US-155 -> S-US-154 -> X-M7 -> K-M8 -> S-US-160 -> S-US-161 -> S-US-162 -> S-US-163 -> S-US-164 -> S-US-165 -> X-M8 -> K-M8b -> S-US-230 -> S-US-231 -> S-US-232 -> S-US-233 -> S-US-234 -> X-M8b -> K-M8c -> S-US-240 -> S-US-241 -> S-US-242 -> S-US-243 -> S-US-248 -> S-US-244 -> S-US-245 -> S-US-246 -> S-US-247 -> X-M8c -> K-M9a -> S-US-260 -> S-US-261 -> S-US-262 -> S-US-263 -> S-US-264 -> S-US-265 -> S-US-266 -> S-US-267 -> S-US-268 -> S-US-269 -> S-US-270 -> X-M9a -> K-M9b -> S-US-280 -> S-US-281 -> S-US-282 -> S-US-283 -> S-US-284 -> X-M9b -> K-M9c -> S-US-290 -> S-US-291 -> S-US-292 -> S-US-293 -> S-US-294 -> X-M9c -> K-M8d -> S-US-250 -> S-US-251 -> S-US-252 -> S-US-256 -> X-M8d -> K-M8e -> S-US-253 -> S-US-254 -> S-US-255 -> S-US-257 -> X-M8e -> K-M9 -> S-US-170 -> S-US-171 -> S-US-172 -> S-US-175 -> S-US-173 -> S-US-174 -> X-M9 -> K-M10 -> S-US-180 -> S-US-181 -> S-US-182 -> S-US-183 -> S-US-186 -> S-US-184 -> S-US-187 -> S-US-185 -> X-M10 -> K-M10b -> S-US-300 -> S-US-301 -> S-US-302 -> S-US-303 -> S-US-304 -> S-US-305 -> X-M10b -> K-M11 -> S-US-190 -> S-US-191 -> S-US-193 -> S-US-194 -> S-US-195 -> S-US-196 -> S-US-192 -> X-M11 -> K-M12 -> S-US-200 -> S-US-201 -> S-US-202 -> S-US-203 -> S-US-204 -> S-US-205 -> S-US-206 -> S-US-207 -> X-M12 -> K-M13 -> S-US-210 -> S-US-211 -> S-US-212 -> S-US-213 -> S-US-214 -> S-US-215 -> S-US-216 -> X-M13 -> K-M14 -> S-US-220 -> S-US-221 -> S-US-226 -> S-US-222 -> S-US-224 -> S-US-223 -> S-US-225 -> X-M14 -> K-M6 -> S-US-090 -> S-US-091 -> S-US-092 -> X-M6

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
| Limit.md | The resume point: what is done, the next prompt, any paused work (branch and what is left), how to continue. Updated after every story and before any planned stop. Read right after CLAUDE.md. | mraw-orchestrator |
| tools/verify.ps1 | The tester's standard verification run (L-01 step 7) and its evidence. | mraw-tester |
| Milestone.md, Milestone-<n>.md | Progress snapshots AP-###, one new file after every story and at milestone exits; newest file has the latest state. | mraw-orchestrator |
| CHANGELOG.md | Every change set with verification evidence, updated before each push or merge. | mraw-writer |
| docs/evidence/US-xxx/ | Raw logs proving acceptance criteria. | mraw-tester |
| docs/reports/ | Assembly reports and checkpoints, one per session or story. | mraw-orchestrator |
| docs/guides/interaction-data.md, docs/guides/dialogue-format.md | The owner's reference for every field, condition function and effect verb; updated in the same commit as any format change (from US-150, US-160). | mraw-writer |
| docs/guides/quests.md, schemas.md, world-editing.md, politics.md, technology.md | The owner's reference for each M10-M14 format; updated in the same commit as any format change. | mraw-writer |
| docs/guides/lighting.md, docs/guides/buildings.md | The owner's reference for light, sky, shadow and building data (M8b-M8e). | mraw-writer |
| docs/adr/ADR-021-sdl-gpu-renderer.md | The renderer decision and the shader compiler (US-230). | mraw-architect |
| assets/worlds/ | World files: a seed, generator settings and the owner's overrides (M12). | Owner (Editor) |
| docs/gates/test-debt.md | The P-009 run that paid the M2d-M6 test debt. | mraw-tester |
| AGENTS.md | Owner rules for any AI agent (for example the changelog rule); points to CLAUDE.md. | Owner |

## 9. Amendment log
| Version | Date | Change |
|---|---|---|
| 1.0 | 2026-09-29 | First Codex, from Mraw's Build Brief (source of truth v1.2). |
| 1.1 | 2026-09-29 | Aligned with Anima's canonical Codex format: verification section and completion condition in every story prompt, state files section, target models. US-053 no longer depends on US-051 (removed a dependency cycle). |
| 1.2 | 2026-09-29 | Luna first (source of truth v1.4, ARC-09): M1 is now the Luna engine walking skeleton (K-M1, S-US-020..S-US-024, X-M1) and M2 the console clan simulator with Kill Gate 1 (K-M2, S-US-010..S-US-016, X-M2); Platform and Engine layers live in src/luna/ (targets luna_platform and luna_engine, namespaces luna::platform and luna::engine); Charter rule 9 keeps Luna game-agnostic; US-003 gains the "Luna stays game-agnostic" scenario; US-024 is Game code that uses Luna; new P-001 migrates v1.1 workspaces. Codex issues resolved: CI-001 (P-000 commits before the toolchain check), CI-002 (no split needed, D-12 Decided 2026-09-29), CI-003 (DoD: CI green from US-002 on, determinism from US-010 on; M0 and M1 verification no longer ask for the determinism test). Owner decisions recorded: D-04 (32x48 px, 8 directions), D-12, D-13. |
| 1.3 | 2026-09-30 | Autonomous assembly with minimal owner intervention (owner instructions of 2026-09-30): Charter human gates reduced to people-dependent kill-gate results, accounts, credentials, money and destructive actions outside the repo; design decisions delegated to Dominus and recorded as "Decided by Dominus (delegated)"; stories branch from and merge into qa, qa merges into main at milestone exits; CHANGELOG.md updated per change set; Milestone-<n>.md snapshot after every story; ADR-016 header convention in coding standards; D-01 and D-03 Decided; new P-002 migrates v1.2 workspaces. Codex issues resolved: CI-004, CI-005. |
| 1.4 | 2026-09-30 | Luna Physics (requirements v1.5, owner decisions of 2026-09-30: own physics, in the MVP, full 3D math, right after M1): Charter now has six layers (Physics between Core and Engine/Simulation) and rule 10 (deterministic fixed-point 32.32 physics, SI units, 1 tile = 1 m, textbook-tested); new milestone M1b with K-M1b, S-US-025..S-US-029 (3D math, hit detection, ballistics, rigid bodies, spear throw in the demo) and X-M1b; P-003 migrates v1.3 workspaces (US-011 paused on its branch); D-02 recorded as delegated; D-15 chain includes M1b. |
| 1.5 | 2026-09-30 | Assembly instructions (owner requests of 2026-09-30): Limit.md is a state file and the resume point, updated after every story and before any stop, read by A-001; L-01 step 2 continues paused story branches; step 7 uses tools/verify.ps1; step 11 updates Limit.md; the Charter says how to stop safely at usage limits; section 0 states that Dominus designs, implements and tests while Anima alone writes the Codex, and that assembly is continuous; new P-004. |
| 1.6 | 2026-09-30 | Kill Gate 1 pivot (source of truth v1.6, Round 9): the owner judged the M2 chronicle "not really a story" (D-GATE-M2: Pivot) and decided the redesign (D-18). New milestone M2b Story engine between M2 and M3 with K-M2b, S-US-110..S-US-115 (reasons for deaths and feuds; quarrels, blame and revenge; sharing, nursing and adoption; courtship, rivals and parting; apprentices and hunting parties; the story told in episodes) and X-M2b, the Kill Gate 1 retry judged by the owner alone; D-GATE-M2, D-18 and D-GATE-M2b in the decision table; D-15 chain includes M2b; new P-005 adopts v1.6. Codex issue resolved: CI-006 (the US-020 first-frame limit is 3 s in Release and 15 s in the Debug build, which AddressSanitizer slows on CI runners). The header and section 0 no longer name AI vendors or models (owner rule of 2026-09-30). |
| 1.7 | 2026-09-30 | Level editor (source of truth v1.7, Round 10): Kill Gate 1 passed (D-GATE-M2b: Go). Before M3 the owner asked for a level editor (D-19) with his own art (D-05 decided: own art, placeholder quality). New milestone M2c Level editor between M2b and M3 with K-M2c, S-US-120..S-US-126 (real art cut from the owner's sheets into atlases; pointer, font and widgets; levels as data; Game mode and Editor mode; painting ground tiles with undo and redo; placing characters with properties; level and character settings and a guide) and X-M2c (no kill gate); design notes for the milestone (layers, art pipeline, stb_image and ADR-018, data files, tests, UX); D-15 chain includes M2c; new P-006 adopts v1.7. Built from Mraw's brief docs/plans/M2c-editor-brief.md. |
| 1.8 | 2026-09-30 | Content and combat (source of truth v1.8, Round 11): M2c done. The owner added seven sprite sheets and took every design decision in six chat rounds (D-21). New milestone M2d Content and combat between M2c and M3 with K-M2d, S-US-130..S-US-138 (content catalogs; hero HP, strike-back and death; effect player; eight weapon classes and a 16-weapon starter set; pickups and a 9-slot hotbar with level format version 2; elements; plants; animals in the Editor; placed effects and random weather) and X-M2d (ends by stopping for the owner before K-M3); design notes for the milestone. Charter: human gate 3, design decisions are the owner's (D-22), replacing delegation to Dominus. D-15 chain includes M2d; new P-007 adopts v1.8. Built from Mraw's brief docs/plans/M2d-content-brief.md. |
| 1.9 | 2026-10-01 | Aiming and ballistics (source of truth v1.9): the owner asked, before plants, to aim weapons with the mouse, to have ballistics and several shootable ranged weapons (D-25, two chat rounds). Three stories S-US-139 (mouse aiming), S-US-140 (arc ballistics with Luna Physics), S-US-141 (bows, crossbows, thrown weapons and staff bolts, with a shooting-range level) are added to M2d between S-US-135 and S-US-136; K-M2d sets the new order; design notes for the milestone; D-25 in the decision table; new P-008 adopts v1.9. Built from Mraw's brief docs/plans/M2d-aiming-brief.md. |
| 2.0 | 2026-10-01 | World interactions, dialogue and their editor (source of truth v2.0, Round 13; Mraw's brief docs/plans/M7-M9-interactions-brief.md). The owner decided D-34 (plain-text .dlg dialogue + JSON interactions; hybrid talk; full visual graph editor plus F5 hot reload; build first, kill gate 2 after) and, with Anima, D-35 (design questions to the owner at each kickoff; P-009 pays the test debt, then per-story verification and CI; the seven proposed world objects; the dialogue panel pauses). New milestones before M6: M7 World interactions (K-M7, S-US-150..S-US-156, X-M7), M8 Speak to NPCs (K-M8, S-US-160..S-US-165, X-M8), M9 Interaction and dialogue editor (K-M9, S-US-170..S-US-175, X-M9), with shared design notes; K-M6 now follows X-M9; D-08 Decided, D-15 chain, D-34 and D-35 in the decision table; new state files (format guides, test-debt record); new P-009 adopts v2.0 and pays the test debt. L-01 step 1 corrected: an undecided D-xx goes to the owner (Charter human gate 3), no longer to Dominus; this contradiction existed since v1.8. Section 0 records that the delegated period of D-30..D-33 ended with M6. |
| 2.1 | 2026-10-01 | Authoring tools, Politics and Technology (source of truth v2.2, Rounds 14 and 15; Mraw's brief docs/plans/M10-M13-authoring-brief.md). The owner decided D-40 (M10 Quests and story authoring, M11 Data editors, M12 World editing, M13 Politics) and, with Anima, D-41 (Technology as the fourth pillar in M14; Politics victory confirmed; Game Rules files; for M10-M14 Dominus decides design questions as delegated and tests run at exit reviews; kill gate 2 after M14). New: P-010; K-M10..X-M14 with 37 story prompts (S-US-180..S-US-187, S-US-190..S-US-196, S-US-200..S-US-207, S-US-210..S-US-216, S-US-220..S-US-226) and shared design notes; exit reviews run the deferred tests and CI before merging into main. Charter: human gate 3 and Definition of Done gain the M10-M14 exceptions; L-01 steps 1, 7 and 10 follow them. K-M6 now follows X-M14; D-15 chain; D-40 and D-41 in the decision table; new state files (guides, assets/worlds/). Codex issues resolved: CI-007 (catalog hot reload joins US-191, which replaces raw pointers into catalogs with ids; dialogue reload as built in M8), CI-008 (world objects ride on the plant machinery; noted in the M12 design notes). |
| 2.2 | 2026-10-01 | Resolution, lighting and buildings (source of truth v2.4, Rounds 16 and 17; Mraw's brief docs/plans/M8b-M8d-render-light-build-brief.md). The owner decided D-42 (960x540 with window modes, camera zoom and UI scale; SDL_GPU shaders; sun, moon, fire, torch and effect lights, seasonal day length, weather light; shadows from the sun, the moon and nearby fires; generated normal maps; buildings from blueprints and pieces, built by the clans, with wear, repair, fire, interiors and Editor prefabs; right after M8) and, with Anima, D-43 (M8d split into M8d Buildings and M8e Building life; US-256 no longer waits for US-254; the D-35 assembly rules apply) and D-06 (a mid-range target PC). New: P-011; K-M8b..X-M8e with 21 story prompts (S-US-230..S-US-234, S-US-240..S-US-247, S-US-250..S-US-252, S-US-256, S-US-253..S-US-255, S-US-257) and shared design notes, between X-M8 and K-M9; K-M9 now follows X-M8e. Charter: architecture rule 11 (rendering). D-06 Decided, D-15 chain, D-42 and D-43 in the decision table; new state files (lighting and buildings guides, ADR-021). |
| 2.3 | 2026-10-01 | Faster verification (owner, D-46): the local check (`tools/verify.ps1`, new `-Config Debug|Release|Both`, default Debug) builds and tests Debug with AddressSanitizer; CI builds and tests Release on qa and both on main; CI runs only on pushes to qa and main, skips docs-only pushes, cancels superseded runs and caches built vcpkg libraries (`.github/workflows/ci.yml`). Charter Definition of Done, L-01 step 7, the M10-M14 notes and every prompt still To do (M8b onward) now say which configuration is checked where. No scope change. |
| 2.4 | 2026-10-01 | Completeness review (source of truth v2.6, Round 19, D-47). Codex issue CI-009 resolved: K-M8b now checks that M8 is done and K-M9 that M8e is done (v2.2 had swapped them by a text replacement). Every remaining exit review (X-M8b..X-M14, 10 prompts) runs `tools/verify.ps1 -Config Release` on the owner's PC for the strict 3-second first-frame check, which CI only checks at 10 s. Decision table: D-07, D-09, D-11 closed, D-10 superseded, D-47 added. No scope change. |
| 2.5 | 2026-10-01 | Remaining open items (source of truth v2.7, Round 20, D-48): K-M13 reminds the owner to start recruiting the eight playtesters (docs/plans/M6-playtest-plan.md, written by Dominus); X-M6 runs the playtest by that plan; D-14 and D-48 in the decision table. No scope change. |
| 2.6 | 2026-10-01 | Merge (source of truth v2.8). During US-231 Mraw found that S-US-231 contradicted the owner's M8b answers (CI-010, D-44) and aligned S-US-231 and S-US-232 itself on its story branch, labelled 2.4 there, while Anima published 2.4 and 2.5 in parallel. This version contains both: the D-44 alignment (four windowed sizes, Whole scaling with Fill in Settings, first-start defaults, zoom and UI scale controls) and every change of 2.4 and 2.5. CI-010 resolved. From now on Codex changes go through Anima (A-002), so versions stay in one line. |
| 2.7 | 2026-10-04 | Celestial bodies (Mraw's brief docs/plans/US-248-celestial-brief.md; owner decision D-50). New story prompt S-US-248 (sun and moon as placeable light-source objects with a clock orbit, sprites in the sky, eclipses as data events, Editor placement) before S-US-244; S-US-244 now depends on US-248 and D-50 and casts its shadows along the light direction of the celestial bodies; K-M8c lists the stories in the order US-240..US-243, US-248, US-244..US-247; the execution order line (section 7) gets S-US-248; D-49 and D-50 added to the decision table; L-01 verification lines say to rerun only failing cases after a failure. M8c now has nine stories. Source of truth: ENV-22 Celestial bodies must be added to the requirements (v2.9) and US-248 to the backlog before US-248 starts (Mraw to reconcile; Anima issue CI-011). |
| 2.8 | 2026-10-04 | NPC roles, talk and trade: first draft of milestone M9a (K-M9a, S-US-260..S-US-267, X-M9a) between X-M8e and K-M9, with the owner's 24 questions open (D-52). Superseded the same day by v2.9 before any prompt ran. |
| 2.9 | 2026-10-04 | NPC foundation, trade economy and NPC life (owner answers D-52, six chat rounds; Mraw's brief docs/plans/M9a-npc-roles-brief.md, revised). The owner's answers tripled the v2.8 scope, so M9a is split into three milestones built right after M8c and before M8d (D-52 Q-01, S-01): M9a NPC foundation (K-M9a, S-US-260..S-US-270: NPC Classes in the Editor, kind files and overrides, placed NPCs as full persons, the person store with detail by distance for 100,000 persons and ADR-022, nine attitudes per pair, talk, Confront, hidden actions and the Actions pop-up, Editor NPC panel, kinds tab and markers, test level), M9b Trade economy (K-M9b, S-US-280..S-US-284: owner-defined currencies, limited daily stock, supply and demand with reputation, one trade screen, Editor trade panel), M9c NPC life (K-M9c, S-US-290..S-US-294: schedules, class, custom and event actions with a quest hook for M10, NPC-to-NPC interactions, partner-type defaults, living test level and soak), each with an exit review; shared design notes. The v2.8 prompts S-US-260..S-US-267 are replaced (none had run). P-012 adopts v2.9; K-M8d also checks that M9c is done; D-52 Decided in the decision table; D-15 chain and execution order updated. Source of truth: epics E17-E19 and their stories must be added to the requirements and the backlog before K-M9a (Mraw to reconcile; Anima issue CI-012). |
| 2.10 | 2026-10-04 | Requirements reconciled (source of truth v2.10, Round 23): Mraw added the M9a-M9c work to Project Odyssey.docx and the backlog. Epic ids E17-E19 were already taken (M10-M12), so the NPC epics are E26 NPC foundation, E27 Trade economy and E28 NPC life; every M9a-M9c story prompt now traces to the new requirements SDC-08..SDC-12, INT-09..INT-12, EDT-08 and NFR-08 instead of placeholders; K-M9a step 2 checks the mirrored requirements version; P-012 adopts v2.10 and mirrors the requirements. CI-012 resolved before it was raised. Open for a later version: SDC-12 builds the schedules early, so S-US-196 (M11 routines) should reuse them; Anima amends S-US-196 when M11 comes closer. |
| 2.11 | 2026-10-04 | S-US-196 (M11 Daily routines as data) reuses the M9c schedules (US-290, SDC-12; owner request): routines are the US-290 schedule format per role and per person, the story adds tag weights per block for the utility scoring, routines for clan members by profession, a 24-hour timeline view in the US-290 schedule form and the schedule schema for the M11 editors; no routines.json and no second scheduler. New dependencies US-290, US-291 and D-52; traces add SDC-12; new scenario "One format". K-M11's design document covers routines on the M9c schedules. P-012 adopts v2.11. |
| 2.12 | 2026-10-05 | M9b and M9c kickoff answers (owner, D-54; amended in the repository copy by Mraw at the owner's order, to be adopted by Anima, CI-013). S-US-291 loses the quest-action hook (action sources are class, custom and event actions; M10 adds the quest hook and changes the action schema); S-US-293 names the defaults files assets/data/interactions/defaults-<type>.json and the four environment interactions; S-US-294 and X-M9c soak 30 in-game days (was 10) with the same save hash on two runs. The M9b trade design (currency items and a balance, item-value currencies, prices from base x stock curve x drift with reputation last, bands, Haggle, weighted daily restock) is in docs/plans/M9-npc-design.md and ADR-023. |
| 2.13 | 2026-10-05 | Anima adopts the repository v2.12 (M9b and M9c kickoff answers D-54, amended by Mraw at the owner order; CI-013 resolved) unchanged, and adds Editor help and live data (owner answers D-58, three chat rounds; Mraw's brief docs/plans/M10b-editor-help-live-data-brief.md; source of truth v2.11, Round 24: epic E29, EDT-09..EDT-11, NFR-09, MVP-18). New milestone M10b between X-M10 and K-M11: K-M10b, S-US-300 (field tooltips from help.json), S-US-301 (suggestion list widget), S-US-302 (suggestions on every field), S-US-303 (live reload of every data file), S-US-304 (file watch for outside edits), S-US-305 (level edits win over the run save), X-M10b (tests plus a 10-minute owner walkthrough). M10b runs under the owner-decides rules (D-22), not the D-41 delegation. New P-013 adopts v2.13. K-M11 (now v2.13) checks M10b and reuses its help source for the schema forms; its old reference to X-M9 is corrected to X-M10. D-15 chain and execution order updated. Not specified here: the exact field list and hover delay (K-M10b design document and owner round). |
