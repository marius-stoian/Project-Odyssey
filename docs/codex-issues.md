# Codex issues

Problems with the Codex, for Anima (owner pastes A-002 in a Dominus session).

| ID | Prompt | Problem | Suggested fix | Status |
|---|---|---|---|---|
| CI-001 | P-000 | Step 8 says "stop" when the toolchain is missing, but step 9 (git init, first commit) only needs Git. Stopping before step 9 leaves the bootstrap uncommitted. Mraw executed step 9 because Git was installed. | Move "git init + first commit" before the toolchain check, or say explicitly that step 9 runs whenever Git is present. | Open |
| CI-002 | K-M0, S-US-001 | D-12 bundles two things: the local toolchain (VS, CMake, vcpkg; needed by US-001) and a GitHub account + private repo (needed only by US-002 and pushes). The owner cannot get a local .exe until GitHub is also set up. | Split D-12 into D-12a (toolchain; blocks US-001, US-003, US-004) and D-12b (GitHub account + private repo; blocks US-002 and every push). | Open |
| CI-003 | S-US-001..S-US-004, Charter DoD | Every M0 story's verification requires "the determinism hash test" and the DoD requires "CI is green on main", but no simulation state exists until US-010 and no CI exists until US-002. Mraw marks these lines "not applicable yet" rather than inventing a placeholder test. | Add to the Charter DoD: "Determinism test: from US-010 on. CI green: from US-002 on." Remove the determinism line from M0 story verification. | Open |
