Status: Active
Source Idea Path: ideas/open/153_bir_select_chain_direct_global_identity.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate Prepared Select-Chain Surface And Evidence Set

# Current Packet

## Just Finished

Step 1 of `plan.md` is complete. Inspected the prepared select-chain
surface with `c4c-clang-tools` plus targeted snippets:

- Primary owner: `src/backend/prealloc/select_chain_lookups.hpp` and
  `src/backend/prealloc/select_chain_lookups.cpp`.
- Source producer query:
  `find_prepared_select_chain_source_producer(...)` is defined at
  `select_chain_lookups.cpp:331`; the AST direct callers inside that
  translation unit are
  `prepared_select_chain_contains_direct_global_load(...)`,
  `find_prepared_direct_global_select_chain_dependency(...)`, and
  `find_prepared_scalar_select_chain_materialization(...)`.
- Direct-global dependency query:
  `find_prepared_direct_global_select_chain_dependency(...)` is defined in
  raw and `PreparedSelectChainDependencyQuery` overloads at
  `select_chain_lookups.cpp:365` and `select_chain_lookups.cpp:397`; AST
  same-TU callers are its query overload,
  `find_prepared_store_source_direct_global_select_chain_dependency(...)`,
  and `find_prepared_scalar_select_chain_materialization(...)`.
- Scalar materialization query:
  `find_prepared_scalar_select_chain_materialization(...)` is defined in raw
  and query overloads at `select_chain_lookups.cpp:430` and
  `select_chain_lookups.cpp:468`; AST same-TU caller is its query overload.
- External direct callers found by targeted text lookup:
  `src/backend/prealloc/publication_plans.cpp`,
  `src/backend/prealloc/call_plans.cpp`,
  `src/backend/prealloc/prepared_printer/select_chains.cpp`,
  `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`,
  `src/backend/mir/aarch64/codegen/alu.cpp`,
  `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`,
  `src/backend/mir/aarch64/codegen/calls.cpp`, and prepared-query tests under
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

Prepared data and return contracts:

- Inputs are prepared names, edge-publication source-producer lookups, block
  label, BIR block pointer, root value, and before-instruction index; the query
  overload packages the same fields in `PreparedSelectChainDependencyQuery`.
- `find_prepared_select_chain_source_producer(...)` resolves a named value,
  requires same block, rejects producers at or after the before-boundary, and
  validates that the indexed BIR instruction still matches the prepared
  producer record.
- Source-producer facts come from
  `PreparedEdgePublicationSourceProducerLookups::producers_by_value_name` and
  may identify `LoadLocal`, `LoadGlobal`, `Cast`, `Binary`, or
  `SelectMaterialization` records.
- `PreparedDirectGlobalSelectChainDependency` returns
  `contains_direct_global_load`, `root_is_select`, and
  `root_instruction_index`; it is available only when a direct global load is
  found through the select-chain walk and the root instruction index exists.
- `PreparedScalarSelectChainMaterialization` returns `available`,
  `root_value_name`, `root_is_select`, `root_instruction_index`, and the
  embedded direct-global dependency; it is available for a valid same-block
  named root even when the embedded direct-global dependency is absent.

Representative proof cases already present or directly reusable:

- Select root with direct-global dependency:
  `verify_direct_global_select_chain_dependency_query()` in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` builds
  `LoadGlobal -> Select` and expects `root_is_select=yes`,
  `root_instruction_index=1`, plus scalar materialization availability.
- Non-select root / direct global root:
  the same helper queries a direct `LoadGlobal` root and expects
  `contains_direct_global_load=yes`, `root_is_select=no`,
  `root_instruction_index=2`.
- No-dependency / missing root:
  the same helper queries `%missing` and expects both dependency and scalar
  materialization to fail closed.
- Direct-global call argument route:
  `check_direct_global_select_chain_call_argument_contract()` in
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
  verifies call-plan routing preserves `direct_global_select_chain=yes`,
  selected source value, select root, and root instruction index.
- Prepared-printer visibility:
  `backend_prepared_printer_test.cpp` checks select-chain materialization rows
  for both select roots and direct-global load roots.
- Store-source/publication path:
  `records_direct_global_select_chain_dependency_from_prepared_authority()` in
  `tests/backend/mir/backend_store_source_publication_plan_test.cpp` proves the
  store-source direct-global select-chain dependency facts.

## Suggested Next

Execute Step 2 of `plan.md`: add target-neutral BIR select-chain identity
vocabulary near the existing MIR/BIR query surface without switching any
consumers. Suggested first code-changing packet:

- Owned implementation files: `src/backend/mir/query.hpp` and
  `src/backend/mir/query.cpp`.
- Owned tests: `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
  for prepared/BIR shape comparison helpers.
- Add request/result records keyed by block, root value, and
  before-instruction index.
- Represent root value/name, whether the root producer is a select, root
  instruction index, direct `LoadGlobalInst` dependency presence, and scalar
  materialization eligibility.
- Reuse the existing BIR same-block producer/source identity foundation where
  possible; keep prepared queries as the oracle and do not switch AArch64 or
  prealloc consumers in this packet.
- Recommended narrow proof command:
  `set -o pipefail && cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure | tee test_after.log`

## Watchouts

- Keep `review/phase_a_steps_1_4_route_review.md` untouched.
- Keep prepared select-chain queries available as the oracle until BIR
  equivalence is proven.
- Do not import target materialization cost, register availability,
  publication routing, call ABI behavior, or final AArch64 move/branch choices
  into BIR select-chain identity.
- The AST caller query is translation-unit scoped; external direct callers were
  identified by exact symbol lookup. Treat `publication_plans.cpp`,
  `call_plans.cpp`, prepared-printer code, and AArch64 codegen as consumers
  only, not Step 2 switch targets.
- `backend_aarch64_instruction_dispatch` is recommended in the first proof
  command as a no-switch smoke check around current AArch64 consumers; Step 2
  should still leave those consumers on prepared authority.

## Proof

Analysis-only packet; no build or backend tests required and no
`test_after.log` was created. Local verification used `c4c-clang-tools`
definition/caller queries for the three requested symbols, targeted `rg`
external caller lookup, and source/test snippet inspection.
