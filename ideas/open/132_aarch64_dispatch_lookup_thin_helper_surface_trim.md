# AArch64 Dispatch Lookup Thin Helper Surface Trim

## Goal

Trim or rehome stale public helper declarations from the AArch64 dispatch
lookup surface while retaining the real lookup hooks that non-dispatch lowering
code still consumes.

## Why This Exists

The post-contract dispatch-family audit found that
`dispatch_lookup.hpp` mixes real cross-file query hooks with declarations that
have no direct callers. The file is a thin query split after prior contract
work, but `make_named_prepared_result_register` and
`emitted_scalar_value_available` remain externally used.

This idea exists to remove stale or local-only lookup surface without deleting
the real query contract.

## In Scope

- `src/backend/mir/aarch64/codegen/dispatch_lookup.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp`
- Natural lookup hook callers such as:
  - `src/backend/mir/aarch64/codegen/calls.cpp`
  - `src/backend/mir/aarch64/codegen/globals.cpp`
  - `src/backend/mir/aarch64/codegen/memory.cpp`
  - `src/backend/mir/aarch64/codegen/memory_store_retargeting.cpp`
  - `src/backend/mir/aarch64/codegen/comparison.cpp`
  - `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/CMakeLists.txt` only if a translation unit is removed
- Removing no-direct-caller declarations such as stale same-block lookup
  helpers when proven unused

## Out Of Scope

- Deleting externally used lookup hooks without a replacement owner
- Reintroducing BIR-name rescans or same-block named-case matching
- Treating small file size as sufficient reason to move code
- Changing prepared-result register selection or call-lowering availability
  semantics

## Acceptance Criteria

- `make_named_prepared_result_register` and
  `emitted_scalar_value_available` remain available unless their callers are
  converted to an equal or clearer owner API.
- No-direct-caller helper declarations are removed, privatized, or rehomed with
  direct evidence.
- Prepared-result register selection and scalar availability behavior remain
  unchanged.
- Any translation-unit removal is paired with build metadata cleanup.

## Proof Route

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_aarch64_' --output-on-failure`

Use matching `test_before.log` and `test_after.log` if public header or
translation-unit shape changes.

## Reviewer Reject Signals

- Externally used lookup hooks are deleted instead of replaced by a real owner.
- Callers start rescanning BIR names or reconstructing prepared facts locally.
- The change is mostly helper renaming while preserving the same stale public
  surface.
- Small file size or file-count reduction is the main acceptance argument.
- Test expectations are weakened to hide lookup behavior regressions.
