Status: Active
Source Idea Path: ideas/open/840_lir_nominal_vector_store_schema_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire Duplicate Vector Mirrors For Migrated Consumers

# Current Packet

## Just Finished

Step 3 is complete for the currently named migrated consumers: required
scalar-to-vector `LirInsertElementOp`, direct vector-index `LirExtractElementOp`,
and required scalar-splat `LirShuffleVectorOp`.

## Suggested Next

Start Step 4 with one narrow retirement gate. First candidate: demote or remove
one row-local vector shape mirror for a fully migrated consumer only where the
verifier already reads the module-owned vector store with equivalent malformed
coverage.

## Watchouts

- `LirNativeVectorShape`, `vec_type`, `elem_type`, `mask_type`, `mask`,
  `second_vector_shape`, and `mask_lanes` remain compatibility mirrors for
  unmigrated paths.
- Store-backed checks now apply only to required `LirInsertElementOp` and the
  required scalar-splat `LirShuffleVectorOp` first-vector/result shape and
  mask lane-count/`mask_type` and poison second-shape paths, plus direct
  `LirExtractElementOp` vector index shape. Nonselected producers are
  intentionally unchanged.
- Aggregate vector elements now fail closed unless their typed element ref is
  backed by an accepted aggregate-store fact from the 838 route; do not add a
  separate aggregate owner for vector work.

## Proof

Step 3 final packet passed:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_native_vector_authority|llvm_gcc_c_torture_src_(pr60960_c|scal_to_vec1_c|scal_to_vec2_c))$' ) > test_after.log 2>&1`.
Regression guard against matching `test_before.log`: PASS.
