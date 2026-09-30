# Handover: M2d assembly paused (2026-09-30)

Break requested by the owner. Everything is committed and pushed; resume with the Limit.md prompt.

## State
- Codex v1.8 / requirements v1.8 (M2d Content and combat, D-21, D-22: **the owner takes every design decision; ask in chat**).
- `main` = `m2c-done`. `qa` has K-M2d, US-130, US-131, US-132 (CI for US-132 merge pending; US-130/131 green).
- **US-133 Weapon classes and the starter set**: code and tests done and verified (`tools/verify.ps1 -Story US-133`: zero warnings, 25/25), committed as work in progress on `story/US-133` (pushed).

## To finish US-133 (next session, first)
1. `git checkout story/US-133`; write `docs/plans/US-133.md` (files: `src/game/weapons.{h,cpp}`, game loadout/attacks/projectiles/held icon/HUD, `apps/atlas --starters`; tests `US-133 Classes`, `Starters fight`, `Starter set`, `In hand`; evidence `docs/evidence/US-133/starters.png|md`, `staff.png`).
2. Learning-journal entry (virtual functions: `WeaponBehaviour`, `MeleeBehaviour`, `RangedBehaviour`), CHANGELOG, status Done, Milestone-45.md (AP-046), Limit.md.
3. Merge into `qa`, push, wait for CI. Known gap to record: icons are mirrored for west, not rotated (renderer has no rotation).

## Then
S-US-134 .. S-US-138, X-M2d (stop before M3). Owner review items: starter set (`docs/evidence/US-133/starters.md`), edible/blocking plants in `plants.json`, contact sheets in `docs/evidence/US-130/`.

## Notes
- Owner's uncommitted `assets/levels/valley.json` edits: never commit or overwrite.
- Sheet measuring scripts: `tools/art/` (PowerShell + C#).
