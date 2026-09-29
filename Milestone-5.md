# Project Odyssey: assembly progress (5)

## AP-006 · 2026-09-30 · after US-021

| | |
|---|---|
| Codex / requirements | v1.3 / v1.4 |
| Repository | `qa` @ `f22a5ef` (CI running at save time; see the next snapshot) |
| Milestone | **M1 Luna engine: 2 of 5** |
| Next | S-US-022 crisp pixel-art sprites |

### Story just finished: US-021 Control the game through intents
- Keyboard (WASD or arrows, E/Space/Enter, Esc) and gamepad (left stick, D-pad, South button, Start) produce the same **intents**: Move Up/Down/Left/Right, Interact, Open Menu. The game reads intents only; an automated review checks that.
- Keys are physical positions, so WASD also works on AZERTY/QWERTZ keyboards; gamepads can be plugged in and out while playing.
- Evidence: 9/9 ctest in Debug and Release; 40 input assertions. No physical gamepad was connected: gamepad input is proven with synthetic SDL events through the real translation and bindings. [Plan](docs/plans/US-021.md).

### Delegated decisions: none new. Codex issues: none.
### Progress: **7 / 44 stories** (M0 4/4, M1 2/5)
