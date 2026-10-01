# Game settings

The game keeps local preferences in `settings.json` inside its save directory. The Settings screen applies window mode, windowed size, and scaling as soon as they are selected and saves them for the next launch. The `--save-dir` command-line option selects a different save directory.

On first launch, the game creates the file with a 1280x720 window, Whole scaling, camera zoom 2x, UI scale 1x, lighting Medium, volume 80, and statistics consent undecided.

## Fields

| Field | Values | Meaning |
|---|---|---|
| `version` | `2` | Settings format version. A file from a later version is kept intact and reported. |
| `resolution.width`, `resolution.height` | `1280x720`, `1600x900`, `1920x1080`, `2560x1440` | Windowed size. The last selected windowed size stays saved while in full screen. |
| `resolution.mode` | `Windowed`, `Borderless`, `Exclusive` | Window, borderless full screen, or exclusive full screen. |
| `resolution.scaling` | `Whole`, `Fill` | Whole enlarges the 960x540 picture by the largest whole multiple that fits, with black bars around it. Fill fits the picture to the available area. |
| `cameraZoom` | `1`, `2` | Saved camera zoom setting. The camera control is added in US-232. |
| `uiScale` | `1`, `2` | Saved interface scale setting. The interface control is added in US-232. |
| `lighting` | `Low`, `Medium`, `High` | Saved lighting preference. |
| `volume` | `0` through `100` | Saved volume preference. |
| `statistics` | `0`, `1`, `2` | Local statistics choice: undecided, agreed, or declined. |

```json
{
  "version": 2,
  "resolution": {
    "width": 1280,
    "height": 720,
    "mode": "Windowed",
    "scaling": "Whole"
  },
  "cameraZoom": 2,
  "uiScale": 1,
  "lighting": "Medium",
  "volume": 80,
  "statistics": 0
}
```

An existing file without `version` is migrated from the earlier format. Its `fullscreen` value becomes Borderless or Windowed, a supported size and valid volume and statistics are retained, and the new fields get their defaults. A damaged file is replaced with defaults and the game reports the problem.
