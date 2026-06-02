# Current Packet

Status: Active
Source Idea Path: ideas/open/90_aarch64_aggregate_lane_helper_table_contraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Aggregate-Lane Helper And Table Surfaces

## Just Finished

Completed Step 1, "Inventory Aggregate-Lane Helper And Table Surfaces", as an
inventory-only packet. Evidence came from the delegated read-only files plus
AST-backed signature discovery for `instruction.cpp`, `calls.cpp`, and
`machine_printer.cpp`.

Helper/table inventory:

- `kAggregateRegisterLaneLoadMnemonics` +
  `aggregate_register_lane_load_mnemonic(width_bytes)`:
  load mnemonic selection for register-lane chunks, currently `{1: ldrb,
  2: ldrh, 4: ldr, 8: ldr}`. Consumers are
  `aggregate_register_lane_printable_chunk(...)` and
  `print_aggregate_register_lane_publication_lines(...)`. Classification:
  `local-helper-table`.
- `aggregate_register_lane_printable_chunk(memory, source_offset, remaining)`:
  printable chunk selection over candidate widths `{8, 4, 2, 1}`. It derives
  a candidate chunk with `aggregate_register_lane_memory(...)`, checks
  `aggregate_register_lane_memory_is_printable(...)`, and rejects chunks with
  no load mnemonic. Consumer is
  `print_aggregate_register_lane_publication_lines(...)`. Classification:
  `local-helper-table`.
- `aggregate_register_lane_memory_is_printable(memory, width_bytes)`:
  chunk printability check for pointer-value/register based addresses through
  `memory_address(...)`, and frame-slot addresses through
  `aggregate_frame_slot_direct_offset_is_encodable(...)` plus
  `memory_address(...)`. Consumers are
  `aggregate_register_lane_printable_chunk(...)` and stack-lane inline-asm copy
  construction in `make_byval_register_lane_stack_publication_instruction(...)`.
  Classification: `local-helper-table`, with the stack-lane consumer treated as
  a retained boundary rather than a contraction target.
- `aggregate_register_lane_destination(destination, lane_index)`:
  destination-lane derivation. It first uses
  `destination.occupied_register_references[lane_index]`; otherwise it derives
  contiguous x-register lanes from `destination.reg.index + lane_index` and
  rejects indexes beyond x30. Consumer is
  `print_aggregate_register_lane_publication_lines(...)`. Classification:
  `local-helper-table`.
- `aggregate_register_lane_memory(memory, byte_offset, width_bytes)`:
  aggregate lane memory helper that adjusts byte offset, width, and alignment.
  Consumers are `aggregate_register_lane_printable_chunk(...)`,
  `print_aggregate_register_lane_publication_lines(...)`, and stack-lane
  inline-asm construction in
  `make_byval_register_lane_stack_publication_instruction(...)`.
  Classification: `local-helper-table`, with the stack-lane consumer retained
  out of the first contraction.
- `aggregate_register_lane_load_register(reg, width_bytes)`:
  chooses the printable load destination view, x for 8-byte chunks and w
  otherwise. Consumer is
  `print_aggregate_register_lane_publication_lines(...)`. Classification:
  `local-helper-table`.
- `aggregate_register_lane_scratch(destination)` +
  `same_aggregate_gp_register_index(...)`:
  scratch exclusion against the destination lane and
  `destination.occupied_register_references`, returning an x-view reserved MIR
  scratch. Consumer is
  `print_aggregate_register_lane_publication_lines(...)`. Classification:
  `local-helper-table`.
- `aggregate_frame_slot_direct_offset_is_encodable(memory, load_width_bytes)`:
  private frame-slot immediate-encoding helper used only by
  `aggregate_register_lane_memory_is_printable(...)`. Classification:
  `local-helper-table`.
- `is_aggregate_register_lane_publication(move)`:
  record-shape recognition for before-call byval aggregate register-lane
  publication: selected move reason, prepared memory source, <=16-byte source,
  GPR x-view destination, and call-argument ABI destination. Consumers are
  `call_boundary_move_selection_status(...)` in `calls.cpp` and
  `print_aggregate_register_lane_publication_lines(...)` in
  `machine_printer.cpp`. Classification: `local-helper-table`; keep public
  until both construction and printer consumers are preserved.

Current construction/printer consumers:

