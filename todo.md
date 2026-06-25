Status: Active
Source Idea Path: ideas/open/373_rv64_object_route_frame_slot_value_call_args.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Rerun Representative

# Current Packet

## Just Finished

Executed Plan Step 3 by rerunning the `src/20000121-1.c` representative after
the scalar frame-slot-value call-argument support slice.

The representative still fails:

- runner result: `total=1 passed=0 failed=1`
- runner log: `build/agent_state/373_step3_20000121-1.runner.log`
- copied case log: `build/agent_state/373_step3_20000121-1.case.log`
- current diagnostic: `unsupported_instruction_fragment: BIR instruction
  requires unsupported RV64 object lowering`

The copied case log does not identify the exact instruction. A focused prepared
dump of `doit` still shows the first call argument as the active frame-slot
value publication shape: call block `0`, instruction `2`, `source_encoding=frame_slot`,
`source_slot=#2`, `source_stack_offset=16`, destination `a0`.

## Suggested Next

Review the Step 2 implementation against the real prepared shape for
`src/20000121-1.c`, because the representative did not move off the in-scope
frame-slot-value call-argument diagnostic. Keep the next packet focused on why
`doit` block `0` instruction `2` is still rejected after the positive unit
fixture passes.

## Watchouts

- Do not include frame-slot address call arguments; those belong to idea 372.
- Do not handle aggregate `va_arg`, byval homes, terminators, or source-shape
  shortcuts in this route.
- The later `doit` calls use register call-argument sources, so the rerun has
  not yet advanced to a distinct residual owner.
- The current failure should not be treated as closure evidence for idea 373.

## Proof

Delegated Step 3 proof ran exactly as requested and wrote non-root logs under
`build/agent_state/`. The command exited nonzero because the representative
still fails.

Command run:

```sh
tmp=$(mktemp); printf '%s\n' src/20000121-1.c > "$tmp"; CASE_TIMEOUT_SEC=20 ALLOWLIST="$tmp" scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/373_step3_20000121-1.runner.log; rc=$?; cp build/rv64_gcc_c_torture_backend/src_20000121-1.c/case.log build/agent_state/373_step3_20000121-1.case.log 2>/dev/null || true; rm -f "$tmp"; exit $rc
```

Result: `total=1 passed=0 failed=1`.
