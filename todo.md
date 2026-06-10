Status: Active
Source Idea Path: ideas/open/169_route3_semantic_memory_access_cache_split.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate And Handoff

# Current Packet

## Just Finished

Step 5 of `plan.md` completed for the selected AArch64 FP same-block
global-load value materialization family.

Validation and handoff:
- The selected FP `LoadGlobalInst` materialization consumer now reads Route 3
  semantic same-block global-load identity through
  `mir::find_bir_same_block_global_load_access_identity`.
- Prepared `PreparedMemoryAccess` remains only as the target-addressing/
  address-policy emission source for this consumer.
- The final narrow proof is green for the Route 3/prepared oracle tests and the
  selected AArch64 memory operand record subset.

Residual prepared same-block global-load surface evidence:

- `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp` no longer calls
  `prepare::find_prepared_same_block_global_load_access` or
  `prepare::find_prepared_global_load_access`; it uses
  `mir::find_bir_same_block_global_load_access_identity` for semantic identity
  and keeps prepared `PreparedMemoryAccess` only as target-addressing/
  address-policy emission data.
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` still
  directly calls `prepare::find_prepared_same_block_global_load_access` through
  `prepared_shape_same_block_scalar_producer` for a remaining AArch64 same-block
  global-load materialization path.
- `src/backend/mir/aarch64/codegen/globals.cpp` still directly calls
  `prepare::find_prepared_global_load_access` through
  `prepared_current_global_load_access` for current-instruction global-load GOT
  materialization policy.
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`
  and `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` still use
  `prepare::find_prepared_same_block_global_load_access` as the prepared oracle
  for BIR/Route 3 equivalence coverage.

Because remaining non-FP codegen consumers still require the public prepared
semantic lookup, and target/addressing payloads must remain public, this
runbook made an explicit no-contraction handoff.

## Suggested Next

The selected consumer migration is complete. If this source idea continues,
the next bounded Route 3 candidate should migrate or retire the remaining
`dispatch_value_materialization.cpp` same-block global-load prepared consumer
before any public helper contraction is attempted.

## Watchouts

- `PreparedSameBlockGlobalLoadAccess`,
  `prepare::find_prepared_global_load_access`, and
  `prepare::find_prepared_same_block_global_load_access` remain public by direct
  consumer evidence.
- Target/addressing payloads remain public and are still required for
  address-policy and memory operand emission.
- Do not close or contract `PreparedSameBlockGlobalLoadAccess`,
  `prepare::find_prepared_global_load_access`, or
  `prepare::find_prepared_same_block_global_load_access` until remaining
  non-FP consumers have migrated or been explicitly retired.

## Proof

Passed:
`(cmake --build build --target backend_aarch64_prepared_memory_operand_records_test backend_prepared_lookup_helper_test && ctest --test-dir build -R '^(backend_aarch64_prepared_memory_operand_records|backend_prepared_lookup_helper)$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log`.
