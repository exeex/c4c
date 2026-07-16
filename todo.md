# Current Packet

Status: Active
Source Idea Path: ideas/open/831_preexisting_baseline_failure_family_decomposition_blocker.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish first-owner decomposition from exact reproductions

## Just Finished

- Lifecycle switch: 830 Steps 1 and 2 remain accepted; its Step 3 baseline
  gate is parked because the rejected 14-failure candidate is evidenced to
  predate `f0fc85e4f` and lies outside its direct-call argument-1 scope.

## Suggested Next

- Reproduce the exact 14-failure subset and establish the first owning seam
  independently for the `frontend_hir_tests` segfault and the 13
  `LirCmpOp.truthiness_lhs_parameter_authority` torture failures.
- Do not implement a combined repair. If ownership differs as the supplied
  evidence indicates, create ordered separately scoped repair successors
  before code changes.

## Watchouts

- Preserve the accepted `f0fc85e4f` focused 1/1 result and do not change 830.
  No expectation downgrade, unsupported marker, allowlist, test filter, or
  weaker harness contract can clear this baseline gate.

## Proof

- Baseline provenance: `test_baseline.log` at `8418036b` accepted 3038/3038;
  `test_baseline.new.log` at `f0fc85e4f` rejected 3024/3038. Step 1 starts
  with the exact 14-failure reproduction command recorded in the source idea.
