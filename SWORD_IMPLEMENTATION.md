# Sword Weapon System Implementation

## Summary
A sword/bow weapon switching system for Project Odyssey (M3 melee combat). Players toggle between sword (melee) and bow (ranged) with Shift or Tab, and attack with the Interact button.

## Features

1. **Weapon intent** (`luna/engine/input.h/cpp`): new `SwitchWeapon` intent, bound to Shift (LShift/RShift) and Tab. The SDL3 platform layer recognizes both scancodes.
2. **Sword combat** (`game/sword.h/cpp`):
   - `SwordConfig`: tunable slash duration, cooldown and range.
   - `Sword` class: a slash state machine with three states: `Idle` (ready), `Slashing` (10 ticks), `Cooldown` (15 ticks, then back to Idle).
   - 4-frame slash animation per facing direction.
   - API: `slash(facing)` starts an attack, `update()` advances the state machine, `animationFrame()` returns the frame (0-3), `isAttacking()` checks for a slash in progress.
3. **Weapon switching** (`game/odyssey_game.h/cpp`): `WeaponType` enum (`Sword`, `Bow`), tracked in `currentWeapon_`, toggled on the `SwitchWeapon` press and logged to the console.
4. **Attack handling** (`game/odyssey_game.cpp`): with the sword active, Interact calls `sword_.slash(hero_.facing())`; with the bow active, the existing spear-throw logic runs. Both use the hero's facing.
5. **Rendering** (`game/odyssey_game.cpp`, `game/placeholder_art.h`): the slash animation overlays the character while slashing; idle shows no weapon. `kSwordFrames` defines frames for all 8 directions (4 each). They are placeholders using character sprite positions; real art can come in M3.

## How to Use

Controls:
- **Shift or Tab**: toggle Sword and Bow
- **E, Space, or Enter**: attack (sword: slash in facing direction; bow: throw a spear, existing behavior)
- **WASD/Arrow Keys**: move
- **Escape**: menu

The game loop handles weapon switching on `SwitchWeapon`, attack input by current weapon, sword state updates each tick, and the slash overlay rendering.

## Architecture Notes

- **Layers**: Sword class and weapon system in the Game layer (`src/game/`); SDL3 key mappings in Platform; the intent system in Engine. No dependency on the Simulation layer. Follows the 6-layer architecture (Charter rule 2).
- **State machine**:
```
Idle ← (slash cooldown expires) ← Cooldown ← (slash animation ends) ← Slashing
        ↑ (SwitchWeapon pressed)             (Interact pressed in Idle)
        └─────── Always available ──────────┘
```
- **Determinism**: no floating-point math in sword state, no random numbers or timing dependencies. Animation frame is `floor(slashTicks / slashDuration * 4) % 4`.

## Files Changed

Created:
- `src/game/sword.h`, `src/game/sword.cpp`: sword state machine
- `SWORD_IMPLEMENTATION.md`: this file

Modified:
- `src/luna/platform/events.h`: Shift/Tab keys
- `src/luna/platform/sdl_events.cpp`: SDL3 key mapping
- `src/luna/engine/input.h`: `SwitchWeapon` intent
- `src/luna/engine/input.cpp`: intent binding
- `src/game/odyssey_game.h`: Sword, WeaponType, weapon tracking
- `src/game/odyssey_game.cpp`: weapon switching and attack logic
- `src/game/placeholder_art.h`: sword frame definitions
- `CMakeLists.txt`: added sword.cpp to the build

## Testing

Integrated with the game executable (`odysseus.exe`). To test:

1. Run `./build/windows-x64/bin/Release/odysseus.exe`
2. Press **Shift** to toggle between Sword and Bow
3. Press **E** or **Space** to attack with the current weapon
4. Watch the console for weapon switch messages
5. Observe the slash animation when the sword is active

## Future Enhancements

1. Melee damage: detect collision between slash and enemies, apply damage
2. Sound effects for slash and hit
3. Screen shake on a successful hit
4. Combo system: chain slashes for higher damage
5. Special abilities: power slash, spinning attack, etc.
6. Weapon upgrades: better swords, faster attacks, weapon-specific speed, range and effects
7. Real art: replace the placeholder sword frames with animated sprites
