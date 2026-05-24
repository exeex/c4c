Status: Active
Source Idea Path: ideas/open/shared-mir-same-block-producer-select-queries.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce Shared MIR Query Records

# Current Packet

## Just Finished

Step 2: Introduce Shared MIR Query Records completed as a code slice. Added
`src/backend/mir/query.hpp` and `src/backend/mir/query.cpp` as the shared
MIR-owned query surface, then wired `src/backend/CMakeLists.txt` so the backend
target builds it.

The shared API now owns target-neutral records and helpers for:

- same-block producer lookup by named result, including producer kind and
  instruction index via `SameBlockProducerRecord`;
- producer pointer-to-index facts via `SameBlockProducerIndex`;
- same-block binary and select producer facts via `SameBlockBinaryProducer` and
  `SameBlockSelectProducer`;
- bounded same-block integer constant facts via `SameBlockIntegerConstant`;
- select/cast/binary dependency traversal facts via
  `DependencyTraversalRecord` and `select_chain_contains_dependency`.

`dispatch_producers.cpp` now keeps the existing AArch64 dispatch producer
function names as thin adapters over the shared MIR query API. The AArch64
direct-global-load question remains target-side by passing a predicate into the
shared dependency traversal, and AArch64-specific global symbol lookup, BIR
block lookup, and join-publication policy stayed local.

## Suggested Next

Execute Step 3: Rewire AArch64 Users To The Shared Query. Replace direct
AArch64 helper ownership at selected consumer sites with the shared MIR query
records where that reduces local policy-free logic, while preserving the
existing dispatch producer wrappers for compatibility until all users are
settled.

## Watchouts

- `dispatch_branch_fusion.cpp` still has local duplicate binary-producer and
  integer-constant helpers; Step 3 should either route that logic through
  `mir::evaluate_same_block_integer_constant` or leave a clear target-policy
  reason if any duplicate remains.
- `dispatch_producers.cpp` is intentionally still the AArch64 compatibility
  surface for existing consumers. Avoid deleting wrappers before call,
  publication, store, and branch-fusion users are migrated coherently.
- The shared traversal is predicate-based; keep target-specific questions such
  as direct-global-load materialization decisions outside `src/backend/mir`.

## Proof

Delegated proof command:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: passed. `ctest` reported `155/155` backend tests passed, `0` failed.
Proof log: `test_after.log`.
