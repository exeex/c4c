Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Retained Metadata Authority Leak

# Current Packet

## Just Finished

Step 5 closure review rejected closure. The source idea remains open because
the surviving AArch64 calls helper set still contains retained `CallInst`
ABI/type metadata reads that decide call-boundary behavior:
`outgoing_stack_argument_bytes` in `calls_common.cpp`,
`call_argument_allows_local_aggregate_address_publication` in
`calls_dispatch_bridge.cpp`, and `byval_register_lane_size_bytes` in
`calls_byval_aggregates.cpp`.

## Suggested Next

Execute Step 1 of `plan.md`: select one retained metadata authority leak,
prove whether an existing prepared fact can replace it, and record the chosen
focused proof command before implementation.

## Watchouts

- Closure is rejected without editing the source idea; this is a runbook-layer
  checkpoint, not source-intent churn.
- Do not touch `ideas/open/03_dispatch_responsibility_reduction.md`.
- Do not count retained BIR identity checks or diagnostics as blockers unless
  they still decide call-planning facts.
- If a required prepared fact is missing, stop and record that blocker instead
  of recreating the decision in AArch64-local code.

## Proof

No new code validation was run during lifecycle review. The prior broader
backend checkpoint is recorded in `test_before.log` after supervisor log
roll-forward, with 162/162 `^backend_` tests passing.
