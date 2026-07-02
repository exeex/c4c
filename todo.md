# Current Packet

Status: Complete
Source Idea Path: ideas/open/529_bir_route_facade_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Move Confirmed Facade Bodies

## Just Finished

Step 2: Move Confirmed Facade Bodies completed.

Moved only the seven confirmed route-index facade bodies from
`src/backend/bir/bir.cpp` into `src/backend/bir/bir_route_facade.cpp`:

- the three `route_index_reference_facade` overloads
- `route_index_validate_current_block_publication_reference`
- `route_index_validate_block_entry_publication_reference`
- `route_index_validate_comparison_operand_reference`
- `route_index_validate_materialized_condition_reference`

Kept public declarations in `src/backend/bir/bir.hpp` unchanged. Left
`find_fused_compare_operand_producer_facts`,
`route7_find_fused_compare_operand_producer_facts`,
`find_materialized_condition_producer_identity`, route4 validators, route7
lookup/validation bodies, and route7 private helpers in their existing owner
files. Added `src/backend/bir/bir_route_facade.cpp` next to explicit
`src/backend/bir/bir.cpp` entries in `tests/backend/bir/CMakeLists.txt`.

## Suggested Next

Supervisor should review and commit this completed Step 2 slice, then delegate
the next packet from `plan.md` if the source idea still has remaining
extraction work.

## Watchouts

- Main backend CMake picked up `bir_route_facade.cpp` through the existing BIR
  source glob during the delegated build.
- Direct-source BIR test targets that explicitly enumerate `bir.cpp` now also
  enumerate `bir_route_facade.cpp`.
- The moved facade bodies include only public-route calls from `bir.hpp`; no
  `bir_private.hpp` dependency was introduced in the new translation unit.

## Proof

Ran exactly:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering)$') > test_after.log 2>&1
```

Result: passed. `test_after.log` contains the successful build and the two
passing tests:

- `backend_aarch64_branch_control_lowering`
- `backend_prepared_lookup_helper`
