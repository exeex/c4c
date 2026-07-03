# Current Packet

Status: Active
Source Idea Path: ideas/open/568_rv64_pointer_result_frame_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct Pointer Address Boundary

## Just Finished

Completed Step 1 (`Reconstruct Pointer Address Boundary`) by reconstructing the
semantic boundary for the pinned pointer-result frame-slot address
materialization shape.

Semantic lowering hook:

- `fragment_for_prepared_instruction(...)` in
  `src/backend/mir/riscv/codegen/object_emission.cpp` should continue to claim
  this before the generic unsupported path, using the existing
  `fragment_for_prepared_frame_address_materialization(...)` boundary or a
  smallest-purpose extraction beside it.
- The generic scalar `fragment_for_prepared_binary(...)` path is not the owner:
  it intentionally accepts only `i32`/`i64` results, while the pinned instruction
  is `bir.add ptr`.

Required prepared fact lookup key:

- Lookup through `lookups.address_materializations`, preferably
  `find_indexed_prepared_address_materializations(..., prepared_block_label)`,
  and select exactly one fact for the current `prepared_block_label` plus
  `instruction_index`.
- The selected fact must be
  `PreparedAddressMaterializationKind::FrameSlot`, default address space,
  non-TLS, with a valid `frame_slot_id` and non-negative byte offset.
- For this pointer-result shape the fact's `result_value_name` identifies the
  pointer base/local frame-slot value, not the `bir.add ptr` result. The pinned
  fact is `block=block_1 inst_index=4 kind=frame_slot result=%lv.r.0 offset=8`.

Base/offset/result value-home expectations:

- Binary shape: `Add`, result type `ptr`, LHS type `ptr`, RHS integer byte
  offset, and all participating values named so homes can be validated through
  `PreparedFunctionLookups`.
- Base value: pointer LHS has the frame-slot address-materialization fact and
  an available GPR home for the current representative (`%lv.r.0`, value id
  `14`, register `s1`, `gpr:callee_saved#0/w1`).
- Offset value: integer RHS is materializable from its prepared home, including
  a frame-slot-backed GPR value (`%t12.byte_offset`, value id `12`,
  `slot#22+stack80`).
- Result value: pointer result must have a prepared destination home, including
  a frame-slot destination (`%t12`, value id `13`, `slot#23+stack88`).

Current fallthrough path:

- Traversal reaches `prepared_function_to_object_function(...)` instruction
  event for block index `3`, instruction index `4`, then
  `fragment_for_prepared_instruction(...)`.
- `fragment_for_prepared_frame_address_materialization(...)` currently misses
  this shape because it requires a GPR destination for the binary result and
  matches the address-materialization fact against the binary result name.
- `fragment_for_prepared_symbol_address_materialization(...)` does not apply,
  and `fragment_for_prepared_binary(...)` declines pointer results, so the
  object route reaches generic
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering`.

Destination publication rule:

- After validating the fact and value homes, materialize `base + dynamic byte
  offset` into a scratch/target GPR using existing RV64 prepared register,
  stack-slot load, add, and stack-offset helpers.
- Publish the computed pointer to the prepared result home. If the result home
  is a frame slot, store the pointer value to that slot; if it is a GPR, leave
  or move it into that register. Missing, ambiguous, incoherent, or non-default
  address facts must stay on the existing unsupported path.

Focused Step 2 coverage target:

- Add a focused prepared object-emission test in
  `tests/backend/mir/backend_riscv_object_emission_test.cpp` that constructs a
  `bir.add ptr` with a pointer base backed by a frame-slot address fact, a
  dynamic integer byte offset in a stack-slot home, and a pointer result in a
  frame-slot home.
- Reuse the nearby prepared frame-slot address local-store, stack-slot scalar
  flow, and stack-slot move-bundle helpers/patterns for frame slots, value
  homes, byte encodings, and fail-closed diagnostics.
- Include malformed/missing prepared fact coverage that preserves unsupported
  behavior; do not key the test or implementation on `src/20001026-1.c`,
  `%t12`, raw instruction text, or diagnostic strings.

## Suggested Next

Execute Step 2 (`Add Focused Pointer-Result Coverage`): add coherent and
fail-closed tests for pointer base plus dynamic integer byte offset producing a
pointer result in a prepared frame-slot destination.

## Watchouts

- Do not reopen div/rem opcode lowering; idea 567 already proved that route was
  not the current owner.
- Do not match `src/20001026-1.c`, `%t12`, raw instruction text, or diagnostic
  strings in implementation work.
- Leave `review/557_step13_vector_local_memory_review.md` untouched.

## Proof

Step 1 metadata-only proof required before handoff:

- `git diff --check -- todo.md`
- `scripts/plan_review_state.py set-step --step-id 1 --step-title 'Reconstruct Pointer Address Boundary'`
