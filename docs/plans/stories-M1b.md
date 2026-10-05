# Story plans: M1b

Per-story plans for milestone M1b, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-025](#us-025)
- [US-026](#us-026)
- [US-027](#us-027)
- [US-028](#us-028)
- [US-029](#us-029)

---

<a id="us-025"></a>

## Plan US-025: Build deterministic 3D math

Codex v1.5, prompt S-US-025. Design: [M1b physics design](M1b-physics-design.md). Traces to PHY-01, ARC-10, ADR-017. Charter rules 1, 9 and 10.

### Files
| File | Layer | What |
|---|---|---|
| `src/luna/physics/boundary.h`, `layer.h` | Physics | The new layer's identity check (`ODYSSEUS_LAYER_PHYSICS`); may be included by Physics, Engine, Simulation and Game only |
| `src/luna/physics/fixed.{h,cpp}` | Physics | `Fixed` 32.32: `+ -` (overflow asserted), `* /` through our own 128-bit schoolbook multiply and long division, rounded to nearest; `fromInt`, `fromRatio`, `fromRaw`; `sqrt` (exact floor), `sin`/`cos` (quadrant folding + Taylor series on [0, pi/4]), `degrees()`, `kPi` and friends |
| `src/luna/physics/vec3.{h,cpp}` | Physics | `Vec3` (x east, y south, z up): arithmetic, `dot`, `cross`, `length`, `normalized` |
| `src/luna/physics/quat.{h,cpp}` | Physics | `Quat`: `fromAxisAngle`, Hamilton product, `conjugate`, `rotate`, `normalized` |
| All six `boundary.h` | every layer | Count the Physics identity; Platform, Engine, Simulation and Game headers refuse Physics code; Physics headers refuse Core and Platform code |
| `cmake/ValidateLayerIncludes.cmake` | build | `luna/physics/` is a layer; Physics may include Core; Engine, Simulation and Game may include Physics |
| `CMakeLists.txt` | build | `luna_physics` (links Core only); Engine and Simulation link it; `luna_physics_tests`; the no-floating-point review |

No floating point in `src/luna/physics/` (Charter rule 10): constants are written as ratios (`Fixed::fromRatio(981, 100)` is 9.81) and doubles appear only in tests as reference values. Drawing conversion (`toDouble`) belongs to the Engine and arrives with US-029.

### Tests
| Scenario | Test |
|---|---|
| Exact arithmetic: add, multiply, divide equal the expected values bit for bit in Debug, Release and CI | `US-025 Exact arithmetic` (expected raw values computed independently with exact fractions) and `US-025 Exact arithmetic matches the 128-bit hardware instructions` (100,000 random multiplications, divisions and square roots against the CPU's `_umul128`/`_udiv128`) |
| Rotations: 4 x 90 degrees about up returns within 1/65536 | `US-025 Rotations` (also: one quarter turn takes (1,2,3) to (-2,1,3); the joined quaternion gives the same full turn) |
| Determinism: two runs of 1,000,000 mixed operations give the same hash | `US-025 Determinism`: equal hashes, a different seed differs, and the hash is pinned (17224312723153614174) so Debug, Release and CI must agree bit for bit |
| Layer rules (story instructions) | `US-025 Physics layer rules`: 11 compiler probes (4 allowed, 7 forbidden, including an absolute-path bypass and Platform including Physics); validator fixture: 4 more violations added after configure |
| Charter rule 10 | `US-025 Physics uses no floating point` (source review of src/luna/physics) |
| Supporting | `US-025 Sine and cosine are accurate to 1e-9` (2,001 angles over +/-4 turns), `US-025 Vectors` |

Manual checks: none needed (everything is headless and automated).

### Risks
- Rounding: every multiply rounds to nearest, so long chains drift by about 1e-10 per step; quaternions are renormalised when they are combined repeatedly.
- Range: squared lengths overflow above about 46,000 m; the MVP region is 256 m across (documented in `vec3.h`).

### Verification results (2026-09-30)
| Check | Result | Evidence |
|---|---|---|
| Builds | Debug and Release, 0 warning lines | `tools/verify.ps1 -Story US-025` |
| ctest | 16/16 Debug and Release | [windows-debug.txt](../evidence/US-025/windows-debug.txt), [windows-release.txt](../evidence/US-025/windows-release.txt) |
| Physics tests | 6 cases, 404,040 assertions; mixed-math hash 17224312723153614174 in Debug and Release | [Debug](../evidence/US-025/doctest-physics-Debug.txt), [Release](../evidence/US-025/doctest-physics-Release.txt) |
| Layer probes | 4 allowed compile; 7 forbidden fail with C1083/C1189 naming the header | [probes.txt](../evidence/US-025/probes.txt) |

Found and fixed during testing: the floating-point review's comment-stripping regex had lost its escaping and failed to compile; corrected, and checked that a planted `double x = 1.5;` is caught.

Acceptor verdict (2026-09-30): **ACCEPT**. Exact arithmetic, Rotations and Determinism pass with evidence in both configurations; every Definition of Done line holds (CI on `qa` confirms after the merge).

---

<a id="us-026"></a>

## Plan US-026: Detect hits between shapes

Codex v1.5, prompt S-US-026. Design: [M1b physics design](M1b-physics-design.md). Traces to PHY-02, ARC-10. Depends on US-025 (Done).

### Files
| File | Layer | What |
|---|---|---|
| `src/luna/physics/shapes.{h,cpp}` | Physics | `Sphere`, `Capsule`, `Box` (axis-aligned), `Shape` = `std::variant` of them; `bounds()`; `overlap()` for all six pairings with contact point, normal (first -> second) and depth; `raycast()`; `sweep()` of a sphere against any shape (ray against the shape grown by the radius: a Minkowski sum; a grown box = 3 slabs + 12 edge capsules, so corners are round and exact); closest-point helpers |
| `src/luna/physics/spatial_grid.{h,cpp}` | Physics | `SpatialGrid` (2 m cells over x/y, shapes filed in every cell their bounds touch, sorted de-duplicated candidate pairs); `findContacts()` = one collision step (broad phase + narrow phase) |

Safety against overflow: every "how far along" division goes through `ratio01` (result limited to [0, 1]) or `limitedRatio` (limited to +/-1024), so nearly parallel segments or nearly still rays never divide by almost zero.

### Tests (`luna_physics_tests`)
| Scenario | Test |
|---|---|
| Overlap: a sphere and a box that touch report a hit with contact point and normal | `US-026 Overlap`: touching at x = 1 -> point (1, 0, 0), normal +x, depth 0; 0.25 m inside -> depth 0.25; reversed order flips the normal; 1 mm apart -> no hit |
| No tunnelling: 10 m per tick towards a 0.2 m target; the swept path finds the hit at the correct time of impact | `US-026 No tunnelling`: both tick positions miss (proof that point checks tunnel), the sweep hits at time 0.498 of the tick = 0.0249 s (box), 0.488 (round target and post); 0.2 m to the side misses |
| Many bodies: 1,000 bodies, one step through the grid, only nearby pairs, under 2 ms | `US-026 Many bodies`: 114 pairs tested instead of 499,500, the same contacts as testing every pair, fastest of 20 steps timed (Release asserts < 2 ms) |
| Supporting | `US-026 Overlap between every pair of shape kinds`, `US-026 Rays and rounded corners` (a ball meets a box edge at t = 1 - 1/(2 sqrt 2)) |

Manual checks: none (headless). The 2 ms budget is measured on the development PC in Release (the build players get); Debug with AddressSanitizer is about 5x slower and only reports its time.

### Verification results (2026-09-30)
| Check | Result | Evidence |
|---|---|---|
| Builds | Debug and Release, 0 warning lines | `tools/verify.ps1 -Story US-026` |
| ctest | 16/16 Debug and Release | [windows-debug.txt](../evidence/US-026/windows-debug.txt), [windows-release.txt](../evidence/US-026/windows-release.txt) |
| US-026 cases | 5 cases pass; 114 pairs tested of 499,500; one step with 1,000 bodies: 0.61 ms Release (3.2 ms Debug with AddressSanitizer) | [Debug](../evidence/US-026/doctest-physics-Debug.txt), [Release](../evidence/US-026/doctest-physics-Release.txt) |

Found and fixed during testing: the capsule-box search stopped up to 1.5e-5 m off the true closest point (distances there differ by less than one Fixed step), tilting the contact normal; two alternating-projection steps now land on the exact closest pair.

Acceptor verdict (2026-09-30): **ACCEPT**. Overlap, No tunnelling and Many bodies pass with evidence; Definition of Done holds (CI on `qa` confirms after the merge).

---

<a id="us-027"></a>

## Plan US-027: Fly projectiles with real ballistics

Codex v1.5, prompt S-US-027. Design: [M1b physics design](M1b-physics-design.md) (Motion). Traces to PHY-03, ADR-017. Depends on US-026 (Done).

### Files
| File | Layer | What |
|---|---|---|
| `src/luna/physics/fixed.{h,cpp}` | Physics | `atan2` (octant folding + arctangent series), needed by the aim formula |
| `src/luna/physics/ballistics.{h,cpp}` | Physics | `Air` (density 1.225 kg/m^3, wind, gravity 9.81), `Projectile` (position, velocity, mass, Cd*A); `projectileAcceleration` (gravity + quadratic drag against the air's motion), `stepProjectile` (semi-implicit Euler), `stepProjectileTick` (10 sub-steps per 1/20 s tick), `flyTick` (sub-steps swept against obstacles: no tunnelling), `flyUntilLanding` (interpolated ground crossing), `launchAngleWithoutDrag` (textbook low-arc formula), `aimLaunchAngle` (vacuum angle refined with drag by the secant method), `launchVelocity` |

Integration: semi-implicit Euler at 200 Hz (10 sub-steps per tick). Its landing-time error is about one sub-step (0.005 s of 2.9 s = 0.17%), well inside the 1% the story allows.

### Tests (`luna_physics_tests`)
| Scenario | Test |
|---|---|
| Arc: 45 degrees, 20 m/s, flat ground, no drag -> range v^2/g = 40.8 m within 1% | `US-027 Arc`: 40.704 m vs 40.775 m (0.17%); flight time 2.878 s vs 2.883 s |
| Drag and wind: falls short and drifts downwind by the amounts the formulas predict | `US-027 Drag and wind`: the drag equation F = 1/2 rho Cd A |v - w| (v - w) has no closed-form solution, so the test solves it independently with doubles and 4th-order Runge-Kutta at 0.1 ms steps. Spear 1.5 kg, Cd*A 0.01 m^2: 36.10 m (formula 36.17 m); with a 5 m/s crosswind 35.92 m and 1.200 m drift (formula 35.999 m, 1.202 m); all within 1% |
| Aim: target 25 m away, solve the launch angle for a speed, the throw hits | `US-027 Aim`: 18 m/s from 1.5 m to a 0.3 m straw target 25 m away diagonally: vacuum 23.2 deg, with drag 25.8 deg; thrown tick by tick with swept collisions it hits the target's front face after 33 ticks; a 100 m target is reported out of reach |
| Supporting | `US-027 atan2 is accurate to 1e-9` |

Manual checks: none (headless).

### Verification results (2026-09-30)
| Check | Result | Evidence |
|---|---|---|
| Builds | Debug and Release, 0 warning lines | `tools/verify.ps1 -Story US-027` |
| ctest | 16/16 Debug and Release | [windows-debug.txt](../evidence/US-027/windows-debug.txt), [windows-release.txt](../evidence/US-027/windows-release.txt) |
| US-027 cases | 4 cases pass; measured values in the MESSAGE lines | [Debug](../evidence/US-027/doctest-physics-Debug.txt), [Release](../evidence/US-027/doctest-physics-Release.txt) |

Found during testing: the first Aim check expected the hit at the target's centre height; a descending spear correctly meets the front of a round target 0.14 m higher. The check now asserts a hit on the target's surface, on the thrower's side.

Acceptor verdict (2026-09-30): **ACCEPT**. Arc, Drag and wind, and Aim pass with evidence; Definition of Done holds (CI on `qa` confirms after the merge).

---

<a id="us-028"></a>

## Plan US-028: Push and bounce bodies

Codex v1.5, prompt S-US-028. Design: [M1b physics design](M1b-physics-design.md) (Motion). Traces to PHY-04, ADR-017. Depends on US-026 (Done).

### Files
| File | Layer | What |
|---|---|---|
| `src/luna/physics/rigid_body.{h,cpp}` | Physics | `SurfaceMaterial` (restitution, Coulomb friction) and the combining rules (max restitution, sqrt(mu1 mu2)); `Ground`; `RigidBody`: a class whose invariants (positive mass, matching inverse mass, sleeping bodies never move) are checked in the constructor and kept by private data. `applyImpulse` (v += J/m), `applyForce` (for the next step), `step()`: flight with the exact constant-acceleration update, impact times solved inside the step (several bounces per step allowed), bounce with restitution and friction impulse, rest below 0.05 m/s, sliding with Coulomb friction (stops exactly at v^2/(2a)), sleep after 10 still steps, wake on any push |

Scope: bodies against the ground, as the acceptance criteria ask. Body-to-body contacts are not part of this story.

### Tests (`luna_physics_tests`)
| Scenario | Test |
|---|---|
| Impulse: 70 kg at rest, 140 N s -> 2 m/s in the impulse direction | `US-028 Impulse`: (1.2, 1.6) m/s towards (3, 4), speed 2 within 1e-9; also 140 N for 1 s on ice -> 2 m/s |
| Bounce and rest: restitution 0.5, each bounce a quarter of the previous height, comes to rest | `US-028 Bounce and rest`: hop heights 0.25, 0.0625, 0.0156, 0.0039 m (ratios 0.24999...), then asleep on the ground; also at the game's 20 ticks per second |
| Friction: a crate sliding on grass stops within the distance friction predicts | `US-028 Friction`: 3 m/s, mu = sqrt(0.4 x 0.35): slid 1.22597 m, v^2/(2 mu g) = 1.22597 m; asleep, then woken by a new push |

Manual checks: none (headless).

### Verification results (2026-09-30)
| Check | Result | Evidence |
|---|---|---|
| Builds | Debug and Release, 0 warning lines | `tools/verify.ps1 -Story US-028` |
| ctest | 16/16 Debug and Release | [windows-debug.txt](../evidence/US-028/windows-debug.txt), [windows-release.txt](../evidence/US-028/windows-release.txt) |
| US-028 cases | 3 cases pass; bounce ratios 0.249999, 0.249999, 0.249994, 0.249999; crate slid 1.22597 m of a predicted 1.22597 m | [Debug](../evidence/US-028/doctest-physics-Debug.txt), [Release](../evidence/US-028/doctest-physics-Release.txt) |

Found and fixed during testing: the impulse multiplied by a rounded 1/m (error 1e-8); it now divides by the mass once (error below 1e-9).

Acceptor verdict (2026-09-30): **ACCEPT**. Impulse, Bounce and rest, and Friction pass with evidence; Definition of Done holds (CI on `qa` confirms after the merge).

---

<a id="us-029"></a>

## Plan US-029: Throw a spear in the demo

Codex v1.5, prompt S-US-029. Design: [M1b physics design](M1b-physics-design.md) (Materials; Drawing 3D top-down). Traces to PHY-02, PHY-03, PHY-05. Depends on US-024, US-027, US-028 (Done).

### Files
| File | Layer | What |
|---|---|---|
| `src/luna/physics/material.{h,cpp}` | Physics | `Material` (density, Mohs hardness, sharpness, surface), `massOf`, `kineticEnergy` |
| `src/luna/physics/ballistics.cpp` | Physics | `flyTick` skips obstacles outside the sub-step's path box (cheap broad phase for about 170 boulders) |
| `src/luna/engine/physics_view.{h,cpp}` | Engine | The only place physics numbers become floats: `toDouble`, `metresFromPixels`, `topDownPosition` (screen y = (y - z) x 32), `groundShadow`, `topDownDirection` |
| `assets/data/materials.json` | Data | Materials (flint, wood, straw, stone), the damage scale, the two spears (shaft mass, tip volume, tip material, throw speed) |
| `src/game/materials.{h,cpp}` | Game | `loadMaterials` with validation naming file and field (reuses the Simulation's JSON helpers and `DataError`); `impactDamage` = kinetic energy x hardness / scale x sharpness |
| `src/game/spear_range.{h,cpp}` | Game | `SpearRange`: rock tiles become 1.5 m boulders, the ground a slab, straw targets (0.6 x 0.6 x 1.6 m); `throwSpear` auto-aims at the target in front (45 degree cone, aim solver with drag); `update` flies each spear one tick with swept collisions; spears stick where they strike |
| `src/game/placeholder_art.{h,cpp}` | Game | Prop sheet: flint and wooden spears in 8 directions, straw target (untouched and hit), soft shadow; `facingForVector` |
| `src/game/odyssey_game.{h,cpp}`, `test_map.cpp`, `apps/odysseus/main.cpp` | Game, App | Interact throws (flint, then wooden, alternating); targets 8 tiles west (open) and 8 tiles north behind a new boulder; drawing order: shadows, targets, hero, spears; the camera frames hero and target for 2 s after a throw; hits and target totals are logged |

Design note (Dominus): the view is 15 x 8.4 tiles, so a target 8 tiles away fits on screen only sideways; the open target stands west and the camera frames the throw. No owner decision was needed.

### Tests
| Scenario | Test |
|---|---|
| Throw: facing a straw target 8 tiles away, Interact -> a spear flies in a visible arc with a shadow and hits the target | `US-029 Throw` (game test with a recording renderer): hand 1.41 m, top 1.61 m, hit at 1.09 m after 11 ticks; every frame draws the shadow exactly the spear's height x 32 px below it. `US-029 Throw in the game` (real window, scripted input): the log reports the hit; screenshot [spear-in-flight.png](../evidence/US-029/spear-in-flight.png) |
| Material: flint-tipped and wooden spears (materials.json) -> damage differs by hardness and density as configured | `US-029 Material`: flint 69.27, wooden 12.01, each equal to the formula computed from the file's numbers. `US-029 Materials are validated`: a hardness of 12 is rejected, naming `materials.json` and `materials.flint.hardness` |
| Blocked: a target behind a rock -> the spear hits the rock and stops; the target is unharmed | `US-029 Blocked`: hits the boulder's south face (y = 26.00) at 1.36 m, stuck, velocity 0; the target has 0 hits and 0 damage |

Manual check (done 2026-09-30, from the automated screenshots): the in-flight frame shows the hero facing west, the spear lifted above its shadow, and the target ahead; the after-hit frame shows the spear stuck in the target.

### Verification results (2026-09-30)
| Check | Result | Evidence |
|---|---|---|
| Builds | Debug and Release, 0 warning lines | `tools/verify.ps1 -Story US-029` |
| ctest | 17/17 Debug and Release (new: `US-029 Throw in the game`) | [windows-debug.txt](../evidence/US-029/windows-debug.txt), [windows-release.txt](../evidence/US-029/windows-release.txt) |
| Game tests | 4 US-029 cases pass | [doctest-game-Release.txt](../evidence/US-029/doctest-game-Release.txt) |
| Real window | Threw a flint spear facing West; hit the straw target at 1.08 m, 12.8 m/s, 69.5 damage; the target behind the boulder untouched | [game-log-throw.txt](../evidence/US-029/game-log-throw.txt), [spear-in-flight.png](../evidence/US-029/spear-in-flight.png), [spear-hit.png](../evidence/US-029/spear-hit.png) |

Found and fixed during testing: the first layout put the open target 8 tiles south, below the bottom of the 8.4-tile-high view, so the arc could not be seen; the target moved west (15 tiles of view) and the camera now frames the throw. The real-window damage varies slightly between runs (69.3 to 69.5) because the scripted turn lasts one or two ticks depending on frame timing; each run is deterministic for its inputs.

Acceptor verdict (2026-09-30): **ACCEPT**. Throw, Material and Blocked pass with evidence, including the real window; Definition of Done holds (CI on `qa` confirms after the merge).
