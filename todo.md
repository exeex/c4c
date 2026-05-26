# Current Packet

Status: Active
Source Idea Path: ideas/open/32_riscv_prepared_edge_publication_stack_destination_scratch_policy.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inspect Current Consumer And Scratch Constraints

## Just Finished

Step 1 audit completed for the current RISC-V prepared edge-publication
consumer and scratch constraints.

Consumer files/functions:
- `src/backend/mir/riscv/codegen/emit.cpp`
  `render_edge_publication_source_operand` records supported source homes,
  `consume_edge_publication_move_intent` performs shared-publication lookup and
  target-local policy checks, and `append_edge_publication_move_instruction`
  emits only when the intent status is `Available`.
- `src/backend/mir/riscv/codegen/emit.hpp` exposes
  `EdgePublicationMoveIntent`, `EdgePublicationMoveIntentStatus`, and the
  consumer helpers used by focused tests.
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
  contains the focused `backend_riscv_prepared_edge_publication` coverage for
  register destinations, stack destinations, unsupported homes, and shared
  lookup authority.

Current `Register -> StackSlot` policy:
- `consume_edge_publication_move_intent` first requires a shared
  `PreparedEdgePublicationLookupStatus::Available` `Move` publication.
- The only supported stack destination form is
  `destination_home.kind == StackSlot`, present destination offset,
  `destination_home.size_bytes == 4`, populated `source_register`, and no
  immediate, stack-source, or pointer-base source metadata.
- The emitted instruction is `sw <source-register>, <offset>(sp)`, and the
  focused test proves `sw a0, 24(sp)` plus `MissingPublication` after clearing
  shared publication lookup authority.

Current fail-closed points for non-register source-to-`StackSlot`:
- `RematerializableImmediate -> StackSlot`: source operand is accepted when
  `immediate_i32` exists, but the stack-destination branch rejects it because
  `intent.source_immediate_i32` is set, so status becomes
  `UnsupportedDestinationHome`.
- `StackSlot -> StackSlot`: source operand is accepted only for size 4 or size
  8 with a signed-12-bit load offset; the stack-destination branch rejects it
  because `intent.source_stack_offset_bytes` is set, so status becomes
  `UnsupportedDestinationHome`.
- `PointerBasePlusOffset -> StackSlot`: source operand is accepted only when
  the base value resolves to a register and byte delta exists; the
  stack-destination branch rejects it because `intent.source_pointer_byte_delta`
  is set, so status becomes `UnsupportedDestinationHome`.
- Malformed stack destinations without an offset, with non-I32 size, or with a
  non-`Move` publication remain explicitly rejected by focused tests.

Nearby scratch/temp conventions observed:
- The broader `RiscvCodegen` helpers use `t0` as the primary accumulator for
  operand materialization and `store_t0_to`, with `t1`/`t2` as secondary temps
  in ALU/atomic/memory paths.
- Existing stack load/store helpers in `returns.cpp` emit direct `s0` memory
  operands without large-offset expansion; prepared-edge stack-destination
  currently emits direct `sp` memory operands and does not have a large-offset
  address policy.
- Memory/address helpers use `t5` as an address register and `t6` for large
  immediates in address/stack adjustment paths; call lowering also stages with
  `t3`, `t4`, and `t5` when it owns that call-sequence lifetime.
- No explicit scratch ownership, reservation, or clobber contract exists in
  the prepared edge-publication consumer path today.

## Suggested Next

Step 2 should define a local scratch contract before code support broadens.
Safest first candidate after that contract is `RematerializableImmediate ->
StackSlot` for I32 stack destinations: it needs one scratch value register for
`li` followed by the existing `sw` shape, avoids a stack load before the store,
and avoids pointer-base address materialization. It should still spell out
direct stack-store offset limits or large-offset address policy before
claiming support.

## Watchouts

- Shared prepared `edge_publications` must remain the only semantic authority
  for supported edge moves.
- Do not use a hard-coded scratch register without an explicit ownership,
  reservation, or clobber contract.
- Do not match fixture labels, value ids, stack slot ids, offsets, or test
  names.
- Existing `Register -> StackSlot` behavior must remain supported.
- Unsupported source or destination homes must remain explicit and fail closed.
- `StackSlot -> StackSlot` likely needs both load and store scratch/address
  policy, including the current source-size and load-offset restrictions.
- `PointerBasePlusOffset -> StackSlot` also needs pointer materialization and
  large-delta/aliasing policy; register-destination pointer support already
  rejects large deltas when destination aliases the base register.
- The current prepared-edge stack-destination path uses `sp` offsets, while
  broader `RiscvCodegen` helpers mostly use `s0` stack slots; do not mix those
  policies without confirming the prepared BIR frame convention.

## Proof

Ran:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$' > test_after.log 2>&1`

Result: passed. Build reported no work to do; `test_after.log` records 1/1
focused test passing.
