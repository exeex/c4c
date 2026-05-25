Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Residual Prepared-Fact Gaps

# Current Packet

## Just Finished

Step 5 lifecycle review for the prior runbook rejected closure. The source idea
is still active because AArch64 call emission retains multiple helper files and
remaining BIR call metadata reads that may duplicate prepared call-plan
authority.

The durable source idea did not need edits. The active runbook was rewritten
as a follow-up checkpoint focused on auditing and removing the next residual
prepared-fact gap.

## Suggested Next

Supervisor should delegate Step 1: audit `calls_common.cpp`,
`calls_argument_sources.cpp`, `calls_byval_aggregates.cpp`,
`calls_moves.cpp`, `calls_preservation.cpp`, and `calls_dispatch_bridge.cpp`
for retained `bir::CallInst`, `arg_abi`, or `arg_types` reads. The executor
should select one coherent duplicate-authority family and record the proof
scope before code changes.

## Watchouts

- Do not close this idea until the remaining AArch64 call helper surface and
  retained BIR metadata reads have been reviewed against prepared call-plan
  authority.
- Do not route broad dispatch cleanup into this plan; `calls_dispatch_bridge.*`
  is in scope only for call-boundary ownership.
- If a required fact is absent from prepared data, record that as a blocker
  instead of rebuilding call planning locally in AArch64 emission.
- No implementation files, test logs, or unrelated ideas were touched during
  this lifecycle review.

## Proof

Lifecycle-only review. Existing accepted proof for the prior Step 4 slice was:

`bash -lc '{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(backend_aarch64_machine_printer|backend_aarch64_call_boundary_owner|backend_call_boundary_effect_plan|backend_x86_call_boundary_effect_ordering)$"; } > test_after.log 2>&1'`

Closure regression guard was not run because source-idea completion is false.
