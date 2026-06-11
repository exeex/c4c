Status: Active
Source Idea Path: ideas/open/188_phase_e_route7_comparison_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Select and Map One Comparison Consumer

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1 from
`ideas/open/188_phase_e_route7_comparison_view_consumer_migration.md`.

## Suggested Next

Delegate Step 1 to an executor: inspect Route 7 comparison provenance, select
one bounded AArch64 comparison consumer, identify its lookup key and fallback
path, and record the narrow proof command for the next packet.

## Watchouts

- Keep return-chain lookup outside this migration.
- Do not move target branch policy or ALU machine-record formation into BIR.
- Do not weaken tests or remove invalid-reference, absent-route, or fallback
  cases.
- Do not claim route-wide comparison helper contraction from one selected
  consumer.

## Proof

Lifecycle-only activation. No code validation run.
