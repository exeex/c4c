# Select-Result Branch Publication

Status: Closed
Type: Terminator/select publication idea
Parent: `ideas/closed/441_terminator_select_publication_authority.md`
Source Evidence: `build/agent_state/441_step4_residual_disposition/`
Close Evidence: `build/agent_state/450_step4_residual_disposition/disposition.md`
Owning Layer: Prepared select-result publication before branch lowering
Closed Disposition: Blocked by split to
`ideas/open/452_select_edge_source_producer_rematerialization.md`

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

## Completion Notes

Steps 1 and 2 audited the candidate select-result branch rows and defined the
prepared select-result publication contract. Steps 3 and 4 found that the first
representative packet is not a direct branch-consumer problem. `20010329-1`
still fails at `unsupported_move_bundle_target_shape` because predecessor-edge
out-of-SSA move materialization would need the selected incoming source
rematerialized on the edge.

The precise blocker is `main.logic.end.14`: `%t22` is a select result feeding
`ne i32 %t22, 0`, and the incoming `%t18 -> %t22` value is a
successor/join-block compare over `%t15` and `%t17`. `%t17` is an `inttoptr`
result with a stack-slot pointer home. A plain copy from `%t18` on the
predecessor edge would be unsound because `%t18` is defined after the edge
executes. The source compare/cast dependency chain needs explicit prepared
select-edge source-producer rematerialization authority.

`20000622-1` remains instruction-side first-owner before select-result branch
consumption is reached. `930930-1` stack-home branch operand and pointer-value
publication residuals remain outside this idea.

Idea 450 is closed as blocked by a split. The selected durable follow-up is
`ideas/open/452_select_edge_source_producer_rematerialization.md`. Until that
authority exists, preserving `unsupported_move_bundle_target_shape` for
`20010329-1` is the correct fail-closed behavior.

## Validation

- Step 4 backend proof passed before log roll-forward:
  `cmake --build build -j2` plus
  `ctest --test-dir build -j2 --output-on-failure -R '^backend_'`.
- Step 4 `git diff --check` passed.

## Reviewer Reject Signals

- Reject RV64 changes that infer select-result publication from raw select
  instructions, raw `ne i32 <select>, 0` shape, block order, filenames,
  function names, or one prepared dump layout.
- Reject testcase-shaped fixes keyed to `20000622-1`, `930930-1`, one select
  root, one block, or one prepared row.
- Reject claiming `root_is_select=yes` alone is sufficient branch publication
  authority without an explicit consumer contract.
- Reject copying a successor/join-block compare result on a predecessor edge
  without explicit rematerialization authority.
- Reject expectation rewrites, unsupported-marker downgrades, allowlists, or
  pass/fail accounting changes claimed as select-result publication progress.
- Reject baseline/log churn that accepts or rewrites `test_baseline.new.log`.
