# Current Packet

Status: Active
Source Idea Path: ideas/open/87_aarch64_call_boundary_record_printer_surface_audit.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Inventory Aggregate-Lane Publication Surfaces

## Just Finished

Completed Step 2, "Inventory Aggregate-Lane Publication Surfaces", by tracing
byval aggregate register-lane publication from prepared aggregate transport
through call-boundary move records and printer output.

### Step 2 Aggregate-Lane Publication Inventory

- `src/backend/mir/aarch64/codegen/calls.cpp`:
  `is_aarch64_byval_register_lane_move` is the construction-side predicate for
  prepared moves whose destination is a call-argument ABI register or stack slot,
  whose op is `Move`, and whose reason is
  `call_arg_byval_aggregate_register_lanes`; this is AArch64 call-argument ABI
  classification, not a generic instruction-schema rule.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  `selected_byval_lane_extent_bytes` and `has_byval_register_lane_transport`
  recognize `PreparedAggregateTransportKind::ByvalRegisterLanes` and carry the
  payload size used by both register-lane and stack-lane publication paths.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  `make_byval_register_lane_prepared_source` validates that the prepared
  aggregate transport has complete required payload chunks and lane coverage,
  a single prepared source slot/base offset, size in the supported 1-16 byte
  range, and matching lane/chunk offsets; it returns a prepared `MemoryOperand`
  over the source frame slot.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  register-sourced byval lane publication in `lower_before_call_move` builds a
  normal `CallBoundaryMoveInstructionRecord`, resets any source register,
  stores the prepared source bytes in `source_memory`, creates an ABI GPR
  destination with X view, and rewrites the record reason to
  `call_arg_byval_aggregate_register_lanes`.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  frame-slot-sourced byval lane publication follows the same
  `CallBoundaryMoveInstructionRecord` path for ABI register destinations, but
  adds diagnostics for missing complete prepared selected source bytes, non-small
  source size, or missing destination register authority.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  indirect byval source-selection handling can reuse
  `make_byval_register_lane_prepared_source` inside the broader frame-slot
  call-argument path; for small selected byval lanes it behaves like a prepared
  source-value load, while larger selected lanes use address materialization.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  stack-lane byval publication is distinct from the register-lane record path:
  the stack-slot destination branch calls
  `make_byval_register_lane_stack_publication_instruction`, which emits an
  inline-asm `InstructionFamily::CallBoundary` node after validating printable
  source and destination chunks, instead of publishing a
  `CallBoundaryMoveInstructionRecord`.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  `make_byval_register_lane_stack_publication_instruction` shares aggregate
  chunk helpers with the register-lane printer, but owns construction-time
  stack-copy spelling through inline asm and is not a machine-printer branch.
- `src/backend/mir/aarch64/codegen/calls.cpp`:
  `call_boundary_move_selection_status` treats aggregate register-lane
  publication as a selected subset of call-boundary moves after generic
  provenance/source/destination checks; it accepts pointer-value memory with a
  base register and prepared frame-slot memory with prepared snapshot offsets.
- `src/backend/mir/aarch64/codegen/instruction.hpp`:
  aggregate register-lane publication has no dedicated record struct; it uses
  `CallBoundaryMoveInstructionRecord` fields: `phase`, `move`, `source_memory`,
  `source_memory_materializes_address`, `destination_register`, source
  bundle/move provenance, and normal call-boundary provenance fields.
- `src/backend/mir/aarch64/codegen/instruction.cpp`:
  aggregate-lane reusable helpers are part of the instruction/codegen helper
  surface: load mnemonics for 1/2/4/8-byte chunks, scratch selection,
  `aggregate_register_lane_memory`, printability checks, printable chunk
  selection, destination-lane register derivation, and the shared
  `is_aggregate_register_lane_publication` predicate.
- `src/backend/mir/aarch64/codegen/instruction.cpp`:
  `is_aggregate_register_lane_publication` is the schema-level gate for the
  printer and selection status; it requires `BeforeCall`, call-argument ABI
  register destination, move reason `call_arg_byval_aggregate_register_lanes`,
  prepared non-address source memory of size 1-16 bytes, and an X-view ABI GPR
  destination.
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`:
  `print_call_boundary_move` dispatches aggregate register-lane records before
  ordinary frame-slot call-boundary loads; a successful aggregate-lane print
  therefore bypasses the generic frame-slot-only validation branch.
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`:
  `print_aggregate_register_lane_publication_lines` owns final assembly
  spelling for register-lane publication. It chooses a non-conflicting scratch
  register, splits the prepared source into at most two 8-byte destination
  lanes, selects printable chunk widths, loads first chunks directly into the
  lane register, loads later chunks into scratch, and ORs them into the lane
  register with byte-shifted placement.
- `src/backend/mir/aarch64/codegen/machine_printer.cpp`:
  printer-side validation is aggregate-specific: it rejects records that fail
  `is_aggregate_register_lane_publication`, lack a scratch, lack a destination
  lane register, have no printable source chunk, or have no load mnemonic for
  the selected chunk width.
- Shared call-boundary ownership:
  aggregate register-lane publication shares `CallBoundaryMoveInstructionRecord`
  provenance, selected-node family/opcode, operand/def/use construction,
  source-memory and destination-register fields, `print_call_boundary_move`
  dispatch, and the general rule that prepared call-argument ABI publication is
  target-local AArch64 call-boundary lowering.
- Distinct aggregate-lane ownership:
  byval lane completeness checks, lane/chunk coverage, 1-16 byte size limits,
  destination-lane expansion, scratch exclusion, chunk-width load mnemonics, and
  OR-based sublane assembly are aggregate-specific behavior. Stack-lane
  publication is even more distinct because it is lowered to inline asm during
  call lowering rather than printed from a call-boundary move record.

## Suggested Next

Execute Step 3, "Classify Duplicate Validation And Spelling Surfaces", using
the Step 1 and Step 2 inventories to classify shared call-boundary validation,
aggregate-lane helpers, stack-lane inline-asm spelling, and printer checks as
target-local ownership, concrete contraction candidates, schema cleanup
candidates, or missing evidence.

## Watchouts

- This is audit-only; do not edit implementation files while executing the
  current plan.
- Do not claim printer cleanup by deleting validation or moving ABI-specific
  call-boundary construction into shared authority.
- Treat unclear duplication as `missing-evidence` until the record path and
  owner boundary are traced.
- Step 1 found that stack argument/result publications often bypass
  `CallBoundaryMoveInstructionRecord` and lower directly to memory or transport
  records; follow-up classification should not describe those as missing
  call-boundary printer coverage.
- Aggregate-lane publication is already visible as a special predicate and
  printer branch on `CallBoundaryMoveInstructionRecord`; Step 2 should decide
  whether that is shared schema ownership or an aggregate-specific surface.
- Do not collapse stack-lane publication into the register-lane printer
  inventory: stack-lane publication is constructed as inline asm in
  `calls.cpp` and has a different owner boundary.
- The same aggregate helper names are used by construction and printing; Step 3
  should classify the owner boundary by record path, not by helper-name
  similarity.

## Proof

Audit-only proof command:

```sh
printf 'Audit-only Step 2; no backend tests required.\n' > test_after.log && git diff --name-only >> test_after.log && if git diff --name-only | rg -q '^src/backend/mir/aarch64/codegen/(calls|instruction|machine_printer)\.(cpp|hpp)$|^plan\.md$|^ideas/'; then printf 'ERROR: non-todo file changed during audit-only aggregate-lane inventory packet.\n' >> test_after.log; exit 1; fi
```

Result: passed; `test_after.log` contains the audit-only Step 2 marker and
dirty file list.
