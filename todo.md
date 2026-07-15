# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 14
Current Step Title: Implement and prove the selected InsertElement row

## Just Finished

Step 14 implemented the row-local `LirInsertElementOp` opt-in used only by the
two scalar-to-vector splat emitters. Selected inserts now require their own
native carrier, native immediate-zero `i64` index facts, and an `elem_type`
matching the carrier result/first-vector element type; generic InsertElement
and accepted ShuffleVector behavior remain unchanged. Nearby verifier coverage
now exercises selected SSA and immediate/coerced elements plus missing carrier,
identity, shape, index, index-type, and element-type mutations.

## Suggested Next

Supervisor to review Step 14 proof and select the next active packet.

## Watchouts

The selected InsertElement gate remains independent of ShuffleVector adjacency;
zero-index validation uses native immediate authority, not display text.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`; all backend 6/6 passed. Proof log: `test_after.log`.
