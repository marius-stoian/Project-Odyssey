# Project Odyssey: assembly progress (61)

## AP-062 · 2026-10-01 · US-152 Done: the context menu from data

| | |
|---|---|
| Assembly plan / requirements | **v2.0** / **v2.0** |
| Repository | `qa` (story/US-152 merged) |
| Milestone | **M7 World interactions**: 4 of 7 stories built |
| Next | **S-US-153** (timed actions and world state) |

### What happened
- US-152: every action of the old right-click menu is now an interaction file (20 files); the menu is built from the registry (label, greyed-out reason and order from the files); the code each item ran moved unchanged into 14 built-in actions that files name with the new effect verb `do`; an unknown built-in is a load error; renaming "Tend the fire" in `tend-fire.json` and pressing F5 changes the menu.
- Verification: `pwsh tools/verify.ps1 -Story US-152`, 0 warnings, 27 of 27 tests in Debug and Release; 6 new test cases; no M5 or M6 test changed. CI on `qa`: US-151 (f4d6e2a) green, US-156 (b0feaca) running.

### Decided by Dominus (delegated technical choices)
- `gather.json` keeps today's behaviour for now (instant, reach 2 m, label "Gather", no winter rule); the brief's example (3 s, winter rule) stays as the US-150 test fixture. Behaviour "must not change" is the contract of this story, so this resolves the heads-up of Milestone-59 for now: winter gathering is unchanged. The winter rule is a design question for the owner when US-153 makes Gather a timed action.
- "Ask to teach you ..." is shown only for a master without an apprentice, by giving such a person the dynamic tag `teaches-<profession>`.

### Still open
- Owner design question (US-153): should Gather keep working in winter? (The brief's example says "Nothing grows in winter".)
