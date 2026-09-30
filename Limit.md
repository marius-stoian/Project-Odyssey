# Limit.md: how to continue the assembly

This file is kept current after every story, so if a session stops (usage limit, crash, closed window), the next one knows exactly where to pick up. The newest progress snapshot is the highest-numbered Milestone-<n>.md.

**Last updated:** 2026-09-30, after US-121 (Milestone-34.md, AP-035). The next session starts here.

## Where we are
- Branch with the latest work: **`qa`** (CI green). `main` gets `qa` at each milestone exit.
- Codex v1.6, requirements v1.6. Done: P-000..P-002, all of M0 (`m0-done`) and all of M1, the Luna engine (`m1-done`).
- K-M2 and US-010 Done (M2 design: `docs/plans/M2-clan-design.md`; D-02 delegated).
- K-M1b Done: the physics design is `docs/plans/M1b-physics-design.md` (read it first).
- US-025..US-029 Done: Luna Physics complete (Fixed 32.32, Vec3, Quat, shapes, sweeps, spatial grid, ballistics, aim solver, rigid bodies, materials) and the spear throw in the demo.
- X-M1b Done: `main` tagged `m1b-done`, CI green on main.
- All of M2 is built (US-010..US-016 Done). **Kill Gate 1 failed** (owner, 2026-09-30: "not really a story"; D-GATE-M2 = Pivot). The owner chose the redesign (D-18): story arcs on a richer social simulation; quarrels, blame and revenge; sharing and nursing; courtship and rivals; teaching and hunting parties; episodes plus lines with reasons; he judges the retry alone.
- Requirements v1.6 (Drive, mirrored in docs/project/requirements/): STO-02, STO-03, SDC-02, milestone M2b, epic E11, US-110..US-115. **Codex v1.6** (Anima) adds P-005, K-M2b, S-US-110..S-US-115, X-M2b; CI-006 resolved.
- P-005 Done (status, decisions, CI-006 fix: first frame 3 s in Release, 15 s in Debug).
- **M2 and M2b are DONE** (tags `m2-done`, `m2b-done`). Codex v1.7 adds **M2c Level editor** (US-120..US-126, brief docs/plans/M2c-editor-brief.md). **Next prompt: S-US-122**, then S-US-123..S-US-126, X-M2c, then K-M3.
- The M2 work stays on `qa`; `main` is still at `m1b-done` (M2 is not merged to main until the retry passes).
- Luna design for all of M1: `docs/plans/M1-luna-design.md`. Delegated decisions so far: D-16, D-17 (docs/decisions.md).

## How to continue
1. Open your AI coding session in `C:\Users\Amek\.amek-ai\Odysseus\odysseus` (this folder, so the Mraw agents, skills and sync hooks load).
2. `git checkout qa && git pull`.
3. Paste:
```text
Mraw, continue assembly.
Read CLAUDE.md, docs/Codex.md, docs/status.md and Limit.md. Check docs/decisions.md for decisions the owner has answered since the last session and unblock those stories. Then continue with the first prompt in Codex order that is To do or newly unblocked, through the build loop L-01, without waiting for the owner except at the Charter's human gates. Save a Milestone-<n>.md after every story and update Limit.md. End with an assembly report.
```

## Standing rules (docs/decisions.md, Codex v1.3 Charter)
- Blocked owner decisions: Dominus decides (recommended option), records "Decided by Dominus (delegated)".
- Stories: branch from `qa`, merge into `qa`, CI green = Done. Milestone exit: `qa` into `main`, tag.
- After every story: new Milestone-<n>.md (next AP-### ID), CHANGELOG.md entry, update this file.
- Only real stops: kill-gate results that need people (X-M2, X-M6), accounts, credentials, money.

## Tools
- `pwsh tools/verify.ps1 -Story US-xxx`: configure, build Debug and Release, count warnings, run all tests, save evidence to docs/evidence/US-xxx/.

## Environment notes
- Toolchain: Visual Studio 2026 (MSVC 19.51), CMake 4.4.3, vcpkg at `C:\dev\vcpkg`, gh logged in as marius-stoian.
- Never build under %TEMP% (warning MSB8029 breaks zero-warnings).
- Debugger for scripted checks: cdb at `C:\Program Files\WindowsApps\Microsoft.WinDbg_1.2606.22001.0_x64__8wekyb3d8bbwe\amd64\cdb.exe`.
