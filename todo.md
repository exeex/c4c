Status: Active
Source Idea Path: ideas/open/655_stack_destination_fan_in_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish The Blocked Failure-Family Baseline

# Current Packet

## Just Finished

Lifecycle switch completed from the blocked idea 647 Step 2 route to the
stack-destination fan-in authority decomposition idea. No implementation or
test files were changed.

## Suggested Next

Execute Step 1: establish the blocked failure-family baseline from
`build/agent_state/647_step2_family_revision/summary.md`, preserving the
rejected `src/20021204-1.c` mutual-exclusion route and the rejected
`src/20011109-2.c` idea 637 route as route-quality evidence.

## Watchouts

- Do not execute idea 647 Step 3.
- Do not select `SelectMaterializationPreservedStackFallback` as non-637
  progress.
- Do not use unrelated select/join facts as authority for a different stack
  destination.
- Keep focused probes tied to authority seams, not GCC torture testcase names.

## Proof

Lifecycle-only switch. No build or ctest proof required. No root-level logs
were created or overwritten.
