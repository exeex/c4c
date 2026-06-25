Status: Active
Source Idea Path: ideas/open/368_rv64_object_route_frame_slot_base_offset_memory.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Rerun Representatives

# Current Packet

## Just Finished

Completed Plan Step 3 representative rerun after the Step 2 local
subobject-offset slice.

The three idea-368 representatives still fail with the shared diagnostic
`unsupported_local_memory_access: RV64 object route requires prepared
frame-slot base-plus-offset local memory addressing`:

- `src/20000217-1.c`: failed; see
  `build/rv64_gcc_c_torture_backend/src_20000217-1.c/case.log`.
- `src/20000121-1.c`: failed; see
  `build/rv64_gcc_c_torture_backend/src_20000121-1.c/case.log`.
- `src/va-arg-13.c`: failed; see
  `build/rv64_gcc_c_torture_backend/src_va-arg-13.c/case.log`.

Representative summary from
`build/agent_state/368_step3_representatives.log`: total=3 passed=0 failed=3.

## Suggested Next

Supervisor route review or plan-owner decision for idea 368. The focused
subobject-offset admission did not advance any representative, so this plan
needs a route decision before more implementation.

## Watchouts

- Keep idea 354 open and inactive until child ideas 368-371 close or are
  intentionally superseded.
- Step 1 identified pointer-value/non-frame-slot local memory and
  vararg/helper-adjacent paths as likely separate owners.
- Do not implement aggregate `va_arg`, byval homes, terminator lowering, or
  unrelated pointer-memory behavior from this plan without a route decision.
- Do not hard-code representative offsets or source-case names; derive address
  facts from prepared metadata.
- Put analysis logs under `build/agent_state/`, not root-level canonical logs.
- The current shared diagnostic text may still be masking multiple residual
  owners; review the prepared facts before assigning another implementation
  packet.

## Proof

No build required for this todo-only result recording.

Evidence recorded from:

- `build/agent_state/368_step3_representatives.log`
- `build/rv64_gcc_c_torture_backend/src_20000217-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_20000121-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_va-arg-13.c/case.log`
