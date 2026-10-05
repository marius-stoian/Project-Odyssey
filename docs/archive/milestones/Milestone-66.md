# Project Odyssey: assembly progress (66)

## AP-067 · 2026-10-01 · K-M8 done: M8 Speak to NPCs kicked off

| | |
|---|---|
| Assembly plan / requirements | **v2.0** / **v2.0** |
| Repository | `chore/k-m8` (merges into `qa` after the M7 merge to `main`) |
| Milestone | **M8 Speak to NPCs**: kickoff done, 0 of 6 stories built |
| Next | **S-US-160** (the `.dlg` format) |

### Owner decisions (D-38, asked in chat at the kickoff)
- One mood word in the panel; at most 5 choices; plain and short small talk; the hero can insult also in small talk.

### What was built
- `docs/plans/M8-dialogue-design.md`: layers (flat `dialogue_*` files), the `.dlg` grammar, the runtime state machine, selection rules, the small-talk generator and its templates, memory, flags and chronicle links, bubbles, the test plan.
- `docs/decisions.md` D-38; `docs/status.md` K-M8 Done.

### Decided by Dominus (delegated technical choices)
- Free-text conversation memories keep the simulation's kind and feeling and add a text side table used only for talk; flags live in `things.json`.
