Status: Active
Source Idea Path: ideas/open/557_bir_local_memory_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused BIR Coverage

# Current Packet

## Just Finished

Step 2 - Add Focused BIR Coverage completed as a producer-boundary test packet.
Extended `tests/backend/bir/backend_lir_to_bir_notes_test.cpp` with
`expect_local_memory_alloca_records_pin_slot_and_source_contracts()` for scalar
alloca local-slot records, fixed local-array source-object/element-slot
records, and aggregate alloca leaf-slot records. Tightened
`expect_runtime_pointer_value_opaque_i32_access_uses_pointer_base()` so the
pointer-value load/store assertions pin `MemoryAddress` offset/size/alignment
plus provenance base identity, requested byte range, range verdict, and opaque
layout authority.

## Suggested Next

Start the first semantic repair packet at the BIR producer boundary identified
in Step 1, using the focused coverage from Step 2 as the guardrail before
touching downstream prepared/RV64 consumers.

## Watchouts

- Existing focused GEP coverage already pins local array/path producer-coordinate
  records through
  `expect_local_array_carrier_constant_gep_publishes_source_derivation_and_layout()`,
  `expect_local_array_carrier_dynamic_gep_preserves_missing_index_range_proof()`,
  and
  `expect_local_array_carrier_lir_producer_coordinate_is_not_bir_inst_index()`.
- Existing scalar/local-memory bridge coverage already checks admitted bridge
  cases through `local_scalar_double_decimal_zero_store`,
  `local_scalar_double_partial_float_memcpy`,
  `local_scalar_i64_partial_i8_memcpy`, `local_byte_storage_overlay`, and the
  local-array scalar load/provenance tests.
- This packet intentionally did not add failing tests for repair-only missing
  producer facts because the current repo style does not commit expected-failing
  backend tests in this harness.
- Keep semantic admission checks, expectations, unsupported markers, allowlists,
  runtime comparison behavior, and downstream prepared/RV64 consumer checks
  unchanged during repair work.

## Proof

Ran exactly `cmake --build --preset default` followed by
`ctest --test-dir build -j --output-on-failure -R '^backend_'`, with combined
output recorded in `test_after.log`. Result: build succeeded and the backend
CTest subset passed.
