# Select-Result Branch Publication

Status: Open
Type: Terminator/select publication idea
Parent: `ideas/closed/441_terminator_select_publication_authority.md`
Source Evidence: `build/agent_state/441_step4_residual_disposition/`
Owning Layer: Prepared select-result publication before branch lowering

## Goal

Define and consume explicit authority for select-result values published into
branch conditions, especially `root_is_select=yes` rows that feed
`ne i32 <select>, 0`.

## Why This Exists

Idea 441 completed the selected fused pointer `eq/ne` branch publication route.
Its Step 4 residual review found later follow-up candidates where prepared
select-chain rows feed branch conditions, including `20000622-1` rows such as
`%t13/%t24` into `ne i32 ..., 0` and `root_is_select=yes` evidence. These rows
need a separate select-result authority contract instead of being inferred from
raw select or branch shape.

## In Scope

- Audit select-result branch publication rows from
  `build/agent_state/441_step4_residual_disposition/`.
- Define prepared facts required to publish select results into branch
  conditions, including root select identity, result value home, condition
  conversion, comparison against zero, target labels, and consumer boundary.
- Add focused coverage for accepted select-result branch publication and
  fail-closed missing/incoherent authority.
- Separate instruction-side blockers from true select-result publication gaps.

## Out Of Scope

- Pointer equality/inequality branch publication completed by idea 441.
- Pointer relational fused branch predicates.
- Direct-global return/select-chain authority, pointer-value memory, local or
  global store-source publication, and unsupported instruction/storage
  residuals.
- Inferring select-result publication or branch conditions from raw BIR select
  shape, block order, filenames, function names, or representative testcase
  shape.
- Accepting or modifying `test_baseline.new.log`.

## Acceptance Criteria

- Select-result branch rows are classified by accepted publication authority
  versus instruction-side or producer blockers with representative evidence.
- Accepted rows are covered by focused tests and consume explicit prepared
  select-result and branch-condition facts.
- Missing or incoherent select-result publication remains fail-closed.
- Any remaining unrelated residuals are routed separately instead of folded
  into this idea.

## Reviewer Reject Signals

- Reject RV64 changes that infer select-result publication from raw select
  instructions, raw `ne i32 <select>, 0` shape, block order, filenames,
  function names, or one prepared dump layout.
- Reject testcase-shaped fixes keyed to `20000622-1`, `930930-1`, one select
  root, one block, or one prepared row.
- Reject claiming `root_is_select=yes` alone is sufficient branch publication
  authority without an explicit consumer contract.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists, or
  pass/fail accounting changes claimed as select-result publication progress.
- Reject baseline/log churn that accepts or rewrites `test_baseline.new.log`.
