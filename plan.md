# `stmt_emitter.cpp` Split Refactor Runbook

Status: Active
Source Idea: ideas/open/02_stmt_emitter_cpp_split_refactor.md
Activated from: ideas/open/02_stmt_emitter_cpp_split_refactor.md

## Purpose

Turn [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) into a multi-file implementation while keeping [`src/codegen/lir/stmt_emitter.hpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.hpp) as the single declaration-oriented surface for `StmtEmitter`.

## Goal

Make statement-emission ownership inspectable by focused `stmt_emitter_*.cpp` files without changing emitted behavior or test results.

## Core Rule

This is a structural refactor only. Do not intentionally change LIR lowering semantics, ABI behavior, emitted instruction ordering, or feature scope beyond what is required to complete the split.

## Read First

- [`ideas/open/02_stmt_emitter_cpp_split_refactor.md`](/workspaces/c4c/ideas/open/02_stmt_emitter_cpp_split_refactor.md)
- [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp)
- [`src/codegen/lir/stmt_emitter.hpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.hpp)
- root [`CMakeLists.txt`](/workspaces/c4c/CMakeLists.txt) entries that currently compile the LIR emitter sources

## Scope

- Keep [`src/codegen/lir/stmt_emitter.hpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.hpp) as the canonical declaration surface.
- Stage and then integrate focused implementation files such as:
  - `stmt_emitter_core.cpp`
  - `stmt_emitter_types.cpp`
  - `stmt_emitter_lvalue.cpp`
  - `stmt_emitter_call.cpp`
  - `stmt_emitter_expr.cpp`
  - `stmt_emitter_stmt.cpp`
- Use build and linker diagnostics to converge ownership once draft boundaries are visible.
- Reduce [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) only after split ownership is stable.

## Non-Goals

- semantic bug fixing unrelated to preserving current behavior
- redesigning the `StmtEmitter` API beyond the split
- scattering declaration ownership across new public headers
- mixing later backend refactors into this runbook

## Working Model

- [`src/codegen/lir/stmt_emitter.hpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.hpp) remains the one obvious summary of the emitter surface.
- Early draft extraction is allowed before build wiring, but integrated ownership must end with exactly one live definition per helper/body.
- Compiler and linker diagnostics are the authoritative signal for missing or duplicate ownership once staged files are wired into the build.

## Execution Rules

- Keep slices narrow and behavior-preserving.
- During the first draft pass, do not edit [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) just to mirror extracted files.
- Wire staged files into the build once their boundaries are coherent enough to generate useful diagnostics.
- Use duplicate-definition and undefined-reference errors to drive batch cleanup by semantic cluster.
- If execution reveals a separate initiative, record it under `ideas/open/` instead of expanding this runbook.

## Ordered Steps

### Step 1: Survey The Monolith And Header

Goal: establish the current ownership surface before any extraction.

Actions:
- inspect [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) and [`src/codegen/lir/stmt_emitter.hpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.hpp)
- inventory major helper families and method clusters by semantic responsibility
- identify candidate file boundaries that match the idea document

Completion Check:
- the first-pass ownership map names the intended split files and the clusters likely to belong in each

### Step 2: Create Draft Staging Files

Goal: make the target ownership boundaries concrete before integration cleanup starts.

Primary Targets:
- `stmt_emitter_stmt.cpp`
- `stmt_emitter_call.cpp`
- `stmt_emitter_expr.cpp`

Actions:
- create draft split files with coherent includes and copied/extracted bodies
- keep [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) intact during the initial draft pass
- minimize overlap where obvious, but prefer visible staging over perfect first-pass partitioning

Completion Check:
- the draft files exist and each represents a plausible semantic ownership cluster

### Step 3: Expand The Draft Set And Normalize Boundaries

Goal: cover the remaining semantic families before build wiring.

Primary Targets:
- `stmt_emitter_core.cpp`
- `stmt_emitter_types.cpp`
- `stmt_emitter_lvalue.cpp`

Actions:
- extract the remaining major helper families into draft files
- identify shared helpers that should stay local versus those that need internal declaration support
- resolve obvious duplicate ownership between draft files before wiring them into the build

Completion Check:
- the full staged source layout exists with one intended owner per major helper family

### Step 4: Wire The Split Files Into The Build

Goal: switch from draft inspection to diagnostic-driven ownership cleanup.

Actions:
- update build wiring to compile the new `stmt_emitter_*.cpp` files
- rebuild and use compiler/linker output to find missing declarations and duplicate definitions
- fix declaration-surface and ownership issues in batches instead of one function at a time

Completion Check:
- the project builds with the split emitter files compiled together
- ownership conflicts are reduced to a manageable cleanup list or eliminated

### Step 5: Reduce The Monolith Cleanly

Goal: leave [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) as a thin coordinator or remove it if no longer needed.

Actions:
- delete or disable stale bodies that are now owned by split files
- remove transitional scaffolding and dead duplicate blocks
- confirm [`src/codegen/lir/stmt_emitter.hpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.hpp) is still the single clear declaration surface

Completion Check:
- [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) is removed or obviously thin
- live statement, call, lvalue, expression, type, and core ownership resides in focused split files

### Step 6: Validate No Regressions

Goal: prove the refactor preserved behavior.

Actions:
- run a clean build
- run targeted checks while ownership is moving
- run the full `ctest --test-dir build -j 8` regression pass before handoff

Completion Check:
- `cmake --build build -j8` succeeds
- `ctest --test-dir build -j 8` shows no regressions

## Final Acceptance Checks

- [`src/codegen/lir/stmt_emitter.hpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.hpp) remains the canonical declaration surface for `StmtEmitter`
- [`src/codegen/lir/stmt_emitter.cpp`](/workspaces/c4c/src/codegen/lir/stmt_emitter.cpp) is removed or reduced to a thin forwarding/coordinator file
- statement emission logic lives in a dedicated `stmt_emitter_stmt.cpp`
- call / builtin / vararg lowering lives in a dedicated `stmt_emitter_call.cpp`
- lvalue and expression lowering are moved into focused implementation files
- the build succeeds
- full-suite tests show no regressions
