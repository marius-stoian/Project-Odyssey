# Project Odyssey: assembly progress (81)

## AP-082 · 2026-10-02 · X-M8b Exit review: M8b is done

| | |
|---|---|
| Assembly plan / requirements | **v2.6** / **v2.8** |
| Repository | `qa` merged into `main` (`ae660b0`), tag `m8b-done` |
| Milestone | M8b Resolution and GPU renderer: DONE |

### State
- All five exit criteria met (`docs/gates/M8b.md`); Release verification on the owner's PC: 27 of 27 including the strict 3-second first-frame limit; CI on `main` (Debug and Release) green.
- CI note: `m4_test.cpp:94` (autosave under 200 ms) failed once on the GitHub runner (346 ms) and passed on re-run; a timing check on a loaded runner, not weakened.

### For the owner
- Default UI scale 1x makes the HUD half the old size; 2x restores it. One number in `settings.h` if 2x should be the default.
- Minimum-PC frame times are estimates until measured on an RX 6600 / RTX 3060 PC.
- The daily autosave stalls one frame (25 to 35 ms) with 500 people.

### Next
- K-M8c (lighting and shadows): the kickoff needs the owner's design answers (Charter human gate 3, D-35): night darkness, dawn and dusk colours, torch look, shadow length.
