Status: Active
Source Idea Path: ideas/open/840_lir_nominal_vector_store_schema_migration.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Trace Current Vector Authority And Pick The First Store Seam

# Current Packet

## Just Finished

Completed `plan.md` Step 1 read-only tracing for the current vector authority.
The first store-backed vector fact should be a module-owned `LirVectorRef`
entry carrying `lane_count` plus a typed element-family ref/`LirTypeRef`,
registered from the scalar-to-vector splat `LirInsertElementOp` producer before
`LirShuffleVectorOp` consumes it.

## Suggested Next

Start `plan.md` Step 2 by adding the smallest nominal vector-store seam:
introduce `LirVectorRef`/`LirVectorStoreEntry`, populate it for the required
scalar-to-vector splat `LirInsertElementOp`, and migrate that insert verifier
path to validate its result/first-vector shape from the store while keeping the
existing row fields as compatibility mirrors.

## Watchouts

- Current producer surfaces: `src/codegen/lir/hir_to_lir/expr/binary.cpp`
  constructs scalar splat `LirInsertElementOp` then `LirShuffleVectorOp`;
  `src/codegen/lir/hir_to_lir/expr/misc.cpp` constructs vector-index
  `LirExtractElementOp`.
- Current verifier consumers: `src/codegen/lir/verify.cpp` checks
  `native_vector_authority` for insert/extract/shuffle and still compares
  `LirNativeVectorShape` against row-local rendered vector type mirrors.
- Current printer receivers: `src/codegen/lir/lir_printer.cpp` prints
  `vec_type`, `elem_type`, `index_type`, `mask_type`, and operands only; it
  does not consume native vector authority.
- Duplicate compatibility-only fields after the first seam: `vec_type`,
  `elem_type`, `index_type`, `mask_type`, `mask`, `result_shape`,
  `first_vector_shape`, `second_vector_shape`, and `mask_lanes` remain mirrors
  until their named consumers migrate. `LirNativeVectorShape` remains
  compatibility-only after the corresponding `LirVectorRef` fact owns lane
  count and typed element family.
- First migrated consumer: `LirInsertElementOp` verifier for the required
  scalar-to-vector splat precursor. Defer `LirShuffleVectorOp` mask/poison
  second-shape migration and `LirExtractElementOp` index-expression migration
  until after the first store fact is proven.
- Aggregate element dependency is accepted 838 consumption only: aggregate
  element families must reference the existing module-owned aggregate store
  facts, not create a new aggregate ownership route.

## Proof

No code proof required for Step 1 tracing. Lightweight searches and source
inspection only:
`rg -n --glob '!ref/**' 'LirNativeVectorShape|LirNativeVectorAuthority|LirShuffleVectorOp|LirInsertElementOp|LirExtractElementOp|native_vector_authority|mask_type|mask' src/codegen/lir tests/backend/bir/backend_lir_native_vector_authority_test.cpp`.
Focused proof for the Step 2 implementation packet should be a fresh build of
`backend_lir_native_vector_authority_test` plus
`ctest -R '^backend_lir_native_vector_authority$' --output-on-failure`, with
output written to `test_after.log`.
