# M8e gate: Building life (X-M8e)

Result of `pwsh tools/verify.ps1 -Story X-M8e -Config Both` (2026-10-05). Evidence: `docs/evidence/X-M8e/windows-debug.txt` and `windows-release.txt`.

| Check | Debug | Release |
|---|---|---|
| Build warnings | 0 | 0 |
| Build exit code | 0 | 0 |
| ctest | 27 of 27 passed | 27 of 27 passed |

No failures in this run, so no fixes were needed.

## Exit criteria

| Criterion | Status |
|---|---|
| US-253: clan members bring materials and build blueprints; rival clans build by season | Met (`building_life_test`, `building_life_game_test`). Rival buildings are lists, not cells of the level (technical choice, `docs/plans/US-253.md`) |
| US-254: roofs fade; a building with an interior level opens it; leaving restores the outside world | Met (`US-254` cases) |
| US-255: raids by rivals at war, fire shots, repair, putting out fires, fire light | Met (`US-255` cases). The hero cannot hit rival buildings because they are not on the level |
| US-257: owners at dawn, warmth for the housed, storage that halves spoilage, `store-food` | Met (`US-257` cases, sim and game) |
| Zero build warnings, Debug and Release | Met |
| GPU screenshots of fire, light and interior | Owner only (needs a real GPU window) |
