Status: Active
Source Idea Path: ideas/open/289_structured_opaque_pointer_byte_range_provenance.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Populate Base Identity and Known Extent Facts

# Current Packet

## Just Finished

Step 3: Populate Base Identity and Known Extent Facts completed for the
prepared local/global/string route subset.

Updated prepared stack-layout memory-address publication so already accepted
direct local-slot, direct global-symbol, and direct string-constant routes keep
their existing behavior while the passive provenance carrier records facts
that were already authoritative on those paths:

- direct frame-slot accesses now populate fallback `LocalSlot` base identity
  from the BIR load/store slot and known complete object extent from the
  resolved `PreparedFrameSlot::size_bytes`
- direct global accesses without a structured BIR address now populate
  `GlobalSymbol` base identity and known complete object extent from the
  resolved `bir::Global`
- direct global and string-constant accesses with a structured BIR address now
  preserve any existing address provenance, fill only missing base identity,
  and populate known complete extent from the resolved `bir::Global` when its
  size is nonzero
- requested ranges still use the existing defensive
  `bir::make_memory_byte_range` construction and `range_verdict` remains the
  neutral `UnknownCompatible`

No prepared/prealloc consumer gates on the new extent facts in this slice.

## Suggested Next

Populate the remaining Step 3 identity/extent routes for pointer SSA/value,
formal, byval, sret, and explicit unknown-runtime bases, still keeping range
proof and consumer behavior unchanged.

## Watchouts

- This packet deliberately treats zero-sized global extents as unknown instead
  of publishing a complete zero-byte object extent.
- The provenance carrier remains passive. Do not route prepared lookup,
  publication, or target-specific acceptance on these facts until the
  supervisor selects the consumer-verdict step.
- This slice uses resolved prepared stack/global facts only; it does not try to
  infer layout authority or prove in-bounds ranges from
  `can_use_base_plus_offset`.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepared_lookup_helper_test backend_riscv_prepared_edge_publication_test backend_prepared_printer_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepared_lookup_helper|backend_riscv_prepared_edge_publication|backend_prepared_printer|backend_aarch64_instruction_dispatch)$' > test_after.log`

CTest result: 5/5 passed.
