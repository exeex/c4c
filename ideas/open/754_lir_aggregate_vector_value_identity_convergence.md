# LIR Aggregate And Vector Value Identity Convergence

Status: Open
Type: bounded LIR aggregate/vector value authority repair
Predecessor: `ideas/open/753_lir_memory_va_pointer_authority_convergence.md`

## Goal

Publish structured result/use authority for aggregate and vector LIR operations
whose values, indices, masks, or element carriers still rely on `%t` and raw
presentation strings.

## Why This Exists

Scalar call/binop/cmp/cast/select work has begun using `fresh_value` and
`LirOperand` authority, but aggregate and vector producers remain largely
text-based. These rows are separate from pointer/object work because they need
opcode-specific aggregate, element, index, and mask validation in addition to
ordinary value ownership.

## In Scope

- Convert representative `LirExtractValueOp`, `LirInsertValueOp`,
  `LirInsertElementOp`, `LirExtractElementOp`, and `LirShuffleVectorOp`
  producers to structured result/use authority.
- Add exact type/index/mask validation needed for those rows.
- Preserve compatibility rendering while preventing result/use recovery from
  display strings.
- Add focused coverage for aggregate extraction/insertion and vector
  insert/extract/shuffle chains, including malformed value/type/index/mask
  cases.

## Out Of Scope

- CFG/PHI predecessor identity, local/object pointer authority, memory/va
  pointer semantics, Raw-BIR receiver work, target lowering, MIR, or emission.
- Opaque inline-asm assembly/constraint text.
- Treating instruction order, `%t` spelling, rendered LLVM, or testcase names
  as identity.

## Acceptance Criteria

- Representative aggregate and vector rows publish structured result/use
  identity and exact row-specific typed facts.
- The verifier rejects invalid result IDs, unknown/cross-function uses, and
  type/index/mask conflicts.
- Misleading display text cannot repair or select aggregate/vector identity.
- Full baseline acceptance requires 100% passing tests. If a baseline run is
  below 100%, reject closure and trace `log/*` by time/commit to identify the
  first bad commit before continuing.

## Resumption Record: Step 2 operand-provenance blocker satisfied

Prior accepted progress: Step 1, `Audit and select one aggregate/vector
authority row`, selected only `LirExtractValueOp` and was committed as
`d8e5ed3a8`. Its accepted audit/proof references are the root
`test_before.log` and `test_after.log` backend baseline/prototype results
(5/5). The earlier prototype was rejected; the later accepted Step 2 is
recorded below.

Interrupted step: Step 2, `Publish structured result and operand authority`.
The proposed minimal `LirExtractValueOp` schema/lowering route was explicitly
rejected and reverted after its backend proof passed 5/5: actual HIR
`extractvalue` producers pass SSA aggregate values as `std::string`, discarding
the originating `LirValueId` before `LirExtractValueOp` construction. A row
local field cannot reconstruct that use identity without forbidden display-text
recovery.

Classification: `separate-blocker`. No already-open idea owned this
operand-provenance prerequisite; now-closed
`ideas/closed/798_lir_operand_provenance_authority_publication.md` owned only
the needed native `LirOperand`/expression-API provenance propagation and its
producer/verifier handoff. It did not publish the `LirExtractValueOp` row, edit
Raw BIR, or recover identity from text.

Satisfied handoff: 798 completed its trace, implementation, and proof at
`4c6865e62`, `f9fa478fb`, and `2c231e342`. Its accepted contract is: a
selected direct composite call creates `LirOperand::ssa(display, LirValueId)`;
unary real/imag forwards that exact operand into `LirExtractValueOp.agg`; the
verifier requires a valid current-function call-result ID, structured
return/aggregate type equality, and a display mirror. Missing, unknown/foreign,
stale-display, and type-incoherent forms reject. There is no text recovery and
no extractvalue result/index schema work in 798.

Accepted proof: fresh build plus `ctest --test-dir build -j
--output-on-failure -R '^(backend_|frontend_hir_tests$)'` passed 6/6 before
and after; the monotonic guard accepted the equal 6/6 result.

