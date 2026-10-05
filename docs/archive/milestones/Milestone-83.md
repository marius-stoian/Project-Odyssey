# Project Odyssey: assembly progress (83)

## AP-084 · 2026-10-02 · US-241 Generated normal maps

| | |
|---|---|
| Assembly plan / requirements | **v2.6** / **v2.8** |
| Repository | `story/US-241` merged into `qa` |
| Milestone | M8c Lighting and shadows, in progress (K-M8c, US-240, US-241 Done) |

### State
- `tools/verify.ps1 -Story US-241` in Debug: zero warnings, 27 of 27; CI (Release) on the push to `qa`.
- `odysseus_atlas --normals` makes nine normal atlases (committed, 3.5 MB together); hand-made `<frame>_n.png` wins; sprites without a map are lit flat. The hero is shaded by his generated normals on the GPU (`docs/evidence/US-241/hero-lit.png`).

### Decisions
- None requested. Technical: height from edge distance and brightness, Sobel; ground tiles use strength 0.6, bodies 2.0; clan members (layered) stay flat until a later story.

### Next
- S-US-242 (day, night and seasons).