# ADR-018: PNG with stb, and our own editor UI

**Status:** Accepted (M2c, US-120, 2026-09-30)

## Context
The owner's art (D-05) arrives as PNG sprite sheets, and the level editor (D-19) needs buttons, lists and text on screen. Luna could read only the pictures it drew by code, and had no UI.

## Decision
1. **stb_image and stb_image_write** read and write PNG. They come from vcpkg's `stb` port (D-13: libraries come from vcpkg). Their code is compiled once, in `src/luna/engine/image_io.cpp`, with their warnings switched off there only; nothing else includes them.
2. **The editor UI is our own small toolkit** in Luna Engine (bitmap font, panel, button, list, number and text fields), drawn with the texture-only renderer. Dear ImGui (D-13) is not used for it: ImGui draws through a platform backend (SDL3 renderer), which would put SDL3 calls outside `src/luna/platform/` (Charter rule 2), and its look does not match pixel art at 480x270.

## Consequences
- Art is cut into atlases by `odysseus_atlas` from rectangles in `assets/sprites/cuts.json`; the game reads only the atlases.
- The toolkit is more code to write than ImGui, but it stays game-agnostic and fully testable headless.
- stb is public domain (or MIT, at the user's choice): no licence obligations for the game.
