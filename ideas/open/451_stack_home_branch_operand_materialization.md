# Stack-Home Branch Operand Materialization

Status: Open
Type: Terminator/select publication follow-up
Parent: `ideas/closed/449_pointer_relational_branch_publication.md`
Source Evidence: `build/agent_state/449_step4_residual_disposition/`
Owning Layer: Prepared branch operand/condition materialization before RV64 lowering

## Goal

Define whether and how RV64 branch consumers may materialize branch operands or
conditions from stack homes, without weakening existing prepared branch
authority predicates.

## Why This Exists

Idea 449 completed the unsigned pointer relational branch route for
GPR-compatible condition and operand homes. Its Step 4 residual review found
`930930-1` still has pointer relational branch rows, but those rows remain
fail-closed because `%t1/%t2/%t7/%t8` use stack-slot homes and the module has
earlier pointer-value/provenance and instruction/storage owners. Stack-home
branch operand/condition materialization must be a separate policy and
consumer decision, not an implicit extension of the GPR-home relational route.

## In Scope

- Audit branch operands and branch conditions with stack-slot homes from
  `build/agent_state/449_step4_residual_disposition/`.
- Define the prepared facts required to materialize stack-home branch operands
  or conditions, including stack object identity, offset, width, load policy,
  clobber safety, value freshness, and consumer boundary.
- Add focused coverage for accepted stack-home branch materialization and
  fail-closed missing or incoherent facts.
- Preserve existing branch publication predicates for GPR-compatible homes.

## Out Of Scope

- Pointer equality/inequality branch publication completed by idea 441.
- Unsigned pointer relational branch publication completed by idea 449.
- Select-result branch publication and move-bundle target materialization,
  tracked by `ideas/open/450_select_result_branch_publication.md`.
- Pointer-value/provenance publication, tracked by
  `ideas/open/442_pointer_value_memory_provenance_publication.md`.
- Inferring stack-home values, freshness, or branch operands from raw BIR shape,
  stack-slot spelling, filenames, function names, or representative testcase
  shape.
- Accepting or modifying `test_baseline.new.log`.

## Acceptance Criteria

- Stack-home branch operand/condition rows are classified by accepted
  materialization policy versus missing, unsupported, or incoherent authority
  with representative evidence.
- Accepted rows are covered by focused tests and consume explicit prepared
  stack-home facts.
- Missing, stale, incoherent, unsupported, or policy-ambiguous stack-home
  branch values remain fail-closed.
- Any unrelated pointer-value/provenance, select-result, instruction, or
  storage residual is routed separately instead of folded into this idea.

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
