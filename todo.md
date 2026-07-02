# Current Packet

Status: Active
Source Idea Path: ideas/open/529_bir_route_facade_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Route Facade Boundaries

## Just Finished

Step 1: Map Route Facade Boundaries completed as a mapping-only packet.

Clang-backed definition/signature inventory for `src/backend/bir/bir.cpp`
identified these exact route facade bodies as the Step 2 move set:

- `route_index_reference_facade(const Route4PublicationAvailabilityIndex&)`
  at `src/backend/bir/bir.cpp:924`
- `route_index_reference_facade(const Route7ComparisonConditionIndex&)`
  at `src/backend/bir/bir.cpp:931`
- `route_index_reference_facade(const Route4PublicationAvailabilityIndex&,
  const Route7ComparisonConditionIndex&)` at `src/backend/bir/bir.cpp:938`
- `route_index_validate_current_block_publication_reference(
  const RouteIndexReferenceFacade&, const Block&, const Value&, std::size_t)`
  at `src/backend/bir/bir.cpp:948`
- `route_index_validate_block_entry_publication_reference(
  const RouteIndexReferenceFacade&, const Block&, const Value&)`
  at `src/backend/bir/bir.cpp:980`
- `route_index_validate_comparison_operand_reference(
  const RouteIndexReferenceFacade&, const Block&, const Value&, std::size_t,
  Route7ComparisonOperandRole)` at `src/backend/bir/bir.cpp:1008`
- `route_index_validate_materialized_condition_reference(
  const RouteIndexReferenceFacade&, const Block&, const Value&, std::size_t)`
  at `src/backend/bir/bir.cpp:1040`

Do not move these adjacent bodies as facade work:

- Route4-owned validators:
  `route4_validate_current_block_publication_reference`,
  `route4_validate_block_entry_publication_reference`
- Route7-owned lookup/validation bodies:
  `route7_find_comparison_instruction`, `route7_find_comparison_operand`,
  `route7_find_materialized_condition`, `route7_find_branch_condition`,
  `route7_validate_comparison_instruction_reference`,
  `route7_validate_comparison_operand_reference`,
  `route7_validate_materialized_condition_reference`,
  `route7_validate_branch_condition_reference`
- Route7 private validation helpers in the anonymous namespace:
  `route7_value_matches`, `route7_missing_block_status`,
  `route7_index_owner_scope`, `route7_reference_status`,
  `route7_reference_key`, `route7_index_owner_is_stale`,
  `route7_instruction_record_points_into_block`,
  `route7_operand_record_points_into_block`, `route7_missing_reference`
- Fused-compare/materialized-condition consumer internals:
  `find_fused_compare_operand_producer_facts`,
  `route7_operand_record_to_public`,
  `route7_find_fused_compare_operand_producer_facts`,
  `find_materialized_condition_producer_identity`

Direct dependency edges from clang-backed caller/callee queries:

- The three `route_index_reference_facade` overloads have no direct callees.
  Same-TU callers are `route7_find_fused_compare_operand_producer_facts`
  and `find_materialized_condition_producer_identity`.
- `route_index_validate_current_block_publication_reference` calls
  `route1_source_value_identity` and
  `route4_validate_current_block_publication_reference`.
- `route_index_validate_block_entry_publication_reference` calls
  `route1_source_value_identity` and
  `route4_validate_block_entry_publication_reference`.
- `route_index_validate_comparison_operand_reference` calls
  `route1_source_value_identity` and
  `route7_validate_comparison_operand_reference`; same-TU caller is
  `route7_find_fused_compare_operand_producer_facts`.
- `route_index_validate_materialized_condition_reference` calls
  `route1_source_value_identity` and
  `route7_validate_materialized_condition_reference`; same-TU caller is
  `find_materialized_condition_producer_identity`.
- Route4 validator definitions are not in `bir.cpp`; clang found only facade
  callers there. Their bodies live in the route4 owner and should remain there.
- Route7 validators depend on route7 private helpers and `route_block_matches`
  from `bir_private.hpp`; these are route7 internals and should remain in
  `bir.cpp` or the route7 owner, not move with the facade.

Cross-translation-unit caller evidence from targeted clang caller queries:

- `src/backend/mir/query.cpp`: `find_bir_block_entry_publication_identity`
  calls the route4 facade constructor and block-entry facade validator.
