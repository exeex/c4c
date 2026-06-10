Status: Active
Source Idea Path: ideas/open/169_route3_semantic_memory_access_cache_split.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate The Selected Consumer

# Current Packet

## Just Finished

Step 3 of `plan.md` completed for the selected AArch64 FP same-block
global-load value materialization family.

The `LoadGlobalInst` producer branch in
`src/backend/mir/aarch64/codegen/fp_value_materialization.cpp` now uses
`mir::find_bir_same_block_global_load_access_identity` as the primary semantic
same-block global-load identity source. The prepared `PreparedMemoryAccess`
record is still looked up by the Route 3 producer instruction index, checked
against the BIR producer instruction, and passed only as the
target-addressing/address-policy source to
`emit_prepared_fp_global_load_to_register`.

## Suggested Next

Execute Step 4 as a bounded contraction pass: remove any now-private prepared
same-block global-load producer/addressing lookup surface that is no longer
needed after this FP consumer migration, but only after confirming no remaining
consumers still require that prepared semantic lookup path.

## Watchouts

- The FP path still intentionally fails closed when Route 3 identity is missing
  while prepared lowering state exists, or when the prepared access no longer
  matches the Route 3 producer instruction.
- Step 4 should not remove prepared addressing records used for
  target-addressing/address-policy emission; the contraction target is only
  prepared semantic same-block global-load producer lookup surface proven
  private.
- Keep broad prepared/addressing APIs and `PreparedFunctionLookups` aggregate
  surfaces unchanged unless Step 4 proves a specific private helper can be
  safely removed.

## Proof

Passed:
`(cmake --build build --target backend_aarch64_prepared_memory_operand_records_test backend_prepared_lookup_helper_test && ctest --test-dir build -R '^(backend_aarch64_prepared_memory_operand_records|backend_prepared_lookup_helper)$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log`.
