# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 9
Current Step Title: Implement and prove the Step 8 selection

## Just Finished

Step 9 audit selected only the scalar-to-vector splat `LirShuffleVectorOp`
seam in `src/codegen/lir/hir_to_lir/expr/binary.cpp` (both local
`emit_splat[_vec]` constructions) with its verifier seam in
`verify_vector_inst`. This is a complete row-local structured contract for
the zero-initializer splat form: a fresh current-function result ID, the
first-vector use equal to the preceding native insert result, a structured
poison second operand with no invented use ID but a matching second shape,
equal native result/first/second shapes, and exactly one selected lane zero
per result lane. It relies on, but does not claim, 811's carrier and
814/815's poison-second/mask-lane handoffs.

Positive matrix: scalar-to-vector arithmetic lowers the native
insert-to-shuffle splat chain with those result/use/shape/mask facts, and the
structured poison-second form verifies. Malformed/fail-closed matrix:
missing/foreign/unknown result or first-use authority; absent or incoherent
poison second shape; a second-use ID attached to poison; wrong vector shape;
missing, wrong-length, inactive, or nonzero selected mask lanes; incoherent
mask type; and a raw `zeroinitializer` token.

Excluded exactly two rows: (1) `LirInsertElementOp` is not reusable from the
former rejected selection and has no row-local verifier assertion that
`elem_type` equals the structured vector element type (nor selected
poison-base/index semantics); 811's carrier IDs/index mirror alone do not
complete that contract. (2) `LirExtractElementOp` has no structured result
element-type field/fact, so the verifier cannot prove that the extracted
result type equals the vector element type; carrier shape and index facts
alone do not close the row contract.

## Suggested Next

Implement and prove only the selected scalar-to-vector splat
`LirShuffleVectorOp` row: make its existing native authority required at this
exact lowering/verifier boundary and add nearby positive plus malformed
row-level coverage. Leave `LirInsertElementOp` and `LirExtractElementOp`
unchanged; do not generalize to arbitrary shuffle masks.

## Watchouts

Steps 1--8 remain accepted. Do not repeat 811/814 prerequisite work, recover
facts from display text, widen into generic provenance/layout or CFG/PHI, or
claim that the 3038/3038 prerequisite baseline proves a 754 row. The selected
contract is only the structured zero-initializer scalar-to-vector splat;
arbitrary shuffle-mask semantics remain unselected.

## Proof

Audit-only packet: no build or tests run and canonical root `test_before.log`
and `test_after.log` were not modified. Recommended implementation proof:
fresh `cmake --build --preset default`; focused
`ctest --test-dir build -j --output-on-failure -R '^backend_lir_native_vector_authority$'`
extended with the selected splat row; matching
`ctest --test-dir build -j --output-on-failure -R '^backend_'`; representative
scalar-to-vector LLVM emission; then the supervisor-owned full checkpoint and
canonical regression logs.
