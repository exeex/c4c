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

## Resumption Update: return to the 802 selector gate

Last accepted progress remains only Step 1, `Trace and select anonymous
aggregate layout facts`, in `827dae5bd3`. Step 2 remains an unaccepted
working-tree repair based on `201f229d3`; its native anonymous-layout and
direct-complex structured-call compatibility changes must stay preserved but
must not be committed or credited as accepted 801 progress.

The supervisor's fresh `cmake --build --preset default` followed by
`ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
passes with the preserved Step 2 repair and the parked 802 selector hunk. When
the 802 hunk alone was temporarily rolled back, that same focused test failed
at `LirSwitch.selector_type_ref`'s selector assertion; the hunk was restored.
This proves the 801 repair now reaches, but does not accept or own, the 802
Step 1 selector-verifier gate.

Classification: `separate-blocker`. Reactivate
`ideas/open/802_lir_switch_selector_type_reference_verifier.md` at unchanged
Step 1 to evaluate only its parked switch verifier/test hunk. The unrelated
754 aggregate-use authority changes and tests in the mixed working tree remain
unaccepted and unowned by both this 801 resumption and 802.

Exact return point: after 802 accepts its bounded selector-type-reference
repair and proof, reactivate 801 at unchanged Step 2. Preserve Step 1's
acceptance and the current unaccepted Step 2 repair; complete its required
fresh build, focused call/frontend/backend checks, and supervisor-accepted
full baseline before Step 3. Do not repeat Step 1 or claim the narrow focused
proof as 801 acceptance.

## Resumption Update: accepted 802 selector return

802 accepted its bounded selector verifier repair in `8218993a5`. The repair
requires a valid structured `selector_type_ref` before comparing its integer
width to the selector-selected definition; nearby coverage proves matching
forms and rejection of missing, non-integer, stale-width, and same-width
incoherent references. Fresh accepted proof was `cmake --build --preset
default && ctest --test-dir build -j --output-on-failure -R
'^frontend_lir_call_type_ref$'`, with matching regression-guard
`test_before.log` / `test_after.log` results.

801 is reactivated at unchanged Step 2, `Repair anonymous layout / structured-call
compatibility`. Step 1 remains the only accepted 801 progress (`827dae5bd3`).
The preserved `args.cpp`, `target.cpp`, and `verify.cpp` LirCallOp/InsertValue
hunks and the related `frontend_hir_tests.cpp` work remain unaccepted and must
not be modified, credited, or broadened by this return. Complete Step 2's
direct-complex call-signature/argument-mirror contract and required focused
call/frontend/backend proof plus supervisor-accepted full baseline before
Step 3; 802's proof does not satisfy that gate.

## Resumption Update: GEP producer-authority baseline blocker

Last accepted progress remains Step 1, `Trace and select anonymous aggregate
layout facts`, in `827dae5bd3`. Step 2, `Repair anonymous layout /
structured-call compatibility`, remains interrupted and unaccepted. The
preserved working-tree `args.cpp`, `target.cpp`, `verify.cpp`, and
`frontend_hir_tests.cpp` repair is reviewed as scope-consistent but is neither
accepted nor committed.

The required fresh build and focused ladder passed:
`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^(frontend_hir_tests|frontend_lir_call_type_ref|backend_)$'`
(2/2). The required full-baseline attempt then exposed 115
`LirGepOp.result: authoritative GEP requires LirValueId result authority`
frontend failures, before 801 Step 2 could be accepted. The root
`test_after.log` is partial due to execution-environment timing; because its
clean-worktree baseline could not include external assets and had no summary,
it is not a matching regression guard and cannot establish a regression or
acceptance. One residual PHI failure remains owned by the existing 804/806
chain, not this switch.

Classification: `separate-blocker`. The LIR GEP producer-authority failure
family is outside anonymous aggregate layout/structured-call compatibility and
has no existing open owner. `ideas/open/810_lir_gep_producer_result_authority_baseline_blocker.md`
owns the bounded trace and, only if evidence supports it, repair of that GEP
producer handoff.

Exact return point: after 810 has an accepted repair/proof sufficient to
reattempt the full baseline, reactivate 801 unchanged at Step 2. Preserve the
unaccepted Step 2 repair; then rerun the required fresh build, focused
call/frontend/backend ladder, and supervisor-accepted comparable full baseline
before Step 3. Do not repeat Step 1 or credit the preserved changes as
accepted progress.

## Resumption Update: accepted 810 GEP baseline return

810 is capability-complete: its accepted Steps 1--2 remain in `f1cb9c510`
and `1f1a1fb38`, and the supervisor accepted its exact fresh comparable gate
on 2026-07-15 at 15:51 UTC:
`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure` passed 3037/3037. The intervening 795 and 796 handoffs
were accepted in `281737387` and `387af7745`.

