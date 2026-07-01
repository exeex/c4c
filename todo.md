Status: Active
Source Idea Path: ideas/open/513_rv64_stack_to_stack_prepared_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Stack-To-Stack Move Evidence

# Current Packet

## Just Finished

Completed Step 1 from `plan.md`: rebuilt stack-to-stack move evidence for the
three primary rows under
`build/agent_state/513_step1_stack_to_stack_move_evidence/`.

Fresh probe commands:

- Build: `cmake --build build --target c4cll` passed.
- Prepared dumps: `build/c4cll --dump-prepared-bir --target riscv64-linux-gnu
  tests/c/external/gcc_torture/src/<row>.c` exited `0` for all three rows.
- Object probes: `build/c4cll --codegen obj --target riscv64-linux-gnu
  tests/c/external/gcc_torture/src/<row>.c -o
  build/agent_state/513_step1_stack_to_stack_move_evidence/<row>/out.o`
  exited `2` for all three rows.

Row evidence:

- `20010518-1.c`: object rejection is
  `unsupported_move_bundle_target_shape` at function `add`, block `entry`
  (`block_index=0`), `instruction_index=0`, `phase=before_instruction`,
  `event_kind=before_instruction_copies`. The prepared BIR site is
  `%t0 = bir.add i32 %p.a, %p.b`. The rejected bundle has
  `authority=none`, `move_count=2`, `parallel_copy=no`,
  `select_edge_suppression_authorized=no`,
  `cast_dependency_stack_publication_authorized=no`, and
  `fragment_status=generic_move_bundle_materialization_failed`. Moves:
  `from_value_id=0` to `to_value_id=13` and `from_value_id=1` to
  `to_value_id=13`, both `destination_kind=value`,
  `destination_storage=stack_slot`, `op_kind=move`,
  `reason=consumer_stack_to_stack`. Source homes are `%p.a value_id=0
  kind=register reg=a0` and `%p.b value_id=1 kind=register reg=a1`;
  storage-plan width/class is `encoding=register bank=gpr width=1` for both.
  Destination home is `%t0 value_id=13 kind=stack_slot slot_id=8 offset=32`;
  storage plan is `encoding=frame_slot bank=gpr spill_slot=slot#8+stack32
  width=1 slot_id=#8 stack_offset=32`. Producer authority is present enough
  to name register sources and a GPR stack destination, but this row is not the
  cleanest initial stack-slot source representative because the first rejected
  sources are registers despite the move reason text.

- `pr27073.c`: object rejection is `unsupported_move_bundle_target_shape` at
  function `foo`, block `entry` (`block_index=0`), `instruction_index=1`,
  `phase=before_instruction`, `event_kind=before_instruction_copies`. The
  prepared BIR site is `%t0 = bir.sext i16 %p.count to i32`. The rejected
  bundle has `authority=none`, `move_count=1`, `parallel_copy=no`,
  `select_edge_suppression_authorized=no`,
  `cast_dependency_stack_publication_authorized=no`, and
  `fragment_status=generic_move_bundle_materialization_failed`. Move:
  `from_value_id=4` to `to_value_id=10`, `destination_kind=value`,
  `destination_storage=stack_slot`, `op_kind=move`,
  `reason=consumer_stack_to_stack`. Source home is `%p.count value_id=4
  kind=register reg=a4`; storage plan is `encoding=register bank=gpr width=1`.
  Destination home is `%t0 value_id=10 kind=stack_slot slot_id=25 offset=68`;
  storage plan is `encoding=frame_slot bank=gpr spill_slot=slot#25+stack68
  width=1 slot_id=#25 stack_offset=68`. Producer authority is coherent for a
  narrow GPR register-to-frame-slot copy, but the first failure again is not a
  true stack-slot source.

- `pr69447.c`: object rejection is `unsupported_move_bundle_target_shape` at
  function `foo`, block `entry` (`block_index=0`), `instruction_index=14`,
  `phase=before_instruction`, `event_kind=before_instruction_copies`. The
  prepared BIR site is `%t9 = bir.zext i16 %t8 to i64`. The rejected bundle has
  `authority=none`, `move_count=1`, `parallel_copy=no`,
  `select_edge_suppression_authorized=no`,
  `cast_dependency_stack_publication_authorized=no`, and
  `fragment_status=generic_move_bundle_materialization_failed`. Move:
  `from_value_id=15` to `to_value_id=16`, `destination_kind=value`,
  `destination_storage=stack_slot`, `op_kind=move`,
  `reason=consumer_stack_to_stack`. Source home is `%t8 value_id=15
  kind=stack_slot slot_id=18 offset=104`; storage plan is
  `encoding=frame_slot bank=none spill_slot=slot#18+stack104 width=1
  slot_id=#18 stack_offset=104`. Destination home is `%t9 value_id=16
  kind=stack_slot slot_id=19 offset=112`; storage plan is
  `encoding=frame_slot bank=gpr spill_slot=slot#19+stack112 width=1
  slot_id=#19 stack_offset=112`. This is the smallest first-failure row with
  both source and destination homes as prepared stack slots, but Step 2 must
  account for the source storage class being `bank=none` rather than silently
  treating it as a coherent GPR scalar.

No fresh artifact contains `unsupported_param_home`; all three rows now fail
later at `unsupported_move_bundle_target_shape`.

Smallest semantic representative for Step 2: use `pr69447.c` first because its
first failing move has explicit stack-slot source and destination homes. Treat
`20010518-1.c` and `pr27073.c` as confirmation/adjacent rows for the currently
misclassified or unsupported register-source-to-stack-destination move reason.

## Suggested Next

Execute Step 2: trace RV64 move-bundle consumption in
`src/backend/mir/riscv/codegen/object_emission.cpp` and define the exact
fail-closed predicate for `pr69447.c`-style prepared stack-slot to stack-slot
moves, including how `bank=none` source storage should be classified.

## Watchouts

Do not infer storage class from testcase names, parameter order, BIR text, or
the `consumer_stack_to_stack` reason alone. `pr69447.c` has the clean stack-home
shape but source storage `bank=none`; `20010518-1.c` and `pr27073.c` have first
failures whose source homes are registers while the reason string still says
`consumer_stack_to_stack`.

## Proof

Evidence-only packet. Ran `cmake --build build --target c4cll` and regenerated
prepared/object probe artifacts under
`build/agent_state/513_step1_stack_to_stack_move_evidence/`. No CTest was
required, and `test_after.log` was not overwritten.
