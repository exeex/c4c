Status: Active
Source Idea Path: ideas/open/shared-mir-same-block-producer-select-queries.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Cross-Target Reuse Proof

# Current Packet

## Just Finished

Step 4: Add Cross-Target Reuse Proof completed as a focused backend test slice.
`backend_x86_shared_producer_query` now compiles from an x86-facing context,
includes `src/backend/mir/query.hpp` without AArch64 codegen headers, and
exercises real shared query behavior over a BIR block: binary producer lookup,
integer-constant evaluation, select producer lookup, named producer
classification, producer index records, and select-chain dependency traversal.

## Suggested Next

Proceed to supervisor review or lifecycle handoff for whether the shared MIR
producer query follow-up is complete enough to close, retire, or split. If more
implementation is requested, start from a new tight API boundary rather than
another broad AArch64 wrapper migration.

## Watchouts

- Compatibility wrappers still have live AArch64 consumers.
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

Result: passed. `ctest` reported `156/156` backend tests passed, `0` failed.
Proof log: `test_after.log`.
