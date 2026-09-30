# Project Odyssey Codex v1.7

Author: **Anima** (Prompt Architect) for **Mraw** (Dominus Full Team / Dominus Avengers) | Date: 2026-09-30 | Source of truth: Project Odyssey.docx v1.7 (chapter 12: MVP; chapter 7: architecture) | Executor: autonomous AI coding agents (the strongest available model for orchestrator, architect and acceptor; any current model for the others) | Human gate: kill-gate results that need people, accounts, credentials and money; design decisions are delegated to Dominus

## 0. How to use this Codex

- Phase 3 of the Amek workflow: Mraw assembles Project Odyssey by executing these prompts **in order**, exactly as written.
- Start: put this file in an empty folder `odysseus/`, open your AI coding session there, paste **A-000**. Every later session: paste **A-001**.
- P-000 turns the Charter into `CLAUDE.md` and the role prompts into `.claude/agents/`, so every agent loads them automatically.
- One story in progress at a time. Every session ends with an assembly report.
- Who does what (owner instruction, 2026-09-30): Dominus, wearing every Mraw hat, designs, implements and tests the game; Anima alone writes and amends this Codex. Mraw never edits docs/Codex.md; problems go to Anima as codex issues.
- Continuous assembly: the owner wants the MVP as fast as quality allows, engine first. After each prompt, continue with the next one in Codex order without waiting, until a Charter human gate or the end of the session.
- Autonomous by default (owner instruction, 2026-09-30): the owner wants the game built with minimal intervention. Design decisions are delegated to Dominus, stories integrate through the `qa` branch, and a Milestone-<n>.md progress snapshot is saved after every story, so the owner can review everything later. The owner answers questions up front (before leaving the team to work), not during the run.
- Blocked, wrong or ambiguous prompts become codex issues; the owner takes them to Anima with **A-002**; Anima issues a new Codex version.

## 1. Delivery format

Hybrid: **stage gates** at milestones M0-M6 (with kill gates at M2 and M6) and **Kanban flow** inside each milestone, WIP 1. Milestone kickoff (K) batches owner decisions; exit review (X) demonstrates exit criteria and tags the repo.

## 2. Charter (C-01)
Written verbatim to `CLAUDE.md` by P-000.

