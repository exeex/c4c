Status: Active
Source Idea Path: ideas/open/04_bir-memory-entrypoints-and-helper-boundaries.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Normalize The Pure Helper Header

# Current Packet

## Just Finished

Step 2 of `plan.md` removed the `memory_helpers.hpp` member-fragment include
from `BirFunctionLowerer`. `lowering.hpp` now explicitly declares the eight
previously macro-included static memory helper members:

- `find_repeated_aggregate_extent_at_offset(...)`
- `find_nested_repeated_aggregate_extent_at_offset(...)`
- `resolve_relative_gep_target(...)`
- `find_pointer_array_length_at_offset(...)`
- `resolve_global_gep_address(...)`
- `resolve_relative_global_gep_address(...)`
- `resolve_global_dynamic_pointer_array_access(...)`
- `resolve_global_dynamic_aggregate_array_access(...)`

`memory_helpers.hpp` is now a normal `#pragma once` pure-helper header and no
longer has a member-fragment macro mode or member-fragment include guards.
No behavior, tests, or implementation bodies changed.

## Suggested Next

Supervisor should either treat Step 3 as satisfied by the accepted
header-normalization slice or delegate a narrow verification packet that checks
`memory_helpers.hpp` exposes only pure helper structs/functions.

## Watchouts

- The active repo path is `src/backend/bir/lir_to_bir/...`, not the stale
  `src/c4c/lir_to_bir/...` path in the runbook.
- `memory_helpers.hpp` still includes `../lowering.hpp` because the remaining
  pure helper declarations use `BirFunctionLowerer` nested aliases/types.
- No local-slot helper movement was attempted in this packet.

## Proof

Ran delegated proof:

`bash -o pipefail -c 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_" | tee test_after.log'`

Result: passed. Build completed, and the backend CTest regex subset reported
100% tests passed, 0 failed out of 97 tests run. The supervisor regression
guard also passed with 97 before / 97 after, and `test_after.log` was rolled
forward to `test_before.log`. Current canonical proof log path:
`test_before.log`.
