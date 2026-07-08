Status: Active
Source Idea Path: ideas/open/596_pointer_rhs_branch_stack_source_policy_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate The `Rhs` Producer Policy Gap

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1.

## Suggested Next

Delegate Step 1 to an executor to locate the shared producer policy gate that
records pointer `Rhs` inventory but leaves it with missing policy.

## Watchouts

- Do not make RV64 or any target-local consumer infer freshness as a workaround.
- Do not use expectation downgrades, unsupported markers, allowlists, or
  testcase-shaped matching as proof of producer capability.

## Proof

Lifecycle-only activation; no build or test proof required.
