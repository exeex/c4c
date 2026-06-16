Status: Active
Source Idea Path: ideas/open/289_structured_opaque_pointer_byte_range_provenance.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove Requested Range and Dynamic Array Bounds

# Current Packet

## Just Finished

Step 4: Prove Requested Range and Dynamic Array Bounds started the passive
requested-range proof route for already-known complete extents.

- Added a carrier-level requested-range finalizer that keeps
  `UnknownCompatible` unless `MemoryByteRange` is available, non-overflowed,
  has an available end, and a known complete object extent can prove the full
  range.
- Known complete extents now passively mark contained requested ranges
  `ProvenInBounds` and negative, overflowing, or trailing requested ranges
  `ProvenOutOfBounds`.
- Unknown or incomplete extents remain `UnknownCompatible`.
- Existing route admission, target policy, prepared/prealloc consumers, address
  usability fields, and compatibility behavior are unchanged.
- Focused helper coverage now checks in-bounds, unknown-extent, negative,
  overflowing, and trailing requested-range verdicts.

## Suggested Next

Finish the Step 4 dynamic-array bound subset by computing passive
`MemoryDynamicArrayFacts::verdict` when element count, stride, base offset, and
requested range prove the dynamic access stays inside the array; keep unproven
dynamic ranges unknown and avoid changing prepared/prealloc consumers.

## Watchouts

- `range_verdict` is still passive metadata; no current consumer gates route
  admission or prepared/prealloc behavior on it.
- Out-of-bounds verdicts are recorded only when a known complete extent exists;
  unknown extents intentionally remain `UnknownCompatible`.
- Dynamic-array facts are preserved but their bound verdict is not computed in
  this slice.
- Layout-authority-specific proof and compatibility bridge removal remain later
  work.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepared_lookup_helper_test backend_riscv_prepared_edge_publication_test backend_prepared_printer_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepared_lookup_helper|backend_riscv_prepared_edge_publication|backend_prepared_printer|backend_aarch64_instruction_dispatch)$' > test_after.log`

CTest result: 5/5 passed.
