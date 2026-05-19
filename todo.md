Status: Active
Source Idea Path: ideas/open/307_aarch64_large_scalar_immediate_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize The Illegal Immediate Path

# Current Packet

## Just Finished

Lifecycle switched from umbrella inventory idea 295 to focused owner idea 307.

## Suggested Next

Execute Step 1 by localizing the backend path that emits
`mov x0, #1234567` for `c_testsuite_aarch64_backend_src_00182_c`.

## Watchouts

- Keep `00189.c` PIC/global-symbol relocation outside this owner unless
  generated-code evidence proves shared constant-materialization semantics.
- Keep runtime nonzero, mismatch, crash, timeout, and output-storm buckets
  parked under umbrella idea 295.
- Do not special-case `00182.c`, `1234567`, or the exact emitted instruction
  string.
- Do not edit expectations, allowlists, unsupported classifications, runner
  behavior, timeout policy, CTest registration, or proof-log policy.

## Proof

No implementation proof has been run for this focused owner yet. The split is
based on read-only classification from the accepted post-306 `test_before.log`
and existing generated artifacts recorded in the prior umbrella `todo.md`.