```markdown
# CLAUDE.md: Project Odyssey Charter (Codex C-01, v1.7)

<role>
You are a member of Mraw, the Dominus Full Team (also called Dominus Avengers), assembling Project Odyssey by following the Codex written by Anima. You build exactly what the current Codex prompt asks, nothing more.
</role>

<project>
Project Odyssey (game codename Odysseus): a 2D pixel-art life and civilization simulation. MVP = Age 1 vertical slice on Windows x64: one procedurally generated region, one hero from age 12 who grows into a clan leader, five professions, Trade and Religion pillars, win by leading the region.
Source of truth for WHAT: Project Odyssey.docx v1.7 (chapter 12: MVP; chapter 7: architecture). Source of truth for HOW and ORDER: docs/Codex.md (this Codex).
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
- Code compiles with zero warnings in Debug and Release (x64).
- All acceptance criteria verified; automated tests written where the story is testable headless.
- CI is green on the qa branch after the merge (from US-002 on, when CI exists); main is checked at milestone exits.
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
Everything else the team decides and records:
- Design decisions (a D-xx that is not Decided, or any question that changes design or scope): Dominus decides using the recommended option in the source of truth or this Codex. Write docs/decision-requests/<ID>.md with the question, 2-4 options, the choice and why; set the decision in docs/decisions.md to "Decided by Dominus (delegated)" with a one-line answer; list it in the next Milestone file; continue. The owner may override later; an override is a new decision.
- If the source of truth must change because of a delegated decision, update the requirements document on Google Drive (bump its version, add a resolution-log line) and raise a codex issue so Anima can follow.
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
<prompt id="L-01" codex="1.7" name="Mraw build loop">
<context>
Used by mraw-orchestrator for every story prompt S-US-xxx. The Charter (CLAUDE.md) is already loaded.
</context>
<instructions>
1. Readiness: for each D-xx in the story's <dependencies>, read docs/decisions.md. If any is not Decided, follow the Charter's human_gates: Dominus decides it (delegated), records it, and the story continues. For each US-xxx dependency, confirm it is Done in docs/status.md.
2. Branch: create story/US-xxx from qa. If that branch already exists with paused work (Limit.md says so), continue on it instead: merge the latest qa into it first, then finish what Limit.md lists as left.
3. Plan: delegate to mraw-architect -> docs/plans/US-xxx.md.
4. Tests first: delegate to mraw-tester -> failing tests for every headless-testable scenario; manual checks for the rest.
5. Content (only if the story needs data): delegate to mraw-designer.
6. Implement: delegate to mraw-programmer until tests pass with zero warnings.
7. Verify: delegate to mraw-tester -> run `pwsh tools/verify.ps1 -Story US-xxx` (configure, Debug and Release builds with zero warning lines, every test in both, results saved to docs/evidence/US-xxx/); add story-specific evidence (end-to-end runs, screenshots via `odysseus.exe --screenshot`, measurements) to the same folder.
8. Accept: delegate to mraw-acceptor. On REJECT, return to step 6 with the reasons. After 3 rejections, mark the story Failed, write a codex issue, and stop this story.
9. Document and teach: delegate to mraw-writer -> docs + teach-back entry.
10. Integrate: update CHANGELOG.md, commit, merge into qa, push; when CI on qa is green, set the story to Done in docs/status.md.
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
| D-06 | Minimum PC spec (OPEN-19) | M4 | US-082 | Open |
| D-07 | Calendar display (OPEN-15) | M4 | US-050, US-083 | Open |
| D-08 | Interactions interview: verbs, objects, crafting (OPEN-09) | M4 | US-061, US-062 | Open |
| D-09 | Confirm the five Age 1 professions (MVP-07) | M4 | US-060 | Proposed |
| D-10 | Confirm MVP pillars Trade + Religion (MVP-08) and victory thresholds (MVP-09) | M5 | US-070..US-073 | Proposed |
| D-11 | Story interview: tone of events, onboarding elder (OPEN-08) | M5 | US-052, US-090 | Open |
| D-12 | Visual Studio, CMake, Git, vcpkg installed; GitHub account and private repo | M0 | US-001, US-002 | Decided |
| D-13 | SDL3, EnTT, Dear ImGui, nlohmann/json, doctest, FastNoiseLite available via vcpkg or third_party | M0-M4 | US-020, US-032, US-083, US-016, US-040 | Decided |
| D-14 | Eight outside playtesters recruited | M6 | Kill gate 2 | Open |
| D-15 | Technical chain: M0 > M1 > M1b > M2 > M2b > M2c > M3 > M4 > M5 > M6 (each milestone needs the previous one) | All | All | Planned |
| D-GATE-M2 | Kill Gate 1 result (M2): did 2 of 3 readers find a story? | X-M2 | M3 | Decided: Pivot (owner, 2026-09-30) |
| D-18 | Story pivot design: story arcs on a richer social simulation; quarrels, blame and revenge; sharing and nursing; courtship and rivals; teaching and hunting parties; episodes plus lines with reasons; the owner judges the retry alone | M2b | US-110..US-115 | Decided (owner, 2026-09-30) |
| D-GATE-M2b | Kill Gate 1 retry: the owner reads the M2b story and judges whether it is a story | X-M2b | M3 | Decided: Go (owner, 2026-09-30: "it is a story") |
| D-19 | Level editor scope v1: settings are level and character properties (level name, map size, default ground, hero start; per character name, HP, facing, sword damage); placed characters stand still with properties; new milestone M2c before M3 | M2c | US-120..US-126 | Decided (owner, 2026-09-30) |

## 6. Assembly prompts

### A-000 Start assembly (owner pastes this once)
```text
Dominus Avengers Assemble.
You are Mraw, the Dominus Full Team, assembling Project Odyssey with Codex v1.7 written by Anima.
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
<prompt id="P-000" codex="1.7" name="Bootstrap the Mraw workspace">
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
<prompt id="P-001" codex="1.7" name="Adopt Codex v1.2 in an existing workspace">
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
<prompt id="P-002" codex="1.7" name="Adopt Codex v1.3 in an existing workspace">
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
<prompt id="P-003" codex="1.7" name="Adopt Codex v1.4 in an existing workspace">
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
<prompt id="P-004" codex="1.7" name="Adopt Codex v1.5 in an existing workspace">
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
<prompt id="P-005" codex="1.7" name="Adopt Codex v1.6 in an existing workspace">
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
<prompt id="P-006" codex="1.7" name="Adopt Codex v1.7 in an existing workspace">
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

