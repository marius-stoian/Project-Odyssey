# Project Odyssey: assembly progress (82)

## AP-083 · 2026-10-02 · K-M8c and US-240 The lighting pipeline

| | |
|---|---|
| Assembly plan / requirements | **v2.6** / **v2.8** |
| Repository | `story/US-240` merged into `qa` |
| Milestone | M8c Lighting and shadows, in progress (K-M8c and US-240 Done) |

### State
- K-M8c: the owner's answers are D-49 (night a light dimming only, warm orange dawn and dusk of about an hour, steady fire glow, shadows up to 2.5 x height); design in `docs/plans/M8c-lighting-design.md`.
- US-240: `tools/verify.ps1 -Story US-240` in Debug: zero warnings, 27 of 27; CI (Release) on the push to `qa`. Lit shader, `lights.json`, 64 lights cost 0.28 ms on the card.

### Decisions
- D-49 (owner). Technical: per-sprite lit shader as the Codex says (the design document's light buffer idea is replaced by it), flat normal for sprites without a map, additive draws never lit.

### Next
- S-US-241 (generated normal maps), US-242 (day, night and seasons).