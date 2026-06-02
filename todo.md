# Current Packet

Status: Active
Source Idea Path: ideas/open/90_aarch64_aggregate_lane_helper_table_contraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Contract Private Helper/Table Spelling

## Just Finished

Completed Step 2, "Contract Private Helper/Table Spelling", as a
behavior-preserving code packet aligned to the `instruction.cpp` helper/table
surface. `instruction.cpp` now owns the
`aggregate_register_lane_printable_chunk_descriptor(...)` helper, which
contracts printable chunk width selection, chunk memory derivation, load
mnemonic selection, and load-register view selection into one aggregate
register-lane descriptor.

Because `machine_printer.cpp` is the external consumer that emits the final
assembly, the descriptor type and helper require a narrow declaration in
`instruction.hpp`; keeping that declaration avoids the hidden cross-translation
unit API mismatch that caused the previous route problem. Printer-owned
behavior remains unchanged: missing scratch, destination lane, or printable
chunk still returns `std::nullopt`, and OR assembly, byte-shifted lane
placement, emitted line shape, and scratch use are still owned by
`machine_printer.cpp`.

Unchanged consumers and boundaries:

- `calls.cpp` construction-owned validation and stack-lane inline-asm
  construction consumers were not edited.
- `aggregate_register_lane_memory(...)` and
  `aggregate_register_lane_memory_is_printable(...)` still serve the stack-lane
  construction path.
- `aggregate_register_lane_printable_chunk(...)` remains public as the
  compatibility width-only helper until Step 3 decides whether the public
  surface can be narrowed further.
- `calls.cpp` remains untouched.

## Suggested Next

Begin Step 3, "Tighten Public Aggregate-Lane Helper Surface If Justified", by
reviewing whether the now-compatibility-only aggregate register-lane width,
memory, mnemonic, and load-register declarations in `instruction.hpp` can be
narrowed around the descriptor without disrupting `calls.cpp` construction
consumers, stack-lane construction, or `machine_printer.cpp` printer consumers.

## Watchouts

- Preserve assembly output, diagnostics, unsupported-path contracts, scratch
  selection, and chunk-width selection.
- Keep byval lane classification, selected-lane extent, prepared-source
  validation, lane/chunk coverage, and size-limit authority in `calls.cpp`.
- Do not fold stack-lane inline-asm publication or broad call-boundary record
  cleanup into this plan.
- The descriptor helper is intentionally declared in `instruction.hpp` because
  `machine_printer.cpp` consumes it across a translation-unit boundary; Step 3
  should decide whether any older aggregate register-lane helper declarations
  can then be narrowed.
- Retain public declarations unless Step 3 proves a helper has no external
  construction/printer or stack-lane construction consumer.

## Proof

Delegated proof command passed and wrote `test_after.log`:

`cmake --build build --target backend_aarch64_call_boundary_owner_test backend_aarch64_machine_printer_test && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_machine_printer)$' --output-on-failure > test_after.log`

Focused subset: `backend_aarch64_call_boundary_owner` and
`backend_aarch64_machine_printer`, both passed.
