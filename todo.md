Status: Active
Source Idea Path: ideas/open/369_rv64_object_route_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Rerun Representatives

# Current Packet

## Just Finished

Completed Step 5 of `plan.md`: reran the two idea 369 representatives after
the pointer-null branch lowering.

Results:

- `src/20000224-1.c`: passed.
- `src/20000112-1.c`: advanced past the prior fused pointer-null
  `ne ptr %reg, 0` terminator failure and now fails at
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64
  object lowering`.

Both previously owned terminator-fragment shapes are no longer the first
failures:

- `src/20000224-1.c`: prior fused `sgt i32 %reg, %reg` conditional branch now
  passes in the representative run.
- `src/20000112-1.c`: prior fused pointer-null `ne ptr %reg, 0` conditional
  branch is no longer the reported object-route blocker.

## Suggested Next

Proceed to Step 6 closure/handoff check for idea 369. The remaining
`src/20000112-1.c` failure is an instruction-fragment owner, not an in-scope
terminator-fragment owner for this source idea.

## Watchouts

- The Step 5 proof command exits nonzero because one representative still
  fails, but the residual diagnostic family is
  `unsupported_instruction_fragment`, outside idea 369's terminator-fragment
  scope.
- Supporting logs:
  `build/agent_state/369_step5_terminator_representatives.runner.log` and
  per-case log
  `build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`.

## Proof

Ran the supervisor-selected proof exactly:

`mkdir -p build/agent_state && printf '%s\n' 'src/20000224-1.c' 'src/20000112-1.c' > build/agent_state/369_step5_terminator_representatives.allowlist.txt && ALLOWLIST=build/agent_state/369_step5_terminator_representatives.allowlist.txt STOP_ON_FAILURE=1 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1`

Result: exited nonzero with `total=2 passed=1 failed=1`.

This is sufficient Step 5 representative proof for idea 369 because the only
remaining representative failure is an out-of-scope instruction-fragment
blocker, while the prior terminator-fragment blockers no longer fail first.

Proof logs: `test_after.log` and
`build/agent_state/369_step5_terminator_representatives.runner.log`
