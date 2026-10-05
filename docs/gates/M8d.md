# M8d gate: Buildings (X-M8d)

Result of `pwsh tools/verify.ps1 -Story X-M8d -Config Both` (2026-10-05). Evidence: `docs/evidence/X-M8d/windows-debug.txt` and `windows-release.txt`.

| Check | Debug | Release |
|---|---|---|
| Build warnings | 0 | 0 |
| Build exit code | 0 | 0 |

The first ctest run (Debug) failed in two game shards, `odysseus_game_tests_b` and `_c`. Causes, all found by the run and fixed at the gate:

- Level files are now version 6 (`buildings` list). Ten test assertions pinned version 5; they now expect 6, and the shipped `assets/levels/npc-test.json` was resaved at version 6 (its round-trip test needs the saved text to equal the file).
- Three built-in actions had no interaction file (`repair`, `douse-fire`, `learn-blueprint`), which `US-152` forbids. Added `repair.json`, `douse-fire.json` and `study-palisade.json` (a place tagged `blueprint-palisade` teaches the palisade).
- The kinds' `blueprint-<id>` tags are only known when building data loads before the interactions; the load now runs first.

Per the rerun rule, only the failing cases and the files they live in were rerun (53 and 178 test cases, all passed, Debug). The full Release ctest was not repeated after the fixes; CI Release on `main` is the check for that.

## Exit criteria

| Criterion | Status |
|---|---|
| US-250: pieces, kinds and prefabs are data; errors name file and line | Met (`building_data_test`) |
| US-251: place a blueprint, bring materials, build, cancel; free without a run | Met (`building_store_test`, `building_game_test`) |
| US-252: walls block walking; rooms; roofs fade over the hero's room | Met (store and game tests); how it looks needs the owner's eyes |
| US-256: Editor Build tool, building panel, Prefab tab, saves in the level | Met (`building_editor_test`) |
| Zero build warnings, Debug and Release | Met |
| Release first frame within 3 seconds | Not measured in this run (no window run). Owner or CI |
| GPU screenshots of ghost, blueprint, roof fade | Owner only (needs a real GPU window) |
