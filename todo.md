Status: Active
Source Idea Path: ideas/open/376_rv64_object_route_prepared_move_bundle_target_shapes.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Rerun Representative

# Current Packet

## Just Finished

Completed Step 5: reran the single `src/20000217-1.c` RV64 gcc-torture backend
object representative after the stack-slot-to-GPR move-bundle support.

The runner advanced past the prior
`unsupported_move_bundle_target_shape: prepared move bundle requires unsupported
RV64 moves` residual. It still exits nonzero, now at a distinct downstream
object-route owner:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

Generated diagnostic artifacts:

- `build/agent_state/376_step5_20000217-1.allowlist.txt`
- `build/agent_state/376_step5_20000217-1.runner.log`
- `build/rv64_gcc_c_torture_backend/src_20000217-1.c/case.log`
- `build/agent_state/376_step5_20000217-1.prepared-bir.txt`
- `build/agent_state/376_step5_20000217-1.codegen-obj.log`

## Suggested Next

Execute Step 6 closure/handoff: decide whether idea 376 is complete under its
move-bundle target-shape acceptance criteria and hand the representative's new
ordinary prepared instruction-fragment lowering residual to the next owner.

## Watchouts

- Do not reopen scalar compare/trunc lowering from idea 375.
- Do not infer move semantics from testcase names, hard-coded value ids, source
  syntax, instruction indexes, or the exact representative expression.
- The Step 5 rerun no longer reports `unsupported_move_bundle_target_shape`; the
  next failure is not owned by the current move-bundle target-shape packet.
- The prepared-BIR artifact shows remaining ordinary instruction candidates in
  `showbug`, including integer casts, arithmetic with stack-slot publication,
  local memory access, and the later compare/trunc path. The runner's object
  diagnostic is generic, so the next owner should add focused instruction-level
  diagnostics or audit the first unsupported prepared instruction before
  implementing a lowering slice.
- Stack-slot destinations, stack-to-stack moves, multi-lane destinations,
  cycle-temp moves, non-integer sources, FPR destinations, aggregate homes,
  missing homes, and ambiguous destinations are still unsupported unless a
  later packet admits them explicitly.
- The implementation is intentionally phase-agnostic inside the move-bundle
  fragment helper, but object traversal still controls which move-bundle phases
  reach this object-emission path.

## Proof

Command:

```sh
ALLOWLIST=build/agent_state/376_step5_20000217-1.allowlist.txt STOP_ON_FAILURE=1 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/376_step5_20000217-1.runner.log 2>&1
```

Result: exited nonzero after one case, with `total=1 passed=0 failed=1`.
Runner log: `build/agent_state/376_step5_20000217-1.runner.log`.
Per-case log: `build/rv64_gcc_c_torture_backend/src_20000217-1.c/case.log`.
No root-level proof logs were written by this packet.
