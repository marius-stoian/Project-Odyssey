# Owner decisions

The only way agents learn a decision. The owner answers in the last column and sets Status to Decided.

## Standing owner instructions (2026-09-30)

The owner wants Mraw to assemble Project Odyssey with minimal intervention:

1. **Delegated decisions.** When an owner decision blocks the next prompt, Dominus decides using the recommended option from the source of truth (or the Codex), records it below with status "Decided by Dominus (delegated)" and the reasoning in docs/decision-requests/<ID>.md, lists it in the next Milestone file, and the team continues. The owner may override any delegated decision later; an override is a new decision and may need rework.
2. **Branches.** Each accepted story merges into `qa` once CI is green (the story is then Done). At each milestone exit review (X-Mx), `qa` merges into `main`, CI on `main` must be green, and `main` is tagged mx-done.
3. **Progress files.** After every story, save a new progress snapshot as Milestone-<n>.md at the repository root (Milestone-3.md, Milestone-4.md, ...), with the next AP-### ID.
4. **Changelog.** Update CHANGELOG.md with every change set before it is pushed or merged (AGENTS.md).
5. **Still reserved for the owner:** kill-gate results that need people (X-M2, X-M6), anything needing accounts, credentials, money or other people, and destructive actions outside the repo.

| ID | Decision | Needed by | Blocks | Status | Owner answer |
|---|---|---|---|---|---|
| D-01 | Confirm time model: pausable real time with speed control (OPEN-04) | M2 | US-010 | Decided | 2026-09-30, owner: confirmed as proposed, pausable real time with speed control. |
| D-02 | Side Characters interview: needs, traits, relationships (OPEN-10) | M2 (week 3) | US-011, US-012, US-013 | Decided by Dominus (delegated) | 2026-09-30: small, legible model: 4 needs (D-03), 6 traits (Brave, Timid, Kind, Greedy, Talkative, Diligent), one opinion -100..100 per pair plus kinship, memories with feelings, gossip at half strength, minor memories forgotten after 60 days. Reasoning in docs/decision-requests/D-02.md. |
| D-03 | Confirm the Age 1 needs set: Hunger, Energy, Warmth, Social | M2 | US-011 | Decided | 2026-09-30, owner: confirmed as proposed, Age 1 needs are Hunger, Energy, Warmth, Social. |
| D-04 | Sprite size and facing directions (OPEN-11) | M1 | US-022, US-024, US-030 | Decided | 2026-09-29, owner to Mraw/Anima: 32x48 px characters, 8 facing directions. |
| D-05 | Art source for placeholders: own, free asset pack, or hired (OPEN-12) | M3 | US-030 | Decided | 2026-09-30, owner: own art, placeholder quality for now (the sprites in assets/sprites/); before any public release he verifies the image generator's licence terms. |
| D-06 | Minimum PC spec (OPEN-19) | M4 | US-082 | Open |  |
| D-07 | Calendar display (OPEN-15) | M4 | US-050, US-083 | Open |  |
| D-08 | Interactions interview: verbs, objects, crafting (OPEN-09) | M4 | US-061, US-062 | Open |  |
| D-09 | Confirm the five Age 1 professions (MVP-07) | M4 | US-060 | Proposed |  |
| D-10 | Confirm MVP pillars Trade + Religion (MVP-08) and victory thresholds (MVP-09) | M5 | US-070..US-073 | Proposed |  |
| D-11 | Story interview: tone of events, onboarding elder (OPEN-08) | M5 | US-052, US-090 | Open |  |
| D-12 | Visual Studio, CMake, Git, vcpkg installed; GitHub account and private repo | M0 | US-001, US-002 | Decided | 2026-09-29, installed by Mraw at the owner's request: Visual Studio Community 2026 (C++ desktop workload + AddressSanitizer), CMake 4.4.3, vcpkg at C:\dev\vcpkg (VCPKG_ROOT set), GitHub CLI 2.101.0. GitHub account marius-stoian; private repo marius-stoian/Project-Odyssey. |
| D-13 | SDL3, EnTT, Dear ImGui, nlohmann/json, doctest, FastNoiseLite available via vcpkg or third_party | M0-M4 | US-020, US-032, US-083, US-016, US-040 | Decided | 2026-09-29, owner to Mraw/Anima: take SDL3, EnTT, Dear ImGui, nlohmann/json and doctest from vcpkg; FastNoiseLite as a single header in third_party/. |
| D-16 | Tile size for the tile grid (ENV-11) | M1 | US-023, US-024 | Decided by Dominus (delegated) | 2026-09-30: 32x32 px tiles, matching 32-px-wide characters (D-04). Reasoning in docs/decision-requests/D-16.md. |
| D-17 | Movement directions: US-024 says four, D-04 gives 8 facing directions | M1 | US-024 | Decided by Dominus (delegated) | 2026-09-30: 8-way movement, diagonals at the same speed; US-024 scenarios unchanged. Reasoning in docs/decision-requests/D-17.md. |
| D-14 | Eight outside playtesters recruited | M6 | Kill gate 2 | Open |  |
| D-15 | Technical chain: M0 > M1 > M1b > M2 > M3 > M4 > M5 > M6 (each milestone needs the previous one) | All | All | Planned |  |
| D-GATE-M2 | Kill Gate 1 result: did 2 of 3 readers find a story in the chronicle? | X-M2 | M3 (all) | Decided: Pivot | 2026-09-30, owner: "Not really a story, needs improvements for the event generation so more things can happen. Need reasons for death and feuds and more interaction types between people. Pivot to fewer more complex events." |
| D-18 | Story pivot design (after Kill Gate 1) | M2b | US-110..US-115 | Decided | 2026-09-30, owner: story arcs on a richer social simulation; new interactions: quarrels, blame and revenge; sharing and nursing; courtship and rivals in love; teaching and hunting parties; the chronicle tells episodes and every death and feud line gives its reason; the owner judges the gate retry alone. Recorded in requirements v1.6 (STO-02, STO-03, SDC-02) and Codex v1.6 (M2b). |
| D-19 | Level Editor scope v1 (Game mode and Editor mode) | M2c | US-120..US-126 | Decided | 2026-09-30, owner: settings = level and character properties (name, map size, default ground, hero start; per character name, HP, facing, sword damage); placed characters stand still with properties; new milestone M2c before M3. Brief: docs/plans/M2c-editor-brief.md |
| D-20 | Which level the game and the tests use | M2c | tests of US-024, US-029, US-122..US-125 | Decided by Dominus (delegated) | 2026-09-30: the owner's edited valley.json stays the game's level; the original demo is assets/levels/demo.json, which every test plays; level backups are not committed. docs/decision-requests/D-20.md |
| D-21 | M2d content and combat | M2d | US-130..US-138 | Decided (owner, 2026-09-30) | Six question rounds in chat; full list in docs/plans/M2d-content-brief.md and Codex v1.8 section 5: full gameplay; 16 starter weapons (one plain + one elemental per class) as pickups in a 9-slot hotbar; 8 weapon classes; elements; hero 100 HP, respawn at start; enemies (goblins, predators, boars) strike back after 0.5 s within 1.5 m; deaths drop nothing; plants block, inspect, chop, heal 10, regrow after 15 s in camera view; effects; random weather every 60-120 s. |
| D-22 | Who takes design decisions | All | All | Decided (owner, 2026-09-30) | "I want to take all design decisions, so Dominus ask me all the things you need clarified and confirmed in interactive questions sessions in chat." Charter human gate 3. Decisions delegated to Dominus before D-22 (D-02, D-16, D-17, D-20) stay valid unless the owner overrides them. |
| D-GATE-M2b | Kill Gate 1 retry: is the M2b printed story a story? | X-M2b | M3 (all) | Decided: Option A, Go (owner, 2026-09-30: "it is a story"; M3 waits for his new feature requests) | |
