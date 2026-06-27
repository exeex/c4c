# Current Packet

Status: Active
Source Idea Path: ideas/open/414_typed_prepared_call_argument_contracts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Existing Call-Argument Routes

## Just Finished

Completed Step 1 inventory for existing `PreparedCallArgumentSourceSelection`
routes and selected the first typed route to migrate.

Existing source-selection bag:

- `None`: absence marker only; `prepared_call_argument_source_selection_available`
  and `find_prepared_call_argument_publication_source_routing` treat this as no
  selected route.
- `FrameSlotAddress`: selected when a frame-slot source is passed as an address.
  Producers currently fill it from sret memory-return facts, latest frame-slot
  address materializations, and the no-addressing local-frame-address
  compatibility lookup. Route fields currently used are `source_value_id`,
  `source_value_name`, `source_home_kind`, `source_slot_id`,
  `source_stack_offset_bytes`, `source_size_bytes`, `source_align_bytes`,
  `address_materialization_block_label`,
  `address_materialization_inst_index`,
  `address_materialization_frame_slot_id`, and
  `address_materialization_byte_offset`.
- `FrameSlotValue`: selected when a frame-slot source value is passed by value.
  Producers copy value-home fields and may refine slot/offset/size/align from a
  prepared memory access. Consumers expect a stack-slot home plus selected
  frame-slot source fields.
- `LocalFrameAddressMaterialization`: selected for local aggregate address and
  byval pointer address materialization from register or computed-address source
  encodings. Producers require a latest frame-slot materialization, selected
  source/base value identity, pointer byte delta, materialization block/instr,
  frame-slot id, byte offset, size, align, and a nonnegative stack offset.
- `PriorPreservation`: selected from
  `find_unique_indexed_prior_preserved_value_source`. Producers copy preserved
  value identity, prior call block/instruction, route, register route fields,
  stack-slot route fields, callee-save save index, and spill-slot placement.
  Callee-saved-register preservation already requires register name/bank,
  contiguous width, occupied-register names, and placement.
- `ByvalRegisterLane`: selected for small pointer `byval_copy` arguments passed
  in GPR lanes. Producers record `byval_lane_extent_bytes`,
  `byval_lane_source_instruction_index`, source slot/offset/size/align from
  contiguous prepared payload stores when available, and aggregate transport
  facts through `plan_prepared_aggregate_transport`.

Typed route candidates:

- `FrameSlotAddressSourceRoute`: first migration target. It should split sret
  frame-slot addresses from materialized local frame-slot addresses only if the
  typed payload needs separate constructors, but both require complete
  frame-slot address authority before target lowering.
- `FrameSlotValueSourceRoute`: typed source value in stack storage, with
  source value/home identity and complete slot/offset/extent/alignment facts.
- `LocalFrameAddressMaterializationRoute`: typed local aggregate/byval pointer
  address route with source/base value identity, pointer delta, materialization
  source location, frame-slot id, selected byte offset, extent, and alignment.
- `PriorPreservationSourceRoute`: typed prior preservation route, likely with
  variants for callee-saved register and stack-slot preservation.
- `ByvalRegisterLaneSourceRoute`: typed small byval-lane route, tied to
  aggregate transport chunks/lanes and selected payload bytes.

Compatibility accessors needed for staged migration:

- Keep `prepared_call_argument_source_selection_available` and
  `find_prepared_call_argument_publication_source_routing` as compatibility
  boundaries while route payloads migrate.
- Add typed query/bridge helpers rather than extending the optional-field bag:
  `as_frame_slot_address_source_route`,
  `as_frame_slot_value_source_route`,
  `as_local_frame_address_materialization_route`,
  `as_prior_preservation_source_route`, and
  `as_byval_register_lane_source_route`.
- Preserve `find_prepared_missing_frame_slot_call_argument_publication_need` as
  the AArch64 publication bridge, but have it consume typed route presence and
  distinguish optional non-applicable arguments from required-but-missing route
  facts.
- Preserve target entry helpers such as AArch64
  `make_selected_call_argument_source` and RV64
  `verified_prepared_selected_frame_slot_address_offset` as consumers, then
  move their required-field checks behind typed accessors.

Selected first typed route:

