# External EASTL Suite Bootstrap Runbook

Status: Active
Source Idea: ideas/open/15_cpp_external_eastl_suite_bootstrap.md

## Purpose

Bootstrap a curated `tests/cpp/external/eastl` suite so EASTL-derived cases can
grow under c4c's native external-test model instead of depending on the full
upstream `EASTLTest` executable harness.

## Goal

Land the suite root, register the first standalone case, and prove it with
c4c's build and `ctest` flow while keeping `ref/EASTL` / `ref/EABase` as the
upstream include sources.

## Core Rule

Keep the boundary honest:

- curated extracted cases live in `tests/cpp/external/eastl`
- upstream library and broader harness code stay in `ref/EASTL` and related EA
  repos

Do not collapse those into one directory just to make one test pass.

## Read First

- [ideas/open/15_cpp_external_eastl_suite_bootstrap.md](/workspaces/c4c/ideas/open/15_cpp_external_eastl_suite_bootstrap.md)
- [tests/CMakeLists.txt](/workspaces/c4c/tests/CMakeLists.txt)
- [tests/cpp/external/CMakeLists.txt](/workspaces/c4c/tests/cpp/external/CMakeLists.txt)
- [tests/cpp/external/clang/README.md](/workspaces/c4c/tests/cpp/external/clang/README.md)
- [tests/cpp/eastl/README.md](/workspaces/c4c/tests/cpp/eastl/README.md)

## Current Scope

- add `tests/cpp/external/eastl` bootstrap files
- register the suite from the main test CMake
- add one frontend-smoke case around `EASTL/internal/piecewise_construct_t.h`
- prove configure/build/test for the new suite path

## Non-Goals

- full upstream `ref/EASTL/test` harness import
- multi-case EASTL expansion in the same slice
- submodule conversion in the same slice

## Working Model

- mirror the repo's external-suite shape: `allowlist.txt` + `RunCase.cmake`
- inject `ref/EASTL/include` and `ref/EABase/include/Common` from the runner
- prefer standalone extracted cases that can pass at the current frontier and
  still leave a clean route toward later runtime coverage
- treat future submodule conversion as follow-on work, not a blocker

## Execution Rules

- keep the first case standalone; do not pull `EATest`, `EAMain`, or the
  monolithic upstream registration harness into this slice
- record upstream provenance in `UPSTREAM.md`
- make the suite auto-detectable from `tests/CMakeLists.txt`
- keep test names filterable under a stable `eastl_cpp_external_...` prefix
- require `build -> targeted ctest` proof before accepting the slice

## Step 1

### Goal

Create the suite root and registration contract.

### Primary Targets

- [tests/CMakeLists.txt](/workspaces/c4c/tests/CMakeLists.txt)
- [tests/cpp/external/CMakeLists.txt](/workspaces/c4c/tests/cpp/external/CMakeLists.txt)
- `tests/cpp/external/eastl/{README.md,UPSTREAM.md,allowlist.txt,RunCase.cmake}`

### Actions

- add a cache variable and enable toggle for the EASTL external suite
- auto-detect `tests/cpp/external/eastl`
- register allowlisted cases under an `eastl_cpp_external_...` test prefix
- implement a runner that injects shared EASTL include paths and supports
  `parse`, `frontend`, and `runtime` modes

### Completion Check

The new suite is discoverable at configure time and has a stable runner
contract for future cases.

## Step 2

### Goal

Land the first proven EASTL-derived case.

### Primary Targets

- [tests/cpp/external/eastl/piecewise_construct/frontend_basic.cpp](/workspaces/c4c/tests/cpp/external/eastl/piecewise_construct/frontend_basic.cpp)
- [tests/cpp/external/eastl/allowlist.txt](/workspaces/c4c/tests/cpp/external/eastl/allowlist.txt)

### Actions

- add one standalone frontend-smoke case around
  `ref/EASTL/include/EASTL/internal/piecewise_construct_t.h`
- keep it minimal and self-contained
- prove it through the new frontend-mode runner

### Completion Check

`ctest` can run the first `eastl_cpp_external_...` case successfully.

## Step 3

### Goal

Leave the route ready for follow-on expansion.

### Actions

- capture provenance and scope boundaries clearly
- record next candidate directions in `todo.md`
- keep the suite compatible with a future dedicated submodule

### Completion Check

Another agent can add the second case or convert the suite layout without
re-deriving the boundary decisions from scratch.
