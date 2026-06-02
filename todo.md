Status: Active
Source Idea Path: ideas/open/91_aarch64_call_boundary_aggregate_lane_record_schema_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Current Call-Boundary Record Shapes

# Current Packet

## Just Finished

Step 1 mapped the current AArch64 call-boundary record shapes without changing
implementation code.

Selected Step 2 route: keep `CallBoundaryMoveInstructionRecord` as the owning
machine-node record and add a typed aggregate-register-lane accessor/schema view
around the existing record fields. Do not move ABI construction, prepared-source
selection, register/scratch choice, or preservation routing out of `calls.cpp`.
The accessor should replace duplicated aggregate-lane shape checks at the
instruction/printer boundary while preserving the current public move record and
ordinary call-boundary shapes.

Current aggregate register-lane publication shape:

- Record owner: `CallBoundaryMoveInstructionRecord` with
  `MachineOpcode::CallBoundaryMove`, `InstructionFamily::CallBoundary`, and
  `RecordSurfaceKind::MachineInstructionNode`.
- Predicate: `is_aggregate_register_lane_publication(move)`.
- Required move facts: `phase == BeforeCall`, destination kind
  `CallArgumentAbi`, destination storage `Register`, op kind `Move`, and reason
  `"call_arg_byval_aggregate_register_lanes"`.
- Required source facts: `source_memory` is present, prepared, non-address
  materialization, `size_bytes` in `1..16`, and source memory is later accepted
  only for pointer-value/register-backed printable memory or prepared frame-slot
  snapshots with `frame_slot_id`, `byte_offset_is_prepared_snapshot`, and
  `can_use_base_plus_offset`.
- Required destination facts: `destination_register` is present, GPR-backed,
  `expected_view == X`, and names the ABI lane base; occupied registers preserve
  the explicit contiguous ABI lane list when available.
- Lane/printer helpers consume only this view: scratch excludes destination and
  occupied lane registers; lane destination uses
  `occupied_register_references[lane_index]` first and otherwise contiguous
  `destination.reg.index + lane_index`; each lane is up to 8 bytes and chunked
  as printable `8/4/2/1` loads, OR-ing later chunks into the lane register.

Ordinary call-boundary shapes that must remain unchanged:

- Register argument/result/return/value moves remain ordinary
  `CallBoundaryMoveInstructionRecord` records with `source_register` plus
  `destination_register`; GPR->GPR, scalar FPR->same-view FPR, and structured
  f128 q-register moves are selected by `call_boundary_move_selection_status`.
- Immediate argument/value publication remains `source_immediate` plus
  `destination_register`, including integer GPR materialization and scalar FP
  immediate materialization through a scratch GPR.
- Prepared frame-slot loads remain `source_memory` plus `destination_register`
  with `source_memory_materializes_address == false`, and use
  `print_call_boundary_frame_slot_load_lines`.
- Prepared frame-slot address materialization remains `source_memory` plus
  `destination_register` with `source_memory_materializes_address == true`, and
  prints address materialization rather than a load.
- Preservation home population/republication and block-entry/before-instruction
  value moves continue to reuse the same move record with their current
  `phase`, `authority_kind`, source/destination roles, `source_bundle`, and
  `source_move` provenance.
- `CallBoundaryAbiBindingInstructionRecord` remains a separate ABI-binding
  record with `source_bundle` and `source_binding`; it is still selected but not
  printable as assembly.

Rejection paths to keep observable:

- Instruction selection still reports missing prepared move provenance when
  `source_bundle`, all source forms, or `function_name` are missing.
- Stack argument moves still defer with `"call-boundary stack argument move
  requires AArch64 stack-copy lowering"`.
- Non-selected move families still defer with `"call-boundary move node is
  outside the selected register call-boundary move subset"`.
- Missing source/destination still defers with `"call-boundary move node
  requires prepared register source and destination"`, except for the existing
  f128 constant argument exception.
- Aggregate lane setup in `calls.cpp` must continue diagnosing bad byval sizes,
  source register/home disagreement, missing prepared selected source bytes,
  missing destination registers, and non-small prepared frame-slot sources.
- Printer validation must remain even after the accessor: missing provenance,
  missing source/destination, non-prepared frame-slot memory, unprintable
  frame-slot address, unprintable aggregate chunks, missing scratch, and
  unprintable f128 q-register moves must still return unsupported diagnostics.

Construction touchpoints for Step 2:

- `instruction.hpp`: add the accessor/schema type and declarations next to
  `AggregateRegisterLanePrintableChunk` and aggregate lane helper declarations,
  keeping `CallBoundaryMoveInstructionRecord` fields intact.
- `instruction.cpp`: make `is_aggregate_register_lane_publication` delegate to
  the accessor and expose any selected fields through the accessor rather than
  duplicating raw predicate checks.
- `machine_printer.cpp`: have
  `print_aggregate_register_lane_publication_lines` consume the accessor view
  while preserving its current unsupported returns.
- `calls.cpp`: continue constructing ordinary and aggregate-lane moves in the
  existing lowering branches; only use the accessor for validation/shape
  publication after construction, not for taking ownership of ABI selection.

## Suggested Next

Step 2 should implement the typed aggregate-register-lane accessor/schema view
around `CallBoundaryMoveInstructionRecord`, wire the predicate/printer through
it, and leave ordinary move construction and ABI authority in `calls.cpp`
unchanged.

## Watchouts

- Keep this idea behavior-preserving.
- Do not move ABI construction, prepared-source authority, source/destination
  selection, scratch decisions, or preservation logic out of `calls.cpp`.
- Do not delete printer-side validation because instruction-record helpers
  validate an aggregate-lane view earlier.
- Avoid adding a new aggregate-lane instruction payload unless the supervisor
  explicitly changes route; that would widen ownership beyond the selected
  accessor/schema cleanup.
- Preserve `source_memory_materializes_address` semantics: aggregate register
  lanes require it to be false, while local aggregate address publication and
  ordinary frame-address materialization depend on true.
- Use matching `test_before.log` and `test_after.log` once the accepted slice
  changes public instruction declarations, record schema, helper predicates, or
  printer dispatch contracts.

## Proof

Mapping-only packet. No build was required and no `test_after.log` was produced.
Suggested Step 2 proof command:

```sh
cmake --build build --target backend_aarch64_call_boundary_owner_test backend_prepare_frame_stack_call_contract_test && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_call_boundary_owner|backend_prepare_frame_stack_call_contract|backend_codegen_route_aarch64_local_aggregate_address_pointer_copy_publishes_frame_address)$' | tee test_after.log
```