Former return point: Step 2 is now accepted and must not be repeated; its
accepted implementation/proof is recorded in the current resumption record.

## Resumption Record: Step 3 anonymous aggregate layout blocker

Last accepted progress: Step 1 is accepted in `d8e5ed3a8`. Step 2, `Publish
structured result and operand authority`, is accepted in `da07100d0`: the
selected direct-complex path publishes the opt-in native extractvalue result
and consumes the closed-798 checked aggregate operand. A fresh build plus
`ctest --test-dir build -j --output-on-failure -R
'^(backend_|frontend_hir_tests$)'` passed 6/6 and the monotonic guard passed.
No Step 3 code or tests changed.

Interrupted step: Step 3, `Verify row-specific index or mask facts`.
The selected direct-complex extract has `agg_type = LirTypeRef("{ float,
float }")`, whose compatibility text/kind carries no native anonymous
composite field list. Exact index bounds and result element type cannot be
validated without forbidden text parsing or a structured anonymous aggregate
layout/fact model.

Classification: `separate-blocker`. New open
`ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md` owns only the
native anonymous aggregate field-layout/type model prerequisite. It must not
publish extractvalue-row validation, Raw-BIR work, or text parsing.

Exact return point: after 801 publishes checked native anonymous aggregate
field-layout/type facts, reactivate 754 at unchanged Step 3 and validate only
`LirExtractValueOp` field/index/result coherence. Do not repeat Steps 1 or 2;
then continue Step 4.

## Resumption Update: Step 2 aggregate-use authority repair

Historical status: Step 1 remains accepted in `d8e5ed3a8`. Step 2 was
previously accepted in `da07100d0` on its focused 6/6 proof, but a fresh
full-baseline attempt while 801 was active exposes that its aggregate-use
authority contract is not valid for all existing aggregate SSA uses:
`LirExtractValueOp.agg: aggregate SSA operand requires valid LirValueId
authority`. The candidate baseline is 2989/3037 against the accepted
3037/3037 baseline, with 47 new failures. Therefore the old Step 2 acceptance
is not sufficient to permit Step 3; treat it as a bounded repair/resumption,
not completed progress to repeat or silently bypass.

Return point: repair and reprove Step 2, `Publish structured result and
operand authority`, for aggregate SSA uses within this idea's selected
`LirExtractValueOp` scope. Retain the established no-text-recovery rule and
do not absorb anonymous layout/index/result-type validation (801/Step 3),
Raw-BIR, or other aggregate/vector rows. The repair must explain and cover
the valid `LirValueId` authority boundary rather than weakening the verifier.
Only after supervisor acceptance, including a 100% full baseline, may 754
continue at its preserved Step 3.

Incoming switch context: 801 Step 1 is accepted in `827dae5bd3`; its Step 2
argument-mirror repair is preserved in the working tree but remains
unaccepted. The aggregate-use failure is outside 801. On completion of this
754 repair, return to 801 Step 2 exactly as recorded there.

## Resumption Update: aggregate SSA producer-authority prerequisite

The reopened Step 2 diagnosis establishes that the required authority is not
limited to the historical selected direct composite call. The failing runtime
and direct-conjugate paths extract from valid non-call aggregate SSA values:
a local load and a constructed `LirInsertValueOp` result. Marking those
extracts as raw moves the runtime failure to `aggregate SSA authority must
select a matching current-function call result type`; permitting a raw fallback
would evade the authority rule and is prohibited.

Classification: `separate-blocker`. 754's historical Step 2 contract and
closed 798 handoff both expressly publish only direct-composite-call
provenance. Extending native provenance across local-load and constructed
aggregate producer boundaries is a producer/operand prerequisite, not a
row-local extractvalue repair; absorbing it here would violate the source's
reject signal against generic operand/expression provenance. New open
`ideas/open/803_lir_aggregate_ssa_producer_authority_publication.md` owns the
bounded prerequisite.

Exact return point: after 803 publishes and proves native `LirValueId`
authority for the selected aggregate local-load and constructed-insertvalue
producer paths, reactivate 754 at unchanged Step 2. Then repair only
`LirExtractValueOp` result/use authority using that accepted handoff; do not
begin Step 3 layout/index/result-type work or repeat Step 1.

