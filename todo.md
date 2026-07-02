# Current Packet

Status: Active
Source Idea Path: ideas/open/530_bir_route_header_split_after_body_moves.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Route Header Boundaries

## Just Finished

Step 1: Map Route Header Boundaries completed as a mapping-only packet.

Clang-backed evidence used:

- `c4c-clang-tool list-symbols src/backend/bir/bir.hpp -- --std=c++20 -I/workspaces/c4c -I/workspaces/c4c/src`
- `c4c-clang-tool function-signatures src/backend/bir/bir.hpp -- --std=c++20 -I/workspaces/c4c -I/workspaces/c4c/src`
- `c4c-clang-tool type-refs src/backend/bir/bir.hpp {Value,Inst,Block,Function,Route4PublicationAvailabilityIndex,Route7ComparisonConditionIndex,RouteIndexReferenceFacade} -- --std=c++20 -I/workspaces/c4c -I/workspaces/c4c/src`
- `c4c-clang-tool-ccdb list-symbols /workspaces/c4c/src/backend/bir/bir_route_facade.cpp build/compile_commands.json`
- `rg` include/user scans under `src/backend/bir`, `src/backend/mir`, `src/backend/prealloc`, and `tests/backend/bir`

Mapped declaration clusters in `src/backend/bir/bir.hpp`:

- Early route-1 identity cluster at lines 490-569: `Route1ProducerKind`,
  `route1_producer_kind_name`, `route1_producer_kind_has_materialization`,
  `Route1SourceValueIdentity`, `Route1ImmediateIntegerConstant`,
  `route1_source_value_identity`, and `route1_immediate_integer_constant`.
  It is embedded before core instruction/model declarations and must not move
  first because later core structs and every route cluster depend on
  `Route1SourceValueIdentity`.
- Route 1 producer/index cluster at lines 4480-4571:
  `Route1ProducerInstructionIdentity`, `Route1MaterializationAvailability`,
  `Route1ProducerRecord`, `Route1ProducerIndex`,
  `Route1SameBlockProducerQuery`, `Route1SameBlockScalarProducer`, and route-1
  producer/query declarations. Complete-type dependencies include `Inst`,
  `Block`, `Value`, and `std::vector<Route1ProducerRecord>`.
- Route 2 select-chain cluster at lines 4573-4677:
  `Route2SelectChainProducerKind`, records/index/query, and declarations.
  It depends on route 1 identity/query records plus `Inst`, `Block`,
  `Value`, `LoadGlobalInst`, and `std::vector<Route2SelectChainValueRecord>`.
- Route 3 memory-access cluster at lines 4679-4890:
  route-3 enums, memory records/index/query, same-block access records, and
  declarations. It depends on route 1 identity/query records, `Inst`, `Block`,
  `Value`, `MemoryAddress`, `MemoryAccessProvenance`, and
  `std::vector<Route3MemoryAccessRecord>`.
- Route 4 publication cluster at lines 4892-5043:
  route-4 enums, current-block/block-entry/value/index records, and
  declarations. It depends on route 1 query/identity, `Inst`, `Block`,
  `Function`, `Value`, `PhiInst`, and route-index validation types used by its
  validation declarations later in the header.
- Route 5 edge/join publication cluster at lines 5045-5247:
  route-5 enums, records/index, and declarations. It depends on route 1,
  route 3, `Inst`, `Block`, `Function`, `Value`, `PhiInst`, and
  `std::vector<Route5...Record>` containers.
- Route 6 call-use cluster at lines 5249-5437 plus declarations at lines
  6071-6165. It depends on call/source model types, route 1, route 2, route 3,
  route 4, route 5, `CallInst`, `Block`, `Function`, `Value`, and
  `std::vector<Route6...Record>` containers.
- Route 7 comparison cluster at lines 5440-5615 plus declarations at lines
  6167-6219. It depends on route 1, `Inst`, `BinaryInst`, `Block`, `Function`,
  `Value`, route-index validation types, and
  `std::vector<Route7...Record>` containers.
- Route 8 return-chain cluster at lines 5617-5688 plus declarations at lines
  6221-6247. It depends on route 1, `Block`, `Function`, `Value`, and
  `std::vector<Route8ReturnChainRecord>`.
- Route-index facade/validation cluster at lines 5690-5787 plus declarations
  at lines 6249-6308. It contains `RouteIndexRoute`,
  `RouteIndexOwnerScope`, `RouteIndexRecordCategory`,
  `RouteIndexRelationshipKind`, `RouteIndexValidationStatus`,
  `RouteIndexRecordReference`, `RouteIndexReferenceFacade`,
  `Route4IndexReferenceValidation`, `Route7IndexReferenceValidation`,
  `route4_validate_*`, `route7_validate_*`, `route_index_reference_facade`,
  and `route_index_validate_*`.

Complete-type and container risks:

- Do not move `Value`, `Inst`, `Block`, `Function`, `Module`, or
  `MemoryAddress`; the AST type-reference queries show they are pervasive
  complete/model dependencies across route records and signatures.
