Status: Active
Source Idea Path: ideas/open/514_rv64_register_source_stack_destination_move_bundles.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Validate And Handoff

# Current Packet

## Just Finished

Step 6 validated the RV64 register-source stack-destination move-bundle
runbook for supervisor/lifecycle handoff.

The delegated build plus backend subset is green. The focused object probes
confirm the current handoff diagnostics:

- `tests/c/external/gcc_torture/src/pr27073.c` still stops at
  `unsupported_move_bundle_target_shape` for function `foo`, block `entry`,
  `instruction_index=1`, with `authority=none`, `move_count=1`,
  `parallel_copy=no`, `move[0].source_home_kind=register`,
  `move[0].source_type=i16`, `move[0].destination_home_kind=stack_slot`, and
  `move[0].destination_type=i32`.
- `tests/c/external/gcc_torture/src/20010518-1.c` still stops at
  `unsupported_move_bundle_target_shape` for function `add`, block `entry`,
  `instruction_index=0`, with `authority=none`, `move_count=2`,
  `parallel_copy=no`, two register-source `i32` moves, and both moves targeting
  the same stack-slot destination value.

Source idea 514 acceptance is only partially satisfied by the current runbook:
fresh validation and representative diagnostics are recorded, and the
multi-move case has a precise follow-up boundary, but the representative rows
do not yet advance through RV64 object emission. Closure should either record
the current runbook as a bounded diagnostic handoff or open a follow-up idea for
the multi-move producer/classification owner before treating the multi-move
acceptance path as complete.

## Suggested Next

Supervisor/lifecycle owner should decide whether to close this runbook as a
validated diagnostic handoff or split a follow-up source idea for the prepared
multi-move producer/classification contract.

## Watchouts

Residual risk remains around the producer/classification owner for multi-source
move bundles. Do not accept `20010518-1.c` by dropping either source, picking
the first or last source, renaming `consumer_stack_to_stack` reasons in place,
or inferring authority from the row name, argument index, source order, or
textual BIR. `pr27073.c` also remains a focused handoff diagnostic rather than
a passing object-route representative.

## Proof

Ran `cmake --build --preset default` followed by
`ctest --test-dir build -j --output-on-failure -R '^backend_'`; the delegated
proof passed with `100% tests passed, 0 tests failed out of 345`, and the
canonical proof log is `test_after.log`.

Ran focused RV64 object probes with
`tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake`:

- `tests/c/external/gcc_torture/src/pr27073.c` failed at the expected current
  handoff diagnostic; log:
  `build/agent_state/514_step6_handoff/src_pr27073.c/case.log`.
- `tests/c/external/gcc_torture/src/20010518-1.c` failed at the expected
  current handoff diagnostic; log:
  `build/agent_state/514_step6_handoff/src_20010518-1.c/case.log`.

Ran `git diff --check -- todo.md`; it passed.
