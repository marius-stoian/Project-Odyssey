# Luna Physics design (M1b)

Architect's design for M1b, shared by US-025..US-029. Codex v1.5; requirements v1.5 (PHY-01..PHY-05, ARC-10, ADR-017). Charter rules 1, 9 and 10.

## The layer
`src/luna/physics/`, CMake target `luna_physics`, namespace `luna::physics`, identity `ODYSSEUS_LAYER_PHYSICS`. Uses Core only. May be used by Engine, Simulation and Game. Never includes Platform, Engine, Simulation, Game or SDL3. US-025 extends the ADR-016 enforcement (boundary.h in every layer, the include validator's layer table, compiler probes).

| From \ may include | core | platform | physics | engine | sim | game |
|---|---|---|---|---|---|---|
| core | yes | | | | | |
| platform | yes | yes | | | | |
| **physics** | yes | | **yes** | | | |
| engine | yes | yes | **yes** | yes | | |
| sim | yes | | **yes** | | yes | |
| game | yes | yes | **yes** | yes | yes | yes |

## Numbers: `Fixed` (32.32)
- A 64-bit signed integer holding value x 2^32: 32 integer bits (range about +/-2.1 billion), 32 fractional bits (resolution 2.3e-10). Metres, seconds, kilograms: plenty for a region (the MVP world is 256 m across); space Ages will use a larger unit (PHY-06).
- `+`, `-`: plain integer add and subtract. `*`: the full 128-bit product, shifted back by 32, computed with our own portable 32-bit-limb multiplication (no compiler-specific 128-bit types), rounded to nearest. `/`: 128-by-64-bit long division, rounded to nearest. Overflow is a bug: asserted in Debug (ADR-015).
- `sqrt`: integer square root of the 128-bit value (bit-by-bit, exact floor). `sin`/`cos`: angle reduction to [0, pi/2] plus a fixed-point polynomial; `atan2`: polynomial with octant folding. All built from integer operations only, so every compiler and CPU gives the same bits.
- Conversions from `double` exist only for literals in tests and for the Engine's drawing (`toDouble()`), never inside physics steps.

## Vectors and rotations
`Vec3` (x east, y south, z up; matches screen x/y for top-down drawing), dot, cross, length, normalise. `Quat` (unit quaternion): from axis-angle, multiply, rotate a vector, normalise. Tests: 4 x 90 degrees about z returns to the start within 1/65536.

## Shapes and hits (US-026)
Sphere, capsule (segment + radius), axis-aligned box. Queries: overlap with contact point, normal and depth; raycast; swept sphere (continuous) against each shape for time of impact. Broad phase: a uniform spatial grid (cell = 2 m) over the region; pairs only from neighbouring cells.

## Motion (US-027, US-028)
- Integrator: semi-implicit Euler at the simulation tick (20 Hz, dt = 1/20 s), with sub-steps for fast projectiles (swept tests prevent tunnelling anyway).
- Ballistics: gravity 9.81 m/s^2 down (z), quadratic air drag (0.5 rho Cd A v^2 / m), wind as air velocity. The aim solver solves the drag-free launch angle analytically (low arc: theta = atan((v^2 - sqrt(v^4 - g(g x^2 + 2 y v^2))) / (g x))), then refines with drag by a few simulated iterations.
- Rigid bodies: mass, velocity, angular velocity (spin shown for spears), impulses, ground contact with restitution and Coulomb friction; bodies sleep when slow.
- Tests compare with textbook results: range v^2 sin(2a)/g, bounce height ratio e^2, friction stopping distance v^2 / (2 mu g).

## Materials (US-029, PHY-05)
`assets/data/materials.json`: density (kg/m^3), hardness (Mohs), sharpness; validated at load (Charter rule 7). Impact damage = kinetic energy x a material factor. Chemistry (heat, reactions) comes with later Ages.

## Drawing 3D top-down (US-029)
One tile (32 px) = 1 m. Screen x = x * 32, screen y = (y - z) * 32: height lifts the sprite; a shadow is drawn at the ground point (z = 0). The spear sprite is chosen by its direction (8 directions).
