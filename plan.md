# `ast_to_hir.cpp` Split Refactor Runbook

Status: Active
Source Idea: ideas/open/01_ast_to_hir_cpp_split_refactor.md
Activated from: ideas/open/01_ast_to_hir_cpp_split_refactor.md

## Purpose

Turn the monolithic [`src/frontend/hir/ast_to_hir.cpp`](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp) implementation into a parser-style layout with one authoritative declaration header and multiple focused implementation files.

## Goal

Make the HIR lowering surface inspectable from one header while preserving existing behavior and test results.

## Core Rule

This is a structural refactor only. Do not intentionally change lowering behavior, ownership semantics, or feature scope beyond what is required to complete the file split.

## Read First

- [`ideas/open/01_ast_to_hir_cpp_split_refactor.md`](/workspaces/c4c/ideas/open/01_ast_to_hir_cpp_split_refactor.md)
- [`src/frontend/hir/ast_to_hir.cpp`](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)
- [`src/frontend/hir/ast_to_hir.hpp`](/workspaces/c4c/src/frontend/hir/ast_to_hir.hpp)
- frontend HIR CMake targets that currently compile `ast_to_hir.cpp`

## Scope

- Expand [`src/frontend/hir/ast_to_hir.hpp`](/workspaces/c4c/src/frontend/hir/ast_to_hir.hpp) into the canonical declaration surface.
- Move implementation bodies out of the monolithic translation unit into cohesive `hir_*.cpp` files.
- Update build wiring for the new source layout.
- Keep helper visibility as private as practical unless cross-file sharing requires a declaration surface.

## Non-Goals

- semantic bug fixing unrelated to preserving current behavior
- redesigning the `Lowerer` API beyond the split
- changing lowering order or ownership architecture
- mixing adjacent refactors from later ideas into this plan

## Working Model

- Public and agent-facing declarations belong in [`src/frontend/hir/ast_to_hir.hpp`](/workspaces/c4c/src/frontend/hir/ast_to_hir.hpp).
- Implementation should be grouped by semantic responsibility rather than by arbitrary line counts.
- Validation should prove no regressions after each meaningful slice and again at the end.

## Execution Rules

- Keep edits narrow and compile frequently.
- Prefer moving declarations first, then migrating one responsibility cluster at a time.
- If private cross-file declarations become noisy, introduce one internal helper header only if clearly justified.
- Update tests only when needed to preserve existing validation coverage for the structural refactor.
- If execution reveals a separate initiative, record it under `ideas/open/` instead of expanding this runbook.

## Ordered Steps

### Step 1: Establish Header Ownership

Goal: make [`src/frontend/hir/ast_to_hir.hpp`](/workspaces/c4c/src/frontend/hir/ast_to_hir.hpp) the authoritative declaration surface.

Actions:
- inventory top-level structs, enums, helper types, free functions, and the `Lowerer` definition currently living in [`src/frontend/hir/ast_to_hir.cpp`](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)
- move declarations into [`src/frontend/hir/ast_to_hir.hpp`](/workspaces/c4c/src/frontend/hir/ast_to_hir.hpp)
- prefer one bulk promotion of the remaining split-critical declaration surface over many tiny out-of-class helper moves when that better unlocks the file split
- keep only declarations in the header and preserve private/internal visibility where possible

Completion Check:
- `Lowerer` and the key helper type declarations are no longer defined only inside the `.cpp`
- the header exposes the full lowering surface needed for the split

### Step 2: Create a Safe Transitional Build

Goal: keep the tree building while the monolith is being peeled apart.

Actions:
- reduce [`src/frontend/hir/ast_to_hir.cpp`](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp) to a temporary implementation coordinator if needed
- introduce the new `hir_*.cpp` files with clear ownership boundaries
- ensure the build system includes the new files without duplicating definitions

Completion Check:
- the project builds with a mixed old/new layout
- symbol ownership is unambiguous and free of duplicate definition errors

### Step 3: Extract Lowering Clusters

Goal: migrate implementation bodies into focused translation units.

Primary Targets:
- `hir_build.cpp`
- `hir_templates.cpp`
- `hir_functions.cpp`
- `hir_expr.cpp`
- `hir_types.cpp`
- `hir_stmt.cpp`

Actions:
- move one semantic cluster at a time
- keep helper functions close to their owning cluster unless shared
- compile and run targeted validation after each extraction slice

Completion Check:
- the major lowering domains live in focused `hir_*.cpp` files
- the old monolithic file is removed or reduced to a thin non-owning coordinator

### Step 4: Finalize Build and Surface

Goal: finish the structural split cleanly.

Actions:
- remove stale declarations and now-unused transitional scaffolding
- confirm CMake reflects the final source layout
- verify the header remains the single obvious entrypoint for HIR lowering structure

Completion Check:
- source layout matches the intended split
- no stale forwarding bodies or dead transitional glue remain without justification

### Step 5: Validate No Regressions

Goal: prove the refactor preserved behavior.

Actions:
- run a clean configure/build
- run targeted subsystem tests during development
- run the full `ctest --test-dir build -j 8` regression pass before handoff

Completion Check:
- `cmake --build build -j8` succeeds
- `ctest --test-dir build -j 8` shows no regressions

## Final Acceptance Checks

- [`src/frontend/hir/ast_to_hir.hpp`](/workspaces/c4c/src/frontend/hir/ast_to_hir.hpp) contains the full declaration surface for HIR lowering
- `Lowerer` is no longer defined in [`src/frontend/hir/ast_to_hir.cpp`](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp)
- [`src/frontend/hir/ast_to_hir.cpp`](/workspaces/c4c/src/frontend/hir/ast_to_hir.cpp) is removed or reduced to a thin forwarding/coordinator file
- HIR lowering implementation is split across multiple focused `hir_*.cpp` files
- the build succeeds
- full-suite tests show no regressions
