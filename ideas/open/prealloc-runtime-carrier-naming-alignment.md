# Prealloc Runtime Carrier Naming Alignment

## Goal

Repair naming and comments around runtime helper contracts and special carrier
contracts while preserving their separate responsibilities.

## Why This Exists

The responsibility map identifies runtime helpers and special carriers as
related but distinct prealloc families. Their names and prepared-printer mirrors
should make helper-call resources, marshal plans, and value-carrier facts
visible without merging those concepts into one less precise contract.

## Target Files

- `src/backend/prealloc/runtime_helpers.hpp`
- `src/backend/prealloc/i128_runtime_helpers.cpp`
- `src/backend/prealloc/i128_runtime_helpers.hpp`
- `src/backend/prealloc/f128_runtime_helpers.cpp`
- `src/backend/prealloc/f128_runtime_helpers.hpp`
- `src/backend/prealloc/special_carriers.cpp`
- `src/backend/prealloc/special_carriers.hpp`
- `src/backend/prealloc/atomics.cpp`
- `src/backend/prealloc/atomics.hpp`
- `src/backend/prealloc/intrinsics.cpp`
- `src/backend/prealloc/intrinsics.hpp`
- `src/backend/prealloc/inline_asm.cpp`
- `src/backend/prealloc/inline_asm.hpp`
- `src/backend/prealloc/prepared_printer/runtime_helpers.cpp`
- `src/backend/prealloc/prepared_printer/special_carriers.cpp`

## Slice Type

Helper/comment naming repair with prepared-printer alignment.

## Durable Owner

Runtime and special carriers.

## In Scope

- Rename helper functions, comments, and local grouping so helper-call
  resources and carrier requirements are not confused.
- Keep `runtime_helpers.hpp` and `special_carriers.hpp` as separate aggregate
  contracts.
- Align prepared-printer runtime-helper and special-carrier terminology with
  the data contracts in the same slice.
- Preserve i128, f128, atomics, intrinsics, and inline-asm ownership as visible
  subfamilies.

## Out Of Scope

- Merging runtime helper resources and special carrier facts into one contract.
- Changing helper-call selection, marshal behavior, carrier requirements,
  intrinsic support decisions, or inline-asm behavior.
- Creating generic helper dispatch that erases i128/f128/atomics/intrinsics
  differences.
- Rewriting supported/unsupported expectations or prepared dump meaning.

## Expected Behavior-Preservation Proof

Run:

```sh
bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'
```

Also run `git diff --check`.

## Acceptance Criteria

- Runtime helper and special carrier names communicate separate ownership.
- Printer mirrors use the same terminology as the data contracts.
- Backend tests remain green with no helper-call or carrier semantic changes.

## Reviewer Reject Signals

- Reject merging helper-call resources and carrier facts into a broader,
  less precise contract.
- Reject shared helper extraction that blurs i128/f128 ABI differences or
  creates target-shaped shortcuts.
- Reject intrinsic, atomics, or inline-asm support behavior changes under a
  naming label.
- Reject expectation rewrites, unsupported downgrades, or dump weakening used
  to claim progress.
- Reject printer files becoming a second ownership taxonomy not present in
  data files.
