# Handover: M2d assembly (2026-09-30)

Progress saved at the owner's request. Resume by finishing the US-134 integration check before starting another story.

## State
- Repository: `C:\Users\Amek\.amek-ai\Odysseus\odysseus`.
- Current branch: `qa`, synchronized with `origin/qa` at `810a41ad81c1285e4e0be33d516c11eafa85eb1b` when this handover was written.
- Assembly plan and requirements: v1.8; M2d Content and combat. K-M2d and US-130..US-133 complete. US-134 is implemented, locally verified, merged and pushed; hosted CI is still running.
- US-134 story commit: `8c595ec`; merge: `fa30d35`; latest pushed handover-status commit: `810a41a`.
- The owner explicitly authorized `push qa`, and it succeeded. The earlier push-approval blocker is resolved for that push. The separate `story/US-134` branch was not pushed; its implementation is included in remote `qa`.
- CI for the pushed head: https://github.com/marius-stoian/Project-Odyssey/actions/runs/36766225054 — `in_progress` at the last check. Do not assume success.

## US-134: Pickups and the hotbar
- Level format version 2 adds weapon pickups; version 1 files still load.
- Editor Weapon palette offers 16 starters and the two built-in demo weapons. Place, move, delete, undo and redo work for pickups.
- The hero starts empty-handed. Walking within 16 pixels collects a pickup into the first free of nine slots, with a spark and log entry. A full hotbar leaves it lying and displays "Hotbar full".
- Keys 1-9 select slots; Shift cycles filled slots. Restarting the level restores pickups and clears the hotbar.
- `assets/levels/demo.json` places "Spear throw" and "Sword" by the hero to preserve earlier demos.
- Owner choices are recorded as D-23 in `docs/decisions.md`. D-22 remains in force: ask the owner about design decisions; never delegate them.

## Verification and evidence
- Final local verification: `tools/verify.ps1 -Story US-134` passed. Debug and Release both built with zero warnings and passed all 25 checks, including simulation determinism and earlier end-to-end tests.
- Fixed the earlier editor test's hard-coded character ID by reading the demo's next available ID and checking the saved goblin identity.
- Restored the approved pickup spark; combat and held-icon tests wait for pickup sparks to finish before counting their own drawings/effects, preserving their assertions.
- One Release paint test missed a scripted click in an earlier run; the complete final rerun passed. If it recurs in CI, investigate it rather than weakening the assertion.
- Evidence: `docs/evidence/US-134/windows-debug.txt`, `windows-release.txt`, `hotbar.png`, `editor-weapons.png`. Both screenshots were visually reviewed.
- Plan: `docs/plans/US-134.md`. Editor guide and learning journal are updated. Progress snapshot: `Milestone-46.md`, AP-047.

## To finish US-134 (next session, first)
1. Check the CI run above against head `810a41ad81c1285e4e0be33d516c11eafa85eb1b`. Fix any failures and repeat the relevant verification.
2. Once CI is green, mark S-US-134 Done in `docs/status.md` and update `CHANGELOG.md`, `Milestone-46.md`, `Limit.md` and the plan's integration note.
3. Those files currently contain pre-push wording such as "awaiting authorization" or "integration pending". This handover supersedes that wording: `qa` has already been pushed, but CI has not yet been confirmed.
4. Preserve this handover and completion documentation in the normal assembly commit workflow. This file was created after the latest push and is not included in `810a41a`.

## Then
- Next story: S-US-135 Elements, followed in order by S-US-136..S-US-138 and X-M2d. Read the current assembly prompt before implementing; do not build directly from requirements.
- M3 only when the owner says so.
- Use the owner's current Amek Protocol naming: Dominus, Avengers and Anima. Anima owns assembly-prompt amendments. Ask the owner if a design or scope decision is needed.

## Notes
- The owner's uncommitted `assets/levels/valley.json` edits remain untouched and uncommitted. Never overwrite, restore or include them in an assembly commit.
- Valley SHA-256 at resume: `BA6E9E0C1A1B3FAB018C80CD2B3297014FBE9AC0811730CBFAECB6B6CEDE298E`.
- The valley has no pickups until the owner places them with F2 → Weapon; F1 returns to play.
- The verification script requires access to the installed dependency cache under `C:\dev\vcpkg`; the restricted execution environment initially denied its lock file. Verification succeeded with approved access.
- Honor the owner's prohibition on vendor/model attribution in project content and commits; credit the team roles.