801 now resumes unchanged at Step 2, `Repair anonymous layout / structured-call
compatibility`. Step 1 remains the only accepted 801 progress (`827dae5bd3`).
The preserved dirty `args.cpp`, `target.cpp`, `verify.cpp`, and
`frontend_hir_tests.cpp` repair remains unaccepted and uncommitted; the next
action is to evaluate that repair strictly within the existing Step 2 scope,
then obtain its required fresh build, focused call/frontend/backend ladder,
and supervisor-accepted comparable full baseline before Step 3.

## Resumption Update: recurring cast-result authority blocker

Last accepted progress remains only Step 1, `Trace and select anonymous
aggregate layout facts`, in `827dae5bd3`. Step 2, `Repair anonymous layout /
structured-call compatibility`, remains unaccepted. Its preserved dirty
`args.cpp`, `target.cpp`, `verify.cpp`, and `frontend_hir_tests.cpp` repair is
native anonymous-layout/direct-complex structured-call work; it remains in the
working tree, uncommitted, and uncredited.

The fresh 801 focused build/subset passed 7/7, but the required comparable
full-tree attempt exposed `LirCastOp.result: expected operand kind mismatch
... got raw-text` across positive, LLVM, and c-testsuite coverage. The
attempted `test_before.log` was not a usable clean-first baseline (its tree
and artifacts were not clean-first and it recorded 24 failures), while the
foreground-capped `test_after.log` ended before its summary. Neither log is a
matching regression guard, and the rejected stale `test_baseline.new.log`
candidate must not be used. Therefore no full-gate result accepts Step 2.

Classification: `separate-blocker`. This raw-text cast-result authority family
is owned by the existing open
`ideas/open/796_lir_instruction_terminator_residual_authority_handoff.md`; it
is outside anonymous layout facts and direct-complex structured-call
compatibility. 801 must not repair, weaken, or reclassify cast verification.

Exact return point: after 796 accepts a bounded repair or separately routes
the re-exposed cast-result family, and after a fresh comparable clean baseline
permits it, reactivate 801 at unchanged Step 2. Preserve the current
unaccepted structured-call repair; rerun the required fresh build, focused
call/frontend/backend ladder, and supervisor-accepted comparable full gate
before Step 3. Do not repeat Step 1 or claim the 7/7 subset as Step 2
acceptance.

## Progress Update: accepted Step 2; Step 3 handoff pending

Step 2, `Repair anonymous layout / structured-call compatibility`, is
supervisor-accepted. The direct-complex by-value call now shares one native
anonymous `{ float, float }` layout across its callee signature,
`arg_type_refs`, and `structured_args`. Recursive anonymous-layout validation
and rejection of incoherent named structured `LirCallOp` carriers remain
fail-closed; no compatibility-text parsing, weakened contracts, extractvalue
validation, or Raw-BIR work was accepted.

The fresh focused command
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_lir_call_type_ref|frontend_lir_function_signature_type_ref|backend_)$'`
passed 3/3. The supervisor's fresh comparable full command completed 3035/3037
with only pre-existing `cpp_qualified_template_call_template_arg_perf` and the
806-owned `llvm_gcc_c_torture_src_20060910_1_c` failure; the former baseline
failure `llvm_gcc_c_torture_src_pr28982b_c` no longer fails, and the failure
set did not expand.

801 advances to Step 3, `Prove and publish the 754 handoff`. The receiver is
`ideas/open/754_lir_aggregate_vector_value_identity_convergence.md` at its
unchanged Step 3. This records only a prospective native field-layout contract
for 754 to consume: it does not begin extractvalue-row work and does not claim
that 754's handoff is accepted.

## Completion Record: accepted 754 layout-fact handoff

801 is capability-complete. Step 2 remains accepted as recorded above:
the direct-complex by-value `LirCallOp` has one native anonymous `{ float,
float }` layout, ordered `float`, `float`, shared structurally by its callee
signature, `arg_type_refs`, and `structured_args`; recursive layout checking
and malformed or incoherent native structured-call carriers reject fail
closed.

Step 3 is accepted on the supervisor-provided fresh proof:
`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^(frontend_hir_tests|frontend_lir_call_type_ref|frontend_lir_function_signature_type_ref)$'`
passed 3/3 (`frontend_hir_tests`, `frontend_lir_call_type_ref`, and
`frontend_lir_function_signature_type_ref`). This publishes only the native
anonymous field-layout/type-fact handoff. It performs no `LirExtractValueOp`
field/index/result validation and does not accept 754's receiver work.

Disposition: close 801 as capability-complete and resume
`ideas/open/754_lir_aggregate_vector_value_identity_convergence.md` at its
unchanged Step 3, `Verify row-specific index or mask facts`.
