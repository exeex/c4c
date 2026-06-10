Status: Active
Source Idea Path: ideas/open/169_route3_semantic_memory_access_cache_split.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Or Confirm Route 3 Oracle Coverage

# Current Packet

## Just Finished

Step 2 of `plan.md` completed for the selected AArch64 FP same-block
global-load value materialization family.

Coverage confirmed and extended:
- `backend_aarch64_prepared_memory_operand_records_test.cpp` now includes an
  explicit `F64` prior `LoadGlobalInst` oracle case showing
  `mir::find_bir_same_block_global_load_access_identity` and Route 3 agree with
  the prepared same-block global-load access oracle for the FP materialization
  shape.
- The same AArch64 oracle also checks FP before-producer fail-closed behavior
  and an explicit Route 3 non-global-root rejection.
- Existing AArch64/shared helper coverage still covers prior global-load
  positive lookup, before-producer rejection, root type mismatch, non-global
  root mismatch, and unsupported/string-load fail-closed shapes.

## Suggested Next

Execute Step 3 for the selected FP same-block global-load family: switch only
the `LoadGlobalInst` producer branch in
`src/backend/mir/aarch64/codegen/fp_value_materialization.cpp` from the
prepared same-block global-load semantic lookup to
`mir::find_bir_same_block_global_load_access_identity`, while keeping
`PreparedMemoryAccess` as the prepared target-addressing/address-policy source
passed to `emit_prepared_fp_global_load_to_register`.

## Watchouts

- Step 3 should use Route 3 only for semantic same-block global-load identity;
  do not copy `PreparedAddress`, `PreparedMemoryAccess`, symbol relocation,
  GOT/direct policy, offsets, base-plus-offset legality, or memory operand
  formation into BIR.
- Preserve prepared access lookup as the target-addressing oracle/source until
  a later contraction step proves it private.
- Keep the Step 3 packet bounded to
  `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp`,
  `todo.md`, and any directly necessary FP-focused oracle update. The Step 2
  proof already covers the shared BIR/prepared lookup oracle.

## Proof

Passed:
`(cmake --build build --target backend_aarch64_prepared_memory_operand_records_test backend_prepared_lookup_helper_test && ctest --test-dir build -R '^(backend_aarch64_prepared_memory_operand_records|backend_prepared_lookup_helper)$' --output-on-failure) > test_after.log 2>&1`

Proof log: `test_after.log`.
