# Changelog

Record every pull request's full change set here before opening or updating it.
Entries describe the final changes and their verification; update an entry when
its PR changes rather than leaving an outdated description.

## Unreleased — US-003 / S-US-003 — 2026-09-29

**State:** Prepared on local `story/US-003`; no remote branch or PR created,
no merge. Required Windows verification is pending GitHub write access.

### Build and source

- `CMakeLists.txt`: replace shared source-root includes with five layer targets;
  link only downward; identify consumers privately; route game/headless through
  Game/Simulation; register separate, serialized architecture CTest scenarios.
- `cmake/LayerRules.cmake`: expose each layer's own headers through a narrow
  forwarding include tree and run architecture validation on every build.
- `cmake/ValidateLayerIncludes.cmake`: check header boundary coverage and normalized
  include directions; restrict SDL3 to Platform and reject uncheckable macro includes.
- `src/core/boundary.h`, `src/core/version.h`: protect Core headers with a single
  source-layer identity check while keeping the existing version API.
- `src/luna/platform/{boundary.h,layer.h,layer.cpp}` and
  `src/luna/engine/{boundary.h,layer.h,layer.cpp}`: add empty Luna scaffolds with
  guards rejecting Simulation/Game dependencies and invalid consumers.
- `src/sim/{boundary.h,layer.h,layer.cpp}` and
  `src/game/{boundary.h,layer.h,layer.cpp}`: add empty, guarded Simulation/Game
  scaffolds; relative and absolute paths cannot bypass the include boundaries.

### Tests

- `tests/architecture/CMakeLists.txt`: 14 real compiler probes for five allowed
  edges and forbidden Simulation/Luna includes, including relative/absolute paths.
- `tests/architecture/layer_rules_test.cpp`: the three named acceptance scenarios
  plus guard completeness; quote diagnostic arguments safely in test commands.
- `tests/architecture/run_probe.cmake`: require the expected compiler/include
  diagnostic for rejected probes rather than accepting arbitrary build failures.
- `tests/architecture/run_validator.cmake`: clean controls and six violations
  injected after configure, proving validation runs on subsequent builds.
- `docs/evidence/US-003/`: preserve the tests-first baseline and supplementary
  Debug/Release test output.

### Documentation and tracking

- `docs/reports/local-checkpoint-2026-09-29.md`: save the local code location,
  resume point, current playability and outstanding owner requests.
- `docs/plans/US-003.md`: implementation plan, tests-first evidence, local results,
  pending acceptance/Windows checks and a teach-back draft awaiting acceptance.
- `docs/adr/ADR-016-layer-boundary-enforcement.md`, `docs/adr/README.md`: record
  the enforcement pattern and index it; no new project library was added.
- `README.md`: explain the five layer targets and architecture include checks.
- `docs/status.md`: keep US-003 Blocked by required Windows CI/integration access.
- `Milestone.md`: prepend AP-002 with unfinished US-003 and the exact resume point.
- `docs/reports/US-003-2026-09-29.md`: assembly report and acceptance limitations.
- `AGENTS.md`, `CHANGELOG.md`: persist the owner's requirement to track every PR's
  complete change set in this changelog.

### Verification

Supplementary GCC Debug and Release builds pass with `-Wall -Wextra -Werror`:
5 doctest cases, 17 assertions, 14 compiler probes and six validator rejection
checks in each configuration. Required MSVC Windows Debug/Release, AddressSanitizer,
Windows CTest and green CI on `main` are unverified. Completion is not accepted.
Determinism testing starts at US-010. No owner design decision is requested.
