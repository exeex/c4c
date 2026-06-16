Status: Active
Source Idea Path: ideas/open/289_structured_opaque_pointer_byte_range_provenance.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove Requested Range and Dynamic Array Bounds

# Current Packet

## Just Finished

Step 4: Prove Requested Range and Dynamic Array Bounds finished the passive
dynamic-array bound verdict subset using existing dynamic-array and requested
range facts.

- Added a carrier-level dynamic-array finalizer that resets
  `MemoryDynamicArrayFacts::verdict` to `Unknown` and only sets
  `BoundedByElementCount` when available element count, nonzero stride, checked
  base offset/envelope arithmetic, and a non-overflowed requested byte range
  prove full containment.
- Valid requested ranges outside the checked dynamic-array envelope are marked
  passive `Unbounded`; missing dynamic facts, zero count/stride, overflowing
  requested ranges, and overflowing dynamic-array envelopes remain `Unknown`.
- Existing route admission, target policy, prepared/prealloc consumers, address
  usability fields, and compatibility behavior are unchanged.
- Focused helper coverage now checks bounded dynamic accesses, before-base and
  after-element unbounded ranges, unavailable dynamic facts, zero count/stride,
  overflowing requested ranges, and overflowing dynamic-array envelopes.

## Suggested Next

Start Step 5 by publishing provenance verdicts to prepared consumers that need
to distinguish syntactic base-plus-offset usability from proven object-range
provenance, while preserving the compatibility bridge until each route has a
structured proof or explicit unknown/fail-closed verdict.

## Watchouts

- `range_verdict` is still passive metadata; no current consumer gates route
  admission or prepared/prealloc behavior on it.
- `dynamic_array.verdict` is also passive metadata; prepared/prealloc consumers
  were intentionally left unchanged in this slice.
- Dynamic-array `Unbounded` is produced only when the requested byte range and
  checked envelope are concrete enough to compare; malformed, zero, missing, or
  overflowing facts remain `Unknown`.
- Layout-authority-specific proof and compatibility bridge removal remain later
  work.

## Proof

Supervisor-selected proof passed and wrote `test_after.log`:

`cmake --build --preset default --target backend_lir_to_bir_notes_test backend_prepared_lookup_helper_test backend_riscv_prepared_edge_publication_test backend_prepared_printer_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|backend_prepared_lookup_helper|backend_riscv_prepared_edge_publication|backend_prepared_printer|backend_aarch64_instruction_dispatch)$' > test_after.log`

CTest result: 5/5 passed.
