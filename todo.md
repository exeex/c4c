Status: Active
Source Idea Path: ideas/open/514_rv64_register_source_stack_destination_move_bundles.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Reassess Multi-Move Bundle Ownership

# Current Packet

## Just Finished

Step 5 reassessed `tests/c/external/gcc_torture/src/20010518-1.c` after the
single-move register-source to stack-destination boundary was settled.

Decision: split/reject the representative multi-move bundle from the current
RV64 object-emission runbook. The precise owner is the prepared move-bundle
producer/classification contract for multiple register sources targeting one
stack value, not the Step 3 single-move materializer.

Evidence:

- Focused object proof still stops at
  `unsupported_move_bundle_target_shape` for function `add`, block `entry`,
  `event_kind=before_instruction_copies`, `instruction_index=0`.
- The failing bundle has `authority=none`, `parallel_copy=no`, and
  `move_count=2`.
- Both moves target the same stack destination value: `move[0]` moves
  `from_value_id=0` to `to_value_id=13`; `move[1]` moves `from_value_id=1` to
  `to_value_id=13`.
- Both sources have explicit register homes and `i32` source types; the
  destination has an explicit stack-slot home and `i32` type.
- The prepared dump records the same first bundle as two
  `consumer_stack_to_stack` moves even though `home %p.a value_id=0
  kind=register`, `home %p.b value_id=1 kind=register`, and `home %t0
  value_id=13 kind=stack_slot slot_id=8 offset=32`.
- No explicit prepared sequencing, merge, or parallel-copy authority explains
  how two register sources should be materialized into one stack destination
  without dropping a move or choosing by source order.

## Suggested Next

Proceed to Step 6 validation/handoff. If the supervisor wants to pursue
`20010518-1.c`, open a separate producer/classification idea for multi-source
prepared move bundles that defines explicit sequencing or merge authority
before RV64 materialization is attempted.

## Watchouts

Do not accept this row by dropping either source, picking the first or last
source, renaming `consumer_stack_to_stack` reasons in place, or inferring
authority from the row name, argument index, source order, or textual BIR. The
current evidence supports a follow-up owner because the missing fact is an
explicit prepared contract for all moves in the bundle.

## Proof

Ran the focused RV64 object proof for
`tests/c/external/gcc_torture/src/20010518-1.c` with
`tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`; it failed
at the expected unsupported owner and wrote its probe log to
`build/agent_state/514_step5_20010518/case.log`.

Ran `build/c4cll -I tests/c/external/gcc_torture --dump-prepared-bir
--mir-focus-function add tests/c/external/gcc_torture/src/20010518-1.c`; it
completed successfully and wrote prepared evidence to
`build/agent_state/514_step5_20010518/prepared_add.txt`.

`test_after.log` was not updated because this packet was evidence-only and
explicitly listed it under Do Not Touch.
