Status: Active
Source Idea Path: ideas/open/290_gate_quarantined_opaque_compatibility_memory_accesses.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Implement the Quarantine Gate

# Current Packet

## Just Finished

Step 3: Implement the Quarantine Gate proof tightened with direct store-side
opaque compatibility coverage.

Implemented behavior:

- `BirFunctionLowerer::try_lower_addressed_pointer_store` now rejects
  `ScalarSubobjectAddressability::OpaqueCompatibility` immediately after the
  existing scalar-subobject classification.
- `BirFunctionLowerer::try_lower_addressed_pointer_load` now rejects
  `ScalarSubobjectAddressability::OpaqueCompatibility` at the same point.
- The pre-existing
  `!is_scalar_subobject_addressable(addressability)` rejection remains intact
  in both helpers.
- The gate does not inspect or gate on `MemoryRangeVerdict::UnknownCompatible`,
  `can_use_base_plus_offset`, unknown extent, or prepared flattened fields.

Focused test coverage:

- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp` keeps
  `expect_loaded_pointer_addressed_store_uses_pointer_base` as the structured
  accepted addressed pointer store check.
- The former casted-byte-pointer acceptance fixture is now
  `expect_casted_byte_pointer_i32_update_fails_closed`; it asserts that the
  casted `i8*` to wider `i32` addressed load route fails before BIR
  publication and reports the load local-memory semantic failure.
- Added `expect_casted_byte_pointer_i32_store_fails_closed`, a store-only
  opaque compatibility fixture using a dynamic byte offset, so
  `try_lower_addressed_pointer_store` is directly covered by a fail-closed
  store local-memory semantic failure instead of being inferred from the
  load-side fixture.

## Suggested Next

Step 4 packet: run the supervisor-selected broader prepared/backend validation
for the behavior-changing acceptance policy, including prepared lookup and
prepared edge-publication coverage if the supervisor wants confidence beyond
the focused lowerer proof.

## Watchouts

- The selected behavior predicate is deliberately only
  `ScalarSubobjectAddressability::OpaqueCompatibility`.
- Prepared stale-row, duplicate-row, and cross-block matching code was not
  touched in this packet.
- `pointer_value_memory_provenance_with_layout_authority` remains available for
  non-rejected non-opaque rows and existing prepared provenance coverage.
- A constant-zero byte-store fixture is not sufficient for this proof because
  it can still lower through structured byte-storage coverage; the direct
  store-side opaque compatibility proof uses a dynamic byte offset.

## Proof

Exact delegated proof command passed:

`cmake --build --preset default --target backend_lir_to_bir_notes_test && ctest --test-dir build -R '^backend_lir_to_bir_notes$' --output-on-failure > test_after.log`

Proof log path: `test_after.log`

Result: build succeeded and `backend_lir_to_bir_notes` passed.
