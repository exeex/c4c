Status: Active
Source Idea Path: ideas/open/840_lir_nominal_vector_store_schema_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Bounded Vector Operation Consumers

# Current Packet

## Just Finished

Completed the first bounded `plan.md` Step 3 packet for the selected
scalar-to-vector splat shuffle seam. The required `LirShuffleVectorOp`
verifier path now consumes the accepted `LirVectorRef` fact for
first-vector/result lane and element shape while keeping the legacy row fields
as compatibility mirrors. Nearby verifier coverage now rejects missing,
out-of-range, zero-lane, empty-element, lane-mismatched, and element-mismatched
shuffle vector-store facts.

## Suggested Next

Select the next bounded Step 3 packet only after supervisor review. A likely
candidate is one adjacent shuffle-vector mirror migration for mask, poison, or
second-vector shape, but those remain separate from this packet.

## Watchouts

- `LirNativeVectorShape`, `vec_type`, `elem_type`, `mask_type`, `mask`,
  `second_vector_shape`, and `mask_lanes` remain compatibility mirrors for
  unmigrated paths.
- Store-backed checks now apply only to required `LirInsertElementOp` and the
  required scalar-splat `LirShuffleVectorOp` first-vector/result shape path.
  ExtractElement, ShuffleVector mask/poison/second-shape, and nonselected
  producers are intentionally unchanged.
- Aggregate vector elements now fail closed unless their typed element ref is
  backed by an accepted aggregate-store fact from the 838 route; do not add a
  separate aggregate owner for vector work.

## Proof

Passed delegated proof:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_native_vector_authority$' ) > test_after.log 2>&1`.
Proof log: `test_after.log`.
