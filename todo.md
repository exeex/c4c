# Current Packet

Status: Active
Source Idea Path: ideas/open/90_aarch64_aggregate_lane_helper_table_contraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Tighten Public Aggregate-Lane Helper Surface If Justified

## Just Finished

Completed Step 3, "Tighten Public Aggregate-Lane Helper Surface If
Justified", as a behavior-preserving public-surface packet. `rg` confirmed
that `aggregate_register_lane_load_mnemonic(...)`,
`aggregate_register_lane_load_register(...)`, and
`aggregate_register_lane_printable_chunk(...)` have no current consumers
outside `instruction.cpp`, so their declarations were removed from
`instruction.hpp` and their definitions now have internal linkage.

Declarations intentionally kept public with current consumer reasons:

- `AggregateRegisterLanePrintableChunk` and
  `aggregate_register_lane_printable_chunk_descriptor(...)` remain public
  because `machine_printer.cpp` consumes the descriptor across a translation
  unit boundary while emitting aggregate register-lane publication.
- `aggregate_register_lane_scratch(...)`,
  `aggregate_register_lane_destination(...)`, and
  `is_aggregate_register_lane_publication(...)` remain public for the
  `machine_printer.cpp` aggregate register-lane path.
- `aggregate_register_lane_memory(...)` and
  `aggregate_register_lane_memory_is_printable(...)` remain public because
  `calls.cpp` still uses them to construct and validate stack-lane inline-asm
  publication chunks.

No record-shape logic, printer rejection checks, stack-lane inline-asm
publication, or broad call-boundary record handling was changed.

## Suggested Next

Begin Step 4, "Prove Equivalence And Prepare Closure Evidence", by comparing
or summarizing behavior evidence for register-sourced and frame-slot-sourced
byval lane publication, including chunk-width load selection, destination-lane
derivation, scratch exclusion, and printer rejection paths.

## Watchouts

- Preserve assembly output, diagnostics, unsupported-path contracts, scratch
  selection, and chunk-width selection.
- Keep byval lane classification, selected-lane extent, prepared-source
  validation, lane/chunk coverage, and size-limit authority in `calls.cpp`.
- Do not fold stack-lane inline-asm publication or broad call-boundary record
  cleanup into this plan.
- The descriptor helper is intentionally declared in `instruction.hpp` because
  `machine_printer.cpp` consumes it across a translation-unit boundary.
- The only public aggregate register-lane helpers narrowed in Step 3 were the
  three helpers proven instruction-local by `rg`; avoid narrowing the
  memory/printability helpers unless `calls.cpp` stops consuming them.

## Proof

Delegated proof command passed and wrote `test_after.log`:

`cmake --build build --target backend_aarch64_call_boundary_owner_test backend_aarch64_machine_printer_test && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_machine_printer)$' --output-on-failure > test_after.log`

Focused subset: `backend_aarch64_call_boundary_owner` and
`backend_aarch64_machine_printer`, both passed.
