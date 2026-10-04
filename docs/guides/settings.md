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
| `cameraZoom` | `1`, `2` | Camera zoom: `2` shows 15 x 8.4 tiles around the hero (the world as it always looked), `1` shows 30 x 17. Settings screen, the keys + and -, or the mouse wheel. |
| `uiScale` | `1`, `2` | Interface scale: `2` draws panels, the font and the hotbar twice as large. Settings screen only. |
| `lighting` | `Low`, `Medium`, `High` | Lighting quality (US-247). `Low`: sprites are lit flat (no normal maps) and fires cast no shadows; sun and moon shadows, fire glow and the day's colour stay. `Medium` and `High`: normal maps and fire shadows on. `High` is the setting the 60 FPS target of the mid-range PC (D-06) is measured on. |
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
