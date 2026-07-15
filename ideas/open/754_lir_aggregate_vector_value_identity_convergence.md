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

Last accepted progress: Step 1, `Audit and select one aggregate/vector
authority row`, selected only `LirExtractValueOp` and was committed as
`d8e5ed3a8`. Its accepted audit/proof references are the root
`test_before.log` and `test_after.log` backend baseline/prototype results
(5/5). No Step 2 implementation is accepted.

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

Exact return point: reactivate 754 at unchanged Step 2, `Publish structured
result and operand authority`, and consume only the published checked aggregate
operand carrier to add the minimum opt-in `LirExtractValueOp` structured result
and aggregate-use authority. Wire existing HIR producers through the preserved
provenance and retain legacy rows as compatibility-only/fail-closed. Then
continue Steps 3 and 4; do not repeat Step 1 or treat the reverted 5/5
prototype as accepted implementation proof.

## Reviewer Reject Signals

- Reject recovery of aggregate-use identity from `%t`, `std::string`, printer
  output, rendered LLVM, instruction order, or testcase names.
- Reject absorbing generic operand/expression provenance into this aggregate or
  vector row idea; that prerequisite belongs to 798.
- Reject expectation weakening, named-case-only behavior, or accepting the
  reverted prototype as structured-authority progress.