- Migrate `FrameSlotAddress` first. It directly hits the Idea 414 recovery rows,
  is narrower than byval lanes and prior preservation, and has clear producer
  facts plus RV64/AArch64 consumer entry points. Required producer-owned facts
  are: selected source value identity when applicable, frame-slot id,
  selected stack byte offset, source extent and alignment, and for materialized
  address cases the materialization block label, instruction index,
  materialization frame-slot id, and materialization byte offset. Contradictory
  `source_slot_id` versus `address_materialization_frame_slot_id`, mismatched
  plan/source value identity, missing extent/alignment, or missing byte-range
  authority should classify as producer-owned missing/incoherent facts before
  target fallback.

Current producer entry points:

- `src/backend/prealloc/call_plans.cpp`: `select_prepared_call_argument_source`,
  `copy_home_source_selection_fields`,
  `copy_access_source_selection_fields`,
  `copy_materialization_source_selection_fields`,
  `find_latest_frame_slot_materialization`, and
  `find_no_addressing_local_frame_address_source_compatibility`.
- `src/backend/prealloc/calls.hpp`: `PreparedCallArgumentSourceSelection`,
  `find_prepared_call_argument_publication_source_routing`, and
  `find_prepared_missing_frame_slot_call_argument_publication_need`.

Current consumer entry points:

- RV64: `src/backend/mir/riscv/codegen/prepared_call_emit.cpp`
  `verified_prepared_selected_frame_slot_address_offset`,
  `emit_riscv_frame_slot_address_argument`, and prepared call lowering around
  source-selection checks.
- AArch64: `src/backend/mir/aarch64/codegen/calls.cpp`
  `StackFrameSlotCallOperandOwner::selected_frame_slot_source`,
  `StackFrameSlotCallOperandOwner::sret_memory_return_address_source`,
  `make_selected_call_argument_source`, and
  `materialize_missing_frame_slot_call_arguments`.

Rows for Step 2 planning:

- Taxonomy rows: `TAX-FAM-CALL-ARG-TYPED-ROUTES-PLACEHOLDER-001` first,
  with `TAX-FAM-CALL-BOUNDARY-ARG-RESULT-001` for call-plan/move/binding
  context, plus call-route portions of
  `TAX-FAM-PUBLICATION-FACTS-PLACEHOLDER-001` and storage/home portions of
  `TAX-FAM-VALUE-HOME-TYPED-STORAGE-001`.
- Audit rows: start with
  `418-AUD-RV64-FRAME-SLOT-CALL-RECOVERY-001` and
  `418-AUD-A64-FRAME-SLOT-PUBLICATION-REVIEW-001`; use
  `418-AUD-A64-STACK-PRESERVE-RECOVERY-001` as a watchout for later prior
  preservation/publication work, and reference
  `418-AUD-RV64-CALL-COHERENT-001`,
  `418-AUD-RV64-BYVAL-COHERENT-001`, and
  `418-AUD-A64-CALL-COHERENT-001` as coherent consumption examples.

## Suggested Next

Begin Step 2 by adding a typed `FrameSlotAddress` payload and compatibility
bridge accessors, without adding optional fields to
`PreparedCallArgumentSourceSelection`.

## Watchouts

- Do not add new optional fields to `PreparedCallArgumentSourceSelection`.
- Do not infer argument homes in RV64/AArch64 when producer facts are absent.
- Do not use named torture-file handling or expectation weakening as progress.
- Keep sret memory-return address fallback explicit: once `FrameSlotAddress` is
  migrated, absence of the required typed route must not silently fall back to
  target reconstruction for required call arguments.
- AArch64 currently has silent `continue` paths in
  `materialize_missing_frame_slot_call_arguments`; Step 2/3 should preserve
  optional non-applicable behavior but prepare fail-closed classification for
  required-but-missing `FrameSlotAddress` facts.
- `ByvalRegisterLane` and `LocalFrameAddressMaterialization` share aggregate
  transport/storage facts, so defer them until the first route bridge proves the
  typed payload/accessor pattern.

## Proof

Inventory-only packet; no build or CTest required by the supervisor. Proof
command: `git diff -- todo.md`, confirming only canonical `todo.md` changed.
