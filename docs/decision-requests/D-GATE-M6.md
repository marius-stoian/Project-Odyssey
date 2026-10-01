# D-GATE-M6: Kill Gate 2, playtest and go/no-go

**Needs the owner (people).** The game is built through M6; agents cannot measure the criteria that need playtesters.

## What to do
1. Build the playtest zip: `cmake --build --preset windows-x64-release`, then `cpack --config build/windows-x64/CPackConfig.cmake -C Release` (a zip with odysseus.exe, SDL3 and assets).
2. Give it to 8 outside playtesters (Windows 10/11 x64). On the first New Game the game asks whether to keep local session statistics; ask them to say yes. Files land in `%APPDATA%\Project Odyssey\Odysseus\saves\sessions\`.
3. Crashes: `%APPDATA%\Project Odyssey\Odysseus\crash\` holds `crash-<n>.log` and `last-save\`.

## Criteria to report back
| Criterion | Measure | Result (owner fills in) |
|---|---|---|
| Voluntary play time: 5 of 8 play 30+ minutes without being asked | observation + `playSeconds` in the session files | |
| Emergent story: 3 of 8 retell a story from the simulation | post-play interview | |
| Stability: no crash in a 2-hour session or a 100-year soak test | crash folder + soak test (owed: `odysseus_headless`) | |

## Recommendation (Dominus)
Run the owed full test pass and CI first (see `docs/gates/M2d.md`, `M4.md`, `M5.md`, `M6.md`): the playtest build should come from a green `qa`. Then go / pivot / stop on the owner's reading of the table above.
