# Project Odyssey: assembly progress (72)

## AP-073 · 2026-10-01 · US-164 Conversations are remembered

| | |
|---|---|
| Assembly plan / requirements | **v2.2** / **v2.4** |
| Repository | `story/US-164` merged into `qa`; `main` at `m7-done` |
| Milestone | M8 Speak to NPCs, in progress (K-M8, US-160..US-164 Done) |

### State
- US-164 passed `tools/verify.ps1 -Story US-164`: zero warnings, every test passing in Debug and Release (11 new cases: 7 simulation, 4 game).
- CI on `qa`: the US-162 content and P-011 are green; the US-163 merge run was still going when this was written.
- Insulting someone (the **Be quiet** answer of small talk) lowers their opinion by 10 and leaves a Quarrel memory of -40; a few days later their friends have heard it at -20. The effects `remember`, `flag` and `chronicle` work in `.dlg` files, flags are saved with the game.

### Decisions
- None requested. Delegated technical choices (Dominus): a conversation memory is an ordinary memory plus a note (so gossip, forgetting and the chronicle needed no new code); `remember` defaults to a feeling of 10; "marked `chronicle` in the script" is a choice that writes the existing `chronicle "line"` effect (no new header, so no format change).

### Next
- Confirm green CI on `qa`, then S-US-165 (NPCs talking to each other in bubbles, `@pair` scripts), then X-M8 (the exit review; the owner reads the 50 small-talk lines and judges the two-bubble greeting rule).
