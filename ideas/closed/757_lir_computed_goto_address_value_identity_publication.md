# LIR Computed-Goto Address Value Identity Publication

Status: Closed (capability complete)
Type: bounded LIR computed-goto producer authority repair
Predecessor: `ideas/open/734_lir_to_new_bir_container_completeness.md`

## Goal

Publish verifier-checked current-function `LirValueId` authority for each
active `LirIndirectBrOp` address so one later Raw-BIR computed-goto receiver
packet can consume it with the already typed ordered successors.

## Why This Exists

Closed idea 750 established ordered `LirIndirectBrOp.successors`, but `addr`
is only a `LirOperand` spelling. Idea 734 cannot reconstruct pointer identity
from that carrier, so its next valid computed-goto receiver row is blocked at
the LIR producer boundary.

## In Scope

- add the smallest typed address-value carrier to active `LirIndirectBrOp`
- populate it from existing current-function pointer authority without parsing
  operand text
- verify presence, validity, current-function ownership, and pointer
  suitability before downstream use
- retain `addr` only as a checked display mirror
- add focused positive and malformed-authority coverage plus an exact handoff
  to idea 734

## Out Of Scope

- Raw-BIR indirect-jump container, importer, verifier, or receiver work; that
  remains idea 734
- computed-goto successor changes already completed by idea 750
- legacy `LirIndirectBr`, `LirCondBr`, `LirSwitch`, PHI, local/object,
  memory/va, aggregate/vector, parameter, or other identity work
- operand/label/printer-text recovery, canonicalization, target lowering, MIR,
  or emission

## Acceptance Criteria

- Every active `LirIndirectBrOp` address controlling computed-goto flow has
  valid current-function typed pointer identity independent of `addr` text.
- Missing, invalid, foreign, or non-pointer address authority rejects before
  printing or downstream consumption, with no text fallback.
- Focused positive and negative tests prove misleading address display text
  cannot select or repair the semantic address.
- The handoff names the typed address field, existing ordered successor field,
  fail-closed boundaries, and proof required for one 734 receiver packet.

## Reviewer Reject Signals

- Reject parsing `addr`, rendered LLVM, printer output, labels, or testcase
  names to derive address identity or type.
- Reject a named-case-only fix, expectation downgrade, or display agreement
  claimed as semantic progress.
- Reject changes to Raw-BIR receiver code, computed-goto successor authority,
  conditional/switch authority, or unrelated LIR families under this bounded
  producer source.
- Reject a carrier that leaves missing, foreign, or non-pointer address failure
  reachable behind a renamed field or permits downstream fallback.

## Resumption Record: rvalue identity-preservation blocker

Historical pre-closure state: last accepted progress was paused idea 734 Step 7.23: commit `0995a3deb`
received typed `LirSwitch` authority, and the matching backend guard accepted
5/5. The full-baseline candidate with 73 failures was rejected and is not
full-suite-green evidence. At that time no 757 step was complete. The separately scoped 758
blocker is now capability-complete at commit `c8a205218`: eligible local and
parameter rvalue identities survive through the rvalue/operand route into
immediate typed consumers, with no display-text recovery.

The interrupted runbook pointer is Step 1, `Publish computed-goto address
authority`. The attempted producer route reached
`StmtEmitter::emit_control_flow_stmt(IndirBrStmt)`, whose
`emit_rval_operand` result was string-only. A local `DeclRef` fell through to
`LirOperand::raw(emit_rval_expr(...))`; although `emit_rval_payload(DeclRef)`
emitted a typed `%dispatch` `LirLoadOp`, it returned only spelling. Closed 758
now preserves that existing result as a typed rvalue identity, so Step 1 can
publish the address without prohibited text recovery.

Classification: `separate-blocker`. Open idea
`ideas/open/758_lir_rvalue_value_identity_preservation_for_computed_goto.md`
owned only preservation of existing local/parameter rvalue typed value identity
through the current rvalue/operand route and is now archived at
`ideas/closed/758_lir_rvalue_value_identity_preservation_for_computed_goto.md`.
It did not own the 757
`LirIndirectBrOp` field, verifier, producer handoff, or Raw-BIR work.

Exact return action after that blocker has an accepted handoff: reactivate 757
at Step 1, `Publish computed-goto address authority`, and publish
`LirIndirectBrOp`'s typed address field from the preserved rvalue identity.
Prove valid, missing, invalid, foreign, non-pointer, and misleading-display
cases; run a fresh build and
`ctest --test-dir build -j --output-on-failure -R '^frontend_lir_call_type_ref$'`.
The uncommitted 757 prototype (field plus verifier) was reverted after the
focused test failed under real lowering because the address ID was absent; it
has no implementation commit. Before that packet, the supervisor recorded a
fresh build and the exact focused test passing 1/1 in `test_before.log`.

## Closure Decision

Close accepted: capability complete. Commit `8527c6dbc` publishes optional
typed `LirIndirectBrOp.addr_value` directly from `emit_rval_operand`; `addr`
is a checked display mirror and is never semantic authority. The LIR verifier
rejects missing, invalid, foreign, non-pointer, and display-mismatched IDs
before printing or downstream use, while the existing ordered `successors`
authority remains unchanged.

Supervisor acceptance evidence is the fresh build plus focused
`^frontend_lir_call_type_ref$` proof passing 1/1, and a matching
`^backend_` before/after regression guard passing 5/5 under
allow-non-decreasing. Direct supervisor review found no text-derived authority
or scope drift.

Handoff to open predecessor
`ideas/open/734_lir_to_new_bir_container_completeness.md`: resume exactly at
Step 7.24, `Receive typed computed-goto authority`. Consume only verified
`addr_value` and ordered `successors` in one transactional Raw-BIR receiver
packet; preserve target order and reject malformed authority before publication.
`addr`, labels, printer output, and rendered text remain non-authoritative.
