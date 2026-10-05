# ADR-016: Enforce layer boundaries with targets, header guards and build validation

Status: Accepted. Implemented by ChatGPT for US-003 (2026-09-29); Windows/MSVC evidence added 2026-09-30 (docs/evidence/US-003/windows-*.txt).

## Context

ADR-004 requires five layers with downward dependencies. Target include directories alone cannot stop a source file including an upper layer through `../` or an absolute path. Simulation must remain headless, and Luna must stay reusable by other games.

## Decision

One static CMake target per layer. Public links expose only the allowed lower layers: Platform -> Core, Engine -> Platform, Simulation -> Core, Game -> Engine and Simulation. Game and headless executables consume Game and Simulation respectively. Each target exports a narrow forwarding include tree, preserving spellings such as `core/version.h` without exporting the whole source root.

A PRIVATE `ODYSSEUS_LAYER_*` definition identifies the source being compiled and never propagates to consumers through target links. Public headers include their own `boundary.h` first. The boundary rejects unknown or multiple identities and forbidden consumers, including relative and absolute include bypasses.

An always-run CMake build target validates production sources and headers. It checks that every header includes its boundary first, normalizes include paths before testing the layer graph, restricts SDL3 includes to Platform, and rejects macro-expanded includes that cannot be classified. Tests keep deliberately invalid fixtures outside production source discovery. CMake and the compiler suffice; no new dependency.

## Consequences

- New headers must begin with `#pragma once` followed by `#include "boundary.h"`. Headers and sources belong to one of the five directories. Shared game-independent utilities belong in Core. Boundary headers are the only guard-header exception.
- The owner gets clear include diagnostics instead of finding accidental coupling later. This prevents architectural mistakes; it is not a security boundary against someone deliberately editing macros or the validator.
- MSVC, Windows x64, existing ASan settings and warnings as errors are unchanged.

## Update 2026-09-30 (US-025): six layers
ARC-10 adds Luna Physics (`src/luna/physics/`, identity `ODYSSEUS_LAYER_PHYSICS`). Every `boundary.h` counts the sixth identity; Physics headers refuse Core and Platform code, and Platform, Engine, Simulation and Game headers refuse Physics code. The include validator allows Physics -> Core and Engine/Simulation/Game -> Physics. Compiler probes `us025_*` and four more validator fixtures prove it (`US-025 Physics layer rules`, `US-003 Guard completeness`).