- `src/backend/prealloc/prepared_lookups.cpp`:
  `attribute_route4_block_entry_publication_if_agreeing` calls the route4
  facade constructor and block-entry facade validator.
- `src/backend/mir/aarch64/codegen/comparison.cpp`:
  `find_valid_route7_materialized_condition_producer_identity` calls the
  route7 facade constructor and route7 comparison/materialized facade
  validators; `read_agreeing_route7_fused_compare_operand_producer_facts` and
  `fused_compare_uses_selected_operand` call
  `route7_find_fused_compare_operand_producer_facts`; fused/materialized
  wrapper bodies should stay put.
- Focused tests with direct callers are
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` and
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`.

Likely Step 2 declarations and build wiring:

- Keep all public declarations in `src/backend/bir/bir.hpp` at the existing
  route-index facade declaration block; do not split the public header.
- A dedicated `src/backend/bir/bir_route_facade.cpp` is justified by the seven
  focused facade bodies above.
- Include `bir.hpp` in the new file. It should not need private declarations
  if the moved bodies call only public route1, route4, and route7 APIs already
  declared in `bir.hpp`.
- Main backend build uses `file(GLOB_RECURSE C4C_BACKEND_BIR_SOURCES
  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/bir/*.cpp")` in
  `src/backend/CMakeLists.txt`, so the new source is picked up there.
- Direct-source test targets in `tests/backend/bir/CMakeLists.txt` that list
  `src/backend/bir/bir.cpp` explicitly will need `bir_route_facade.cpp` added
  beside it, especially `backend_prepare_phi_materialize_test` and
  `backend_lir_to_bir_notes_test`.

## Suggested Next

Step 2 should create `src/backend/bir/bir_route_facade.cpp`, move only the
seven confirmed facade bodies from `src/backend/bir/bir.cpp`, and add the new
translation unit to direct-source BIR test targets that still enumerate
`bir.cpp`.

## Watchouts

- Do not move `find_fused_compare_operand_producer_facts`,
  `route7_find_fused_compare_operand_producer_facts`, or
  `find_materialized_condition_producer_identity`; they consume the facade but
  also own fused-compare/materialized-condition logic.
- Do not move route7 anonymous-namespace helpers with the facade. If Step 2
  needs any of them, that is a route-drift signal because the confirmed facade
  body set should only call public declarations.
- Keep public declarations in `src/backend/bir/bir.hpp`; Step 2 should not
  introduce a public header split.
- Preserve missing-index fallback construction exactly, including
  `RouteIndexRecordReference`, `RouteIndexRoute`, `RouteIndexOwnerScope`,
  `RouteIndexRecordCategory`, `RouteIndexRelationshipKind`, block labels,
  instruction indexes, and `route1_source_value_identity` fields.

## Proof

Mapping-only packet; no build or tests were required and `test_after.log` was
not created or updated.

Evidence commands run:

- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/bir/bir.cpp build/compile_commands.json`
- `c4c-clang-tool-ccdb function-signatures /workspaces/c4c/src/backend/bir/bir.cpp build/compile_commands.json`
- `c4c-clang-tool function-signatures src/backend/bir/bir.hpp -- --std=c++17 -I/workspaces/c4c/src -I/workspaces/c4c/src/codegen/lir -I/workspaces/c4c/src/frontend/parser`
- Targeted `c4c-clang-tool-ccdb function-callees` and
  `function-callers` for the facade, route4/route7 validators,
  fused-compare, and materialized-condition symbols in `bir.cpp`.
- Targeted `c4c-clang-tool-ccdb function-callers` in
  `src/backend/mir/query.cpp`, `src/backend/prealloc/prepared_lookups.cpp`,
  `src/backend/mir/aarch64/codegen/comparison.cpp`,
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, and
  `tests/backend/mir/backend_aarch64_branch_control_lowering_test.cpp`.
- `c4c-clang-tool-ccdb type-refs` for `RouteIndexReferenceFacade`,
  `RouteIndexRecordReference`, `Route4IndexReferenceValidation`,
  `Route7IndexReferenceValidation`, `FusedCompareOperandProducerFacts`, and
  `MaterializedConditionProducerIdentity`.

Recommended focused Step 2 proof after the move:

- Build: `cmake --build build --target c4c_backend backend_prepared_lookup_helper_test backend_aarch64_branch_control_lowering_test`
- Tests: `ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering)$' --output-on-failure`
