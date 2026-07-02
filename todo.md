# Current Packet

Status: Active
Source Idea Path: ideas/open/528_bir_route6_call_publication_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create Route6 Call Publication Owner

## Just Finished

Step 1: Map Route6 Call Publication Boundaries.

AST-backed boundary map completed with `c4c-clang-tools` plus targeted symbol
use scans before any body movement. No implementation files, build
registration, tests, source idea, logs, or `plan.md` were edited.

Exact route6 call-use / call-argument publication / call-result source /
publication-routing bodies to move from `src/backend/bir/bir.cpp` into the
route6 owner:

- `call_argument_binary_source_producer_opcode_is_materializable`
  (`bir.cpp:280`)
- `find_call_argument_source_relationship` (`bir.cpp:312`)
- anonymous helper `raw_call_argument_source_producer_materialization`
  (`bir.cpp:346`)
- anonymous helper `route6_public_call_argument_source_producer_kind`
  (`bir.cpp:403`)
- `find_call_argument_source_producer_materialization` (`bir.cpp:422`)
- anonymous helpers `route6_indexed_call_inst`,
  `route6_call_argument_relationship_count`,
  `route6_source_kind(CallArgumentSourceEncodingKind)`,
  `route6_source_kind(CallArgumentSourceProducerKind)`,
  `route6_selection_is_abi_bound`, `route6_values_match`,
  `route6_duplicate_lane_match` (`bir.cpp:452-540`)
- `route6_call_argument_source_record` (`bir.cpp:542`)
- `route6_call_argument_source_producer_record` (`bir.cpp:618`)
- `route6_call_argument_direct_global_dependency_record` (`bir.cpp:655`)
- `route6_call_argument_publication_source_record` (`bir.cpp:705`)
- `route6_call_result_source_record` (`bir.cpp:788`)
- `route6_call_result_lane_source_record` (`bir.cpp:822`)
- anonymous lookup helpers `route6_block_matches`, `route6_call_key_matches`,
  `route6_missing_call_status` (`bir.cpp:856-913`)
- `route6_build_call_use_source_index` (`bir.cpp:915`)
- `route6_find_call_argument_source` (`bir.cpp:962`)
- `route6_call_argument_source_matches_argument_value_record` (`bir.cpp:987`)
- `route6_find_call_argument_source_producer` (`bir.cpp:1005`)
- `route6_find_call_argument_direct_global_dependency` (`bir.cpp:1028`)
- `route6_find_call_argument_publication_source` (`bir.cpp:1051`)
- `route6_find_call_result_source` (`bir.cpp:1073`)
- `route6_find_call_result_lane_source` (`bir.cpp:1103`)
- `find_call_result_source_identity` (`bir.cpp:2027`)
- `find_call_result_lane_source_identity` (`bir.cpp:2044`)
- `find_call_argument_publication_source_routing` (`bir.cpp:2101`)

Public declarations that must remain in `bir.hpp`:

- route6 public record/index/find APIs at `bir.hpp:6071-6166`
- call-result identity public helpers at `bir.hpp:6312-6320`
- route6 record/index public types at `bir.hpp:5257-5440`
- `call_argument_binary_source_producer_opcode_is_materializable` declaration
  at `bir.hpp:4230`

Route1-route5 APIs consumed by the moved route6 bodies:

- Route1: `Route1SameBlockProducerQuery`, `route1_source_value_identity`,
  `route1_producer_record`, `route1_build_producer_index`
- Route2: `call_argument_direct_global_select_chain_dependency_available`,
  `route2_select_chain_value_record`
- Route3: `route3_memory_access_record`; route6 stores the resulting
  `Route3MemoryAccessRecord` as `memory_source`
- Route4: `route4_current_block_publication_record`; route6 stores the
  resulting `Route4CurrentBlockPublicationRecord`
- Route5: `Route5CfgEdgePublicationRecord` remains a field on
  `Route6CallArgumentPublicationSourceRecord`; no direct route5 builder/find
  call was found in the current route6 body cluster
- Shared call argument/source helpers:
  `call_argument_source_selection_available`,
  `call_argument_binary_source_producer_opcode_is_materializable`, and
  `find_call_argument_source_relationship`

Relevant direct caller/callee notes from AST:

- `route6_build_call_use_source_index` has no direct caller inside
  `bir.cpp`; external users include AArch64 dispatch/calls, x86 debug/x86
  helpers, `backend_prepared_lookup_helper_test`, and
  `backend_aarch64_call_boundary_owner_test`.
- `route6_call_argument_source_record` is called by the route6 producer,
  direct-global, publication-source, and build-index bodies.
