# AArch64 Dispatch Lookup Thin Helper Surface Trim Runbook

Status: Active
Source Idea: ideas/open/132_aarch64_dispatch_lookup_thin_helper_surface_trim.md

## Purpose

Trim stale or local-only public declarations from the AArch64 dispatch lookup surface while preserving the real query hooks that non-dispatch lowering code still consumes.

## Goal

Reduce `dispatch_lookup.hpp` to the lookup API that has a clear cross-file owner contract.

## Core Rule

Do not delete externally used lookup hooks unless their callers are moved to an equal or clearer owner API in the same slice.

## Read First

- `ideas/open/132_aarch64_dispatch_lookup_thin_helper_surface_trim.md`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- Natural lookup callers:
  - `src/backend/mir/aarch64/codegen/calls.cpp`
  - `src/backend/mir/aarch64/codegen/globals.cpp`
  - `src/backend/mir/aarch64/codegen/memory.cpp`
  - `src/backend/mir/aarch64/codegen/memory_store_retargeting.cpp`
  - `src/backend/mir/aarch64/codegen/comparison.cpp`
  - `src/backend/mir/aarch64/codegen/alu.cpp`

## Current Targets

- `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- Direct callers of public lookup declarations
- `src/backend/CMakeLists.txt` only if a translation unit is removed

## Non-Goals

- Do not change prepared-result register selection semantics.
- Do not change scalar availability semantics.
- Do not reintroduce BIR-name rescans or same-block named-case matching.
- Do not treat small file size or file-count reduction as sufficient reason to move code.
- Do not weaken backend test expectations to make a surface change pass.

## Working Model

`dispatch_lookup.hpp` may expose real cross-file query hooks such as `make_named_prepared_result_register` and `emitted_scalar_value_available`. Public declarations with no direct callers, local-only use, or stale same-block lookup responsibilities should be removed, privatized, or rehomed only after direct evidence proves the boundary.

## Execution Rules

- Start with declaration and caller evidence before editing headers.
- Preserve public hooks that are still consumed by non-dispatch lowering code unless the same packet provides a clearer owner API.
- If a helper is only used inside `dispatch_lookup.cpp`, make it file-local instead of leaving it public.
- If a helper is unused, remove both declaration and dead definition when proven safe.
- If a translation unit is removed, update `src/backend/CMakeLists.txt` in the same packet.
- For code-changing packets, run `cmake --build --preset default` and `ctest --test-dir build -R '^backend_aarch64_' --output-on-failure`.
- If public header or translation-unit shape changes, preserve matching canonical `test_before.log` and `test_after.log` for the same proof command.

## Steps

### Step 1: Build Lookup Surface Inventory

Goal: Classify every public declaration in `dispatch_lookup.hpp` before changing visibility.

Primary Target: `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp`

Actions:

- List every public type, function, and helper declaration in `dispatch_lookup.hpp`.
- Map each declaration to its definition, direct callers, and include users.
- Separate externally used lookup hooks from local-only helpers and no-direct-caller declarations.
- Record whether each declaration is retained, privatization candidate, rehome candidate, or removal candidate.

Completion Check:

- `todo.md` records a declaration/call-site table with evidence for every public declaration in `dispatch_lookup.hpp`.
- No implementation files are edited in this step.

### Step 2: Privatize Or Remove Proven Local Lookup Helpers

Goal: Contract public lookup surface where Step 1 proved declarations are local-only or unused.

Primary Target: `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp`

Actions:

- Remove local-only helper declarations from `dispatch_lookup.hpp`.
- Move corresponding definitions into anonymous namespace scope in `dispatch_lookup.cpp` when still used locally.
- Delete unused helper definitions only when direct evidence shows no callers remain.
- Keep externally used hooks public unless this step also updates every caller to a clearer owner API.

Completion Check:

- The header exposes only declarations with a real cross-file contract.
- The build passes.
- `^backend_aarch64_` tests pass.

### Step 3: Rehome Any Lookup Contract With A Clearer Owner

Goal: Move any remaining public lookup declaration only when another owner has a clearer contract.

Primary Target: Direct callers and the selected owner header/source, if any.

Actions:

- For each retained public lookup hook, decide whether `dispatch_lookup` remains the clearest owner.
- If a clearer owner exists, move the declaration and implementation with all direct callers updated.
- Do not create bridge wrappers that preserve the same stale surface under a new name.
- Do not move target-local emission policy into shared code.

Completion Check:

- Every moved hook has a clearer owner and no stale declaration remains in `dispatch_lookup.hpp`.
- `make_named_prepared_result_register` and `emitted_scalar_value_available` remain available unless replaced by an equal or clearer owner API.
- The build passes.
- `^backend_aarch64_` tests pass.

### Step 4: Clean Build Metadata Only If Shape Changed

Goal: Keep build metadata aligned with any translation-unit removal.

Primary Target: `src/backend/CMakeLists.txt`

Actions:

- Inspect whether `dispatch_lookup.cpp` or any other touched translation unit became empty or obsolete.
- Remove build metadata entries only when a translation unit is actually removed.
- Leave build metadata unchanged for header-only visibility contraction.

Completion Check:

- CMake metadata matches the final file set.
- The build passes from the updated metadata.

### Step 5: Finalize Surface Contract And Proof

Goal: Close the active runbook only after the lookup surface contract and proof are coherent.

Primary Target: `todo.md`

Actions:

- Summarize final public `dispatch_lookup.hpp` declarations and their owner rationale.
- Confirm no BIR-name rescans, same-block named-case matching, or expectation weakening were introduced.
- Run the required proof command and preserve matching canonical logs if public header or translation-unit shape changed.
- Identify whether any broader shared-query work belongs to ideas 133-135 rather than this plan.

Completion Check:

- `todo.md` records final retained public hooks, removed/private helpers, validation command, and result.
- Regression logs are available when required by the public header or translation-unit change.
- The source idea acceptance criteria are satisfied without expanding into shared-query initiatives.
