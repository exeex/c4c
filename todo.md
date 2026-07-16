Status: Active
Source Idea Path: ideas/open/840_lir_nominal_vector_store_schema_migration.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce The Nominal Vector Store Fact

# Current Packet

## Just Finished

Completed `plan.md` Step 2 for the selected scalar-to-vector splat insert seam.
Added a module-owned `LirVectorRef`/`LirVectorStoreEntry` carrying
`lane_count` and typed element `LirTypeRef`, populated it from both scalar splat
insert producers in `binary.cpp`, and migrated the required
`LirInsertElementOp` verifier path to validate result/first-vector shape and
element coherence from the vector store while keeping legacy row fields as
compatibility mirrors.

## Suggested Next

Start `plan.md` Step 3 with one bounded vector consumer migration. Recommended
next packet: migrate the adjacent `LirShuffleVectorOp` scalar-splat consumer to
read the accepted `LirVectorRef` fact for first-vector/result lane and element
shape while leaving mask, poison, and second-vector shape migration out unless
the supervisor explicitly selects them.

## Watchouts

- `LirNativeVectorShape`, `vec_type`, `elem_type`, `mask_type`, `mask`,
  `second_vector_shape`, and `mask_lanes` remain compatibility mirrors for
  unmigrated paths.
- Store-backed checks currently apply only to `LirInsertElementOp` when
  `requires_native_vector_authority` is set. ExtractElement, ShuffleVector
  mask/poison/second-shape, and nonselected producers are intentionally
  unchanged.
- Aggregate vector elements now fail closed unless their typed element ref is
  backed by an accepted aggregate-store fact from the 838 route; do not add a
  separate aggregate owner for vector work.

## Proof

Passed delegated proof:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_native_vector_authority$' ) > test_after.log 2>&1`.
Proof log: `test_after.log`.
