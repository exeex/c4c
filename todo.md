Status: Active
Source Idea Path: ideas/open/593_rv64_branch_stack_source_freshness_consumption.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Producer Contract And RV64 Consumers

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/593_rv64_branch_stack_source_freshness_consumption.md`.

## Suggested Next

Start Step 1 by reading the idea 592 closure note and tracing the RV64
branch/stack-source consumer paths that can now consume selected shared
`BranchStackLoadSource` freshness.

## Watchouts

- Do not start 594 before 593 closes with a concrete closure-note handoff.
- Do not use RV64 target-local stack-home, frame-slot, aggregate-lane, clobber,
  register, or operand-shape evidence as freshness.
- If a producer fact promised by 592 is missing, record that as a blocker for
  the 592 family instead of manufacturing fallback freshness in RV64.

## Proof

No build or test proof is required for this lifecycle-only activation.
