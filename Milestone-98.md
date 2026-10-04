# Project Odyssey: assembly progress (98)

## AP-99 · 2026-10-04 · US-265 Talk with placed NPCs

| | |
|---|---|
| Assembly plan / requirements | **v2.11** / **v2.10** (mirror) |
| Repository | `story/US-265` merged into `qa` |
| Milestone | M9a NPC foundation, in progress |

### State
- Debug build with zero warnings; tests written, not run (one full verify at X-M9a). `tests/game/npc_talk_test.cpp`.
- New `Npc` subject kind, `speaks` tag, `talk.json` targets `speaks`, placed people have a right-click menu and a conversation without a hero run, dialogue conditions and effects for opinion and memory. Manual checks: `docs/plans/US-265.md`.

### Decisions
- Technical: a placed NPC's class dialogue names a script by file name (`npc-trader.dlg` is the script `npc-trader`); scripts for placed people should use an `@who` nobody has so the clan's script choice never picks them.
- Technical: outside a run (a level played with no hero life) the right-click menu now exists for placed people only; every other thing still needs a run, as before.
- Technical: placed NPCs carry no `person` tag, so the actions written for the clan (give berries, ask to teach) are not offered to them.

### Next
- S-US-266 (Confront: its own key C and menu entry, five confront actions with data amounts, spreading to NPCs in hearing range), then US-267..US-270, X-M9a.
