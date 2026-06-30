# Pointer Relational Branch Publication

Status: Open
Type: Terminator/select publication idea
Parent: `ideas/closed/441_terminator_select_publication_authority.md`
Source Evidence: `build/agent_state/441_step4_residual_disposition/`
Owning Layer: Prepared branch-condition publication before RV64 lowering

## Goal

Define and consume explicit authority for fused pointer relational branch
predicates such as `uge ptr` and `ult ptr`, without treating them as a small
extension of pointer equality/inequality publication.

## Why This Exists

Idea 441 completed the selected fused pointer `eq/ne` branch publication route.
Its Step 4 residual review found `20010329-1` still failing through
`unsupported_terminator_fragment`, with the first visible terminator owner a
pointer relational fused compare such as
`branch_condition entry ... compare=uge ptr %t5, %t7`. `930930-1` also shows
pointer relational branch candidates. Those shapes need an explicit predicate,
ordering, null/provenance, and target-consumer policy before RV64 can consume
them.

## In Scope

- Audit pointer relational fused branch conditions from
  `build/agent_state/441_step4_residual_disposition/`.
- Define the accepted predicate policy for pointer relational compares,
  including signedness/order semantics, null handling, operand provenance,
  operand homes, result home, and branch target labels.
- Add focused producer/prepared and RV64 consumer coverage for accepted facts
  and fail-closed missing/incoherent policy.
- Preserve the closed pointer `eq/ne` route without broadening it.

## Out Of Scope

- Pointer equality/inequality branch publication completed by idea 441.
- Select-result publication into branch conditions.
- Pointer-value memory provenance, local/global store-source publication, and
  unsupported instruction/storage residuals.
- Inferring relational pointer branch semantics from raw BIR shape, block
  order, filenames, function names, or representative testcase shape.
- Accepting or modifying `test_baseline.new.log`.

## Acceptance Criteria

- Pointer relational branch rows are classified by accepted policy versus
  missing or unsupported authority with representative evidence.
- Accepted rows are covered by focused tests and consume explicit prepared
  branch-condition facts.
- Missing, incoherent, unsupported, or policy-ambiguous relational pointer
  branches remain fail-closed.
- Any remaining unrelated residuals are routed separately instead of folded
  into this idea.

## Reviewer Reject Signals

- Reject RV64 changes that infer pointer relational branch semantics from raw
  `bir.cond_br`, raw compare instructions, block order, filenames, function
  names, or one dump layout.
- Reject testcase-shaped fixes keyed to `20010329-1`, `930930-1`, one block,
  one predicate, or one prepared row.
- Reject treating pointer relational predicates as already covered by the
  closed pointer `eq/ne` branch route.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists, or
  pass/fail accounting changes claimed as relational branch progress.
- Reject baseline/log churn that accepts or rewrites `test_baseline.new.log`.
