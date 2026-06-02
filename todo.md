# Current Packet

Status: Active
Source Idea Path: ideas/open/87_aarch64_call_boundary_record_printer_surface_audit.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Classify Duplicate Validation And Spelling Surfaces

## Just Finished

Completed Step 3, "Classify Duplicate Validation And Spelling Surfaces", by
classifying the repeated call-boundary, aggregate-lane, instruction-schema, and
printer surfaces from the Step 1 and Step 2 inventories.

### Step 3 Duplicate Surface Classifications

- `lower_before_call_move`, `lower_after_call_move`, before-return moves,
  preservation moves, and value-publication moves in `calls.cpp`:
  `target-local-abi-printer-ownership`. Evidence: these constructors decide
  AArch64 ABI phase, source kind, destination kind, f128 carrier facts,
  prepared-move provenance, scratch/preservation source material, diagnostics,
  and whether a move remains a `CallBoundaryMoveInstructionRecord` or lowers to
  another record family.
- Stack argument/result publication bypasses in `calls.cpp`:
  `target-local-abi-printer-ownership`. Evidence: stack ABI destinations often
  lower directly to memory or f128 transport records, so this is not duplicate
  missing printer coverage; it is the selected target lowering boundary.
- Immediate call-argument synthetic move construction in `calls.cpp`:
  `target-local-abi-printer-ownership`. Evidence: the synthetic
  `call_arg_immediate_to_register` record owns ABI publication of literals and
  is later printed as a concrete call-boundary move.
- `make_call_boundary_move_instruction` in `calls.cpp`:
  `target-local-abi-printer-ownership`. Evidence: it binds the machine record
  family/opcode, operands, defs, uses, surface kind, and selection status for
  AArch64 call-boundary nodes; contracting this without a separate schema idea
  would move selected-node authority rather than remove local duplication.
- `call_boundary_move_selection_status` in `calls.cpp`:
  `target-local-abi-printer-ownership`. Evidence: the repeated source and
  destination checks protect selected AArch64 ABI subsets before printing,
  including register, result, return, value, preservation, immediate,
  frame-slot, aggregate-lane, scalar FPR, and f128 q-register cases.
- `call_boundary_abi_binding_selection_status` and
  `print_call_boundary_abi_binding`: `target-local-abi-printer-ownership`.
  Evidence: binding records are an explicit unsupported sentinel until lowered
  to concrete moves; replacing that with shared spelling would weaken a
  diagnostic/unsupported-path contract.
- `CallBoundaryMoveInstructionRecord` fields in `instruction.hpp`:
  `call-boundary-record-schema-cleanup-candidate`. Evidence: the same single
  record currently carries ordinary call-boundary moves, value/preservation
  moves, immediate publication, f128 facts, prepared frame-slot memory, and
  aggregate register-lane publication. A narrow follow-up could audit whether
  the record should expose a tagged source/destination sub-schema or validated
  accessor helpers, provided assembly and diagnostics stay unchanged.
- `CallBoundaryAbiBindingInstructionRecord` in `instruction.hpp`:
  `missing-evidence`. Evidence: Step 1 found the record and unsupported printer
  sentinel, but did not trace enough repeated validation or spelling to justify
  schema cleanup beyond preserving the sentinel.
- `CallInstructionRecord` and `print_call` direct/indirect call spelling:
  `target-local-abi-printer-ownership`. Evidence: this is the call-site schema
  and printer path, not duplicate call-boundary move spelling.
- `RecordSurfaceKind`, `InstructionFamily`, `MachineOpcode`, spelling helpers,
  and public printer entry points in `instruction.*` and `machine_printer.hpp`:
  `missing-evidence`. Evidence: Step 1 named these as structured surface
  identifiers, but no repeated validation or spelling owner boundary was traced
  for this packet.
- Generic `print_call_boundary_move` validation and assembly spelling in
  `machine_printer.cpp`: `target-local-abi-printer-ownership`. Evidence:
  printer-side checks repeat selected-node facts because they own final
  printable assembly constraints, diagnostics, register/immediate/memory
  presence, materialized-address emission, scalar integer/FP immediate moves,
  f128 q-register moves, scalar FPR moves, and ordinary move spelling.
- Call-boundary scratch/address-materialization printer helpers in
  `machine_printer.cpp`: `target-local-abi-printer-ownership`. Evidence: they
  duplicate some construction-time encodability concerns, but the printer owns
  the final scratch choice and materialized frame-slot address spelling.
- Frame-slot load/address materialization checks split between
  `call_boundary_move_selection_status` and `print_call_boundary_move`:
  `missing-evidence`. Evidence: this is plausible local helper-contraction
  territory, but the Step 1 inventory does not prove a stable no-semantics
  helper boundary that preserves diagnostics and scratch behavior.
- `is_aarch64_byval_register_lane_move`, `selected_byval_lane_extent_bytes`,
  `has_byval_register_lane_transport`, and
  `make_byval_register_lane_prepared_source` in `calls.cpp`:
  `aggregate-lane-record-surface`. Evidence: these own AArch64 byval lane
  classification, selected-lane extent, prepared-source validation, lane/chunk
  coverage, size limits, and source frame-slot construction before any printer
  record is emitted.
