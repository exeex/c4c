Status: Active
Source Idea Path: ideas/open/369_rv64_object_route_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Rerun Representatives

# Current Packet

## Just Finished

Completed Step 5 of `plan.md`: reran the representative RV64 GCC torture
backend subset after the `sgt i32` fused branch lowering.

Representative results:

- `src/20000224-1.c`: pass. Its prior first unsupported
  `cond_branch` shape, `fused_compare compare=sgt i32 %t0, %t1`, no longer
  fails in the representative object-route run.
- `src/20000112-1.c`: fail. The run still stops at
  `unsupported_terminator_fragment`; the prior Step 1 audit identifies its
  first unsupported branch shape as `fused_compare compare=ne ptr %t0, 0`.

## Suggested Next

Implement the next in-scope terminator shape for `src/20000112-1.c`: semantic
RV64 object-route lowering for a fused pointer-null `ne` conditional branch
(`ne ptr %reg, 0`). Keep the packet scoped to the general pointer/null branch
shape and add focused object-emission coverage plus representative rerun proof.

## Watchouts

- The Step 5 runner log records the remaining `src/20000112-1.c` failure only
  as `unsupported_terminator_fragment`; use
  `build/agent_state/369_step1_terminator_audit.summary.log` and
  `build/agent_state/369_step1_terminator_audit.prepared_bir.log` for the
  known first unsupported shape evidence.
- `src/20000224-1.c` passing is the confirmation that the prior `sgt i32`
  fused branch shape advanced out of the representative blocker set.
- Do not turn this into named-case handling for `20000112-1.c`; the next owner
  should lower the semantic pointer/null branch form and fail closed for
  non-owned pointer compare variants.

## Proof

Ran the supervisor-selected proof exactly:

`mkdir -p build/agent_state && printf '%s\n' 'src/20000224-1.c' 'src/20000112-1.c' > build/agent_state/369_step5_terminator_representatives.allowlist.txt && ALLOWLIST=build/agent_state/369_step5_terminator_representatives.allowlist.txt STOP_ON_FAILURE=1 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > test_after.log 2>&1`

Result: exited nonzero because the representative subset still has one
in-scope unsupported terminator. This is sufficient Step 5 pass/next-owner
evidence: total=2, passed=1, failed=1.

Proof logs and supporting artifacts:

- `test_after.log`
- `build/agent_state/369_step5_terminator_representatives.runner.log`
- `build/agent_state/369_step5_terminator_representatives.allowlist.txt`
- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- `build/rv64_gcc_c_torture_backend/src_20000224-1.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_20000112-1.c/case.log`
