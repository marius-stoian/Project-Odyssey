# Project Odyssey: assembly progress (97)

## AP-98 · 2026-10-04 · US-264 Attitudes and opinions

| | |
|---|---|
| Assembly plan / requirements | **v2.11** / **v2.10** (mirror) |
| Repository | `story/US-264` merged into `qa` |
| Milestone | M9a NPC foundation, in progress |

### State
- Debug build with zero warnings; tests written, not run (one full verify at X-M9a). `tests/sim/opinion_test.cpp`, `tests/game/npc_attitude_test.cpp`.
- Nine attitude words from an opinion number (`opinions.json`), sparse pair opinions in the population, events and dialogue amounts as data, same-family default, kind-file NPCs fight when hostile, menu title shows the word. Manual checks: `docs/plans/US-264.md`.

### Decisions
- Technical, for the owner to review: the thresholds of the words (hostile below -59, wary to -30, suspicious to -10, neutral to 9, friendly to 39, enchanted to 69, lovingly from 70) and the starting opinions per attitude are my numbers, all in `opinions.json`. Scared and enviously are moods that win over the number until cleared (fear and envy are not a point on the scale of liking).
- Technical: the family opinion is the default for a pair, not a stored entry, so a family of 50 does not create 2,450 entries (acceptance 'Sparse'). A placed NPC gets a `family` id field in the level now (the Editor form comes with US-268).
- Technical: a hostile NPC that is also a person (a wanderer set hostile) fights as an enemy and stays in the population; persons do not die in M9a.

### Next
- S-US-265 (talk with placed NPCs: dialogues by partner type, no script no Talk), then US-266..US-270, X-M9a.
