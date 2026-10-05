# Project Odyssey: assembly progress (2)

Continues [Milestone.md](../../../Milestone.md) (AP-001 by Mraw, AP-002 by ChatGPT). From now on a new Milestone-<n>.md is saved after every story, each with the next AP-### ID.

## AP-003 · 2026-09-30

| | |
|---|---|
| Snapshot ID | **AP-003** |
| Codex | v1.2 (Anima, Luna first); v1.3 in preparation (CI-004, CI-005, owner's standing instructions) |
| Source of truth | Project Odyssey.docx v1.4 |
| Repository state | Branch `qa` (M0 work, CI green: [run 36635345962](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36635345962)); `main` @ `76ee34e` until the M0 exit review |
| Current milestone | **M0 Tooling ready**: 3 of 4 stories Done |
| Next prompt | **S-US-004** Log what happens and stop on broken assumptions |
| Blocked by owner decisions | Nothing: D-01, D-03, D-04, D-12, D-13 Decided; other decisions are delegated to Dominus (standing instructions in docs/decisions.md) |

### What happened since AP-002

1. **ChatGPT built US-003** (layer rules) on 2026-09-29 but could not verify it on Windows: GitHub refused its push (HTTP 403). Its full output arrived as an upload in docs/.
2. **Mraw integrated it**: recreated ChatGPT's branch exactly (`2170dfb`, 90 files verified), merged it with `main` into the new `qa` branch, and resolved one README conflict.
3. **Windows verification found a real portability bug** in the tests (not in the layer rules): MSBuild writes `fatal  error C1083` with two spaces, so 2 of 5 tests failed on Windows while passing on Linux. One-line fix; now 5/5 pass in Debug and Release, locally and on GitHub's Windows runner.
4. **US-003 accepted and Done**: Allowed use, Forbidden use (including `../` and absolute-path bypasses) and Luna stays game-agnostic all pass on MSVC.
5. **Records**: CHANGELOG covers the whole project; US-003 teach-back in the learning journal; new docs/README.md index; redundant upload copies removed.
6. **Owner's standing instructions** for overnight assembly recorded in docs/decisions.md.

### Milestones

| ID | Milestone | Stories done | Planned (likely) | State |
|---|---|---|---|---|
| M0 | Tooling ready | 3 / 4 | 05 Oct - 25 Oct 2026 | In progress, ahead of plan |
| M1 | Luna engine: walking skeleton | 0 / 5 | 26 Oct - 13 Dec 2026 | Ready (D-04, D-13 Decided) |
| M2 | Console clan simulator (KILL GATE 1) | 0 / 7 | 14 Dec 2026 - 07 Mar 2027 | D-01, D-03 Decided; D-02 delegated to Dominus |
| M3 | Living clan on screen | 0 / 3 | 08 Mar - 11 Apr 2027 | |
| M4 | Region, tools and saves | 0 / 6 | 12 Apr - 06 Jun 2027 | |
| M5 | Vertical slice feature-complete | 0 / 16 | 07 Jun - 21 Nov 2027 | |
| M6 | Playtest and go/no-go (KILL GATE 2) | 0 / 3 | 22 Nov - 19 Dec 2027 | |
| | **MVP total** | **3 / 44** | | |

### Done stories

| Story | By | Evidence |
|---|---|---|
| US-001 Build and debug from a clean checkout | Mraw | [plan](../../plans/stories-M0.md#us-001) |
| US-002 Run the build and tests on every push | Mraw | [plan](../../plans/stories-M0.md#us-002) |
| US-003 Enforce the layer rules in the build | ChatGPT (build), Mraw (Windows verification, fix) | [plan](../../plans/stories-M0.md#us-003), [report](../../reports/US-003-2026-09-30.md), [evidence](../../evidence/US-003) |

### Owner decisions and Codex issues

| Decided | Delegated to Dominus | Open Codex issues |
|---|---|---|
| D-01 time model, D-03 needs set (2026-09-30); D-04, D-12, D-13 | Every other blocking decision, recorded as "Decided by Dominus (delegated)" | CI-004 (Milestone files), CI-005 (changelog, boundary.h, qa branch): both go into Codex v1.3 |

### Next
1. Anima: Codex v1.3 with the owner's standing instructions; Dominus and Anima skills synced to the same goal.
2. S-US-004 logging and asserts, then X-M0 (merge `qa` into `main`, tag `m0-done`).
3. K-M1: the Luna engine.
