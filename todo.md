Status: Active
Source Idea Path: ideas/open/244_phase_f1_x86_route6_call_wrapper_diagnostic_oracle_replacement.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish the x86 Route 6 diagnostic/oracle baseline

# Current Packet

## Just Finished

Lifecycle activation only. No implementation packet has run yet.

## Suggested Next

Execute Step 1 from `plan.md`: inspect the x86 scalar `i32` Route 6
direct-call argument diagnostic/oracle baseline, prepared fallback behavior,
wrapper output expectations, and `ConsumedPlans` compatibility proof surfaces.
Record the exact first code-changing target and any missing fail-closed
coverage here before changing implementation.

## Watchouts

- Do not broaden this into x86 call-wrapper migration or riscv convergence.
- Do not claim prepared aggregate or draft retirement.
- Do not weaken expected strings, helper-oracle statuses, supported-path
  contracts, fallback behavior, or wrapper assembly as proof.
- Route 6 source authority must be agreement-gated against the current
  prepared call argument source selection.

## Proof

No build or ctest proof required for lifecycle-only activation.
