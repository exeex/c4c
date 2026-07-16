# LIR Remaining Authority Owner Triage - Current Evidence

Status: current evidence bundle for idea 866 Step 1
Source: `ideas/open/866_lir_remaining_authority_owner_triage.md`
Parent: `ideas/open/734_lir_to_new_bir_container_completeness.md`
Predecessor: `ideas/closed/865_lir_next_non_body_parameter_authority_handoff.md`

## Accepted Receiver History

Idea 734 is accepted through Step 7.51 at commit `750b6b3ba`, which received
the one-argument floating direct-call result row. That commit is current
receiver history and must be treated as already consumed by 734.

Commit `0c44e810ad` already receives the selected scalar integer `LirAbsOp`
selected-global/i32 result authority in
`tests/backend/bir/backend_lir_to_bir_interface_test.cpp` through
`selected_global_i32_abs_module` and
`test_selected_global_i32_abs_receipt_and_rejections`. That row must not be
reopened as producer handoff work or as a new 734 receiver route.

## 865 Disposition

Idea 865 produced no handoff. Its selected row, scalar integer `LirAbsOp`
selected-global/i32 result authority consumed as a same-function integer
`add` LHS, was rejected because the accepted receiver commit `0c44e810ad`
already covers it. The 865 route found no bounded replacement inside its
one-row non-body-parameter scope.

No Raw-BIR receiver return row is authorized by 865. Any later 734 successor
must come from a fresh current first-owner classification or from a reconciled
existing idea, not from the concluded 865 route.

## Accepted Or Stale Rows Not To Reopen

Do not reopen these accepted or stale rows while triaging idea 866:

- Fixed direct-call argument 0 and argument 1 parameter rows.
- Function body-parameter rows.
- Accepted direct call-result rows, including zero-argument floating results,
  the `double(double)` direct call-result row, and the one-argument floating
  call-result row accepted at `750b6b3ba`.
- Scalar integer `LirAbsOp` selected-global/i32 received by `0c44e810ad`.
- Local-object rows.
- VLA stack-save and stack-restore rows.
- Accepted CFG/PHI rows where the current receiver history already proves the
  typed Raw-BIR receipt.

These rows may remain useful as evidence, but they are not selectable as new
producer handoff or receiver implementation packets.

## Broad Remaining Families From The 865 Blocker

The remaining authority space needs first-owner classification before any
implementation route is selected. The broad families are:

- CFG/PHI residuals not already accepted.
- Memory and variadic-argument authority, including object and lifetime
  questions not covered by accepted local-object or VLA rows.
- Aggregate and vector value/type authority.
- Module, type, global, and metadata semantic authority.
- Residual instruction and terminator authority.
- Inline-assembly authority, preserving templates and constraints as opaque
  unless a separate policy explicitly owns parsing.
- Generic residual sweeps that are too broad for direct receiver work.

## Stale Open Idea Reconciliation Rule

Stale open ideas may contain useful evidence, but they require reconciliation
before becoming current 734 successors. Their older return records must be
checked against post-Step-7.51 734 receiver history, accepted rows above, and
the current 866 first-owner classification. A stale idea is not current
execution authority merely because its family overlaps a remaining 734
blocker.
