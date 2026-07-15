# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Implement and prove the selected remaining row

## Just Finished

Step 5 audit selected exactly one next row: the terminal direct-complex
`LirInsertValueOp` producer emitted by
`StmtEmitter::emit_complex_binary_arith` in
`src/codegen/lir/hir_to_lir/expr/binary.cpp` (the
`require_direct_aggregate_ssa` branch).  The existing structured seam is
`LirInsertValueOp.result : LirOperand::ssa` plus
`requires_native_result_authority` and `aggregate_result_type`, constructed
at the terminal insert and checked by `verify_insert_value_authority`; the
same result/type pair is consumed as the selected aggregate producer in the
current-function definition check for `LirExtractValueOp.agg`.

This selection does not reopen accepted `LirExtractValueOp` work.  It is a
single producer-row extension: native result ownership and aggregate result
type are already factual, while terminal insert's row-local aggregate/index/
element coherence remains to be published and proved.  No identity is to be
recovered from the terminal result text, `with_real`, `out_imag`, or rendered
LLVM.

## Suggested Next

Step 6: implement only the selected terminal direct-complex
`LirInsertValueOp` row.  Keep its existing current-function result ID and
aggregate-result-type handoff, then add/check only native row-local facts
needed to prove aggregate result/type, selected index, and inserted element
coherence.  The positive/malformed matrix is:

- Positive: a terminal direct-complex insert with a valid current-function
  result ID, matching aggregate result/type, valid selected field, and an
  element matching that field; retain the existing immediate extract consumer
  as a compatibility consumer, not as the source of identity.
- Malformed: missing/unknown/foreign terminal result ID; missing or
  aggregate-type-conflicting result fact; negative/out-of-range index;
  selected-field/element type conflict; stale display paired with an otherwise
  valid ID, which must not repair or select authority.

Excluded and unchanged: `LirInsertElementOp`, `LirExtractElementOp`, and
`LirShuffleVectorOp`.  Their lowering paths carry vector result/vector/index/
mask values through unselected raw `LirOperand` presentations and expose no
row-local native vector lane/layout authority.  Making those paths authoritative
would require a separate generic vector producer/use or vector-layout
publication family; classify that prerequisite as `separate-blocker` if it is
needed later, rather than extending this packet.  Likewise, do not follow the
selected insert's `with_real` or `out_imag` strings into generic binop/extract
provenance; that is outside this row-local contract.

## Watchouts

Do not revisit accepted `LirExtractValueOp` work, recover identity from display
text, weaken contracts, absorb generic binop/extract provenance or generic
vector-layout publication, or claim the source complete before all
representative aggregate/vector rows have accepted bounded coverage.

## Proof

The accepted 754 full-baseline proof is in root `test_before.log` and
`test_after.log`; each reports 3037/3037 passed. New code packets require a
fresh build, nearby same-feature proof, and matching regression guard. A later
full baseline is required before source closure.
