Status: Active
Source Idea Path: ideas/open/134_shared_select_chain_same_block_dependency_queries.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Local Dependency Queries

# Current Packet

## Just Finished

Step 1 inventory completed for AArch64-local select-chain direct-global and
same-block scalar/constant materialization dependency queries.

Concrete local sites:
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
  has three anonymous adapters:
  `prepared_same_block_scalar_producer` calls
  `prepare::find_prepared_same_block_scalar_producer`,
  `prepared_same_block_integer_constant` calls
  `prepare::evaluate_prepared_same_block_integer_constant`, and
  `prepared_scalar_select_chain_materialization` calls
  `prepare::find_prepared_scalar_select_chain_materialization`.
- `emit_value_publication_to_register` consumes those adapters for immediate
  folding, same-block producer lookup, binary operand hazard ordering through
  `value_publication_may_write_scratch_register`, and select-chain
  materialization. The select path passes
  `PreparedDirectGlobalSelectChainDependency` into
  `emit_select_chain_value_to_register`.
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
  still exposes `select_chain_contains_direct_global_load`, which returns only
  the boolean from `prepare::find_prepared_direct_global_select_chain_dependency`
  and repeats prepared lookup fallback setup. Focused search found no current
  in-tree caller for this boolean wrapper.
- `src/backend/mir/aarch64/codegen/alu.cpp` directly calls
  `prepare::find_prepared_scalar_select_chain_materialization` around the
  select-result stack-publication path and then checks the embedded
  `direct_global_dependency`.
- `src/backend/mir/aarch64/codegen/calls.cpp` has
  `find_prepared_indirect_callee_direct_global_select_chain`, a local context
  adapter over `prepare::find_prepared_direct_global_select_chain_dependency`
  used before indirect callee select-chain materialization.
- `src/backend/mir/aarch64/codegen/memory.cpp` already uses the shared store
  alias `prepare::find_prepared_store_source_direct_global_select_chain_dependency`
  while planning store-source publication, then consumes the planned dependency
  fields during AArch64 store emission.

Prepared/shared owner candidates:
- Select-chain direct-global ownership is already concentrated in
  `src/backend/prealloc/select_chain_lookups.cpp` and surfaced through
  `src/backend/prealloc/publication_plans.hpp`:
  `find_prepared_direct_global_select_chain_dependency`,
  `find_prepared_store_source_direct_global_select_chain_dependency`, and
  `find_prepared_scalar_select_chain_materialization`.
- Same-block scalar and constant ownership is in
  `src/backend/prealloc/prepared_lookups.cpp` and
  `src/backend/prealloc/prepared_lookups.hpp`:
  `find_prepared_same_block_scalar_producer` and
  `evaluate_prepared_same_block_integer_constant`.
- The first shared-query implementation target should be a reusable prepared
  query-input facade for block-local prepared source facts, likely in
  `prepared_lookups.hpp/.cpp`, that accepts prepared names, source-producer
  lookups, block label, BIR block, value, and before-index once and returns the
  same-block scalar producer plus integer-constant fact. Replace the duplicate
  anonymous adapters in `dispatch_value_materialization.cpp` first, because
  that file has both target-neutral fact queries side by side while leaving
  AArch64 register, scratch, and emission policy local.

## Suggested Next

Step 2 should add the prepared same-block value materialization query facade
and route `dispatch_value_materialization.cpp` through it. Keep
`emit_value_publication_to_register`, scratch ordering, and select emission
unchanged; only replace the duplicated prepared fact lookup/adaptation.

## Watchouts

- Do not add named-case select-chain or direct-global shortcuts.
- Do not move AArch64 emission, hazard, scratch, or materialization-order
  policy into shared code.
- Do not weaken test expectations or mark supported paths unsupported.
- Keep matching `test_before.log` and `test_after.log` for backend proof.
- Nearby shared-query tests already exist in
  `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`:
  `verify_direct_global_select_chain_dependency_query` covers select-chain
  root/direct-load/fail-closed facts, and
  `verify_prepared_same_block_scalar_source_facts` covers same-block scalar
  producer and integer-constant folding/fail-closed facts.
- Nearby AArch64/backend contract coverage includes
  `check_direct_global_select_chain_call_argument_contract` in
  `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`;
  if Step 2 touches only the same-block facade, the supervisor can still use
  the prepared lookup helper plus the backend frame/stack call contract as a
  representative narrow subset.

## Proof

Evidence-only inventory packet; no build or tests run and no `test_after.log`
updated. Recommended Step 2 proof after code edits: `cmake --build --preset
default` then `ctest --test-dir build -R
'^(backend_prepared_lookup_helper|backend_prepare_frame_stack_call_contract)$'
--output-on-failure`.
