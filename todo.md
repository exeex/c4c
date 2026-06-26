Status: Active
Source Idea Path: ideas/open/375_rv64_object_route_scalar_compare_trunc_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Rerun Representative

# Current Packet

## Just Finished

Completed Plan Step 3 by rerunning the
`tests/c/external/gcc_torture/src/20000217-1.c` representative through the
RV64 gcc-torture backend object runner with a single-case allowlist.

The representative does not pass yet, but it now reports a different
object-route blocker:

```text
unsupported_move_bundle_target_shape: prepared move bundle requires unsupported RV64 moves
```

Runner artifacts:

- Allowlist: `build/agent_state/375_step3_20000217-1.allowlist.txt`
- Runner log: `build/agent_state/375_step3_20000217-1.runner.log`
- Per-case log: `build/rv64_gcc_c_torture_backend/src_20000217-1.c/case.log`
- Runner summary: `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- Runner failed list: `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`

## Suggested Next

Execute Plan Step 4 by closing or handing off this compare/trunc slice. The
recommended handoff owner is the prepared move-bundle target-shape route for
unsupported RV64 moves; no further compare/trunc implementation appears owned
by this plan step from the representative rerun.

## Watchouts

- Do not reopen frame-slot address call-argument materialization, pointer-value
  local memory, or frame-slot payload-value call arguments; ideas 372, 368, and
  373 already own those routes.
- Do not absorb non-register parameter homes, aggregate `va_arg`, byval homes,
  terminators, CFG reconstruction, or source-shape shortcuts.
- Progress must come from prepared compare/trunc metadata, not testcase names,
  hard-coded constants, value ids, frame slots, or registers.
- The admitted RHS immediate must fit the object-route `slti` immediate field;
  wider immediate materialization remains fail-closed.
- The source path named in the packet as `src/20000217-1.c` exists in this
  checkout as `tests/c/external/gcc_torture/src/20000217-1.c`.
- The Step 3 runner result should be treated as advancement to an out-of-scope
  object-route owner, not as a passing representative.

## Proof

Delegated proof ran and failed on the out-of-scope move-bundle target-shape
diagnostic:

```sh
ALLOWLIST=build/agent_state/375_step3_20000217-1.allowlist.txt STOP_ON_FAILURE=1 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/375_step3_20000217-1.runner.log 2>&1
```

Proof logs:

- `build/agent_state/375_step3_20000217-1.runner.log`
- `build/rv64_gcc_c_torture_backend/src_20000217-1.c/case.log`
