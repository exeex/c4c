Status: Active
Source Idea Path: ideas/open/513_rv64_stack_to_stack_prepared_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace RV64 Move-Bundle Consumption

# Current Packet

## Just Finished

Completed Step 2 from `plan.md`: traced RV64 move-bundle consumption in
`src/backend/mir/riscv/codegen/object_emission.cpp` and classified the narrow
future stack-slot to stack-slot materialization boundary.

Current lowering path:

- `fragment_for_prepared_move_bundle` first suppresses authorized select-edge
  and cast-dependency bundles, then handles out-of-SSA parallel copies and
  before-return stack-to-register ABI moves before the generic move loop.
- The generic loop already accepts simple before-instruction
  `destination_storage=stack_slot` moves only when the bundle is
  `phase=before_instruction`, `authority=none`, `destination_kind=value`,
  `op_kind=move`, no cycle-temp source, no source immediate, no explicit
  destination stack offset, contiguous width `1`, and reason is either
  `consumer_register_to_stack` or `consumer_stack_to_stack`.
- Existing register-to-stack support gets the source from
  `gpr_register_number_for_home`, derives the destination byte width from the
  destination BIR type via `rv64_scalar_memory_size_for_type`, validates the
  prepared destination home with `prepared_stack_slot_home_absolute_offset`,
  then emits `append_rv64_store_register_to_stack_offset`.
- Existing stack-to-register support elsewhere uses the same scalar size and
  stack-home validation before `append_rv64_load_stack_offset_to_register`.
- The current generic `consumer_stack_to_stack` branch attempts a load from the
  source home into `rv64_unoccupied_temporary_gpr` and a store into the
  destination home, using the destination type size for both sides. It therefore
  fail-closes when either home is missing/non-stack, the source slot cannot
  satisfy that size/alignment, there is no free scratch GPR, the scalar width is
  unsupported, or the stack offsets cannot be materialized.

Accepted predicate for Step 3 should be made explicit rather than relying on
the current broad reason string:

- Bundle: one before-instruction value move, `authority=none`, `parallel_copy`
  absent, no cycle-temp source, no immediate source, `destination_storage` is
  `stack_slot`, `destination_stack_offset_bytes` absent, contiguous width `1`,
  and reason is `consumer_stack_to_stack`.
- Source and destination: both `prepared_value_home_for_id` records exist and
  both homes are `kind=stack_slot` with stable slot id, offset, size, and align
  facts accepted by `prepared_stack_slot_home_absolute_offset`.
- Type/class: source and destination BIR types are the same supported RV64
  integer or pointer-sized scalar copy width from
  `rv64_scalar_memory_size_for_type`; no F128, aggregate, memory-class,
  floating, vector, or conversion/widen/narrow move is admitted.
- Storage class: source and destination prepared storage must be coherent GPR
  scalar frame-slot storage. `bank=none` is not positive copy authority.
- Emission: load source stack slot into a temporary GPR chosen by
  `rv64_unoccupied_temporary_gpr`, then store that same scratch GPR to the
  destination stack slot using the existing offset-aware load/store helpers.

`pr69447.c` caveat: the first failing move (`from_value_id=15` `%t8` to
`to_value_id=16` `%t9`) has stack-slot homes on both sides, but the source
storage plan is `encoding=frame_slot bank=none` and the source object is `i16`
while the destination object is `i64` at a `zext i16 to i64` site. Step 3
should leave this first failure fail-closed under the stack-to-stack scalar
copy predicate. Admitting it would infer a widening/zero-extension copy from
home shape and destination size rather than consuming coherent producer
storage authority. A later producer or conversion-specific packet may publish
separate authority for this shape; this packet should not.

Adjacent malformed cases that must remain rejected:

- missing source home or missing destination home
- source or destination home not a stack slot for the stack-to-stack helper
- register-source moves with `reason=consumer_stack_to_stack`
  (`20010518-1.c` and `pr27073.c` first failures)
- source and destination BIR types differ, including zext/sext/trunc-like
  conversion moves
- unsupported scalar width, F128, aggregate, memory-class, vector, floating, or
  multi-register/contiguous-width moves
- incoherent home size/alignment, mismatched frame-slot object facts, dynamic
  stack locations, or non-materializable offsets
- unavailable scratch GPR or load/store helper rejection

## Suggested Next

Execute Step 3 by factoring a narrow helper under
`fragment_for_prepared_move_bundle`, for example a
`fragment_for_prepared_stack_slot_to_stack_slot_move` helper that owns only the
coherent same-scalar GPR frame-slot copy predicate above. Add a focused
synthetic object-emission test for an accepted same-type stack-slot to
stack-slot copy, plus reject coverage for missing homes, non-stack homes,
register-source reason mismatch, type/size mismatch, `bank=none`, and
unsupported widths/classes.

## Watchouts

The row label `consumer_stack_to_stack` is weaker than the facts needed for
object emission. `pr69447.c` is not an accepted proof case for the coherent
same-scalar helper until producer storage bank/type authority is coherent;
`20010518-1.c` and `pr27073.c` should remain rejected by the stack-to-stack
helper because their first failing source homes are registers. Step 3 proof
should use synthetic semantic coverage first, then rerun the three row probes
to classify any remaining first failures without widening the predicate.

## Proof

Trace-only packet. Ran `git diff --check -- todo.md`. No build or CTest was
required by the packet, and `test_after.log` was not overwritten.
