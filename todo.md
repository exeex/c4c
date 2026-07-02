# Current Packet

Status: Complete
Source Idea Path: ideas/open/524_bir_route4_publication_body_extraction.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Map Route4 Ownership And Dependencies

## Just Finished

Step 1 mapped the route4 publication ownership boundary with
`c4c-clang-tools` before targeted source reads. The expected extraction owner
is `src/backend/bir/bir_route4_publication.cpp`; public declarations remain in
`src/backend/bir/bir.hpp`, and `src/backend/CMakeLists.txt` already glob-picks
`src/backend/bir/*.cpp`.

Move these route4 publication bodies from `src/backend/bir/bir.cpp`:

- `route4_publication_source_kind`
- `route4_current_block_publication_record`
- `route4_block_entry_publication_record`
- `route4_current_block_publication_value_record`
- `route4_block_entry_publication_value_record`
- `route4_build_publication_availability_index`
- `route4_find_current_block_publication`
- `route4_validate_current_block_publication_reference`
- `route4_validate_block_entry_publication_reference`

Move these route4-private validation helpers with the route4 bodies:

- `route4_index_contains_block`
- `route4_reference_status`
- `route4_reference_key`
- `route4_missing_reference`
- `route4_value_record_matches`

Keep or promote shared/non-route4 ownership in place instead of moving it as a
route4-only helper:

- `route4_record_matches_block` is shared by route4 find/validation and route5
  finders (`route5_find_cfg_edge_publication`,
  `route5_find_current_block_join_source`). If route4 validators move, expose
  this exact behavior through a private shared declaration/helper rather than
  hiding it inside the new route4-only translation unit.
- Keep route-index facade bodies in `bir.cpp`:
  `route_index_reference_facade`,
  `route_index_validate_current_block_publication_reference`, and
  `route_index_validate_block_entry_publication_reference`.
- Keep route6 publication-source selection bodies in `bir.cpp`:
  `route6_call_argument_publication_source_record`,
  `route6_build_call_use_source_index`, and
  `route6_find_call_argument_publication_source`.

Route6/facade consumers found:

- `route6_call_argument_publication_source_record` directly calls
  `route4_current_block_publication_record`; route6 does not consume the built
  `Route4PublicationAvailabilityIndex` for this path.
- The route-index facade validates by delegating to
  `route4_validate_current_block_publication_reference` and
  `route4_validate_block_entry_publication_reference`.
- Cross-translation-unit route4 index/validation consumers include
  `src/backend/mir/query.cpp`, `src/backend/prealloc/prepared_lookups.cpp`,
  `src/backend/mir/aarch64/codegen/calls.cpp`,
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`, and
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

## Suggested Next

Execute Step 2 from `plan.md`: create
`src/backend/bir/bir_route4_publication.cpp`, move the mapped route4 bodies
without changing signatures or semantics, keep public declarations in
`bir.hpp`, and handle `route4_record_matches_block` as a shared private
dependency because route5 still calls it.

## Watchouts

- Keep this as behavior-preserving body extraction.
- Do not move public route4 declarations out of `bir.hpp`.
- Do not move route-index facade bodies, route6 source-selection policy, or
  route-specific validation records.
- Do not rewrite expectations or weaken tests as proof.
- Do not make `route4_record_matches_block` route4-only unless route5 is also
  updated to use an equivalent shared helper with unchanged block matching
  semantics.
- `backend_prepared_lookup_helper` has broader route4/facade coverage, but the
  selected narrow command below is the route6-consuming starting subset.

## Proof

Mapping-only packet; no build, CTest, or `test_after.log` was required or run.
Selected future narrow proof command:
`cmake --build --preset default && ctest --test-dir build -R '^backend_prepare_frame_stack_call_contract$' --output-on-failure`.