- Construction-owned `calls.cpp` surfaces include
  `selected_byval_lane_extent_bytes(...)`,
  `has_byval_register_lane_transport(...)`,
  `make_byval_register_lane_prepared_source(...)`, and the
  `lower_before_call_moves(...)` branches that validate byval lane size,
  source home/register agreement, prepared source byte coverage, destination
  register authority, and frame-slot source size. Classification:
  `construction-owned`.
- `call_boundary_move_selection_status(...)` consumes
  `is_aggregate_register_lane_publication(...)` only to mark already-shaped
  register-lane publications selected for pointer-value or prepared frame-slot
  memory sources. Classification: `construction-owned` consumer of a
  `local-helper-table` recognizer.
- Printer-owned `machine_printer.cpp` surfaces include
  `print_aggregate_register_lane_publication_lines(...)`, which owns the final
  emitted loads, `orr` merge lines, scratch use, byte-shifted lane placement,
  and fail-closed nullopt behavior for missing scratch, destination lane,
  printable source chunk, or chunk load mnemonic. Classification:
  `printer-owned`.
- Stack-lane inline assembly publication in
  `make_byval_register_lane_stack_publication_instruction(...)` consumes
  `aggregate_stack_copy_*` helpers plus
  `aggregate_register_lane_memory(...)` and
  `aggregate_register_lane_memory_is_printable(...)`. Classification:
  `not-this-idea` for stack-lane publication itself; do not fold it into the
  first register-lane contraction.
- Generic call-boundary frame-slot load helpers in `machine_printer.cpp`
  (`call_boundary_load_width_bytes(...)`,
  `call_boundary_frame_slot_direct_offset_is_encodable(...)`,
  `call_boundary_address_scratch_register(...)`,
  `materialize_call_boundary_frame_slot_address_lines(...)`, and
  `print_call_boundary_frame_slot_load_lines(...)`) are adjacent printer-owned
  frame-slot move helpers, not the first aggregate-lane contraction target.

Smallest first contraction boundary:

- Contract the printer-facing aggregate register-lane chunk descriptor inside
  `instruction.cpp`: combine chunk memory derivation, printability, load
  mnemonic, and load-register view selection for
  `print_aggregate_register_lane_publication_lines(...)`.
- Keep `is_aggregate_register_lane_publication(...)` public until the
  construction and printer consumers are deliberately addressed in a later
  public-surface step.
- Do not move byval lane classification, selected-lane extent, prepared-source
  validation, lane/chunk coverage, destination-register authority, or
  diagnostics out of `calls.cpp`.
- Do not move OR assembly, byte-shifted placement, printer diagnostics, or
  scratch failure policy out of `machine_printer.cpp`.
- Do not include stack-lane inline-asm copy helpers in the first contraction.

## Suggested Next

Begin Step 2, "Contract Private Helper/Table Spelling", by adding a private
aggregate register-lane printable chunk descriptor/helper in `instruction.cpp`
for the printer path, then update
`print_aggregate_register_lane_publication_lines(...)` to consume that
descriptor without changing construction-owned `calls.cpp` validation or
printer-owned diagnostics.

## Watchouts

- Preserve assembly output, diagnostics, unsupported-path contracts, scratch
  selection, and chunk-width selection.
- Keep byval lane classification, selected-lane extent, prepared-source
  validation, lane/chunk coverage, and size-limit authority in `calls.cpp`.
- Do not fold stack-lane inline-asm publication or broad call-boundary record
  cleanup into this plan.
- Retain the current public declarations unless Step 3 proves a helper has no
  external construction/printer consumer.
- Focused proof commands proposed for the next code-changing slice:
  `cmake --build build --target backend_aarch64_call_boundary_owner_test backend_aarch64_machine_printer_test && ctest --test-dir build -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_machine_printer)$' --output-on-failure`
- For register-sourced aggregate register-lane publication, the focused
  construction proof is
  `ctest --test-dir build -R '^backend_aarch64_call_boundary_owner$' --output-on-failure`.
- For frame-slot-sourced aggregate register-lane publication and final printer
  emission, the focused proof should include the same owner test plus a
  printer-targeted assertion path under `backend_aarch64_machine_printer` if
  Step 2 changes printer-consumed helper shape.

## Proof

Inventory-only Step 1. Delegated proof command ran and wrote `test_after.log`;
no implementation tests were required because only `todo.md` changed.
