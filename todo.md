Status: Active
Source Idea Path: ideas/open/377_rv64_object_route_instruction_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Rerun Representative

# Current Packet

## Just Finished

Step 4: Rerun Representative completed for the single
`src/20000217-1.c` RV64 gcc-torture backend object representative after scalar
integer call-result stack-slot publication support.

The runner still exits nonzero, but the prior audited call-result publication
blocker is no longer reported by the runner or per-case log. The representative
now stops at the generic residual object-route owner:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

Because the available runner/per-case diagnostic does not identify the next
prepared instruction shape, this packet cannot classify the residual as a
distinct out-of-scope owner for lifecycle closure.

## Suggested Next

Run a narrow follow-up audit packet for the residual
`unsupported_instruction_fragment` in `src/20000217-1.c`, using only diagnostic
evidence needed to name the next prepared instruction shape. If that shape is
outside idea 377, Step 5 can close/handoff with a concrete owner; if it is still
an ordinary supportable instruction fragment, keep it in-scope as the next
focused lowering packet.

## Watchouts

- The admitted lowering is metadata-driven; it does not key on function names,
  value ids, frame-slot ids, concrete registers beyond prepared ABI/source
  metadata, instruction indexes, source syntax, or prepared-BIR text.
- FPR/non-integer, pointer-typed result publication, non-stack destination,
  missing home, mismatched home/plan slot facts, offset drift, and multi-lane
  result plans remain rejected by the object route.
- The representative does not pass yet. The current public diagnostic is still
  generic, so treating Step 4 as lifecycle-close evidence would be premature
  without a sharper first-residual instruction-fragment audit.

## Proof

Supervisor-selected representative proof ran exactly as delegated and wrote
only `build/agent_state/` runner artifacts plus the per-case log:

```sh
ALLOWLIST=build/agent_state/377_step4_20000217-1.allowlist.txt STOP_ON_FAILURE=1 VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/377_step4_20000217-1.runner.log 2>&1
```

Result: nonzero runner exit after one case.

Artifact paths:

- `build/agent_state/377_step4_20000217-1.allowlist.txt`
- `build/agent_state/377_step4_20000217-1.runner.log`
- `build/rv64_gcc_c_torture_backend/src_20000217-1.c/case.log`
