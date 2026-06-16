Status: Active
Source Idea Path: ideas/open/289_structured_opaque_pointer_byte_range_provenance.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Retire or Quarantine the Compatibility Bridge

# Current Packet

## Just Finished

Step 6: Retire or Quarantine the Compatibility Bridge started by preserving the
opaque pointer compatibility bridge behavior while making its accepted route
explicit in structured provenance metadata.

- `can_address_scalar_subobject` now classifies scalar-subobject acceptance as
  rejected, normal accepted, or opaque compatibility while preserving its old
  boolean wrapper for existing non-opaque callers.
- Addressed pointer load/store lowering still accepts the existing
  `allow_opaque_ptr_base && stored_type == I8` bridge case, but memory
  addresses produced through that route now carry
  `layout_authority = OpaqueCompatibility` and keep
  `range_verdict = UnknownCompatible`.
- The zero-offset opaque pointer spelling bridge for empty/`ptr`/`i8`
  pointer-address type text remains accepted and is also marked as
  `OpaqueCompatibility` with an unknown-compatible range verdict.
- No prepared/prealloc consumer or target lowering gate was changed to reject
  these rows.

## Suggested Next

Decide the behavior-changing retirement policy for opaque compatibility rows:
either reject the quarantined addressed pointer cases at lowering time or teach
the relevant prepared/target consumers a supervisor-approved admission gate.

## Watchouts

- Actual bridge retirement is deferred because rejecting either the
  `allow_opaque_ptr_base && stored_type == I8` case or the zero-offset
  empty/`ptr`/`i8` opaque spelling case would remove currently accepted
  addressed pointer load/store behavior.
- `layout_authority` is structured carrier metadata only in this slice; current
  prepared publication and target consumers continue to key on the existing
  range and dynamic-array verdicts, so no accepted rows start failing here.
- Existing stale-row/fail-closed behavior remains unchanged.
- The audit used `rg` plus `c4c-clang-tool-ccdb` callers for the key prepared
  helpers.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepared_lookup_helper_test backend_riscv_prepared_edge_publication_test backend_prepared_printer_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepared_lookup_helper|backend_riscv_prepared_edge_publication|backend_prepared_printer|backend_aarch64_instruction_dispatch)$' > test_after.log`

CTest result: 5/5 passed.
