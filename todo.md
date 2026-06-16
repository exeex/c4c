Status: Active
Source Idea Path: ideas/open/289_structured_opaque_pointer_byte_range_provenance.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Populate Base Identity and Known Extent Facts

# Current Packet

## Just Finished

Step 3: Populate Base Identity and Known Extent Facts finished the remaining
owned pointer-value `MemoryAddress` producer inventory/population for
intrinsic, local-slot, provenance fallback, and AArch64 semantic-intrinsic
paths without changing route admission, address kind, byte offsets, sizes,
alignments, memory access kind, or prepared/prealloc consumer behavior.

- intrinsic memcpy pointer-source leaf/scalar loads now publish `PointerValue`
  identity plus requested byte range with unknown extent and neutral
  `UnknownCompatible` verdict.
- local aggregate stores from byval parameters now mirror existing aggregate
  parameter materialization by publishing `ByvalParameter` identity, complete
  extent from the selected byval layout, and requested byte range.
- local aggregate loads from addressed pointer values now preserve existing
  `PointerAddress` provenance, dynamic-array facts when present, and requested
  byte range while leaving the verdict neutral.
- pointer-provenance fallback load/store paths for pointer-valued local-slot
  state now publish `PointerValue` identity plus requested byte range with
  unknown extent.
- AArch64 cache-maintenance and vector-load intrinsic memory operands are
  target-specific/non-prepared routes, but now publish passive `PointerValue`
  identity and requested byte range facts for inventory completeness.

## Suggested Next

Step 3 is ready for supervisor review. If accepted, the next coherent packet is
Step 4 requested-range proof for the already populated carrier facts.

## Watchouts

- The provenance carrier remains passive. No prepared/prealloc consumer gates
  on the new facts in this slice.
- Complete extents are recorded only where an existing authoritative layout was
  already selected; intrinsic pointer sources, fallback pointer-value local-slot
  paths, and AArch64 intrinsic operands keep unknown extent.
- This slice still does not prove requested ranges, dynamic array bounds,
  layout authority, negative-offset rejection beyond existing checks, or
  overflow rejection beyond existing construction behavior; `range_verdict`
  remains neutral `UnknownCompatible`.
- No remaining owned Step 3 `PointerValue` `MemoryAddress` producer is known to
  emit blank provenance after this packet. Any further gaps should be treated as
  outside the current owned inventory or Step 4+ proof/consumer work.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepared_lookup_helper_test backend_riscv_prepared_edge_publication_test backend_prepared_printer_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepared_lookup_helper|backend_riscv_prepared_edge_publication|backend_prepared_printer|backend_aarch64_instruction_dispatch)$' > test_after.log`

CTest result: 5/5 passed.
