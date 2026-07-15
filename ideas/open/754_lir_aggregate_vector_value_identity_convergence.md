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

## Reviewer Reject Signals

- Reject recovery of aggregate-use identity from `%t`, `std::string`, printer
  output, rendered LLVM, instruction order, or testcase names.
- Reject absorbing generic operand/expression provenance into this aggregate or
  vector row idea; that prerequisite belongs to 798.
- Reject expectation weakening, named-case-only behavior, or accepting the
  reverted prototype as structured-authority progress.
