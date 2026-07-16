Status: Active
Source Idea Path: ideas/open/840_lir_nominal_vector_store_schema_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire Duplicate Vector Mirrors For Migrated Consumers

# Current Packet

## Just Finished

Completed the first bounded `plan.md` Step 4 packet. Required
scalar-to-vector `LirInsertElementOp` no longer treats row-local
`result_shape` / `first_vector_shape` mirrors as semantic authority; it reads
the module-owned vector store for lane and element facts and still checks
`vec_type` / `elem_type` as compatibility mirrors. Nearby verifier coverage now
accepts a stale insert shape mirror when the vector-store fact and operation
mirrors remain coherent.

## Suggested Next

Select the next Step 4 retirement gate only after supervisor review. Likely
bounded candidates are row-local shape mirror demotion for direct
`LirExtractElementOp` or required scalar-splat `LirShuffleVectorOp`.

## Watchouts

- `LirNativeVectorShape`, `vec_type`, `elem_type`, `mask_type`, `mask`,
  `second_vector_shape`, and `mask_lanes` remain compatibility mirrors for
  unmigrated paths.
- Store-backed checks now apply only to required `LirInsertElementOp` and the
  required scalar-splat `LirShuffleVectorOp` first-vector/result shape and
  mask lane-count/`mask_type` and poison second-shape paths, plus direct
  `LirExtractElementOp` vector index shape. Nonselected producers are
  intentionally unchanged.
- Required `LirInsertElementOp` row-local shape mirrors are now demoted; do not
  reintroduce rejection based on those mirrors while vector-store facts remain
  coherent.
- Aggregate vector elements now fail closed unless their typed element ref is
  backed by an accepted aggregate-store fact from the 838 route; do not add a
  separate aggregate owner for vector work.

## Proof

Passed focused proof:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_native_vector_authority$' ) > test_after.log 2>&1`.
Additional guard passed:
`ctest --test-dir build -j --output-on-failure -R 'llvm_gcc_c_torture_src_(pr60960_c|scal_to_vec1_c|scal_to_vec2_c)$'`.
