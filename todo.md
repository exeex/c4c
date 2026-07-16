Status: Active
Source Idea Path: ideas/open/840_lir_nominal_vector_store_schema_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate Bounded Vector Operation Consumers

# Current Packet

## Just Finished

Completed a third bounded `plan.md` Step 3 packet for the selected
scalar-to-vector splat shuffle seam. The required `LirShuffleVectorOp`
verifier path now consumes the accepted `LirVectorRef` fact for the poison
second operand's vector shape compatibility mirror, while keeping the poison
token and absent second-use evidence as structured compatibility validation.
Nearby verifier coverage now rejects non-poison second operands, invented
second-use evidence, missing second-vector shape, and vector-store-backed
second-shape lane or element mismatches for required scalar-splat shuffles.

## Suggested Next

Select the next bounded Step 3 packet only after supervisor review. A likely
candidate is the next vector operation family, such as bounded
`LirExtractElementOp` vector-store consumption, but it remains separate from
this packet.

## Watchouts

- `LirNativeVectorShape`, `vec_type`, `elem_type`, `mask_type`, `mask`,
  `second_vector_shape`, and `mask_lanes` remain compatibility mirrors for
  unmigrated paths.
- Store-backed checks now apply only to required `LirInsertElementOp` and the
  required scalar-splat `LirShuffleVectorOp` first-vector/result shape and
  mask lane-count/`mask_type` and poison second-shape paths. ExtractElement and
  nonselected producers are intentionally unchanged.
- Aggregate vector elements now fail closed unless their typed element ref is
  backed by an accepted aggregate-store fact from the 838 route; do not add a
  separate aggregate owner for vector work.

## Proof

Passed delegated proof:
`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_lir_native_vector_authority$' ) > test_after.log 2>&1`.
Proof log: `test_after.log`.
