# Branch Stack-Load Authority Metadata

Status: Closed
Type: Producer/prepared branch-stack-load authority metadata idea
Parent: `ideas/closed/451_stack_home_branch_operand_materialization.md`
Source Evidence: `build/agent_state/451_step4_residual_disposition/`
Owning Layer: Prepared branch operand/condition stack-load authority

## Goal

Publish explicit prepared metadata that says whether a branch operand or branch
condition value with a stack-slot home may be materialized by loading that
stack slot at the branch site.

## Completion Notes

Closed after Step 6 residual disposition. Idea 469 completed the
producer/prepared record-surface prerequisite:

- prepared branch-stack-load authority contract;
- available/unavailable planner statuses;
- real prepared-module collector;
- prepared-printer/probe visibility;
- focused coverage for coherent synthetic available records and fail-closed
  unavailable records;
- fresh `930930-1` representative evidence.

Representative rows remain unavailable, which is correct for this source idea:

- `f.block_1` condition `%t2`: `unsupported_terminator`;
- `f.block_1` lhs `%t1`: `unsupported_terminator`, `pointer_status=unknown`;
- `f.block_4` condition `%t8`: `unsupported_terminator`;
- `f.block_4` lhs `%t7`: `unsupported_terminator`, `pointer_status=unknown`;
- `f.logic.end.14` condition `%t23`: `missing_policy`;
- `f.logic.end.14` lhs `%t22`: `missing_policy`.

No RV64 consumer was added, and existing GPR-compatible branch predicates were
not weakened.

Follow-up source idea:
`ideas/open/470_branch_stack_load_policy_freshness.md`.

That follow-up owns branch-site `load_from_stack_slot` policy, freshness, and
clobber-safety production before any RV64 branch-load consumer resumes.

Close validation used existing canonical regression logs and `git diff --check`.

## Reviewer Reject Signals

- Reject RV64 branch-load emission or target lowering changes presented as
  producer/prepared metadata work.
- Reject accepting stack homes as branch-load authority from slot ids, offsets,
  stack-slot spelling, raw BIR shape, block order, filenames, function names,
  or testcase shape alone.
- Reject weakening GPR-compatible branch predicates so stack homes become
  implicitly target-consumable.
- Reject folding pointer-value/provenance or select-result stack-destination
  repair into this metadata idea.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists,
  pass/fail accounting changes, baseline/log churn, or classification-only
  changes presented as capability progress.
