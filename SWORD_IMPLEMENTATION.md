# Sword Weapon System Implementation

## Summary
Implemented a complete sword/bow weapon switching system for the Project Odyssey game (M3 melee combat). Players can now toggle between sword (melee) and bow (ranged) weapons using the Shift or Tab key, and attack with the Interact button.

## Features Implemented

### 1. **Weapon Intent System** (`luna/engine/input.h/cpp`)
- Added `SwitchWeapon` intent to the Intent enum
- Maps **Shift key** (LShift/RShift) and **Tab key** to weapon switching
- Updated SDL3 platform layer to recognize Shift and Tab key scancodes

### 2. **Sword Combat System** (`game/sword.h/cpp`)
- **SwordConfig**: Tunable parameters for slash duration, cooldown, and range
- **Sword class**: Manages slash state machine with 3 states:
  - `Idle`: Ready to attack
  - `Slashing`: Performing the attack animation (10 ticks)
  - `Cooldown`: Cannot attack (15 ticks before returning to Idle)
- **Animation**: 4-frame slash animation per facing direction
- **API**:
  - `slash(facing)`: Initiate a slash attack
  - `update()`: Advance state machine
  - `animationFrame()`: Get current animation frame (0-3)
  - `isAttacking()`: Check if currently slashing

### 3. **Weapon Switching** (`game/odyssey_game.h/cpp`)
- Added `WeaponType` enum: `Sword` and `Bow`
- Tracks current weapon in `currentWeapon_` member
- Toggles weapon on `SwitchWeapon` intent press
- Logs weapon switch to console

### 4. **Attack Handling** (`game/odyssey_game.cpp`)
- **Sword**: Calls `sword_.slash(hero_.facing())` on Interact when sword is active
- **Bow**: Existing spear throw logic when bow is active
- Both weapons work with the hero's facing direction

### 5. **Rendering** (`game/odyssey_game.cpp`, `game/placeholder_art.h`)
- Sword slash animation overlays on character when slashing
- Added `kSwordFrames` sprite frame definitions for all 8 facing directions (4 frames each)
- Frames are currently placeholders using character sprite positions; real art can be added in M3
- Sword only renders when actively slashing; idle state shows no weapon

## How to Use

### In-Game Controls
- **Shift or Tab**: Toggle between Sword and Bow
- **E, Space, or Enter**: Attack with current weapon
  - Sword: Performs a slash in facing direction
  - Bow: Throws a spear (existing behavior)
- **WASD/Arrow Keys**: Move
- **Escape**: Menu

### Code Integration
```cpp
// Game loop automatically handles:
1. Weapon switching on SwitchWeapon intent
2. Attack input based on current weapon
3. Sword state updates each tick
4. Rendering sword slash animation overlay
```

## Architecture Notes

### Layer Compliance
- **Game layer** (`src/game/`) - Sword class and weapon system
- **Platform layer** - SDL3 keyboard mappings
- **Engine layer** - Input intent system
- No dependencies on Simulation layer
- Follows 6-layer architecture (Charter rule 2)

### State Machine
The sword uses a 3-state machine:
```
Idle ← (slash cooldown expires) ← Cooldown ← (slash animation ends) ← Slashing
        ↑ (SwitchWeapon pressed)             (Interact pressed in Idle)
        └─────── Always available ──────────┘
```

### Determinism
- No floating-point math in sword state
- Animation frame is deterministic: `floor(slashTicks / slashDuration * 4) % 4`
- No random numbers or timing dependencies

## Future Enhancements

1. **Melee Damage System** - Detect collision between slash and enemies
2. **Sound Effects** - Slash and hit sounds
3. **Screen Shake** - Feedback on successful hit
4. **Combo System** - Chain slashes for higher damage
5. **Special Abilities** - Power slash, spinning attack, etc.
6. **Weapon Upgrades** - Better swords, faster attacks
7. **Real Art** - Replace placeholder sword frames with animated sprites

## Files Changed

### Created
- `src/game/sword.h` - Sword state machine
- `src/game/sword.cpp` - Sword implementation
- `SWORD_IMPLEMENTATION.md` - This file

### Modified
- `src/luna/platform/events.h` - Added Shift/Tab keys
- `src/luna/platform/sdl_events.cpp` - SDL3 key mapping
- `src/luna/engine/input.h` - Added SwitchWeapon intent
- `src/luna/engine/input.cpp` - Intent binding
- `src/game/odyssey_game.h` - Added Sword, WeaponType, weapon tracking
- `src/game/odyssey_game.cpp` - Weapon switching and attack logic
- `src/game/placeholder_art.h` - Sword frame definitions
- `CMakeLists.txt` - Added sword.cpp to build

## Testing

The sword system has been integrated with the game executable (`odysseus.exe`). To test:

1. Run the game: `./build/windows-x64/bin/Release/odysseus.exe`
2. Press **Shift** to toggle between Sword and Bow
3. Press **E** or **Space** to attack with the current weapon
4. Watch the console for weapon switch messages
5. Observe slash animation when sword is active

## Next Steps

This foundation enables:
- Melee collision detection (detecting enemies in slash range)
- Damage application to enemies
- Weapon-specific mechanics (different speed, range, effects)
- Combo systems and special attacks
- Weapon progression and upgrades
