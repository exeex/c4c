# Current Packet

Status: Active
Source Idea Path: ideas/open/838_lir_canonical_module_owned_aggregate_ref_store_convergence.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the canonical aggregate identity and store seam

## Just Finished

- Plan Step 1: added a HIR-module-issued, fail-closed `HirAggregateRef` and
  module-owned LIR aggregate ref/store. Re-registering an owned ref returns
  its original LIR ref; invalid/incomplete and foreign-source refs are
  rejected by both registration and lookup. Legacy owner-key/tag lowering
  remains an adapter.

## Suggested Next

- Migrate the first aggregate-bearing lowering occurrence to populate and use
  the canonical HIR ref/store seam, with focused valid and invalid coverage.

## Watchouts

- The new seam deliberately has no producer migration yet: existing lowering
  still uses owner-key/tag compatibility paths. Do not recover canonical refs
  from tags, parser pointers, rendered text, or owner keys.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log` passed (6 tests); log: `test_after.log`.
