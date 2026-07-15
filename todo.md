# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 14
Current Step Title: Implement and prove the selected InsertElement row

## Just Finished

Step 13 audit accepted in `21791ae14` and selected exactly one row: the scalar-to-vector splat
`LirInsertElementOp` precursor emitted by the two `emit_splat[_vec]` helpers in
`src/codegen/lir/hir_to_lir/expr/binary.cpp`. This is a fresh, complete seam,
not a relabel of the rejected pre-811 poison seed: each producer now emits a
native result ID, poison vector token, coerced scalar element (with optional
native element-use ID), immediate index value `0`, native `i64` index type, and
matching result/first-vector lane-and-element shapes through the accepted 811
carrier. The current generic carrier verifier already validates owner,
result/vector/element/index identity, defined value uses, immediate index
authority, and shape mirrors; it does not yet make this row mandatory or compare
`elem_type` with the structured vector element type.

Step 14 is current. Its contract is to add a row-local opt-in requirement on those two producer
instances only, then require their carrier and require `elem_type` to equal the
carrier result/first-vector element type, with the native index type exactly
`i64`. Keep raw/other InsertElement forms unselected. Positive coverage: actual
scalar-to-vector lowering (including `scal-to-vec1`) and a selected immediate-
zero/i64 InsertElement carrier with both SSA and immediate/coerced scalar
elements. Malformed coverage: selected row missing carrier; bad/foreign/undefined
result, vector, or element IDs; missing or mismatched result/first-vector shapes;
missing, mismatched, or undefined index authority; non-`i64` index type; and
`elem_type`/structured-element mismatch. Existing generic InsertElement carrier
coverage remains the nearby base for the identity, shape, and index mutations.

## Suggested Next

Implement and prove only the selected scalar-to-vector splat InsertElement
contract. Add nearby selected-row positives/malformed mutations without
generalizing unselected InsertElement producers, ExtractElement, or
ShuffleVector.

## Watchouts

The accepted ShuffleVector precursor is a strict boundary, not InsertElement
authority: it may continue to require its immediately preceding native insert
result, but InsertElement must independently opt in and verify its own
result/vector/element/index/type facts even with no following shuffle. Preserve
the accepted shuffle mask and adjacency checks unchanged. Do not recover any
fact from `poison`, rendered vector text, or the index display string.

## Proof

Audit only; no build or tests run. `git diff --check -- todo.md` is required
before handoff.
