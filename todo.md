Status: Active
Source Idea Path: ideas/open/shared-mir-same-block-producer-select-queries.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Rewire AArch64 Users To The Shared Query

# Current Packet

## Just Finished

Step 3: Rewire AArch64 Users To The Shared Query completed as a narrow code
slice. `dispatch_store_sources.cpp` now calls
`mir::find_same_block_select_producer` directly in
`store_local_value_has_select_producer`, replacing the remaining AArch64
named-producer wrapper use for that policy-free select-producer fact.

The AArch64 compatibility wrapper remains in `dispatch_producers.cpp` for
unmigrated users, and no store-source planning, pointer writeback,
frame-slot addressing, publication planning, or call-boundary semantics moved.

## Suggested Next

Proceed to Step 4 or supervisor review before another code slice. The remaining
wrapper consumers in value materialization and store sources are mostly coupled
to prepared-memory access, local-load offset reuse, direct-global-load
materialization, scalar FP emission, cast emission, pointer base writeback, or
store/publication planning, so another migration should start from a tighter
API boundary instead of broad rewiring.

## Watchouts

- Step 3 is not fully complete yet: compatibility wrappers still have live
  AArch64 consumers.
- `dispatch_store_sources.cpp` still has direct AArch64 wrapper users for
  byval frame-slot load checks, wide-load/narrow-store checks, cast producer
  emission, scalar FP binary producer checks, stored value materialization, and
  pointer-base offset emission.
- `dispatch_value_materialization.cpp` still has direct AArch64 wrapper users
  in FP value materialization and generic value publication. Those flows mix
  producer/index facts with frame-slot addressing, load-global symbol policy,
  current-block entry publication, and prepared value homes.
- The shared traversal is predicate-based; keep target-specific questions such
  as direct-global-load materialization decisions outside `src/backend/mir`.

## Proof

Delegated proof command:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"; } 2>&1 | tee test_after.log'`

Result: passed. `ctest` reported `155/155` backend tests passed, `0` failed.
Proof log: `test_after.log`.
