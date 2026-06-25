Status: Active
Source Idea Path: ideas/open/368_rv64_object_route_frame_slot_base_offset_memory.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Rerun Representatives and Route Residuals

# Current Packet

## Just Finished

Plan Step 3 (`Rerun Representatives and Route Residuals`) reran the three
idea-368 representatives after pointer-value local-memory support.

Result: `total=3 passed=0 failed=3`.

- `src/20000217-1.c`: failed, but advanced off
  `unsupported_local_memory_access` to generic unsupported instruction
  lowering. Follow-up inspection showed `showbug` pointer-value local accesses
  are no longer the reported blocker; `main` carries local-frame-address call
  arguments for `%lv.x` and `%lv.y`, so this residual belongs with idea 372,
  not more pointer-value local memory work.
- `src/20000121-1.c`: failed, but advanced off
  `unsupported_local_memory_access` to generic unsupported instruction
  lowering. Follow-up inspection showed the `doit` pointer-value i8 load from
  `%p.id` is no longer the reported blocker; the next visible boundary is a
  frame-slot-value call-argument publication for spilled `%t1`, not an
  in-scope pointer-value memory blocker.
- `src/va-arg-13.c`: failed at `unsupported_param_home: RV64 object route
  requires all parameters in supported GPR or prepared FPR register homes`.
  Follow-up inspection still found the expected `dummy` pointer-value accesses
  and later frame-slot-address call arguments for `%t7` and `%t14`, but the
  current object-route stop is the parameter-home boundary before those later
  owners.

## Suggested Next

Run Plan Step 4 closure or handoff decision for idea 368. The representative
rerun shows semantic pointer-value local-memory repair moved all three cases
off the old local-memory diagnostic, but no representative passes yet.

## Watchouts

- Do not add another idea-368 pointer-value local-memory packet based on these
  three representatives; the current residuals are call-argument publication
  or parameter-home boundaries.
- Route `src/20000217-1.c` local-frame-address call arguments to idea 372.
- Decide whether the `src/20000121-1.c` frame-slot-value call-argument
  publication needs a separate child idea or belongs to an existing call
  publication route.
- Treat `src/va-arg-13.c` as currently stopped at parameter-home support before
  its later frame-slot-address and `va_arg` helper boundaries.
- Do not implement aggregate `va_arg`, byval homes, terminator lowering, or
  source-shape shortcuts from this plan.

## Proof

Delegated representative command wrote
`build/agent_state/368_step3_representatives_after_pointer.log` and exited
nonzero because all three selected representatives still fail:

`tmp=$(mktemp); printf '%s\n' src/20000217-1.c src/20000121-1.c src/va-arg-13.c > "$tmp"; CASE_TIMEOUT_SEC=20 ALLOWLIST="$tmp" scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/368_step3_representatives_after_pointer.log; rc=$?; rm -f "$tmp"; exit $rc`

Result summary:

- `src/20000217-1.c`: fail, case log
  `build/rv64_gcc_c_torture_backend/src_20000217-1.c/case.log`
- `src/20000121-1.c`: fail, case log
  `build/rv64_gcc_c_torture_backend/src_20000121-1.c/case.log`
- `src/va-arg-13.c`: fail, case log
  `build/rv64_gcc_c_torture_backend/src_va-arg-13.c/case.log`
