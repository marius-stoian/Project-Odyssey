# Project Odyssey: assembly progress (117)

## AP-118 · 2026-10-05 · M8e Building life (K-M8e, US-253, US-254, US-255, US-257)

| | |
|---|---|
| Assembly plan / requirements | **v2.12** (repository copy, CI-013) / **v2.10** (mirror) |
| Repository | `qa` |
| Milestone | M8e Building life |

### State
- K-M8e answered by the owner; design in `docs/plans/M8e-building-life-design.md`.
- US-253 (clan and rival builders), US-254 (interior maps), US-255 (raids, fire, repair), US-257 (owners, warmth, storage) are written. Their cases ran alone and pass; the full verify is X-M8e.
- Owner-only: GPU screenshots of fire and interior.

### Decisions
- Technical, not design: rival buildings are lists of kinds and progress (rival camps are far from the played level), so the hero cannot hit them; raids on the hero's buildings are real. A design question if the owner wants them drawn on the map.
- `Subject::Kind::Place`: a named place with tags can be right-clicked (needed for the way out of a building).

### Next
- X-M8e: full verify (Debug and Release), `docs/gates/M8e.md`, merge into main, tag m8e-done.
