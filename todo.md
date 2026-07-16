Status: Active
Source Idea Path: ideas/open/840_lir_nominal_vector_store_schema_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Bounded Vector Operation Consumers

# Current Packet

## Just Finished

Completed a fourth bounded `plan.md` Step 3 packet for direct vector indexing.
`LirExtractElementOp` lowering now registers a module-owned `LirVectorRef`, and
the verifier consumes that vector-store fact for direct vector index lane and
element shape validation while keeping row-local shape fields as compatibility
mirrors. Nearby verifier coverage now rejects missing, out-of-range, zero-lane,
empty-element, lane-mismatched, and element-mismatched extract vector-store
facts.

## Suggested Next

Select the next bounded Step 3 packet only after supervisor review. A likely
candidate is a narrow parity/broader proof decision for migrated Step 3
consumers before moving to mirror retirement, but that remains separate from
this packet.

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

Passed delegated proof:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_native_vector_authority$' ) > test_after.log 2>&1`.
Proof log: `test_after.log`.