### M0 Tooling ready
Exit criteria: A clean checkout builds in Visual Studio; you pause the program on a breakpoint; CI runs on push.

```xml
<prompt id="K-M0" codex="1.7" name="Kick off M0 Tooling ready">
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
<prompt id="S-US-001" codex="1.7" milestone="M0" story="US-001" priority="Must" size="S">
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
<prompt id="S-US-002" codex="1.7" milestone="M0" story="US-002" priority="Must" size="S">
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
<prompt id="S-US-003" codex="1.7" milestone="M0" story="US-003" priority="Must" size="S">
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
<prompt id="S-US-004" codex="1.7" milestone="M0" story="US-004" priority="Must" size="S">
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
<prompt id="X-M0" codex="1.7" name="Exit review M0">
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
<prompt id="K-M1" codex="1.7" name="Kick off M1 Luna engine: walking skeleton">
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
<prompt id="S-US-020" codex="1.7" milestone="M1" story="US-020" priority="Must" size="M">
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
<prompt id="S-US-021" codex="1.7" milestone="M1" story="US-021" priority="Must" size="S">
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
<prompt id="S-US-022" codex="1.7" milestone="M1" story="US-022" priority="Must" size="M">
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
<prompt id="S-US-023" codex="1.7" milestone="M1" story="US-023" priority="Must" size="M">
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
<prompt id="S-US-024" codex="1.7" milestone="M1" story="US-024" priority="Must" size="M">
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
<prompt id="X-M1" codex="1.7" name="Exit review M1">
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
<prompt id="K-M1b" codex="1.7" name="Kick off M1b Luna Physics">
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
<prompt id="S-US-025" codex="1.7" milestone="M1b" story="US-025" priority="Must" size="M">
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
<prompt id="S-US-026" codex="1.7" milestone="M1b" story="US-026" priority="Must" size="L">
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
<prompt id="S-US-027" codex="1.7" milestone="M1b" story="US-027" priority="Must" size="M">
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
<prompt id="S-US-028" codex="1.7" milestone="M1b" story="US-028" priority="Must" size="M">
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
<prompt id="S-US-029" codex="1.7" milestone="M1b" story="US-029" priority="Must" size="M">
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
<prompt id="X-M1b" codex="1.7" name="Exit review M1b">
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
<prompt id="K-M2" codex="1.7" name="Kick off M2 Console clan simulator (KILL GATE 1)">
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
<prompt id="S-US-010" codex="1.7" milestone="M2" story="US-010" priority="Must" size="M">
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
<prompt id="S-US-011" codex="1.7" milestone="M2" story="US-011" priority="Must" size="M">
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
<prompt id="S-US-012" codex="1.7" milestone="M2" story="US-012" priority="Must" size="L">
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
<prompt id="S-US-013" codex="1.7" milestone="M2" story="US-013" priority="Must" size="M">
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
<prompt id="S-US-014" codex="1.7" milestone="M2" story="US-014" priority="Must" size="M">
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
<prompt id="S-US-015" codex="1.7" milestone="M2" story="US-015" priority="Must" size="S">
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
<prompt id="S-US-016" codex="1.7" milestone="M2" story="US-016" priority="Must" size="M">
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
<prompt id="X-M2" codex="1.7" name="Exit review M2">
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
<prompt id="K-M2b" codex="1.7" name="Kick off M2b Story engine">
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
<prompt id="S-US-110" codex="1.7" milestone="M2b" story="US-110" priority="Must" size="M">
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
<prompt id="S-US-111" codex="1.7" milestone="M2b" story="US-111" priority="Must" size="L">
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
<prompt id="S-US-112" codex="1.7" milestone="M2b" story="US-112" priority="Must" size="L">
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
<prompt id="S-US-113" codex="1.7" milestone="M2b" story="US-113" priority="Must" size="M">
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
<prompt id="S-US-114" codex="1.7" milestone="M2b" story="US-114" priority="Must" size="L">
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
<prompt id="S-US-115" codex="1.7" milestone="M2b" story="US-115" priority="Must" size="L">
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
<prompt id="X-M2b" codex="1.7" name="Exit review M2b = Kill Gate 1 retry">
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
<prompt id="K-M2c" codex="1.7" name="Kick off M2c Level editor">
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
<prompt id="S-US-120" codex="1.7" milestone="M2c" story="US-120" priority="Must" size="L">
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
<prompt id="S-US-121" codex="1.7" milestone="M2c" story="US-121" priority="Must" size="M">
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
<prompt id="S-US-122" codex="1.7" milestone="M2c" story="US-122" priority="Must" size="M">
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
<prompt id="S-US-123" codex="1.7" milestone="M2c" story="US-123" priority="Must" size="S">
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
<prompt id="S-US-124" codex="1.7" milestone="M2c" story="US-124" priority="Must" size="M">
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
<prompt id="S-US-125" codex="1.7" milestone="M2c" story="US-125" priority="Must" size="M">
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
<prompt id="S-US-126" codex="1.7" milestone="M2c" story="US-126" priority="Must" size="S">
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
<prompt id="X-M2c" codex="1.7" name="Exit review M2c">
<instructions>
1. Demonstrate the exit criteria: The owner switches between Game mode and Editor mode; in the Editor he paints ground tiles, places characters with properties and sets the level's name, size, default ground and hero start; he saves, reloads and plays the level; the game draws his own art; the spear and sword demos still work.
2. Collect evidence into docs/gates/M2c.md, one section per criterion, each marked met or not met: a scripted end-to-end session (paint, place, set, save, restart, load, play, strike) with screenshots in docs/evidence/X-M2c/; the full test run; the guide.
3. No kill gate: invite the owner to try the editor with docs/guides/editor.md and record his notes when he gives them; they become new stories through Anima, never silent changes.
4. Merge qa into main, push, confirm CI on main is green, tag the repository m2c-done and push the tag, and save a Milestone-<n>.md snapshot. Then continue with K-M3.
</instructions>
<output_format>Assembly report with the review result.</output_format>
</prompt>
```

