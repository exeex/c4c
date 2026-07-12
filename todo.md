# Current Packet

Status: Active
Source Idea Path: ideas/open/709_aarch64_named_handoff_materializer_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2.3
Current Step Title: Migrate select and comparison consumers

## Just Finished

- Route review split the former broad Step 2 into bounded consumer families.
  Step 2.1's return-chain authority gap was resolved through common-producer
  ideas 727 and 728 before the AArch64 reconstruction was deleted, and Step
  2.2's direct dispatch-producer migration is complete.

## Suggested Next

- Execute Plan Step 2.3 only against the select/comparison value-home lookup
  rebuilding family. Stop for lifecycle review if no existing attached common
  query owns a required relation.

## Watchouts

- Do not reopen the completed return-chain consumer or reproduce its relation
  in AArch64. The scalability timeout predates Step 2.2, and the pre-existing
  selected-global-load fused-branch stale-stack-home failure remains. Both
  belong in the supervisor's broader proof assessment, not in Step 2.3 scope.

## Proof

- Lifecycle-only route reset; no code proof was run. Step 2.1 previously
  passed its delegated build and 10/10 focused subset. Step 2.2 implementation
  is complete, while its exact delegated command stopped on the pre-existing
  scalability timeout before refreshing the focused proof log. Step 3 retains
  the required broader AArch64 checkpoint.
