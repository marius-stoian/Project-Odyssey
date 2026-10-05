# Exit review M2c: Level editor (Game mode and Editor mode)

Codex v1.7, prompt X-M2c, 2026-09-30. Exit criteria: the owner switches between Game mode and Editor mode; in the Editor he paints ground tiles, places characters with properties and sets the level's name, size, default ground and hero start; he saves, reloads and plays the level; the game draws his own art; the spear and sword demos still work. No kill gate.

**Result: all criteria met.** `qa` is merged into `main` and tagged `m2c-done`.

## 1. Game mode and Editor mode: **Met**
F2 opens the Editor (the world pauses, the camera pans); F1 plays the level again from the hero start. The corner says which mode. Tests: `US-123 Switch`, `US-123 Back to play`, `US-123 Game untouched`, ctest `US-123 Modes in the game`.

## 2. Paint, place, set: **Met**
- Brush, rectangle, flood fill and eraser with 16 grounds (`US-124 ...`, ctest `US-124 Paint in the game`).
- Characters can be placed, selected, moved, turned, deleted and given a name, HP and sword damage (`US-125 ...`, ctest `US-125 Place in the game`).
- Name, size, default ground, hero start, New and Open (`US-126 ...`).
- Every change is undoable (`US-124 Undo`: random sequences).

## 3. Save, reload, play: **Met**
A scripted session on a copy of the demo level ([evidence](../evidence/X-M2c)): the Editor painted water (3 cells), placed a goblin (#2), renamed the level "Exit Review" in the settings panel, and saved ([1-edited.png](../evidence/X-M2c/1-edited.png), [log](../evidence/X-M2c/1-editor-session.log)). A new run loaded "Exit Review" with its 2 characters, took the sword and struck the goblin twice: 55 then 50 of 60 HP, flashing red ([2-played.png](../evidence/X-M2c/2-played.png), [log](../evidence/X-M2c/2-game-session.log)).

## 4. The owner's art: **Met**
Hero, ground and monsters come from the owner's sheets, cut into atlases by `odysseus_atlas` (`US-120 ...`; [contact sheet](../evidence/US-120/contact-sheet.png)).

## 5. The demos still work: **Met**
`US-024 Walk to the rock`, `US-029 Throw in the game` and the spear tests pass on `assets/levels/demo.json` (D-20). ctest: 25 of 25 in Debug and Release, zero warnings.

## Found and fixed during the review
Scripted input: two holds of the same intent pressed it on every tick (an inactive hold released the active one each frame). Fixed in `application.cpp`; regression test `X-M2c Two scripted presses`.

## For the owner
Try it with [the Editor guide](../guides/editor.md): `odysseus.exe --editor`. Your notes become new stories through Anima.
