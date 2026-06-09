# Current Packet

Status: Active
Source Idea Path: ideas/open/148_same_block_load_local_stored_value_owner.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Rewire consumers to the narrow owner

## Just Finished

Step 3 rewired the direct AArch64 consumer to include the narrow addressing
owner for the moved same-block load-local stored-value API.

- `src/backend/mir/aarch64/codegen/calls.cpp` now includes
  `../../../prealloc/addressing.hpp` directly for
  `PreparedSameBlockLoadLocalStoredValueSource` and
  `find_prepared_same_block_load_local_stored_value_source`.
- `../../../prealloc/prepared_lookups.hpp` remains included because
  `calls.cpp` still consumes unrelated prepared lookup state through
  `PreparedFunctionLookups` / `prepared_lookups`.
- No AArch64 emission, register, scratch, extension, or prepared memory-access
  construction behavior changed.

## Suggested Next

Supervisor should decide whether the Step 4 proof-only closeout is already
satisfied by this packet's delegated build plus backend subset proof, or whether
to send a lifecycle/review packet before closing the runbook.

## Watchouts

- `prepared_lookups.hpp` still transitively includes `addressing.hpp`, so the
  direct include is an ownership/documentation fix as well as compile hygiene.
- This packet did not move the finder definition out of
  `prepared_lookups.cpp`; Step 2 intentionally left behavior identical there.

## Proof

AST checks:

- `c4c-clang-tool-ccdb type-refs` confirmed the
  `PreparedSameBlockLoadLocalStoredValueSource` type reference in
  `calls.cpp`.
- `c4c-clang-tool-ccdb function-callees` confirmed
  `find_prepared_indirect_callee_stored_value_source` resolves
  `find_prepared_same_block_load_local_stored_value_source` from
  `src/backend/prealloc/addressing.hpp`.

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
Result: passed, 179/179 backend tests. Regression guard passed with 179/179
backend tests before and after, no new failures.
Proof log: accepted after-proof was rolled forward to `test_before.log`;
there is no current root `test_after.log`.
