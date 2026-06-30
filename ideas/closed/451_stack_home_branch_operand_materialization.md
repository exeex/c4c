# Stack-Home Branch Operand Materialization

Status: Closed
Type: Terminator/select publication follow-up
Parent: `ideas/closed/449_pointer_relational_branch_publication.md`
Source Evidence: `build/agent_state/449_step4_residual_disposition/`
Owning Layer: Prepared branch operand/condition materialization before RV64 lowering

## Goal

Define whether and how RV64 branch consumers may materialize branch operands or
conditions from stack homes, without weakening existing prepared branch
authority predicates.

## Completion Notes

Closed by split/blocker after Step 4 residual disposition. Idea 451 answered
its core question: current prepared stack-home branch facts are not sufficient
to justify RV64 branch operand/condition materialization.

Existing facts include stack homes, slot ids, offsets, object facts, and
prepared branch conditions. Missing facts include:

- explicit branch-site stack-load policy;
- value freshness at the branch site;
- clobber-safety proof across calls, helpers, stores, move bundles, and
  publication events;
- scratch/order constraints for consumers;
- pointer-provenance status for pointer operands.

The exact follow-up owner is producer/prepared branch-stack-load authority
metadata:
`ideas/open/469_branch_stack_load_authority_metadata.md`.

Boundary decisions:

- `f.block_1` `%t2 = ult ptr %t1, %p.reg2` is the best future metadata
  candidate but is not currently target-consumable.
- `f.block_4` `%t8 = ult ptr %t7, %p.mr_HB` also depends on pointer-value
  memory with unknown layout/range authority and remains separate.
- `f.logic.end.14` `%t23 = ne i32 %t22, 0` depends on select-result /
  block-entry stack-destination work and remains separate.
- GPR-compatible branch routes are already owned by earlier closed ideas.

Close validation used existing canonical regression logs and `git diff --check`.

## Reviewer Reject Signals

- Reject RV64 changes that infer stack-home branch operands, condition values,
  freshness, or loads from raw BIR shape, stack-slot spelling, block order,
  filenames, function names, or one prepared dump layout.
- Reject testcase-shaped fixes keyed to `930930-1`, one block, one stack slot,
  or one prepared row.
- Reject weakening existing branch publication predicates so stack homes become
  target-consumable without an explicit materialization policy.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists, or
  pass/fail accounting changes claimed as stack-home branch progress.
- Reject baseline/log churn that accepts or rewrites `test_baseline.new.log`.
