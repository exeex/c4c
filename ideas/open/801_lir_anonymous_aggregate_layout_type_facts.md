# LIR Anonymous Aggregate Layout Type Facts

Status: Open
Type: bounded structured type-model prerequisite
Blocks: `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md` Step 3

## Goal

Publish checked native field-layout and field-type facts for anonymous LIR
aggregate types, so a bounded downstream row can validate aggregate field
indices and result element types without interpreting `LirTypeRef` display
text.

## Why This Exists

754's selected direct-complex `LirExtractValueOp` has
`agg_type = LirTypeRef("{ float, float }")`. Existing structured type support
covers arrays and named structs but does not retain anonymous composite field
lists. Therefore index bounds and selected-element type cannot be verified
without forbidden compatibility-text parsing.

## In Scope

- Trace the smallest type construction, ownership, and verifier seams for
  anonymous aggregate field lists used by the selected direct-complex path.
- Select and implement the minimum native anonymous aggregate layout/type fact
  representation, with checked construction and compatibility rendering.
- Prove native field-count/field-type access and rejection of malformed or
  incoherent anonymous layout facts, then publish the exact 754 handoff.

## Out Of Scope

- `LirExtractValueOp` result, aggregate-use, field-index, or result-type
  validation; 754 owns that row work after this handoff.
- Raw-BIR, other aggregate/vector row conversion, generic type-system rewrite,
  target lowering, MIR, emission, or identity recovery from display text.

## Acceptance Criteria

- Anonymous aggregate layouts retain checked native ordered field-type facts
  sufficient for a consumer to establish bounds and selected field type.
- Missing, malformed, foreign, or type-incoherent layout facts reject without
  parsing compatibility strings.
- Focused positive and malformed proof publishes a precise 754-consumable
  handoff; existing named-struct/array paths remain unchanged or fail closed.

## Reviewer Reject Signals

- Reject parsing `{ ... }` text, printer output, LLVM text, instruction order,
  or testcase names to obtain fields or types.
- Reject adding `LirExtractValueOp` row validation, a Raw-BIR receiver, or a
  broad aggregate/vector conversion under this prerequisite.
- Reject an abstraction-only carrier that leaves the direct-complex anonymous
  aggregate path without checked native field facts.
- Reject testcase-shaped shortcuts, expectation downgrades, or weaker verifier
  contracts claimed as native layout/type progress.

## Resumption Record: Step 2 switch-selector verifier blocker

Last accepted progress: Step 1, `Trace and select anonymous aggregate layout
facts`, is accepted in `827dae5bd3`. It selected the bounded native layout
contract and did not implement extractvalue-row validation.

Interrupted step: Step 2, `Repair anonymous layout / structured-call
compatibility`. The exact remaining objective is to repair only the anonymous
layout construction/model/validation seam so a direct-complex `LirCallOp` has
coherent `callee_signature`, `arg_type_refs`, and structured arguments while
native ordered field facts stay authoritative and checked. The unaccepted
implementation base is `201f229d3`; current working-tree changes are an
in-progress Step 2 repair and are not accepted progress.

Failure evidence: clean rebuilds in separate worktrees at `827dae5bd` and
`fcdda3415` both deterministically fail `frontend_lir_call_type_ref` with
`LirCallOp.callee_signature: structured callee signature does not match call
arguments`. This shows the recorded pre-change passing baseline did not
reproduce from source. The current uncommitted in-scope repair makes that call
mismatch pass, then deterministically exposes
`LirSwitch.selector_type_ref: must match the selector-selected integer value
definition`. Historical commit `a6c013ed0` introduced the latter verifier
defect; it is outside this anonymous aggregate layout/call contract scope.

Classification: `separate-blocker`. New open
`ideas/open/802_lir_switch_selector_type_reference_verifier.md` owns only the
switch selector type-reference verifier defect. It must not accept, discard,
or broaden the in-progress 801 Step 2 repair.

Exact return point: after 802 has accepted its bounded verifier repair and
proof, reactivate 801 at unchanged Step 2. Preserve the current working-tree
repair for evaluation, repair the direct-complex structured-call mismatch
without weakening that verifier, add nearby relevant coverage, then obtain the
Step 2 required fresh build, focused call/frontend/backend checks, and the
supervisor-accepted full baseline before advancing to Step 3. Do not repeat
Step 1 and do not treat a clean narrow subset as acceptance.

## Resumption Update: Step 2 argument-mirror prerequisite

802 Step 1 cannot reach its selector switch check in the preserved working
tree. `frontend_lir_call_type_ref` now first rejects at
`LirCallOp.arg_type_refs`: `argument 0 mirror does not match call text; shadow
'i32', call argument type 'rendered arguments are not'`. This is an in-scope
Step 2 compatibility defect: the argument-mirror verifier must recover its
native structured argument comparison without using rendered diagnostic text
as type authority and without weakening either the mirror or callee-signature
contract.

801 is reactivated unchanged at Step 2 for that repair. The prior Step 1
acceptance in `827dae5bd3`, its unaccepted implementation base `201f229d3`,
and all earlier resumption constraints remain in force. First prove
`frontend_lir_call_type_ref` reaches the selector switch check; then return to
802 Step 1, whose isolated verifier/test hunk remains unaccepted and parked.

## Resumption Update: Step 2 aggregate-use authority blocker

Last accepted progress remains Step 1, `Trace and select anonymous aggregate
layout facts`, in `827dae5bd3`. Step 2 is still unaccepted: `201f229d3` is
its unaccepted implementation base and the current working-tree
argument-mirror repair is preserved but unaccepted. Its fresh build plus
`frontend_lir_call_type_ref` proof passes, but that narrow result is not its
required focused call/frontend/backend ladder or supervisor-accepted full
baseline.

Interrupted step: Step 2, `Repair anonymous layout / structured-call
compatibility`. The active full-baseline attempt first fails in the aggregate
extract path at `LirExtractValueOp.agg: aggregate SSA operand requires valid
LirValueId authority` (for example,
`positive_sema_ok_call_builtin_runtime_c` and
`llvm_gcc_c_torture_src_complex_2_c`). The candidate baseline is 2989/3037
versus the accepted 3037/3037 baseline, so Step 2 cannot be accepted or
advanced.

Classification: `separate-blocker`. The failure is aggregate-use authority
owned by `ideas/open/754_lir_aggregate_vector_value_identity_convergence.md`,
not anonymous layout construction, structured-call compatibility, or switch
selector verification. The active 754 repair must reassess its formerly
accepted Step 2 aggregate operand contract before Step 3 can be runnable.

Exact return point: after 754 accepts the bounded aggregate-use authority
repair with its required proof, reactivate 801 at unchanged Step 2. Preserve
the current unaccepted Step 2 repair and 802's parked unaccepted hunk; first
complete 801's fresh build, focused call/frontend/backend proof, and
supervisor-accepted full baseline before Step 3. Do not repeat Step 1 or
claim the preserved repair as accepted progress.