### M3 Living clan on screen
Exit criteria: The clan from M2 runs inside the game; NPCs are visible, dressed in layered outfits, and act on their needs.

```xml
<prompt id="K-M3" codex="1.7" name="Kick off M3 Living clan on screen">
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
<prompt id="S-US-030" codex="1.7" milestone="M3" story="US-030" priority="Must" size="M">
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
<prompt id="S-US-032" codex="1.7" milestone="M3" story="US-032" priority="Must" size="L">
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
<prompt id="S-US-031" codex="1.7" milestone="M3" story="US-031" priority="Should" size="S">
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
<prompt id="X-M3" codex="1.7" name="Exit review M3">
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
<prompt id="K-M4" codex="1.7" name="Kick off M4 Region, tools and saves">
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
<prompt id="S-US-040" codex="1.7" milestone="M4" story="US-040" priority="Must" size="L">
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
<prompt id="S-US-041" codex="1.7" milestone="M4" story="US-041" priority="Must" size="M">
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
<prompt id="S-US-042" codex="1.7" milestone="M4" story="US-042" priority="Must" size="M">
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
<prompt id="S-US-043" codex="1.7" milestone="M4" story="US-043" priority="Should" size="M">
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
<prompt id="S-US-080" codex="1.7" milestone="M4" story="US-080" priority="Must" size="M">
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
<prompt id="S-US-083" codex="1.7" milestone="M4" story="US-083" priority="Should" size="M">
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
<prompt id="X-M4" codex="1.7" name="Exit review M4">
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
<prompt id="K-M5" codex="1.7" name="Kick off M5 Vertical slice feature-complete">
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
<prompt id="S-US-050" codex="1.7" milestone="M5" story="US-050" priority="Must" size="M">
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
<prompt id="S-US-053" codex="1.7" milestone="M5" story="US-053" priority="Must" size="S">
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
<prompt id="S-US-051" codex="1.7" milestone="M5" story="US-051" priority="Must" size="M">
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
<prompt id="S-US-052" codex="1.7" milestone="M5" story="US-052" priority="Must" size="L">
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
<prompt id="S-US-054" codex="1.7" milestone="M5" story="US-054" priority="Must" size="S">
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
<prompt id="S-US-060" codex="1.7" milestone="M5" story="US-060" priority="Must" size="S">
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
<prompt id="S-US-061" codex="1.7" milestone="M5" story="US-061" priority="Must" size="L">
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
<prompt id="S-US-062" codex="1.7" milestone="M5" story="US-062" priority="Must" size="M">
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
<prompt id="S-US-063" codex="1.7" milestone="M5" story="US-063" priority="Should" size="M">
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
<prompt id="S-US-070" codex="1.7" milestone="M5" story="US-070" priority="Must" size="M">
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
<prompt id="S-US-055" codex="1.7" milestone="M5" story="US-055" priority="Must" size="M">
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
<prompt id="S-US-071" codex="1.7" milestone="M5" story="US-071" priority="Must" size="L">
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
<prompt id="S-US-072" codex="1.7" milestone="M5" story="US-072" priority="Must" size="L">
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
<prompt id="S-US-073" codex="1.7" milestone="M5" story="US-073" priority="Must" size="S">
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
<prompt id="S-US-081" codex="1.7" milestone="M5" story="US-081" priority="Should" size="S">
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
<prompt id="S-US-082" codex="1.7" milestone="M5" story="US-082" priority="Must" size="S">
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
<prompt id="X-M5" codex="1.7" name="Exit review M5">
<instructions>
1. Demonstrate the exit criteria: A player can start a new game, live the Growing Period, take a profession, pursue Trade or Religion, and win or lose.
2. Collect evidence (test output, headless run logs, FPS logs, screenshots) into docs/gates/M5.md, one section per criterion, each marked met or not met.
3. If all are met: merge qa into main, push, confirm CI on main is green, tag the repository m5-done and push the tag, and save a Milestone-<n>.md snapshot. If not: list what is missing as new stories in docs/codex-issues.md (for Anima) and stop.
</instructions>
<output_format>Assembly report with the gate result.</output_format>
</prompt>
```

