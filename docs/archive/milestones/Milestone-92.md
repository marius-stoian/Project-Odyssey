# Project Odyssey: assembly progress (92)

## AP-093 · 2026-10-04 · X-M8c Exit review

| | |
|---|---|
| Repository | `qa` merged into `main`, tag `m8c-done` (bfa158d), pushed |
| Milestone | M8c Lighting and shadows: DONE |

### State
- One full verify (owner's rule): Debug and Release, zero warnings, 27 of 27 groups each. First run found three faults (level version 3 test, Light tool click spot, fire shadows by day under dim weather); fixed and rerun green. Evidence: docs/gates/M8c.md, docs/evidence/X-M8c/.
- Not measured, need the owner's PC and GPU: High-lighting 60 FPS table and the time-lapse screenshot sheet (docs/plans/stories-M8c.md#us-246, US-247.md).
- CI on main runs from the merge; read it before relying on the tag.

### Decisions
- None requested. Medium and High lighting are identical today.

### Next
- K-M9a stops on CI-012 (requirements lack E17-E19 and US-260..294; check the Drive mirror first).