- `route6_call_argument_source_producer_record` calls
  `route1_producer_record` and
  `raw_call_argument_source_producer_materialization`; it is called by the
  public materialization helper, publication-source record builder, and
  build-index body.
- `route6_call_argument_publication_source_record` calls
  `find_call_argument_publication_source_routing`,
  `route6_call_argument_source_producer_record`,
  `route3_memory_access_record`, and
  `route4_current_block_publication_record`.
- `route6_call_result_source_record` calls
  `find_call_result_source_identity` and `route1_source_value_identity`.
- `route6_call_result_lane_source_record` calls
  `route6_call_result_source_record`,
  `find_call_result_lane_source_identity`, and
  `route1_source_value_identity`.
- `find_call_argument_publication_source_routing` calls
  `find_call_argument_source_relationship`,
  `call_argument_source_selection_available`, and
  `call_argument_direct_global_select_chain_dependency_available`.
- `find_call_result_lane_source_identity` calls
  `find_call_result_source_identity`.

Cross-file facade/users observed by symbol scan:

- LIR-to-BIR call publication consumer:
  `src/backend/bir/lir_to_bir/module.cpp` calls
  `find_call_argument_source_relationship`.
- AArch64 backend consumers:
  `src/backend/mir/aarch64/codegen/dispatch.cpp`,
  `src/backend/mir/aarch64/codegen/calls.cpp`, and
  `src/backend/mir/aarch64/codegen/select_materialization.cpp` call route6
  index/find APIs.
- x86 backend consumers:
  `src/backend/mir/x86/x86.hpp` and `src/backend/mir/x86/debug/debug.cpp`
  call route6 index/find/match APIs.
- Focused test users:
  `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`,
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`,
  `tests/backend/bir/backend_prealloc_call_boundary_classification_test.cpp`,
  and `tests/backend/mir/backend_aarch64_call_boundary_owner_test.cpp`.

Surfaces that must not move in the route6 owner packet:

- Route-index facade bodies stay in `bir.cpp`: `route_index_reference_facade`
  overloads and `route_index_validate_*` helpers at `bir.cpp:1772-1889`.
- Facade-backed route7/materialized-condition query helpers stay in
  `bir.cpp`: `find_fused_compare_operand_producer_facts` and
  `find_materialized_condition_producer_identity` use
  `route_index_reference_facade` at `bir.cpp:1964` and `bir.cpp:2010`.
- Memory provenance and memory lowering surfaces stay in place:
  `src/backend/bir/lir_to_bir/memory/*`,
  `src/backend/bir/lir_to_bir/calling.cpp` provenance helpers, and route3
  memory owner bodies in `src/backend/bir/bir_route3_memory.cpp`.
- Existing route1-route5 owner bodies stay in their current files; route6
  consumes their public APIs only.

Likely private helper declarations needed after the move:

- If only route6 bodies move, keep or expose a private declaration for the
  generic `indexed_call_inst(const Block&, const CallInst&, std::size_t)` helper
  used by `raw_call_argument_source_producer_materialization` and
  `find_call_result_source_identity`, unless this helper is moved with the
  route6 owner as a local support helper.
- No new private declarations are needed for the anonymous route6 helpers if
  they move with the route6 owner inside that file's anonymous namespace.
- No private declaration should be introduced for route-index facade or memory
  provenance helpers; those are out of scope for the route6 move.

Focused proof recommendation for the implementation packet:

- Build/link coverage: `cmake --build --preset default`.
- Route6 call-publication and call-result lookup coverage:
  `backend_prepared_lookup_helper`.
- Publication-routing / relationship consumers:
  `backend_lir_to_bir_notes`.
- Call result identity boundary coverage:
  `backend_prealloc_call_boundary_classification`.
- Link-time backend consumer coverage:
  `backend_aarch64_call_boundary_owner`; include broader `backend_` coverage
  if the moved owner is registered in core backend build files.

## Suggested Next

Supervisor can delegate Step 2 from `plan.md`: create the route6 call
publication owner and move only the mapped bodies into
`src/backend/bir/bir_route6_call_publication.cpp`, preserving public
declarations in `bir.hpp`.

## Watchouts

- Keep this activation aligned to
  `ideas/open/528_bir_route6_call_publication_body_extraction.md`.
- Do not move route6 public declarations, route-index facade bodies, memory
  provenance headers, or implementation bodies outside the route6
  call-publication scope in the first mapping packet.
- Do not change call ABI lowering, LIR-to-BIR call generation, route1-route5
  behavior, or idea 422 producer capability.

## Proof

Mapping-only packet; no build or tests were run. Proof basis is AST-backed
symbol queries through `c4c-clang-tools` plus targeted direct use scans.
