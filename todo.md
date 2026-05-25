Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Dispatch Responsibilities

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/03_dispatch_responsibility_reduction.md`.

## Suggested Next

Begin Step 1 by mapping AArch64 dispatch responsibilities and selecting the
first small extraction target. Exclude any candidate that depends on the
blocked prepared call-argument source fact recorded in
`ideas/open/02_aarch64_calls_emission_consolidation.md`.

## Watchouts

- Do not treat the blocked AArch64 calls consolidation idea as active.
- Do not resume AArch64-local call move/source cleanup unless the missing
  shared prepared fact exists or a fresh mapping proves a different helper
  already duplicates an available prepared fact.
- Do not claim progress from mechanical dispatch renames without responsibility
  reduction.

## Proof

No implementation proof required for lifecycle activation. The executor should
record the supervisor-selected build and focused backend proof commands after
the first code-changing packet.
