Status: Active
Source Idea Path: ideas/open/516_rv64_multi_source_prepared_move_bundle_classification.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Multi-Source Bundle Evidence

# Current Packet

## Just Finished

Step 1 - Rebuild Multi-Source Bundle Evidence completed for
`tests/c/external/gcc_torture/src/20010518-1.c`.

Representative prepared facts:

- Focused BIR for `add` is a straight-line chain of binary adds. The first
  instruction is `%t0 = bir.add i32 %p.a, %p.b`; each subsequent binary add
  similarly has two named operands and one named result.
- Focused prepared BIR for `add` reports storage values in GPR bank with width
  1. `%p.a` has home `kind=register reg=a0`; `%p.b` has home
  `kind=register reg=a1`; destination `%t0` has home `kind=stack_slot
  slot_id=8 offset=32`. Storage plans agree: `%p.a` and `%p.b` are
  `encoding=register bank=gpr width=1`, while `%t0` is
  `encoding=frame_slot bank=gpr spill_slot=slot#8+stack32 width=1`.
- Object-mode rejection confirms the first bundle at `function=add
  block_index=0 block_label=entry instruction_index=0` is
  `phase=before_instruction`, `authority=none`, `move_count=2`,
  `parallel_copy=no`, `select_edge_suppression_authorized=no`, and
  `cast_dependency_stack_publication_authorized=no`.
- The rejected bundle contains two `i32` moves to the same stack-slot
  destination value: `move[0] from_value_id=0 to_value_id=13
  source_home_kind=register destination_home_kind=stack_slot`, and `move[1]
  from_value_id=1 to_value_id=13 source_home_kind=register
  destination_home_kind=stack_slot`. No source is stack-backed under explicit
  prepared facts.
- Ordering/authority facts are absent rather than contradictory:
  `source_parallel_copy_step_index`, predecessor label, and successor label are
  absent; `uses_cycle_temp_source=no`; both move records and the bundle carry
  `authority=none`. The only ordering visible is vector order in the bundle,
  which is not semantic authority.

Producer/classifier point:

- Producer creation starts in
  `src/backend/prealloc/regalloc/consumer_moves.cpp`:
  `append_consumer_move_resolution` visits `bir::BinaryInst` and calls
  `append_for_instruction(inst.result, {&inst.lhs, &inst.rhs}, ...)`. For the
  first `add`, it records one consumer move for `%p.a -> %t0` and one for
  `%p.b -> %t0`, both at block 0 instruction 0, with
  `PreparedMoveAuthorityKind::None`, no parallel-copy step, and reason derived
  from assigned storage.
- The bundle shape is created/preserved in
  `src/backend/prealloc/regalloc.cpp`: `build_prepared_value_location_function`
  iterates `regalloc_function.move_resolution`, and
  `append_prepared_move_bundle` groups records by phase, authority,
  block/instruction, and parallel-copy labels. It does not split or reject
  multiple sources targeting one value, so the two records become one
  `authority=none`, `move_count=2`, non-parallel bundle.
- The object-consumer classifier in
  `src/backend/prealloc/prepared_object_traversal.cpp`:
  `classify_prepared_object_move_bundle_consumer` preserves the ambiguity by
  returning `Available` for this non-parallel before-instruction bundle when
  phase and block match. It does not classify multi-source-to-one-destination
  ownership or sequencing.
- RV64 object emission is the rejection point, not the owner:
  `src/backend/mir/riscv/codegen/object_emission.cpp` calls the classifier,
  then `fragment_for_prepared_move_bundle`; the latter cannot materialize the
  mixed multi-source stack-destination bundle and reports
  `unsupported_move_bundle_target_shape ... fragment_status=generic_move_bundle_materialization_failed`.

## Suggested Next

Execute Step 2 by defining whether the producer/classifier should split these
binary-consumer operand moves into explicit independent moves, publish coherent
sequencing/authority, or reject the multi-source-to-one-stack-value bundle
before RV64 object emission.

## Watchouts

Do not accept the multi-source bundle by dropping a source, choosing the first
or last source by order, or inferring ownership from the row name, argument
order, ABI formula, raw BIR text, or source variable names. The first rejected
bundle uses misleading `consumer_stack_to_stack` reasons, but the explicit
prepared homes and object diagnostic prove both sources are register-backed;
do not treat the reason string as source-home authority.

## Proof

Evidence-only packet; no code-changing proof required and no canonical
regression log was written. Focused probes run:

- `./build/c4cll --dump-bir --target riscv64-unknown-linux-gnu
  --mir-focus-function add tests/c/external/gcc_torture/src/20010518-1.c`
- `./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu
  --mir-focus-function add tests/c/external/gcc_torture/src/20010518-1.c`
- `./build/c4cll --codegen obj --target riscv64-linux-gnu
  tests/c/external/gcc_torture/src/20010518-1.c -o /tmp/c4c_20010518.o`
  reproduced the expected object-route rejection.
- `c4c-clang-tool-ccdb` symbol queries confirmed producer/classifier
  definitions for `append_prepared_move_bundle` and
  `classify_prepared_object_move_bundle_consumer`.
- `git diff --check -- todo.md` passed.
