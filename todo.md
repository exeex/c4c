Status: Active
Source Idea Path: ideas/open/144_source_producer_same_block_materialization_owner.md
Source Plan Path: plan.md
Current Step ID: 2-4
Current Step Title: Move Shared Same-Block Source-Producer APIs; Update Consumers And Narrow Includes; Prove And Record The Ownership Move

# Current Packet

## Just Finished

Completed Steps 2-4, "Move Shared Same-Block Source-Producer APIs; Update
Consumers And Narrow Includes; Prove And Record The Ownership Move".

`src/backend/prealloc/select_chain_lookups.hpp` now publicly owns:

- `PreparedSameBlockScalarProducer`
- `PreparedSameBlockValueMaterializationQuery`
- `PreparedCurrentBlockPublicationConsumption`
- `make_prepared_edge_publication_source_producer_lookups`
- `find_indexed_prepared_edge_publication_source_producer`
- `find_prepared_current_block_publication_consumption`
- the public `find_prepared_same_block_scalar_producer` overloads
- the public `evaluate_prepared_same_block_integer_constant` overloads

`src/backend/prealloc/prepared_lookups.hpp` no longer declares those APIs; it
includes the narrow owner because residual prepared lookup declarations still
embed `PreparedSameBlockScalarProducer` by value. Publication fact structs
remain owned by `src/backend/prealloc/publication_plans.hpp`.

Definitions stayed behavior-preserving in their existing translation units.
Consumers that directly used the moved declarations and lacked the narrow owner
now include `select_chain_lookups.hpp`:
`fp_value_materialization.cpp`, `comparison.cpp`, and
`dispatch_edge_copies.cpp`.

## Suggested Next

Supervisor should review and commit this ownership-move slice if the diff
matches the source idea. No executor follow-up is currently required for this
packet.

## Watchouts

No behavior or expectation changes were made. `prepared_lookups.hpp` still
includes `select_chain_lookups.hpp` for residual declarations that contain the
moved same-block scalar producer type by value; this keeps compatibility while
moving public ownership to the narrow source-producer owner.

## Proof

Ran `cmake --build --preset default` successfully.

Ran `ctest --test-dir build -R '^backend_' --output-on-failure >
test_after.log 2>&1` successfully. `test_after.log` contains the backend CTest
output and reports total real test time of 5.39 seconds.
