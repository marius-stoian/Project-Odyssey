# Project Odyssey: assembly progress

A snapshot of where the assembly stands, saved at the end of each session. Each save gets the next ID (AP-001, AP-002, ...) and records the Codex and requirements versions it was measured against. The newest snapshot is on top; older ones stay below. Live details: [docs/status.md](docs/status.md) (every prompt), [docs/decisions.md](docs/decisions.md) (owner decisions).

**Newer snapshots:** [Milestone-2.md](Milestone-2.md) (AP-003) and later Milestone-<n>.md files.

---

## AP-002 · 2026-09-29

| | |
|---|---|
| Snapshot ID | **AP-002** |
| Codex | v1.2 (checked-in copy) |
| Source of truth | Project Odyssey.docx v1.4 referenced by Codex; external document not read this session |
| Repository state | Local `story/US-003` based on `main` @ `fb21b48`; changes not pushed or merged |
| Current milestone | **M0 Tooling ready** (2 of 4 stories Done) |
| Next prompt | Resume **S-US-003** at Windows verification/acceptance |
| Blocker | GitHub branch creation rejected with HTTP 403; required Windows CI and integration unavailable |

### Assembly outcome

US-003 implements five build targets, private layer identities, guarded headers, narrow include visibility and per-build validation. Supplementary GCC Debug and Release pass 5 named CTests, 5 doctest cases / 17 assertions, 14 compiler probes and six validator rejection checks. Windows/MSVC, ASan and main CI remain unverified; the acceptor rejected completion pending those checks. US-003 stays **Blocked**.

Full change tracking: [CHANGELOG.md](CHANGELOG.md). Evidence and resume instructions: [assembly report](docs/reports/US-003-2026-09-29.md), [plan](docs/plans/US-003.md). The owner requested changelog coverage for every future PR; `AGENTS.md` records it.

### Decisions and Codex issues

D-04, D-12 and D-13 are Decided; no newly answered decision unblocks a currently Blocked story. No new design requests or Codex issues. Existing CI-004 remains Open. M1-M6 remain To do; M0's US-004 and X-M0 remain To do. The formal US-003 teach-back is drafted in its plan and will enter the learning journal after acceptance.

### Next

1. Verify the external master Codex, DOCX charter and workbook when Drive tools are available.
2. Enable repository write access, push the prepared story and run Windows CI.
3. Obtain acceptance, publish teach-back, integrate and confirm main CI before marking Done.
4. Continue S-US-004, then X-M0.

---

## AP-001 · 2026-09-29

| | |
|---|---|
| Snapshot ID | **AP-001** |
| Codex | v1.2 (Anima, Luna first) |
| Source of truth | Project Odyssey.docx v1.4 |
| Repository state | `main` @ `2d7dc23` "P-001: adopt Codex v1.2 (Luna first)", CI green |
| Current milestone | **M0 Tooling ready** (2 of 4 stories done) |
| Next prompt | **S-US-003** Enforce the layer rules in the build |
| Blocked by owner decisions | Nothing until M2 |

### Milestones

| ID | Milestone | Stories done | Planned (likely) | State |
|---|---|---|---|---|
| M0 | Tooling ready | 2 / 4 | 05 Oct - 25 Oct 2026 | In progress, ahead of plan (started 29 Sep) |
| M1 | Luna engine: walking skeleton | 0 / 5 | 26 Oct - 13 Dec 2026 | Ready: D-04 and D-13 decided |
| M2 | Console clan simulator (KILL GATE 1) | 0 / 7 | 14 Dec 2026 - 07 Mar 2027 | Needs D-01, D-02, D-03 |
| M3 | Living clan on screen | 0 / 3 | 08 Mar - 11 Apr 2027 | |
| M4 | Region, tools and saves | 0 / 6 | 12 Apr - 06 Jun 2027 | |
| M5 | Vertical slice feature-complete | 0 / 16 | 07 Jun - 21 Nov 2027 | |
| M6 | Playtest and go/no-go (KILL GATE 2) | 0 / 3 | 22 Nov - 19 Dec 2027 | |
| | **MVP total** | **2 / 44** | 05 Oct 2026 - 19 Dec 2027 | |

### Done since the start

| Prompt | What it delivered | Evidence |
|---|---|---|
| P-000 | Repo, Charter (CLAUDE.md), 7 Mraw agents, state files; toolchain installed (D-12) | commits `3d96f58`, `c8c6301` |
| K-M0 | M0 kickoff | docs/status.md |
| S-US-001 | One CMake preset builds odysseus.exe, odysseus_headless.exe, odysseus_tests.exe with zero warnings; debuggable; missing libraries named | [docs/plans/US-001.md](docs/plans/US-001.md) |
| S-US-002 | GitHub Actions builds and tests every push (1 min 48 s); a broken test turns CI red and is named | [docs/plans/US-002.md](docs/plans/US-002.md) |
| P-001 | Adopted Codex v1.2: Luna folders (src/luna/), new prompt order, D-04 and D-13 recorded | commit `2d7dc23` |

Also done outside the Codex, at the owner's request: Codex sync from Google Drive at session start (tools/sync-codex.ps1), Dominus and Anima skills in .claude/skills/, requirements v1.3 -> v1.4 (Luna, ARC-09) and Codex v1.1 -> v1.2.

### Owner decisions

| Decided | Open (needed next) |
|---|---|
| D-04 sprites 32x48 px, 8 directions · D-12 toolchain and GitHub · D-13 libraries from vcpkg | D-01 time model, D-02 Side Characters interview, D-03 needs set: all for M2 |

### Codex issues
CI-001, CI-002, CI-003 resolved in Codex v1.2. Open: CI-004 (make Milestone.md an official state file, so every session adds a snapshot); take it to Anima with A-002.

### Next
1. S-US-003: build targets for all five layers; the build rejects forbidden includes (including game code inside Luna).
2. S-US-004: logging with session log rotation and asserts.
3. X-M0: exit review, tag `m0-done`.
4. K-M1: start the Luna engine.
