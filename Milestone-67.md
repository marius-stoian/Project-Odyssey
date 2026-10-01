# Project Odyssey: assembly progress (67)

## AP-068 · 2026-10-01 · Paused before M8 (owner asked for a break)

| | |
|---|---|
| Assembly plan / requirements | **v2.0** / **v2.0** |
| Repository | M7 on `qa` (X-M7 done, not yet on `main`); `chore/k-m8` (K-M8 docs); `story/US-160` (work in progress) |
| Milestone | M7 built; M8 kicked off, US-160 built but not yet verified or merged |

### State
- X-M7 review is committed on `qa` (`399da4c`): `docs/gates/M7.md`, screenshots in `docs/evidence/X-M7/`. CI on `qa` for that commit was still running; **merge `qa` into `main`, wait for green CI on `main`, tag `m7-done`** is still to do.
- K-M8 is done (D-38, `docs/plans/M8-dialogue-design.md`) on branch `chore/k-m8`; `story/US-160` is based on it.
- US-160 (the `.dlg` format) is built: parser, writer, library, elder-fire.dlg, guide, game load and F5; its 10 simulation tests and the game test pass in Debug. The full `tools/verify.ps1 -Story US-160` (Debug and Release, all tests) has **not** been run to a pass yet (the one run failed on a compile error in a new test, now fixed).
