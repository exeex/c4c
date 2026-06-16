Status: Active
Source Idea Path: ideas/open/289_structured_opaque_pointer_byte_range_provenance.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Retire or Quarantine the Compatibility Bridge

# Current Packet

## Just Finished

Step 6: Retire or Quarantine the Compatibility Bridge continued by publishing
prepared source-memory layout-authority metadata without changing opaque
compatibility acceptance.

- Prepared addressing dumps now print `layout_authority` alongside
  `range_verdict` and `dynamic_array_verdict`, so `OpaqueCompatibility` rows are
  externally visible in prepared reporting.
- Prepared edge-publication source-memory snapshots and edge-copy source facts
  now carry a flattened `source_memory_layout_authority` field copied from
  `PreparedMemoryAccess::address.provenance.layout_authority`.
- Stale-row/source-memory matching now compares the flattened layout-authority
  fact with the current prepared memory access, matching the existing range and
  dynamic-array verdict behavior.
- Focused prepared lookup and prepared printer tests cover the
  `OpaqueCompatibility` carrier path.
- This satisfies the reviewer Step 6 precondition for explicit prepared
  metadata publication. Bridge retirement can remain quarantine-only for this
  slice; an actual reject/admission policy still requires a separate
  behavior-changing packet.

## Suggested Next

Decide whether Step 6 should stop at quarantine-only metadata publication or
delegate a separate behavior-changing packet to reject/gate
`OpaqueCompatibility` rows.

## Watchouts

- `source_memory_layout_authority` is carrier and consistency metadata only; no
  prepared or target consumer accepts or rejects based on it in this packet.
- Existing opaque pointer compatibility behavior remains accepted, including
  the earlier structured-provenance marking of those rows as
  `OpaqueCompatibility`.
- The only stricter check added here is stale-row/source-memory consistency:
  a publication whose flattened layout authority no longer matches the current
  prepared memory access no longer matches that access.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepared_lookup_helper_test backend_riscv_prepared_edge_publication_test backend_prepared_printer_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepared_lookup_helper|backend_riscv_prepared_edge_publication|backend_prepared_printer|backend_aarch64_instruction_dispatch)$' > test_after.log`

CTest result: 5/5 passed.