### M6 Playtest and go/no-go (KILL GATE 2)
Exit criteria: 8 outside playtesters play; success criteria measured; go/no-go decision recorded.

```xml
<prompt id="K-M6" codex="1.7" name="Kick off M6 Playtest and go/no-go (KILL GATE 2)">
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
<prompt id="S-US-090" codex="1.7" milestone="M6" story="US-090" priority="Should" size="M">
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
<prompt id="S-US-091" codex="1.7" milestone="M6" story="US-091" priority="Must" size="S">
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
<prompt id="S-US-092" codex="1.7" milestone="M6" story="US-092" priority="Should" size="S">
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
<prompt id="X-M6" codex="1.7" name="Exit review M6">
<instructions>
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
P-000 -> P-001 -> P-002 -> P-003 -> P-004 -> P-005 -> P-006 -> K-M0 -> S-US-001 -> S-US-002 -> S-US-003 -> S-US-004 -> X-M0 -> K-M1 -> S-US-020 -> S-US-021 -> S-US-022 -> S-US-023 -> S-US-024 -> X-M1 -> K-M1b -> S-US-025 -> S-US-026 -> S-US-027 -> S-US-028 -> S-US-029 -> X-M1b -> K-M2 -> S-US-010 -> S-US-011 -> S-US-012 -> S-US-013 -> S-US-014 -> S-US-015 -> S-US-016 -> X-M2 -> K-M2b -> S-US-110 -> S-US-111 -> S-US-112 -> S-US-113 -> S-US-114 -> S-US-115 -> X-M2b -> K-M2c -> S-US-120 -> S-US-121 -> S-US-122 -> S-US-123 -> S-US-124 -> S-US-125 -> S-US-126 -> X-M2c -> K-M3 -> S-US-030 -> S-US-032 -> S-US-031 -> X-M3 -> K-M4 -> S-US-040 -> S-US-041 -> S-US-042 -> S-US-043 -> S-US-080 -> S-US-083 -> X-M4 -> K-M5 -> S-US-050 -> S-US-053 -> S-US-051 -> S-US-052 -> S-US-054 -> S-US-060 -> S-US-061 -> S-US-062 -> S-US-063 -> S-US-070 -> S-US-055 -> S-US-071 -> S-US-072 -> S-US-073 -> S-US-081 -> S-US-082 -> X-M5 -> K-M6 -> S-US-090 -> S-US-091 -> S-US-092 -> X-M6

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
