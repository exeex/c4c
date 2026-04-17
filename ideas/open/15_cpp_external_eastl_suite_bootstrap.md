# C++ External EASTL Suite Bootstrap

## Goal

Create a curated `tests/cpp/external/eastl` suite that can grow toward real
`ref/EASTL/test` coverage without importing the entire upstream EASTL test
executable harness as-is.

The intended end state is:

- `ref/EASTL` remains the upstream header/library reference tree
- `tests/cpp/external/eastl` becomes the c4c-owned external-suite entrypoint
  for curated EASTL-derived C++ cases
- the suite uses c4c-native allowlist + runner wiring like other external test
  suites in this repo
- the first case is proven through the c4c external-suite harness so later
  follow-up can expand coverage or convert the directory into a dedicated
  submodule without rethinking the test contract

## Why This Is Separate

EASTL coverage is now beyond ad hoc parser bring-up cases, but the upstream
`ref/EASTL/test` tree is not shaped like the external suites already used by
c4c.

The upstream harness:

- builds a monolithic `EASTLTest` executable
- pulls additional EA support libraries
- assumes its own runner and registration model

The repo's external-suite pattern is different:

- curated case inventory under `tests/cpp/external/...`
- an explicit `allowlist.txt`
- a repo-owned `RunCase.cmake`
- bounded proof per extracted case

This idea exists to bridge those two shapes without coupling c4c to the full
upstream EASTL test harness in one jump.

## Scope

### 1. Bootstrap The External Suite Root

- create `tests/cpp/external/eastl`
- register it from `tests/CMakeLists.txt` and `tests/cpp/external/CMakeLists.txt`
- give it the same top-level affordances as the existing clang external suite:
  allowlist, runner, README, and provenance notes

### 2. Keep Upstream Library Inputs Separate

- continue to consume headers from `ref/EASTL/include` and
  `ref/EABase/include/Common`
- do not copy large upstream library trees into the external suite root
- treat the external suite as curated case inventory, not as a second EASTL
  checkout

### 3. Land The First Proven Case

- add one standalone EASTL-derived case under `tests/cpp/external/eastl`
- prove it under c4c's own runner contract
- prefer a case that can pass at the current compiler frontier without
  importing the full `EASTLTest` executable harness

### 4. Preserve The Follow-On Route

- document provenance for the extracted case
- keep the layout compatible with a future dedicated submodule if we decide to
  split the suite out later
- leave clear next-step room for additional type-traits, tuple, utility,
  memory, or vector-derived cases

## Core Rule

Do not claim progress by moving the entire upstream `ref/EASTL/test` harness
under `tests/cpp/external/eastl` without adapting it to c4c's external-suite
model.

This idea is about establishing the correct suite boundary first:

- curated external cases in `tests/cpp/external/eastl`
- upstream library sources in `ref/EASTL` / `ref/EABase`

## Acceptance Criteria

- [ ] `tests/cpp/external/eastl` exists and is auto-discoverable by the main
      test build
- [ ] the suite has a repo-owned allowlist and runner contract
- [ ] at least one standalone EASTL-derived case is registered and passing
- [ ] provenance for the first case is recorded
- [ ] the route leaves room for later submodule conversion and coverage growth

## Non-Goals

- importing the full upstream `EASTLTest` executable harness in the first patch
- pretending upstream `ref/EASTL/test/CMakeLists.txt` can be dropped into c4c
  unchanged
- broad EASTL completion from one bootstrap case
- claiming runtime parity in the same bootstrap patch when the first stable
  external proof is still frontend-only

## Good First Patch

Create `tests/cpp/external/eastl`, wire it into the test build with an
allowlist and runner that inject shared EASTL include paths, then land one
small frontend smoke test around an EASTL-owned header surface and prove it
through `ctest`.