## Resumption Update: 803 aggregate producer-authority prerequisite satisfied

803 is capability-complete and closed. Its accepted implementation is
`3d2e8ddd1` (`lir: publish aggregate producer authority`), with Step 3 proof
recorded in `17d221ccb`: selected current-function aggregate local-load and
terminal constructed-`LirInsertValueOp` producers retain native IDs through
the immediate extract path; their aggregate carrier/type equality and display
mirror are checked. Missing, foreign, stale-display, unselected, and
type-incoherent authority rejects, while legacy extracts remain ungated.

Accepted proof is a fresh build; matching canonical guard logs from a 6/8
baseline (the runtime and direct-complex positives failed) to 8/8 after, guard
PASS; and `^backend_` 5/5. This is prerequisite proof, not 754 Step 2's
required full-baseline acceptance.

Exact return point: reactivate 754 at unchanged Step 2, `Repair structured
result and aggregate operand authority`. Consume 803 only to repair the
selected `LirExtractValueOp` result/use contract. Do not repeat Step 1 or
begin Step 3 index/layout/result validation; retain the Step 2 fresh-build,
focused-proof, and supervisor-owned 100% full-baseline gate.

## Resumption Update: full-baseline PHI authority blocker

Last accepted progress: Step 1 remains accepted in `d8e5ed3a8`. The 803
prerequisite is capability-complete at `3d2e8ddd1`, with its accepted Step 3
proof recorded in `17d221ccb`. No Step 2 implementation change was accepted
after this route resumed.

Interrupted step: Step 2, `Repair structured result and aggregate operand
authority`.

Blocker evidence: before any Step 2 implementation, a clean-HEAD build
succeeded, but `ctest --test-dir build -j --output-on-failure` stopped at
1447/3037 on `llvm_gcc_c_torture_src_vrp_2_c`. The failure is
`LirPhiIncoming.value: must identify a known current-function LirValueId`.
`git blame` attributes the verifier enforcement to `6ece9fe8f` (`Publish typed
LIR PHI incoming authority`); this is evidence only, not a root-cause claim.

Classification: `separate-blocker`. CFG/PHI authority is expressly outside
this idea. Open `ideas/open/804_lir_phi_incoming_producer_authority_repair.md`
owns the narrow producer-handoff diagnosis and repair needed to restore the
mandatory full-baseline gate without reopening accepted CFG/PHI work.

Exact return point: after 804 has an accepted bounded producer-side repair and
the supervisor accepts a 100% full baseline, reactivate 754 at unchanged Step
2. Then perform only the selected `LirExtractValueOp` result/aggregate-operand
authority repair using 798/803; do not repeat Step 1, begin Step 3, or absorb
PHI work.

## Reviewer Reject Signals

- Reject recovery of aggregate-use identity from `%t`, `std::string`, printer
  output, rendered LLVM, instruction order, or testcase names.
- Reject absorbing generic operand/expression provenance into this aggregate or
  vector row idea; that prerequisite belongs to 798.
- Reject expectation weakening, named-case-only behavior, or accepting the
  reverted prototype as structured-authority progress.

## Resumption Record: Step 8 native vector authority blocker

Last accepted progress: Steps 1--4 completed the selected `LirExtractValueOp`
route.  Step 5 selected the terminal direct-complex `LirInsertValueOp` route
in `270c6a93e`; Step 6 completed it in `8fe6c3569`; and Step 7 correctly
rejected source closure.  The accepted implementation retains the matching
`^backend_` regression guard at 5/5 and the supervisor-accepted 3037/3037 full
baseline.  Those accepted steps and proofs must not be repeated.

Interrupted step: Step 8, `Audit and select one remaining vector authority
row`.

Blocker evidence: `LirInsertElementOp` splat lowering emits raw `fresh_tmp`
results, `std::string` elements, rendered vector types, and the display index
`"i64 0"`; vector `IndexExpr` emits `LirExtractElementOp` result/vector/index
through `fresh_tmp`, `emit_rval_id`, and `coerce` strings; and splat
`LirShuffleVectorOp` emits raw result/input strings plus rendered mask type and
`poison`/`zeroinitializer` display tokens.  The row schemas have no opt-in
authority fields, and the verifier only checks operand kinds and type-reference
well-formedness.  `LirOperand` supports `LirValueId`, but these seams do not
populate it; `LirTypeRef` has no native vector layout or mask-lane facts.

