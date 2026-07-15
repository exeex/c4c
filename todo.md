# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 9
Current Step Title: Implement and prove the Step 8 selection

## Just Finished

Step 9 implemented the selected scalar-to-vector zero-initializer splat
`LirShuffleVectorOp` row. Both existing `emit_splat[_vec]` lowerings now mark
only that structured splat as requiring native vector authority. The verifier
rejects a missing carrier and requires its first-vector use to equal the
immediately preceding native `LirInsertElementOp` result; the existing carrier
checks retain the current-function result ID, structured poison second with no
use ID and matching shape, equal vector shapes, and selected-zero mask lanes.
Nearby coverage verifies the selected positive splat and rejects missing
authority plus a first-vector use that is not the preceding native insert
result. `LirInsertElementOp` and `LirExtractElementOp` remain unchanged.

## Suggested Next

Supervisor should make the Step 10 lifecycle reassessment; do not widen this
completed row into arbitrary shuffle masks, `LirInsertElementOp`, or
`LirExtractElementOp`.

## Watchouts

Steps 1--8 remain accepted. This packet did not run the supervisor-owned full
checkpoint, and the matching `^backend_` guard is row proof rather than a full
754 closure decision. Do not recover facts from display text or generalize the
selected structured zero-initializer splat to arbitrary shuffle masks.

## Proof

Passed: `cmake --build --preset default`; representative
`./build/c4cll --codegen llvm tests/c/external/gcc_torture/src/scal-to-vec1.c
-o /tmp/scal-to-vec1.ll`; focused
`ctest --test-dir build -j --output-on-failure -R
'^backend_lir_native_vector_authority$'` (1/1); and matching
`ctest --test-dir build -j --output-on-failure -R '^backend_'` (6/6), logged
at `test_after.log`. The supervisor-selected packet proof is sufficient for
this row; the full checkpoint remains supervisor-owned.
