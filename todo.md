Status: Active
Source Idea Path: ideas/open/840_lir_nominal_vector_store_schema_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Bounded Vector Operation Consumers

# Current Packet

## Just Finished

Completed `plan.md` Step 2 for the selected scalar-to-vector splat insert seam
in commit `65406cab0`. Added a module-owned
`LirVectorRef`/`LirVectorStoreEntry`, populated it from scalar splat insert
producers, and migrated the required `LirInsertElementOp` verifier path to
validate result/first-vector lane and element coherence from the vector store
while keeping legacy row fields as compatibility mirrors.

## Suggested Next

Execute the first bounded `plan.md` Step 3 packet: migrate the adjacent
`LirShuffleVectorOp` scalar-splat consumer to read the accepted `LirVectorRef`
fact for first-vector/result lane and element shape.

## Watchouts

- `LirNativeVectorShape`, `vec_type`, `elem_type`, `mask_type`, `mask`,
  `second_vector_shape`, and `mask_lanes` remain compatibility mirrors for
  unmigrated paths.
- Store-backed checks currently apply only to `LirInsertElementOp` when
  `requires_native_vector_authority` is set. ExtractElement, ShuffleVector
  mask/poison/second-shape, and nonselected producers are intentionally
  unchanged.
- In this first Step 3 packet, leave mask, poison, and second-vector shape
  migration out unless the supervisor later selects them.
- Aggregate vector elements now fail closed unless their typed element ref is
  backed by an accepted aggregate-store fact from the 838 route; do not add a
  separate aggregate owner for vector work.

## Proof

Passed delegated proof:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_native_vector_authority$' ) > test_after.log 2>&1`.
Proof log: `test_after.log`.