Classification: `separate-blocker`.  New open
`ideas/open/811_lir_native_vector_authority_carrier_publication.md` owns only
the reusable vector carrier prerequisite: current-function result/use IDs,
vector lane/element facts, index value/type facts, and shuffle mask-lane facts.
It must not implement an 754 row, recover authority from display text, or
widen into non-vector provenance.

Exact return point: after 811 publishes and proves that carrier handoff,
reactivate 754 at Step 9, `Implement and prove the Step 8 selection`, and
first rerun a fresh one-row vector audit against the handoff.  Select exactly
one row then; do not repeat Steps 1--7 or the accepted insertvalue route.

## Resumption Update: accepted 801 native-layout handoff

801 is capability-complete and has published its accepted Step 2--3 native
anonymous aggregate layout handoff. For the selected direct-complex carrier,
the callee signature, `arg_type_refs`, and `structured_args` structurally share
one checked `{ float, float }` layout with ordered `float`, `float` fields;
malformed or incoherent native structured `LirCallOp` carriers reject fail
closed. Its fresh focused proof
`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^(frontend_hir_tests|frontend_lir_call_type_ref|frontend_lir_function_signature_type_ref)$'`
passed 3/3.

Resume unchanged at Step 3, `Verify row-specific index or mask facts`. Consume
only those native field-layout/type facts to validate the selected
`LirExtractValueOp` field/index/result coherence. Do not repeat Steps 1--2,
claim 801 as extractvalue-row validation, widen to other rows, or recover any
fact from display text.

## Resumption Update: Step 4 full-baseline PHI residual blocker

Last accepted progress: Steps 1--2 remain accepted in `d8e5ed3a8` and
`da07100d0`; 801's native-layout handoff remains accepted. Step 3, *Verify
row-specific index or mask facts*, is accepted in `a351cde2a` (`lir: verify
extractvalue field coherence`). It verifies the selected direct-complex
`LirExtractValueOp` result-element type against 801's ordered native anonymous
aggregate fields. The Step 4 bookkeeping checkpoint is `c01affeba`.

Interrupted step: Step 4, *Prove and hand off the bounded row*.

