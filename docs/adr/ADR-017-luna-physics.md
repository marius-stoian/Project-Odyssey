# ADR-017: Own deterministic 3D physics in Luna (Luna Physics)

Status: Accepted (owner decision, 2026-09-30; requirements v1.5, PHY-01..PHY-06, ARC-10).

## Context
The owner wants physics in the engine: hit detection, ballistics, rigid bodies, and later explosions, vehicles, orbits and element chemistry ("hard math"). The world is drawn top-down in 2D pixel art, but projectiles need real height, arcs and rotation. Simulation results must be identical on every computer (ADR-011, Charter rule 6), and the Simulation must be able to use physics without graphics.

## Options
| Option | For | Against |
|---|---|---|
| **Own physics, deterministic fixed-point 3D (chosen)** | Exact, repeatable results on every PC and every run; tailored to top-down worlds with real height; the math is part of the learning goal; no dependency | The most code and math to write and test |
| Box2D v3 (MIT, vcpkg) | Proven, fast, deterministic | Side-view 2D model; height and 3D rotation would still be ours; less learning |
| Own physics with floats | Simpler code | Floating-point results can differ between compilers and CPUs; breaks determinism |

## Decision
- A new Luna layer, **Physics** (`src/luna/physics`, target `luna_physics`, namespace `luna::physics`), between Core and the Engine and Simulation layers. It uses Core only; Engine and Simulation may use it (ARC-10, Charter rule 1).
- All physics state uses **fixed-point 32.32** numbers (`luna::physics::Fixed`: a 64-bit integer with 32 fractional bits), never float or double; the Engine converts results to floats only for drawing.
- **SI units**: metres, seconds, kilograms. One 32-pixel tile is 1 metre (a 48-pixel person is 1.5 m tall).
- Full 3D vectors and quaternion rotations; the top-down view draws height by lifting the sprite and keeping a shadow on the ground.
- Every feature is tested against its textbook formula (projectile range, bounce heights, friction distance).
- MVP scope (M1b): 3D math, hit detection, ballistics, basic rigid bodies, material properties. Later Ages: explosions, vehicles, orbital mechanics, heat, fire spread and chemistry (PHY-05, PHY-06).

## Consequences
More code to write than with a library, and overflow rules must be designed carefully (documented in the M1b design). In return, a replay or a save always reproduces the same hits and throws, and Luna Physics can be reused by any future game.
