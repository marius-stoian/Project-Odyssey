# Game rules: `assets/data/rules/<name>.json` (US-195, M11)

One file is one set of rules: how a new game is set up, when it is won or lost, and which whole systems run. The **New Game** screen offers every file of the folder (a **Rules** row appears when there is more than one), a level may name one, and the Editor's **Data** tab edits them as forms (open a file under `rules/`: a heading and one line say what the saved file switches on and off).

Every part is optional except in `standard.json`: a part a file leaves out is the one of `standard.json`, and a switch it leaves out is **on**.

## `newGame`: the choices of the New Game screen
```json
"newGame": {
  "presets":  [ { "name": "Full", "startAge": 12, "mantleAge": 26 } ],
  "comforts": [ { "name": "Gentle", "needsPercent": 70, "foodPercent": 140 } ]
}
```
`presets` are the Growing Periods (the age the hero starts at, the age the leader's mantle is taken); `comforts` say how fast needs fall and how full the store starts, in percent. These used to live in `hero/hero.json`; that file is still read for them (with a warning in the log) when `rules/standard.json` leaves them out, for this version.

## `victory`: when the game ends
```json
"victory": { "winPercent": 60, "combinedWinPercent": 50, "loseBelowPeople": 3, "rivalFollowerPercent": 60 }
```
`winPercent`: the share of Trade or of Religion in the region that wins it. `combinedWinPercent`: the average of the two that wins it. `loseBelowPeople`: a clan with fewer people than this is defeated. `rivalFollowerPercent`: the share of a rival clan's people that can follow the hero's faith. Change `winPercent` to 40 and a game is won at 40%.

## `systems`: switches of whole systems
```json
"systems": { "weather": false, "combat": true, "rivals": true, "tutorial": true, "markers": true, "chronicle": true, "politics": true }
```
| Switch | When it is off |
|---|---|
| `weather` | the weather cycle does not run: the sky stays clear |
| `combat` | placed characters that would fight stand as bystanders, and nobody can be provoked into a fight |
| `rivals` | no rival clans in a region game |
| `tutorial` | the first-day quest of a new game does not start |
| `markers` | the arrow at the tracked quest step is not drawn |
| `chronicle` | conversations add no lines to the clan's chronicle |
| `politics` | read by the politics of M13 |

Systems that start things (rivals, the placed characters) are read when a game starts or a level is played; a saved change to the file of the rules in play reaches the weather, the markers and the chronicle at once (the **mechanics** set, see [editor.md](editor.md)) and the rest at the next start.

## Which rules apply
1. A level that names `"rules": "<name>"` (level version 7) is played under them.
2. Else the pick of the New Game screen.
3. Else `standard`.

A run keeps the rules it began with: they are in its save (`rules` in the hero's save, version 2; a save from before it plays under `standard`).

Two files are shipped: `standard.json` and `peaceful.json` (no combat, no rival clans, victory at 40%).

Try it: **Data**, `rules/standard.json`, set `systems.weather` to no, Ctrl+S: the summary line says `weather off` and the sky of the running game clears. Put it back and Ctrl+S.