Accepted proof and observed baseline: fresh selected same-feature proof
`frontend_hir_tests|frontend_lir_call_type_ref|frontend_lir_function_signature_type_ref`
passed 3/3. The initial `^backend_` checkpoint exposed a test-fixture issue;
after the test-only `d69501785`, it passed 5/5. A fresh full command
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure`
completed 3036/3037 with the sole failure
`llvm_gcc_c_torture_src_20060910_1_c`:
`LirPhiIncoming.value: must identify a known current-function LirValueId`
(supervisor log `/tmp/754_step4_full.log`). Therefore Step 4 has not met its
required 100% full-baseline closure gate.

Classification: `separate-blocker`. This PHI producer-family failure is
outside 754's aggregate/vector scope and is not claimed to be caused by its
accepted Step 3. `ideas/open/804_lir_phi_incoming_producer_authority_repair.md`
already records the residual PHI family successor
`ideas/open/806_lir_phi_residual_producer_family_authority_trace.md`; 806 owns
tracing and repairing its remaining producer cases.

Exact return point: after 806 has an accepted same-family repair and the
supervisor accepts a 100% full baseline, reactivate 804 at its unchanged Step
3 as its source directs. Then return through that recorded dependency chain to
754 Step 4's full-baseline/closure gate. Do not repeat Steps 1--3, absorb PHI
producer work, or claim source closure from the partial baseline.

## Resumption Update: 804 blocker closure returns Step 2

804 is capability-complete and archived. Its accepted scalar unary-minus
repair is `308fff39c`; the closed residual chain is postfix `961ce9fda`,
`fneg` `8f31e2535`, `xor` `b86df3b9d`, and scalar dereference-load
`4d29f7b3e`. The supervisor accepted a fresh full baseline at 3037/3037 from
`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure` (build had no work; about 29.11s).

Exact return point: resume unchanged at Step 2, *Repair structured result and
aggregate operand authority*. Do not repeat Step 1, 804/806 work, or their
accepted baseline proof. This resumption follows the explicit current parent
return decision; it does not begin Step 3/4 work or absorb PHI scope.

## Resumption Update: Step 2 repair accepted; proceed to Step 3

Last accepted progress: Step 1 remains accepted in `d8e5ed3a8`. The repaired
Step 2, *Repair structured result and aggregate operand authority*, is accepted
in `33a6c21cc`: selected `LirExtractValueOp` result-authority coverage rejects
missing and cross-function result IDs while result display spelling remains
non-authoritative. The accepted aggregate-operand handoffs remain limited to
798's direct-composite path and 803's local-load / terminal-insertvalue paths.

Accepted proof: the focused selected-row proof passed 6/6. A fresh build had
no work; matching full CTest before/after captures each passed 3037/3037; and
the monotonic guard passed with `--allow-non-decreasing-passed` for the equal
repeat capture. This satisfies Step 2's required supervisor-owned full
baseline gate.

Exact return point: proceed to Step 3, *Verify row-specific index facts*.
Consume only 801's accepted native anonymous aggregate layout/type handoff to
validate the selected `LirExtractValueOp` field-index bounds and result-element
type coherence. Do not repeat Steps 1--2, publish layout facts, widen to other
aggregate/vector rows, reopen PHI work, or recover facts from display text.

## Resumption Update: Step 3 index/result coherence accepted

Step 3, *Verify row-specific index facts*, is accepted in `97137f39d`.
The bounded selected direct-complex `LirExtractValueOp` coverage consumes
801's already-accepted native anonymous-layout facts and rejects index `-1`,
index `2`, and an `i32` result type, while retaining the nearby valid form.
This is selected-row validation only: it neither publishes generic layout
facts nor widens to any other aggregate/vector row or display-text recovery.

Accepted proof: the fresh focused target build and
`ctest --test-dir build --output-on-failure -R '^frontend_lir_call_type_ref$'`
passed 1/1 before and after; the matching regression guard passed with
`--allow-non-decreasing-passed`.

Exact return point: proceed to Step 4, *Prove and hand off the bounded row*.
The supervisor must obtain and accept a 100% full baseline, then record the
one-row handoff. If the baseline is not 100%, preserve an executable repair
route without reopening Steps 1--3 or absorbing out-of-scope work.

## Resumption Update: semantic closure rejected; continue remaining source rows

The former four-step `LirExtractValueOp` runbook is exhausted, and its Step 4
closure proof is accepted: fresh `cmake --build --preset default && ctest
--test-dir build -j --output-on-failure` before and after each passed
3037/3037 (29.30s/30.72s); root `test_before.log` and `test_after.log` are the
canonical captures; the matching guard passed with
`--allow-non-decreasing-passed`.

Closure disposition: **close rejected**. The source acceptance criterion that
representative aggregate and vector rows publish structured result/use identity
and exact row-specific typed facts is unmet for `LirInsertValueOp`,
`LirInsertElementOp`, `LirExtractElementOp`, and `LirShuffleVectorOp`. The
associated malformed value/type/index/mask coverage is therefore also unmet.
The full-baseline criterion is satisfied for the completed extractvalue route,
but it cannot substitute for the unimplemented representative rows.

Classification: `repair-current-route`. These rows are expressly in this
source's scope, so no successor is required. The repaired runbook resumes at
Step 5, *Audit and select the next remaining representative row*; it must
select exactly one row and preserve the no-display-recovery and no-generic-
provenance boundaries. A discovered prerequisite outside this source must be
recorded as a separately scoped blocker with this parent return point.

## Resumption Update: 811 native vector carrier blocker accepted

811 is capability-complete and is archived as
`ideas/closed/811_lir_native_vector_authority_carrier_publication.md`. Its
accepted implementation is `76f92ad60` (*Publish native vector authority
carrier*) and its carrier-focused coverage is `56203cbf9` (*Cover native
vector authority carrier*). The handoff publishes checked current-function
result/use IDs, native vector lane/element facts, checked insert/extract index
value/type facts, and exact ordered shuffle mask-lane facts for the three
existing vector seams. Missing, foreign, malformed, and incoherent carriers
reject; rendering remains a compatibility mirror and is never recovered as
authority. 811 does not select, enable, or validate an insert/extract/shuffle
row.

Accepted proof: a fresh build plus matching `^backend_` canonical regression
captures passed from 5 to 6 tests with zero failures; the supervisor accepted
the monotonic guard. The closure-quality fresh
`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure` checkpoint also passed 3038/3038. This prerequisite proof
is a handoff, not 754 vector-row proof.

Exact return point: resume at Step 9, *Implement and prove the Step 8
selection*. Before selecting or changing a row, rerun a fresh one-row vector
audit against 811's carrier and record the selected seam, positive/malformed
matrix, and two excluded rows in `todo.md`. Select exactly one of
`LirInsertElementOp`, `LirExtractElementOp`, or `LirShuffleVectorOp` only when
the carrier supports its complete row-local contract. Do not repeat Steps
1--8, repeat 811 publication work, or implement a vector row merely because
the carrier exists.

## Resumption Update: Step 9 shuffle poison-second-shape blocker

Last accepted progress: Steps 1--8 remain accepted. Step 9 completed its required fresh audit and selected only the scalar-to-vector splat `LirInsertElementOp` seam. `LirExtractElementOp` was excluded because it has no row-local result-element type; `LirShuffleVectorOp` was excluded because its mask semantics are not row-specific. No Step 9 implementation is accepted.

Interrupted step: Step 9, *Implement and prove the Step 8 selection*.

Blocker evidence: the rejected candidate packet's fresh build and selected `^backend_` proof passed 6/6, and its matching regression guard remained 6 to 6. The supervisor's fresh full command `cmake --build --preset default && ctest --test-dir build -j --output-on-failure` completed 3037/3038, with the sole failure `llvm_gcc_c_torture_src_scal_to_vec1_c`: `LirShuffleVectorOp.native_vector_authority.second_vector_shape: must be present`. The existing unselected shuffle splat lowering emits a native carrier without the required second-vector shape when its second operand is poison.

Classification: `separate-blocker`. New open `ideas/open/814_lir_shuffle_vector_poison_second_shape_carrier_repair.md` owns only the native carrier/lowering repair, structured poison-second-operand treatment, and nearby valid/malformed coverage. It must not claim a 754 row capability, select shuffle semantics, or widen into aggregate, ExtractElement, InsertElement, generic provenance, CFG/PHI, target/MIR/emission, or parse display text.

Exact return point: after 814 has an accepted narrow repair and the supervisor accepts a 100% fresh full baseline, reactivate 754 at unchanged Step 9. Reuse the completed audit but make a fresh row selection/proof decision. Do not repeat accepted Steps 1--8 or silently implement ExtractElement/ShuffleVector as a 754 row.

## Resumption Update: 814 poison-second-shape and mask-lane handoff accepted

814 is capability-complete and archived as
`ideas/closed/814_lir_shuffle_vector_poison_second_shape_carrier_repair.md`.
Its accepted second-shape repair is `c1cde8430`; its returned mask-lane
prerequisite is closed 815's `510388751`, recorded by `e97445276`. The
supervisor accepted the fresh build, matching `^backend_` guard at 6/6 with no
new failures, representative `scal-to-vec1.c` LLVM emission, and a fresh full
baseline at 3038/3038. This proves only prerequisite coherence, not any 754
row capability.

Exact return point: resume unchanged at Step 9, *Implement and prove the Step
8 selection*. Steps 1--8 remain accepted. Before implementation, make a fresh
one-row vector audit and selection/proof decision using the accepted carrier;
record the selected seam, positive/malformed matrix, and two excluded rows in
`todo.md`. Do not reuse the rejected scalar-to-vector `LirInsertElementOp`
selection, silently choose `LirExtractElementOp` or `LirShuffleVectorOp`, or
repeat the completed Steps 1--8.
