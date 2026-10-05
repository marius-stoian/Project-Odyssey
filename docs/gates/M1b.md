# Exit review M1b: Luna Physics (deterministic 3D)

Codex v1.5, prompt X-M1b, 2026-09-30. Exit criteria: Luna Physics passes its math, hit-detection, ballistics and rigid-body tests identically on every build; in the demo the hero throws a spear that flies in an arc with a shadow and hits a target.

| Criterion | Result | Evidence |
|---|---|---|
| Math tests pass identically on every build | **Met** | US-025: exact raw values (independent exact fractions, plus 100,000 cases against the CPU's 128-bit instructions); one million mixed operations hash to **17224312723153614174** in Debug, Release and on GitHub's CI runner (pinned, so any difference fails) |
| Hit-detection tests pass | **Met** | US-026: touching sphere/box contact point and normal; a spear tip at 10 m per tick finds a 0.2 m target at 0.498 of the tick; 1,000 bodies: 114 of 499,500 pairs tested, same contacts as all pairs, 0.56-0.61 ms per step (Release) |
| Ballistics tests pass | **Met** | US-027: see the textbook table below; aim solver hits a straw target 25 m away |
| Rigid-body tests pass | **Met** | US-028: 140 N s on 70 kg gives 2 m/s; bounce heights and friction distance as below |
| ... identically on every build (all of Luna Physics together) | **Met** | New gate test `M1b Whole physics is identical on every build`: 12 throws with drag and wind, 400 ticks of a bouncing, sliding ball, and a 1,000-body contact step hash to **17309765312882650619** in Debug and Release (pinned; CI on `qa` and `main` must match) |
| In the demo the hero throws a spear that flies in an arc with a shadow and hits a target | **Met** | US-029: `US-029 Throw` (hand 1.41 m, top 1.61 m, hit at 1.09 m; the shadow is drawn exactly height x 32 px below the spear in every frame) and `US-029 Throw in the game` in the real window: "Spear (flint) hit the straw target ... 69.5 damage" ([log](../evidence/US-029/game-log-throw.txt)); screenshot below |

## Textbook comparisons
| Check | Luna Physics | Textbook | Difference |
|---|---|---|---|
| Range, 20 m/s at 45 degrees, no air | 40.704 m | v^2/g = 40.775 m | 0.17% (1% allowed) |
| Flight time, same throw | 2.878 s | 2 v sin(a)/g = 2.883 s | 0.17% |
| Range with drag (1.5 kg spear, Cd*A 0.01 m^2) | 36.098 m | drag equation (Runge-Kutta, 0.1 ms) 36.174 m | 0.21% |
| Drift in a 5 m/s crosswind | 1.1998 m | drag equation 1.2017 m | 0.16% |
| Bounce heights, restitution 0.5, from 1 m | 0.249999, 0.0624997, 0.0156245, 0.00390611 m | e^2 per bounce: 0.25, 0.0625, 0.015625, 0.00390625 m | below 0.01% |
| Friction stop, 3 m/s, mu = sqrt(0.4 x 0.35) | 1.22597 m | v^2/(2 mu g) = 1.22597 m | below 0.001% |

Evidence: [physics-Debug.txt](../evidence/M1b/physics-Debug.txt), [physics-Release.txt](../evidence/M1b/physics-Release.txt) (19 cases, 404,518 assertions), `tools/verify.ps1 -Story M1b`: 0 warning lines, ctest 17/17 in Debug and Release ([Debug](../evidence/M1b/windows-debug.txt), [Release](../evidence/M1b/windows-release.txt)).

![The flint spear in flight, lifted above its shadow, towards the straw target](../evidence/M1b/spear-in-flight.png)

Stories: US-025, US-026, US-027, US-028, US-029, all Done.
Decisions in M1b: none needed from the owner. Design choices Dominus made inside the stories (recorded in the plans): portable 128-bit arithmetic, rounding to nearest, Minkowski-sum sweeps, semi-implicit Euler with 10 sub-steps for projectiles, exact constant-acceleration steps for bodies, the damage formula in materials.json, the open target placed west so the throw fits on screen.

**Result: all criteria met.** `qa` merges into `main`, CI on `main` must be green, then `main` is tagged `m1b-done`. M2 resumes with S-US-011 (paused branch `story/US-011`).
