Status: Active
Source Idea Path: ideas/open/386_rv64_object_route_unsupported_instruction_fragment.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Frame-Slot Address GPR Call Argument Route

# Current Packet

## Just Finished

Step 2 classified the active `va-arg-13.c` route as a real RV64 object-route
support slice for GPR call arguments selected by
`PreparedCallArgumentSourceSelectionKind::FrameSlotAddress`, not as a
diagnostic-only or expectation route.

Selected route: teach `fragment_for_prepared_call` in
`src/backend/mir/riscv/codegen/object_emission.cpp` to accept the
`FrameSlotAddress` GPR argument selection when the prepared facts prove a
same-instruction default-address-space frame-slot materialization for the
selected local slot. The emitted object fragment should materialize the address
directly into the destination call register with `addi dest, sp, offset`, using
the selected frame slot/materialization offset. For the first failing call this
means `a0 = sp + 24` for `bir.call void dummy(ptr %t7)` at `test` block 0 inst
9, even though `%t7`'s ordinary value home remains spill slot #9 at stack offset
48.

Likely owned implementation file/functions:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `fragment_for_prepared_call`, in the GPR argument handling path currently
  rejecting `FrameSlotAddress`
- `prepared_frame_slot_address_call_argument_offset`, either generalized or
  paired with a sibling helper for `FrameSlotAddress`
- nearby RV64 encoding helpers already used by the supported local-frame
  address path: `rv64_register_number`, `fits_signed_12_bit_immediate`,
  `encode_i_type`, and `append_le32`

Exact prepared facts to consume:

- `argument.value_bank == Gpr`
- `argument.destination_register_bank == Gpr`
- `argument.destination_register_name == "a0"` for the first failing call
- `argument.destination_contiguous_width == 1`
- `argument.source_encoding == FrameSlot`
- `argument.source_value_id == 15`
- `argument.source_slot_id == #9` and `argument.source_stack_offset_bytes == 48`
  are the stored scalar value home and must not be used as the pointer payload
  offset for this selection
- `argument.source_selection.kind == FrameSlotAddress`
- `selection.source_value_id == argument.source_value_id`
- `selection.source_home_kind == StackSlot`
- `selection.source_slot_id == #7`
- `selection.source_stack_offset_bytes == 24`
- `selection.source_size_bytes == 8`
- `selection.source_align_bytes == 8`
- `selection.address_materialization_block_label == entry`
- `selection.address_materialization_inst_index == 9`
- `selection.address_materialization_frame_slot_id == #7`
- `selection.address_materialization_byte_offset == 24`
- indexed prepared address materializations for `entry` contain exactly the
  matching `FrameSlot` materialization at inst 9, default address space,
  non-TLS, slot #7, byte offset 24
- stack layout contains slot #7, the materialization offset is within that slot
  and within the static frame, and the final SP-relative immediate fits signed
  12-bit RV64 `addi`
- `find_prepared_missing_frame_slot_call_argument_publication_need(argument)`
  reports `available=yes`, `kind=frame_slot_address`,
  `source_materializes_address=yes`, and
  `may_emit_local_aggregate_address_payload=no`; that is evidence for address
  materialization only, not a local payload copy

Adjacent variants that must remain fail-closed:

- `920908-1.c` same-module memory-return/sret calls remain owned by idea 387;
  do not relax the `call_plan->memory_return.has_value()` admission gate under
  this packet
- indirect calls, callee-value calls, outgoing stack argument areas, variadic
  wrappers, and non-same/direct fixed wrappers
- aggregate-address/byval transports and `ByvalRegisterLane`
- `FrameSlotAddress` without a source selection, without source value id, or
  with non-GPR value/destination banks
- missing destination register name, destination width other than one, or
  destination stack placement
- selection/source mismatches: selection value id differs from argument source
  value id, missing selected source slot, selected slot differs from the
  materialization slot, missing materialization block/inst/offset, duplicate
  conflicting materializations at the same instruction, negative offset,
  non-default address space, TLS materialization, offset outside the selected
  slot or static frame, or an immediate outside signed 12-bit range
- ordinary `FrameSlotValue` scalar loads should keep using
  `prepared_frame_slot_call_argument_offset`; `FrameSlotAddress` must not load
  from the ordinary value home slot #9/offset 48
- `LocalFrameAddressMaterialization` should keep its existing support route;
  the new route should not broaden register/computed-address handling beyond
  the selected `FrameSlotAddress` frame-slot source

## Suggested Next

Implement the classified RV64 object support slice in
`src/backend/mir/riscv/codegen/object_emission.cpp`: add the guarded
`FrameSlotAddress` GPR call-argument address materialization path, then add or
extend focused object-emission coverage for the success case and fail-closed
shape mutations before proving `va-arg-13.c`.

## Watchouts

- This is a semantic frame-slot address publication route; do not implement it
  with testcase-shaped matching for `dummy`, `%t7`, slot numbers, or
  `va-arg-13.c`.
- The current object helper named
  `prepared_frame_slot_address_call_argument_offset` only accepts
  `LocalFrameAddressMaterialization` plus register/computed-address sources.
  Reusing it requires explicit generalization for `FrameSlotAddress` rather
  than weakening its existing guards.
- Text emission has a related `emit_riscv_frame_slot_address_argument`, and
  AArch64 has `materialize_missing_frame_slot_call_arguments`, but the object
  implementation still needs encoded RV64 fragment/fixup behavior in
  `object_emission.cpp`.
- Keep the split sret family out of scope: `920908-1.c` is rejected by the
  memory-return call admission gate, not by this GPR argument switch.
- `va-arg-13.c` has a second same-shape `dummy` call at inst 16; support should
  be general enough for both, while the first diagnostic trigger remains inst
  9.

## Proof

Classification-only. No canonical proof log was requested or written; left
`test_before.log` and `test_after.log` untouched. Ran read-only inspection:

- `git status --short`
- `rg -n "fragment_for_prepared_call|PreparedCallArgumentSourceSelectionKind|FrameSlotAddress|prepared call|FrameSlot" src tests -S`
- `c4c-clang-tool-ccdb find-definition /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp fragment_for_prepared_call build/compile_commands.json`
- `c4c-clang-tool-ccdb function-callees /workspaces/c4c/src/backend/mir/riscv/codegen/object_emission.cpp fragment_for_prepared_call build/compile_commands.json`
- targeted `sed` reads of `object_emission.cpp`, `calls.hpp`,
  `prepared_lookups.cpp`, `prepared_call_emit.cpp`, AArch64 `calls.cpp`, and
  focused backend tests