- Route index containers are complete types by value: `Route1ProducerIndex`,
  `Route2SelectChainValueIndex`, `Route3MemoryAccessIndex`,
  `Route4PublicationAvailabilityIndex`, `Route5EdgeJoinSourceIndex`,
  `Route6CallUseSourceIndex`, `Route7ComparisonConditionIndex`, and
  `Route8ReturnChainIndex` each own `std::vector<...Record>` members. Do not
  split these into a header that relies on forward declarations for contained
  record types.
- `RouteIndexReferenceFacade` itself only stores pointers to route-4 and
  route-7 indexes, but its adjacent validation result structs store
  `RouteIndexRecordReference` by value and route-4/route-7 statuses by value,
  so the facade boundary still needs route-1 identity and route-4/route-7
  status declarations available before it.
- The facade bodies are already isolated in `src/backend/bir/bir_route_facade.cpp`.
  AST `list-symbols` for that TU reports exactly seven facade definitions:
  three `route_index_reference_facade` overloads and four
  `route_index_validate_*` wrappers.

Include/user evidence:

- Current BIR route implementation files include broad `bir.hpp` directly:
  `bir_route_facade.cpp`, `bir_route1.cpp`, `bir_route2.cpp`,
  `bir_route3_memory.cpp`, `bir_route4_publication.cpp`,
  `bir_route5_publication.cpp`, `bir_route6_call_publication.cpp`,
  `bir_route7_comparison.cpp`, and `bir_route8.cpp`.
- `bir_private.hpp` includes `bir.hpp`; `bir.cpp`, `bir_printer.cpp`,
  `bir_render.cpp`, `bir_validate.cpp`, and `lir_to_bir.hpp` also include
  broad `bir.hpp`.
- Direct-source BIR tests in `tests/backend/bir/CMakeLists.txt` already list
  `bir.cpp`, `bir_route_facade.cpp`, and route1-route8 implementation files
  for the broad prepared lookup/helper target. The smaller direct-source group
  lists `bir.cpp` and `bir_route_facade.cpp`.
- External facade consumers are not limited to BIR tests: `rg` found
  route-index facade/status users in `src/backend/mir/aarch64/codegen`,
  `src/backend/mir/query.cpp`, and `src/backend/prealloc`, plus focused uses in
  `backend_prepared_lookup_helper_test.cpp`,
  `backend_prepare_frame_stack_call_contract_test.cpp`,
  `backend_prepared_printer_test.cpp`, and
  `backend_prealloc_block_entry_publications_test.cpp`.

Selected first boundary:

- Recommend one narrow first split: create `src/backend/bir/bir_route_index.hpp`
  for the route-index facade/validation declaration cluster only:
  `RouteIndexRoute`, `RouteIndexOwnerScope`, `RouteIndexRecordCategory`,
  `RouteIndexRelationshipKind`, `RouteIndexValidationStatus`,
  `RouteIndexRecordReference`, `RouteIndexReferenceFacade`,
  `Route4IndexReferenceValidation`, `Route7IndexReferenceValidation`,
  the `route4_validate_*` and `route7_validate_*` declarations, the
  `route_index_reference_facade` overloads, and the `route_index_validate_*`
  declarations.
- Keep `bir.hpp` as the compatibility aggregator and include the new header
  after route-4 and route-7 record/status declarations are available. This is a
  declaration-cluster split, not a standalone replacement for the core BIR
  model header yet.
- Do not move route1-route8 record/index clusters in Step 2. Moving them first
  would either create a catch-all route monolith or require fragile complete
  type ordering around `Value`, `Inst`, `Block`, `Function`, `MemoryAddress`,
  and `std::vector<...Record>` members.

## Suggested Next

Step 2 should introduce `src/backend/bir/bir_route_index.hpp` and move only the
route-index facade/validation declaration cluster listed above, keeping
`bir.hpp` as the public aggregator. Add `#include "bir_route_index.hpp"` from
`bir.hpp` at the current cluster location after required route-4/route-7
record/status declarations are visible. No implementation body should move.

## Watchouts

- This mapping packet intentionally edited no implementation or header files
  other than `todo.md`.
- Do not move `Value`, `Inst`, `Block`, `Function`, `Module`, or
  `MemoryAddress`.
- Do not create a new catch-all route monolith or change public signatures.
- Keep body movement, memory-provenance readiness, and local-array
  semantic-GEP readiness out of this idea.
- `bir_route_index.hpp` may be an aggregator-included declaration fragment at
  first. Treat direct replacement of broad `bir.hpp` includes as Step 3 work,
  and only at sites that need the facade/status surface without the full BIR
  model.
- Leave route1-route8 record/index clusters in `bir.hpp` for now because their
  complete-type/container dependencies are dense and cross-route.

## Proof

Mapping-only packet. No build or tests were required, and `test_after.log` was
not created or updated.

Recommended focused Step 2 proof:

```bash
(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_branch_control_lowering)$') > test_after.log 2>&1
```

If Step 2 exposes include churn outside `src/backend/bir`, escalate proof to
include the affected MIR/prealloc backend tests or a broader backend CTest
subset selected by the supervisor.
