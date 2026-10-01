# Test debt (P-009)

Since M2d the owner asked for no tests or CI, so the M2d-M6 tests were compiled but not all run. P-009 paid that debt on `qa` (2026-10-01).

## Result of `pwsh tools/verify.ps1 -Story P-009`

| Check | Debug | Release |
|---|---|---|
| Build warnings | 0 | 0 |
| Build exit code | 0 | 0 |
| ctest | 25 of 25 passed | 25 of 25 passed |

Evidence: `docs/evidence/P-009/windows-debug.txt` and `windows-release.txt`.

- Failures found: **none**. The fixes made at the gate (commit `03fc5f4`, "Fix tests and small bugs found at the gate") and the window-test fix (`4f2ba10`) already cover everything the full run exercises.
- Fixes in this run: none, so no `P-009: fix <test>` commits.
- Tests changed: none.
- CI on `qa`: the first push (757f813 and 8f0d6a7) failed once on the CI Release runner: `US-029 Throw in the game` ended with the spear already landed, because one slow frame (12 FPS runner) let the spear land before the wall-clock quit. Fix `9a4b200` ("P-009: fix US-029 flight run timing on slow CI runners"): the in-flight run quits at 0.75 s and is tried up to 3 times; the check is unchanged. Green: [run 36831204118](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36831204118).
- CI speed (owner request): Debug tests had taken 499 s (game tests) and 225 s (sim tests) one after another. Commit `820d629` runs the non-window tests with `-j 4`, splits `odysseus_game_tests` into three runs by source file, and thins three Debug-only loops (Release keeps the full sizes). Green: [run 36832983954](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36832983954); Test Debug 5 min 38 s, Test Release 1 min 28 s, whole run 17.5 min.

## Exit checks that were owed, now closed by this run

| Gate file | Owed check | Status |
|---|---|---|
| M2d | Debug and Release verify, zero warnings, all tests (US-136..US-138, hero orientation) | Done by this run |
| M2d | Toolbar labels changed; the scripted end-to-end tests click by position | Done: the scripted window tests pass (`US-123`, `US-124`, `US-125`, `X-M2c`) |
| M4 | Tests of `tests/sim/region_test.cpp` and `tests/game/m4_test.cpp` | Done by this run (inside `odysseus_sim_tests` and `odysseus_game_tests`) |
| M5 | `tests/sim/hero_test.cpp` never run | Done by this run |
| M6 | `tests/game/m6_test.cpp` never run | Done by this run |

## Still owed (not a test-run matter)

- M5: `tests/game/run_test.cpp` was never written; the owner's full play-through.
- M6: the zip on a clean PC, a 100-year headless soak, the 2-hour stability session, and the 8 playtesters (X-M6 waits for X-M9).
