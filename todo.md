Status: Active
Source Idea Path: ideas/open/840_lir_nominal_vector_store_schema_migration.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove The Vector Migration And Return The Queue

# Current Packet

## Just Finished

Completed the third bounded `plan.md` Step 4 packet. Required scalar-splat
`LirShuffleVectorOp` no longer treats row-local `result_shape`,
`first_vector_shape`, or `second_vector_shape` mirrors as semantic authority;
it reads the module-owned vector store for lane and element facts and still
checks operation mirrors such as `vec_type`, mask lanes, poison use, and
`mask_type` through the already migrated paths. Nearby verifier coverage now
accepts stale or missing shuffle shape mirrors when the vector-store fact and
operation mirrors remain coherent.

## Suggested Next

Run the final Step 5 acceptance proof for the vector-store migration route and
return the queue to lifecycle review. Use a fresh build plus focused vector
lowering, verifier, and printer coverage. Include:

- `backend_lir_native_vector_authority`
- `llvm_gcc_c_torture_src_pr60960_c`
- `llvm_gcc_c_torture_src_scal_to_vec1_c`
- `llvm_gcc_c_torture_src_scal_to_vec2_c`

Do not perform additional mirror deletion in this proof packet. Treat remaining
`LirNativeVectorShape`, mask, and type fields as compatibility boundaries for
unmigrated or non-required paths unless a later source explicitly selects them.

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
- Direct `LirExtractElementOp` row-local shape mirrors are now demoted; keep
  malformed lane/element rejection on the vector-store entry itself.
- Required scalar-splat `LirShuffleVectorOp` row-local shape mirrors are now
  demoted; keep malformed lane/element rejection on the vector-store entry and
  keep mask and poison checks on their migrated structured facts.
- Remaining `LirNativeVectorShape`, mask, `vec_type`, `elem_type`,
  `mask_type`, and related row-local fields still protect compatibility for
  unmigrated or non-required vector paths; deleting them now would weaken
  checks outside the completed Step 4 packets.
- Aggregate vector elements now fail closed unless their typed element ref is
  backed by an accepted aggregate-store fact from the 838 route; do not add a
  separate aggregate owner for vector work.

## Proof

Passed Step 5 proof:

`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_native_vector_authority|llvm_gcc_c_torture_src_(pr60960_c|scal_to_vec1_c|scal_to_vec2_c))$' ) > test_after.log 2>&1`

Result: 4/4 tests passed. `git diff --check` also passed.