- Register-sourced and frame-slot-sourced byval lane publication in
  `lower_before_call_move`: `aggregate-lane-record-surface`. Evidence: these
  paths reuse `CallBoundaryMoveInstructionRecord` but rewrite the move reason,
  reset the source register, publish prepared source memory, and create X-view
  ABI GPR destinations for aggregate lane records.
- Indirect byval source-selection reuse of
  `make_byval_register_lane_prepared_source`: `aggregate-lane-record-surface`.
  Evidence: the repeated helper use is tied to aggregate selected-source
  semantics and diverges between small selected lanes and address
  materialization.
- Stack-lane byval publication and
  `make_byval_register_lane_stack_publication_instruction` in `calls.cpp`:
  `aggregate-lane-record-surface`. Evidence: this path shares chunk helpers but
  emits inline asm during call lowering rather than a machine-printer
  `CallBoundaryMoveInstructionRecord`; collapsing it into register-lane printer
  cleanup would cross owner boundaries.
- Aggregate helper surface in `instruction.cpp`:
  `local-table-or-helper-contraction-candidate`. Evidence: load mnemonics,
  printable chunk selection, chunk printability, destination-lane derivation,
  aggregate lane memory, and scratch exclusion are local helper/table-shaped
  surfaces already shared by construction and printing. A narrow follow-up
  could contract helper/table spelling if it preserves all byval lane
  diagnostics, scratch choices, and assembly output.
- `is_aggregate_register_lane_publication` in `instruction.cpp`:
  `call-boundary-record-schema-cleanup-candidate`. Evidence: the predicate is
  the schema-level gate for aggregate-lane printer dispatch and selection
  status, and it revalidates phase, move reason, prepared non-address source
  memory, size range, and X-view ABI GPR destination on the generic
  `CallBoundaryMoveInstructionRecord`. A narrow schema/accessor cleanup could
  make this aggregate lane record shape explicit without changing behavior.
- `print_call_boundary_move` aggregate-lane dispatch in
  `machine_printer.cpp`: `aggregate-lane-record-surface`. Evidence: aggregate
  records deliberately bypass the ordinary frame-slot branch once the aggregate
  predicate succeeds, so this is distinct aggregate print routing rather than
  duplicate generic call-boundary validation.
- `print_aggregate_register_lane_publication_lines` in
  `machine_printer.cpp`: `aggregate-lane-record-surface`. Evidence: it owns
  final register-lane assembly spelling, scratch selection, chunk-width load
  choice, lane splitting, scratch OR assembly, and byte-shifted placement.
- Aggregate printer rejection checks for missing scratch, missing destination
  lane, missing printable source chunk, or missing chunk load mnemonic:
  `target-local-abi-printer-ownership`. Evidence: these are final printer-owned
  diagnostics for printable assembly constraints and should not be deleted just
  because construction and schema predicates also validate related facts.
- Shared use of `CallBoundaryMoveInstructionRecord` by ordinary
  call-boundary and aggregate-lane publication:
  `call-boundary-record-schema-cleanup-candidate`. Evidence: Step 2 confirms
  aggregate register-lane publication has no dedicated record struct and relies
  on generic source-memory/destination-register/provenance fields; that is the
  concrete schema-cleanup boundary if a follow-up is created.

## Suggested Next

Execute Step 4, "Create Follow-Up Ideas Only For Concrete Candidates", by
creating narrow ideas only for the local aggregate helper/table contraction
candidate and the call-boundary/aggregate-lane record schema cleanup
candidate, or by documenting why either candidate is not concrete enough.

## Watchouts

- This is audit-only; do not edit implementation files while executing the
  current plan.
- Do not claim printer cleanup by deleting validation or moving ABI-specific
  call-boundary construction into shared authority.
- Preserve printer-owned diagnostics and scratch decisions even for surfaces
  classified as cleanup candidates.
- Do not turn `missing-evidence` areas into Step 4 implementation ideas.
- The concrete candidates are narrow: aggregate helper/table contraction and
  generic call-boundary record schema/accessor cleanup for aggregate-lane shape.
- Stack-lane inline-asm publication remains aggregate-specific construction
  ownership and should not be folded into register-lane printer cleanup.
- Any Step 4 idea needs a focused proof route for unchanged assembly output and
  unchanged unsupported/diagnostic behavior.

## Proof

Audit-only proof command:

```sh
printf 'Audit-only Step 3; no backend tests required.\n' > test_after.log && git diff --name-only >> test_after.log && if git diff --name-only | rg -q '^src/backend/mir/aarch64/codegen/(calls|instruction|machine_printer)\.(cpp|hpp)$|^plan\.md$|^ideas/'; then printf 'ERROR: non-todo file changed during audit-only classification packet.\n' >> test_after.log; exit 1; fi
```

Result: passed; `test_after.log` contains the audit-only Step 3 marker and
dirty file list.
